from flask import Flask, request, render_template
import serial
import time
import threading

app = Flask(__name__)

arduino = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
time.sleep(2)

scan_data = []
scan_done = threading.Event()
data_lock = threading.Lock()
scan_in_progress = False
last_move_time = time.time()


def read_serial():
    while True:
        line = arduino.readline().decode().strip()
        if line.startswith("SCAN_DATA:"):
            parts = line.split(":")
            angle = int(parts[1])
            distance = int(parts[2])
            with data_lock:
                scan_data.append((angle, distance))
        elif line == "SCAN_DONE":
            scan_done.set()


def watchdog():
    already_stopped = False
    while True:
        time.sleep(0.1)
        if not scan_in_progress and time.time() - last_move_time > 0.5:
            if not already_stopped:
                arduino.write(b"DRIVE:0\n")
                already_stopped = True
        else:
            already_stopped = False


@app.route('/')
def index():
    return render_template('index.html')


@app.route('/light', methods=['POST'])
def light():
    if scan_in_progress:
        return "", 409
    r = request.form['r']
    g = request.form['g']
    if r == '0' and g == '0':
        arduino.write(b"LIGHT:0\n")
    else:
        arduino.write(f"LIGHT:{r},{g},0\n".encode())
    return "", 204


@app.route('/move', methods=['POST'])
def move():
    global last_move_time
    if scan_in_progress:
        return "", 409
    data = request.json
    move_type = data['type']
    i = data['value']
    arduino.write(f"{move_type}:{i}\n".encode())
    last_move_time = time.time()
    return "", 204


@app.route('/scan', methods=['POST'])
def scan():
    global scan_in_progress
    scan_in_progress = True
    scan_data.clear()
    scan_done.clear()
    arduino.write(b"SCAN\n")
    scan_done.wait()
    with data_lock:
        result = scan_data.copy()
    scan_in_progress = False
    return {"data": result}


threading.Thread(target=read_serial, daemon=True).start()
threading.Thread(target=watchdog, daemon=True).start()

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
