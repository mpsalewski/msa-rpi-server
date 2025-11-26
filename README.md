# Raspberry Pi REST API Backend

**Author:** Mika Paul Salewski  
**Created:** 2025-11-26  
**Version:** 2025.11.26

---

## Short description
Lightweight Flask backend for collecting sensor data (I2C / Wi‑Fi), storing it in SQLite, and exposing a small web UI and REST endpoints. Supports API key and Basic Auth. Includes configuration and firewall helper scripts.

---

## Table of contents
1. [Overview](#overview)
2. [Features](#features)
3. [Requirements](#requirements)
4. [Quick start](#quick-start)
5. [Configuration (`.env`)](#configuration-env)
6. [Running (development & production)](#running)
7. [Authentication](#authentication)
8. [API Endpoints](#api-endpoints)
9. [I2C Reader](#i%C2%B2c-reader)
10. [Database](#database)
11. [Firewall helper (UFW)](#firewall-helper-ufw)
12. [Development & testing](#development--testing)
13. [Troubleshooting](#troubleshooting)
14. [License](#license)

---

## Overview
This small Flask-based REST API is designed for Raspberry Pi projects that need a compact backend to receive sensor readings (for example from an Arduino over I2C or from Wi‑Fi sensors), store them in a local SQLite database (`server.db`) and provide basic visualization and HTTP endpoints to retrieve and add data.

The repository contains:
- `server.py` — the Flask app and routes
- `requirements.txt` — Python dependencies
- `src/py/read_i2c.py` — example I2C reader that posts readings
- `firewall.py` — UFW helper to restrict access to the LAN
- `server.db` (created automatically on first run)

---

## Features
- Lightweight Flask REST API
- Minimal web UI for quick inspection of readings
- Endpoints for listing and adding sensor readings (JSON)
- Authentication via API key (header `X-API-Key`) or HTTP Basic Auth
- SQLite persistence with automatic table creation
- UFW helper script to restrict access to the Pi

---

## Requirements
- Raspberry Pi (or any Linux machine)
- Python 3.8+
- `pip3`
- I2C enabled on the Pi if using I2C reader

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
gunicorn -w 4 -b 0.0.0.0:5000 server:app
```

Also ensure the Pi firewall (see `firewall.py`) is configured and that you use strong credentials.

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
curl -X POST http://<RPI-IP>:5000/sensors/add   -H "Content-Type: application/json"   -H "X-API-Key: $API_KEY"   -d '{"sensor_type":"temperature","value":23.5}'
```

**Response** (JSON, example):

```json
{ "status": "ok", "id": 123, "inserted_at": "2025-11-26T12:34:56Z" }
```

Errors are returned as JSON with an appropriate HTTP status code.

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

