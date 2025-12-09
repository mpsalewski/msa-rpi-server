# Raspberry Pi REST API Server

**Author:** Mika Paul Salewski  
**Created:** 2025-11-26  
**Version:** 2025.11.26

---

## Short description
Compact Flask-based server for collecting sensor readings on a Raspberry Pi (SQLite) with ready-to-run client examples for ESP32 / Arduino and exposing a small web UI. Supports API key and Basic Auth. Includes configuration and firewall helper scripts.

---

## Table of contents
1. [Overview](#overview)
2. [Features](#features)
3. [Requirements](#requirements)
4. [Quick start](#quick-start)
5. [Configuration (`.env`)](#configuration-env)
6. [Running (development & production)](#running-development--production)
7. [Authentication](#authentication)
8. [API Endpoints](#api-endpoints-summary)
9. [Client sketches (`src/c`)](#client-sketches-src-c)
10. [I2C Reader](#i2c-reader)
11. [Database](#database)
12. [Firewall helper (UFW)](#firewall-helper-ufw)
13. [Development & testing](#development--testing)
14. [Troubleshooting](#troubleshooting)
15. [License](#license)

---

## Overview
This project provides a server-first solution for small IoT setups: a Flask app running on a Raspberry Pi that receives sensor data, stores it locally (SQLite) database (`server.db`), and exposes both a UI and REST endpoints for integrations. Client examples for ESP32 (WiFi), Arduino (I2C,...) and a Python I2C reader demonstrate typical integration patterns.



The repository contains:
- `server.py` — the Flask app and routes
- `requirements.txt` — Python dependencies
- `src/py/read_i2c.py` — example I2C reader that posts readings
- `firewall.py` — UFW helper to restrict access to the LAN
- `server.db` (created automatically on first run)
- TBD

---

## Features
- RESTful JSON API for inserting and retrieving sensor readings  
- Minimal HTML UI for quick inspection and simple admin tasks  
- Authentication: API key (`X-API-Key`) and HTTP Basic Auth  
- SQLite persistence with automatic table creation and suggested WAL mode  
- Example clients: ESP32 (Wi-Fi), Arduino (I2C/serial patterns), Python I2C script  
- UFW helper script for simple LAN-only firewall rules


---

## Requirements
- Raspberry Pi (or any Linux machine)
- Python 3.8+
- `pip3`; optional: gunicorn, nginx for production 
- I2C enabled if using the Python I2C reader

Install Python deps:

```bash
cd api
pip3 install -r requirements.txt
```

---

## Quick start
1. Clone repository and change into the API folder:

```bash
git clone https://github.com/<your-username>/msa-rpi-server.git
cd msa-rpi-server/api
```

2. Install dependencies (see `requirements.txt`):

```bash
pip3 install -r requirements.txt
```

3. Create a `.env` file in the `api/` directory (example below).
```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

4. Run in development mode:

```bash
python3 server.py
```

5. Visit the UI in your browser:

```
http://<RPI-IP>:5000
```

---

## Configuration (`.env`)
Create `api/.env` with these variables (do **not** commit secrets):

```ini
API_KEY=your-api-key-here
ADMIN_USER=admin
ADMIN_PASSWORD=your-admin-password
REALM_NAME=My-RPi-Server
```

Notes:
- `API_KEY` is used for X-API-Key header authentication.
- `ADMIN_USER` / `ADMIN_PASSWORD` can be used for Basic Auth on protected endpoints or the admin UI.

---

## Running

### Development
Run the app (auto-creates DB and tables on first start):

```bash
python3 server.py
```

Default binds to `0.0.0.0:5000` (check `server.py` for exact behavior). Use FLASK or environment options if your app is set up to honor them.

### Production (example)
For production, run the Flask app behind a production WSGI server such as `gunicorn` and optionally put an nginx reverse proxy in front. Example:

```bash
pip3 install gunicorn
gunicorn -w 2 -b 0.0.0.0:5000 server:app
```
Also ensure the Pi firewall (see `firewall.py`) is configured and that you use strong credentials.
- NOTE: There are many options to run this in production mode or as an autostart server on Raspberry Pi. This is just a very simple example.


---

## Authentication
Two authentication methods are supported:

1. **API Key** via the request header `X-API-Key: <API_KEY>`
2. **HTTP Basic Auth** using `ADMIN_USER` / `ADMIN_PASSWORD`

Example `curl` using the API key to POST a reading:

```bash
curl -X POST http://<RPI-IP>:5000/sensors/add   -H "X-API-Key: $API_KEY"   -d "sensor_type=temperature"   -d "value=23.5"
```

Example `curl` with Basic Auth:

```bash
curl -u admin:your-admin-password -X POST http://<RPI-IP>:5000/sensors/add   -d "sensor_type=humidity" -d "value=44.2"
```

Always prefer HTTPS and strong secrets in production. If you expose the API to the internet, require authentication and consider rate-limiting and logging.

---

## API Endpoints (summary)
> All endpoints are relative to the app root (e.g. `http://<RPI-IP>:5000`)

- `GET /` — Home page / status (uptime, IP address, simple health check)
- `GET /sensors` — Simple sensor UI (HTML)
- `GET /sensors/data` — Returns last 50 entries as JSON
- `POST /sensors/add` — Add a reading (form or JSON). Fields: `sensor_type`, `value` (and optionally `timestamp`)

### POST `/sensors/add` example (JSON):

```bash
curl -X POST "http://<RPI-IP>:5000/sensors/add" \
  -H "Content-Type: application/json" \
  -H "X-API-Key: $API_KEY" \
  -d '{"sensor_type":"temperature","value":23.5}'
```

**Response** (JSON, example):

```json
{ "status": "ok", "id": 123, "inserted_at": "2025-11-26T12:34:56Z" }
```

Errors are returned as JSON with an appropriate HTTP status code.

---
## Client sketches (`src/c`)

### Short description
This folder contains the microcontroller and Pi-side client examples that integrate with the Raspberry Pi REST API backend. The server is the primary component; the sketches and scripts in `src/c/` are **examples** that demonstrate typical integration patterns (ESP32 HTTP POST clients, low-power sketches, an Arduino I2C LCD informer and a Pi-side I2C writer).

> Important: these are examples — adapt pin assignments, power-management and timing to your hardware and deployment. Do **not** commit filled-in secret files.

---

### Contents (actual files in this repo)
- `src/c/room_monitor/`  
  - `room_monitor.ino` — ESP32 sketch (DHT11) that reads temperature & humidity and POSTs form-encoded data to `POST /sensors/add`. Uses `WiFi.h`, `HTTPClient.h`, `DHT.h` / `DHT_U.h`.
  - `secrets.h.example` — template for Wi-Fi, server URL and API key.

- `src/c/bathroom_main_monitor/`  
  - `bathroom_main_monitor.ino` — occupancy/door monitor sketch (PIR / door sensor) that reports bathroom status to the server.
  - `secrets.h.example`

- `src/c/apartment_traffic_detection/`  
  - `apartment_traffic_detection.ino` — motion + door event detector that determines traffic (enter/exit) and reports to the server.
  - `SR501_PIR_Motion_Sensor_datasheet.pdf` — sensor reference.
  - `secrets.h.example`

- `src/c/iot_informer/`  
  - `iot_informer.ino` — Arduino / ESP32 firmware acting as an I2C **slave** that displays bathroom occupancy on a 16×2 LCD (custom chars in `msa_lcd.h`). I2C address used in repo: `0x08`. This sketch does **not** use Wi-Fi (it expects the Pi to be the networked master).
  - `msa_lcd.h` — LCD helpers / characters.
  - `rpi_server_iot_informer.py` — Raspberry Pi script that polls the REST API for the latest `bathroom_main` value and writes it to an I2C slave (the informer). Requires `smbus2` and `requests`.

- `src/c/low_power_apps/` (examples focused on battery/low-power operation)  
  - `apartment_traffic_detection_lowP/*` — low-power variant of the traffic detector.
  - `bathroom_main_monitor_lowP/*` — low-power bathroom monitor that briefly connects to Wi-Fi and reports state.
  - `room_monitor_low_power/*` — low-power room monitor example.
  - Each low-power sketch includes a `secrets.h.example`.

---

### How the clients talk to the server
- The ESP32 sketches (e.g. `room_monitor.ino`) **POST** form-encoded data to the server using the same endpoint the README documents:

  - Endpoint: `POST /sensors/add`  
  - Headers: `X-API-Key: <API_KEY>`  
  - Example form body sent by sketches:
    ```
    sensor_type=temperature&value=23.50
    ```

  - The ESP32 code uses `HTTPClient` and sets `Content-Type: application/x-www-form-urlencoded`.

- The I2C informer pattern:
  - `iot_informer.ino` runs on a microcontroller as I2C slave (address `0x08`) and displays occupancy.
  - `rpi_server_iot_informer.py` (Pi-side) polls the REST API (example query: `/sensors/get?sensor_type=bathroom_main&limit=1`) and writes the latest status to the I2C device.
  - `rpi_server_iot_informer.py` requires `smbus2` (I2C access) and `requests` (HTTP) and should be run on the Pi with I2C enabled.

---

### Quick usage notes & commands

1. **Secrets** — copy and edit per-sketch secrets:
   ```bash
   # from within each sketch folder, once copied:
   cp secrets.h.example secrets.h
   # Fill: WIFI_SSID, WIFI_PASSWORD, SERVER_URL (e.g. "http://192.168.1.10:5000/sensors/add"), API_KEY


---

## I2C Reader
> **Note:** In the future, I2C will likely be used less frequently, as most sensors are expected to communicate over Wi-Fi or other wireless protocols.

`src/py/read_i2c.py` contains an example script to read data from an I2C device and POST to `/sensors/add`.

- The script expects the device to return `"temperature,humidity"` style strings (adjust parsing as needed).
- Run it with Python on the Pi (I2C must be enabled):

```bash
python3 src/py/read_i2c.py
```

You can modify the script to read from other I2C sensors or to post additional metadata (e.g. `location`, `sensor_id`).

- The script can also be started from `server.py`, but the lines are currently commented out:

```python
# Optionally start external I2C data-reading script
# i2c_script = os.path.join(os.path.dirname(__file__), "../src/py/read_i2c.py")
# subprocess.Popen(["python3", i2c_script])
```


---

## Database
The app uses SQLite (`server.db` in the `api/` directory by default). Tables are created automatically on first start.

Suggested minimal schema (created automatically by the app):

```sql
CREATE TABLE IF NOT EXISTS readings (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  sensor_type TEXT NOT NULL,
  value REAL NOT NULL,
  timestamp TEXT NOT NULL DEFAULT (datetime('now'))
);
```

Backup strategy: copy `server.db` while the app is stopped or use `sqlite3`'s online backup APIs.

---

## Firewall helper (UFW)
`firewall.py` contains helper functions to configure UFW to allow access only from your LAN to ports 5000 (app) and 22 (SSH). Typical usage:

```bash
sudo python3 firewall.py --allow-lan
```

**Important:** Verify the script before running. Locking yourself out of the Pi is easy if rules are misconfigured.

---

## Development & testing
- Use a virtual environment (recommended):

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

- Run unit tests (if available) or manually exercise endpoints with `curl` or Postman.

---

## Troubleshooting
- **App not starting** — check Python version and installed dependencies. Check logs printed to the console.
- **I2C errors** — ensure I2C is enabled (`raspi-config` → Interface Options → I2C) and that wiring is correct.
- **DB locked** — another process may be writing; stop the app and inspect `server.db` with `sqlite3`.
- **Firewall issues** — verify UFW rules with `sudo ufw status verbose` and ensure your client IP is allowed.

---

## Security notes
- Do **not** store secrets in version control. Keep `.env` out of git (`.gitignore` it).
- Use HTTPS in any environment exposed to untrusted networks.
- Use strong, random API keys and passwords; rotate them periodically.

---

## License
This project is licensed under **CC BY-NC-SA 4.0**. See the LICENSE file for details.

---

## Contributing
Contributions, bug reports, and feature requests are welcome. Please open an issue or a pull request in the repository.

---

