# Backend V2 Code    #
# Om Patil           #
# 2026-11-05         #

import glob
import threading
import time

import serial
from flask import Flask, request, render_template, jsonify, Response
from waitress import serve

app = Flask(__name__)

CAR_PORT = "/dev/ttyACM0"
CAR_BAUD = 9600
GLOVE_BAUD = 115200
GLOVE_TIMEOUT = 1
HTTP_LINK_TIMEOUT = 5
RETRY_SECONDS = 1

state_lock = threading.Lock()
car_lock = threading.Lock()
car = None
glove = None
glove_port = None
latest_prox = {"dist": 400, "blocked": 0}
latest_wheels = {"left": 0, "right": 0, "pitch": 0, "roll": 0, "neutralPitch": 0, "neutralRoll": 0}
last_glove = 0
last_car = 0
last_http_wheels = 0
last_http_seen = 0
http_ip = None
#last_log = 0


# ---------- Helpers ----------
def glove_active():
    return time.time() - last_glove < GLOVE_TIMEOUT


def http_glove_active():
    return time.time() - last_http_wheels < GLOVE_TIMEOUT


def http_glove_connected():
    return time.time() - last_http_seen < HTTP_LINK_TIMEOUT


def clamp(value, limit):
    return max(-limit, min(limit, value))


def parse_wheels(line):
    if not line.startswith("WHEELS:"):
        return None
    parts = line[7:].split("|")
    try:
        left, right = map(int, parts[0].split(","))
        fields = {"left": clamp(left, 100), "right": clamp(right, 100)}
        if len(parts) > 1:
            pitch, roll = map(int, parts[1].split(","))
            fields.update(pitch=clamp(pitch, 45), roll=clamp(roll, 70))
        if len(parts) > 2:
            neutral_pitch, neutral_roll = map(int, parts[2].split(","))
            fields.update(neutralPitch=clamp(neutral_pitch, 45), neutralRoll=clamp(neutral_roll, 70))
    except ValueError:
        return None
    return fields


def open_serial(path, baud):
    try:
        return serial.Serial(path, baud, timeout=1)
    except (serial.SerialException, OSError):
        return None


def close_serial(port):
    try:
        port.close()
    except (serial.SerialException, OSError):
        pass


def find_glove_port():
    for path in sorted(glob.glob("/dev/ttyACM*")):
        if path != CAR_PORT:
            return path
    return None


def write_car(left, right):
    with car_lock:
        if car is None:
            return False
        try:
            car.write(f"WHEELS:{left},{right}\n".encode())
        except (serial.SerialException, OSError):
            return False
    return True


def write_glove(dist, blocked):
    port = glove
    if port is None:
        return
    try:
        port.write(f"PROX:{dist},{blocked},\n".encode())
    except (serial.SerialException, OSError):
        pass


# ---------- Serial Readers ----------
def read_car():
    global car, last_car
    warned = False
    while True:
        if car is None:
            port = None if CAR_PORT == glove_port else open_serial(CAR_PORT, CAR_BAUD)
            if port is None:
                if not warned:
                    print(f"WARNING: car not found on {CAR_PORT}, retrying", flush=True)
                    warned = True
                time.sleep(RETRY_SECONDS)
                continue
            with car_lock:
                car = port
            warned = False
            print(f"Car connected on {CAR_PORT}", flush=True)
        try:
            line = car.readline().decode(errors="ignore").strip()
        except (serial.SerialException, OSError):
            print("WARNING: car disconnected", flush=True)
            with car_lock:
                close_serial(car)
                car = None
            with state_lock:
                latest_prox.update(dist=400, blocked=0)
            continue
        if not line.startswith("PROX:"):
            continue
        try:
            dist, blocked = map(int, line[5:].split("|")[0].split(","))
        except ValueError:
            continue
        with state_lock:
            latest_prox.update(dist=dist, blocked=blocked)
            last_car = time.time()
        write_glove(dist, blocked)


def read_glove():
    global glove, glove_port, last_glove
    while True:
        if glove is None:
            path = find_glove_port()
            port = open_serial(path, GLOVE_BAUD) if path else None
            if port is None:
                time.sleep(RETRY_SECONDS)
                continue
            glove = port
            glove_port = path
            print(f"Glove connected on {path}", flush=True)
        try:
            line = glove.readline().decode(errors="ignore").strip()
        except (serial.SerialException, OSError):
            print("Glove disconnected", flush=True)
            close_serial(glove)
            glove = None
            glove_port = None
            continue
        fields = parse_wheels(line)
        if fields is None or http_glove_active():
            continue
        with state_lock:
            latest_wheels.update(fields)
            last_glove = time.time()
        write_car(fields["left"], fields["right"])


threading.Thread(target=read_car, daemon=True).start()
threading.Thread(target=read_glove, daemon=True).start()


# ---------- Routes ----------
@app.route("/")
def index():
    return render_template("index_V2.html")


@app.route("/state")
def state():
    with state_lock:
        copy_prox = latest_prox.copy()
        copy_wheels = latest_wheels.copy()
        copy_wheels["gesture"] = int(glove_active())
    return jsonify(prox=copy_prox, wheels=copy_wheels)


@app.route("/glove", methods=["POST"])
def glove_http():
    global last_glove, last_http_wheels, last_http_seen, http_ip
#    global last_log
    body = request.get_data(as_text=True)[:200].strip()
#    now_log = time.time()
#    print(f"{body.split(':')[0]} gap={int((now_log - last_log) * 1000)}ms", flush=True)
#    last_log = now_log
    if body != "PING":
        fields = parse_wheels(body)
        if fields is None:
            return jsonify(ok=False), 400
    now = time.time()
    with state_lock:
        last_http_seen = now
        http_ip = request.remote_addr
        if body != "PING":
            latest_wheels.update(fields)
            last_glove = now
            last_http_wheels = now
        dist, blocked = latest_prox["dist"], latest_prox["blocked"]
    if body != "PING":
        write_car(fields["left"], fields["right"])
    return Response(f"PROX:{dist},{blocked},", mimetype="text/plain")


@app.route("/status")
def status():
    now = time.time()
    car_open = car is not None
    glove_open = glove is not None
    with state_lock:
        car_age = round(now - last_car, 1) if last_car else None
        glove_age = round(now - last_glove, 1) if last_glove else None
    http_age = round(now - last_http_seen, 1) if last_http_seen else None
    return jsonify(
        car={"connected": car_open, "port": CAR_PORT if car_open else None, "type": "serial", "baud": CAR_BAUD, "age": car_age},
        glove={
            "inUse": glove_active(),
            "age": glove_age,
            "serial": {"connected": glove_open, "port": glove_port if glove_open else None, "type": "serial", "baud": GLOVE_BAUD},
            "http": {"connected": http_glove_connected(), "ip": http_ip, "type": "http", "age": http_age},
        },
    )


@app.route("/wheels", methods=["POST"])
def wheels():
    if glove_active():
        return jsonify(ok=False, reason="glove active"), 409
    data = request.get_json(silent=True)
    try:
        left, right = int(data["left"]), int(data["right"])
    except (ValueError, KeyError, TypeError):
        return jsonify(ok=False), 400
    left, right = clamp(left, 100), clamp(right, 100)
    with state_lock:
        latest_wheels.update(left=left, right=right)
    if not write_car(left, right):
        return jsonify(ok=False, reason="car not connected"), 503
    return jsonify(ok=True)


# ---------- Main ----------
if __name__ == "__main__":
    serve(app, host="0.0.0.0", port=5000, threads=8)
