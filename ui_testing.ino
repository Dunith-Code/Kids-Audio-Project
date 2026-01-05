#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Redmi_Note_14";  
const char* password = "ABC123456";
const char* mqtt_server = "192.168.137.219";  // Raspberry Pi's IP address (MQTT)
const char* serverUrl = "http://192.168.137.219:5000/trigger";  // Flask Server URL

WiFiClient espClient;
PubSubClient client(espClient);
HTTPClient http;

const int buttonPin = 4;
bool buttonPressed = false;
volatile bool buttonStateChanged = false;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

void IRAM_ATTR handleButtonPress() {
    buttonStateChanged = true;  // Interrupt flag
}

void setup() {
    Serial.begin(115200);
    pinMode(buttonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonPress, CHANGE);

    connectWiFi();
    connectMQTT();
}

void loop() {
    if (!client.connected()) {
        Serial.println("Reconnecting to MQTT Broker...");
        connectMQTT();
    }
    client.loop();

    if (buttonStateChanged) {
        int buttonState = digitalRead(buttonPin);
        delay(50);  // Extra debounce

        if ((millis() - lastDebounceTime) > debounceDelay) {
            if (buttonState == LOW && !buttonPressed) {
                Serial.println("Button Pressed - Sending play_music");
                client.publish("test/topic", "play_music");
                sendHttpRequest("play");
                buttonPressed = true;
            } 
            else if (buttonState == HIGH && buttonPressed) {
                Serial.println("Button Released - Sending stop_music");
                client.publish("test/topic", "stop_music");
                sendHttpRequest("stop");
                buttonPressed = false;
            }
            lastDebounceTime = millis();
        }
        buttonStateChanged = false;  // Reset interrupt flag
    }
}

void connectWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 20) {  // Timeout after 20 attempts
        delay(1000);
        Serial.print(".");
        attempt++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
    } else {
        Serial.println("\nWiFi Connection Failed!");
    }
}

void connectMQTT() {
    Serial.print("Connecting to MQTT Broker...");
    client.setServer(mqtt_server, 1883);

    int retry = 0;
    while (!client.connected() && retry < 5) {  // Max retries = 5
        if (client.connect("ESP8266_Client")) {
            Serial.println("\nConnected to MQTT Broker!");
        } else {
            Serial.println("\nMQTT connection failed. Retrying...");
            delay(5000);
            retry++;
        }
    }

    if (!client.connected()) {
        Serial.println("MQTT Connection Failed. Check Broker!");
    }
}

void sendHttpRequest(const String& action) {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        String url = String(serverUrl) + "?action=" + action;

        Serial.print("Sending HTTP GET request to: ");
        Serial.println(url);

        http.begin(client, url);
        int httpResponseCode = http.GET();
        
        Serial.print("HTTP Response: ");
        Serial.println(httpResponseCode);

        if (httpResponseCode == 200) {
            Serial.print("Action ");
            Serial.print(action);
            Serial.println(" received");
        } else {
            Serial.println("Error in HTTP request");
        }

        http.end();  // Close HTTP connection
    } else {
        Serial.println("WiFi not connected! Cannot send HTTP request.");
    }
}

