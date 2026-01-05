from flask import Flask, render_template, request, send_from_directory, jsonify
import os
import pygame
import threading

app = Flask(__name__)

MUSIC_FOLDER = os.path.expanduser("~/hp/myenv/dunith/dunith-player/uploads/")
UPLOAD_FOLDER = "uploads"
SELECTED_SONG_FILE = "selected_song.txt"  # Store selected song

if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

app.config["UPLOAD_FOLDER"] = UPLOAD_FOLDER

# Initialize pygame mixer
pygame.mixer.init()

# Store selected song path
selected_song_path = None

@app.route("/")
def index():
    files = os.listdir(UPLOAD_FOLDER)
    return render_template("index.html", files=files)

ALLOWED_EXTENSIONS = {"mp3"}

def allowed_file(filename):
    return "." in filename and filename.rsplit(".", 1)[1].lower() in ALLOWED_EXTENSIONS

@app.route("/upload", methods=["POST"])
def upload_file():
    if "file" not in request.files:
        return jsonify({"error": "No file part"}), 400

    file = request.files["file"]
    if file.filename == "":
        return jsonify({"error": "No selected file"}), 400
    if not allowed_file(file.filename):
        return jsonify({"error": "Invalid file type. Only MP3 files are allowed"}), 400

    file.save(os.path.join(app.config["UPLOAD_FOLDER"], file.filename))
    return jsonify({"message": "File uploaded successfully"}), 200

def play_music():
    global selected_song_path
    if not selected_song_path or not os.path.exists(selected_song_path):
        return

    pygame.mixer.music.load(selected_song_path)
    pygame.mixer.music.play()
    print(f"Playing: {selected_song_path}")

def stop_music():
    pygame.mixer.music.stop()
    print("Music stopped.")

@app.route("/play", methods=["POST"])
def play_song():
    global selected_song_path

    if selected_song_path is None:
        files = sorted(os.listdir(UPLOAD_FOLDER), key=lambda x: os.path.getmtime(os.path.join(UPLOAD_FOLDER, x)))
        if not files:
            return jsonify({"error": "No files available"}), 400
        selected_song_path = os.path.join(UPLOAD_FOLDER, files[-1])  # Play the most recently uploaded file

    if not os.path.exists(selected_song_path):
        return jsonify({"error": f"File not found: {selected_song_path}"}), 404

    if pygame.mixer.music.get_busy():
        pygame.mixer.music.stop()

    threading.Thread(target=play_music, daemon=True).start()  # Play in a separate thread

    return jsonify({"message": f"Playing {selected_song_path}"}), 200

@app.route("/stop", methods=["POST"])
def stop_song():
    if pygame.mixer.music.get_busy():
        stop_music()
        return jsonify({"message": "Playback stopped"}), 200
    return jsonify({"error": "No song is playing"}), 400

@app.route("/uploads/<filename>")
def uploaded_file(filename):
    return send_from_directory(app.config["UPLOAD_FOLDER"], filename)

@app.route("/delete", methods=["POST"])
def delete_file():
    filename = request.json.get("filename")
    filepath = os.path.join(UPLOAD_FOLDER, filename)

    if os.path.exists(filepath):
        os.remove(filepath)
        return jsonify({"message": f"Deleted {filename}"}), 200
    return jsonify({"error": "File not found"}), 404

@app.route("/select_song", methods=["POST"])
def select_song():
    global selected_song_path
    filename = request.json.get("filename")
    filepath = os.path.join(UPLOAD_FOLDER, filename)

    if not os.path.exists(filepath):
        return jsonify({"error": "File not found"}), 404

    selected_song_path = filepath
    return jsonify({"message": f"Song {filename} selected successfully!"}), 200

@app.route('/trigger', methods=['GET'])
def trigger_action():
    action = request.args.get('action')

    if action == 'play':
        threading.Thread(target=play_music, daemon=True).start()
        return jsonify({"message": "Play action triggered"}), 200
    elif action == 'stop':
        stop_music()
        return jsonify({"message": "Stop action triggered"}), 200
    else:
        return jsonify({"error": "Invalid action"}), 400

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001, debug=True, threaded=True)
