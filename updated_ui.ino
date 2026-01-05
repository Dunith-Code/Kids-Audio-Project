#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Redmi_Note_14";  
const char* password = "ABC123456";
const char* mqtt_server = "192.168.137.219";  // Raspberry Pi's IP address (MQTT)
const char* serverUrl = "http://192.168.137.219:5001/trigger";  // Flask Server URL

WiFiClient espClient;
PubSubClient client(espClient);
HTTPClient http;

const int buttonPin = 4;
bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

void setup() {
    Serial.begin(115200);
    pinMode(buttonPin, INPUT_PULLUP);

    // Connect to Wi-Fi
    connectWiFi();
    
    // Connect to MQTT
    connectMQTT();
}

void loop() {
    // Reconnect to MQTT if disconnected
    if (!client.connected()) {
        Serial.println("Reconnecting to MQTT Broker...");
        connectMQTT();
    }
    client.loop();

    int buttonState = digitalRead(buttonPin);
    // Check button state and debounce
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
        lastDebounceTime = millis(); // Update the debounce time
    }
}

void connectWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 20) {
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
    client.setKeepAlive(60); // Set keep-alive time to 60 seconds

    int retry = 0;
    while (!client.connected() && retry < 5) {
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

        // Retry logic if request fails
        if (httpResponseCode != 200) {
            Serial.println("Error in HTTP request. Retrying...");
            int retries = 3;
            while (retries-- > 0) {
                httpResponseCode = http.GET();
                if (httpResponseCode == 200) break;
                delay(1000);
            }
        }

        if (httpResponseCode == 200) {
            Serial.print("Action ");
            Serial.print(action);
            Serial.println(" received");
        } else {
            Serial.println("Failed to receive valid response from server.");
        }

        http.end();  // Close HTTP connection
    } else {
        Serial.println("WiFi not connected! Cannot send HTTP request.");
    }
}
