#!/usr/bin/env python3
#-----------------------------------------------------------------------------#
"""
File:           sensors.py
Author(s):      Mika Paul Salewski
Created:        2025-11-20
Last Updated:   2025-11-26
Version:        2025.11.20

Title:
    Sensor data endpoints for the Raspberry Pi REST API backend.

Short Description:
    Implements the Flask blueprint responsible for displaying, retrieving, 
    and storing sensor readings. Provides HTTP endpoints used by the
    Web UI and external devices for data submission.

License:
    CC BY-NC-SA 4.0
    See: https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en

Notes:
    • Routes under this blueprint expose sensor information stored in the 
      SQLite database managed by db_models.py.
    • /sensors          → Renders a simple HTML page for viewing sensor data.
    • /sensors/data     → Returns the latest 50 entries as JSON.
    • /sensors/add      → Accepts POST form data and stores a new reading.
    • Optional Security:
          - @require_basic_auth can secure the dashboard UI.
          - @require_api_key can restrict write-access to IoT devices.
    • Tables are guaranteed to exist at import via create_tables().
"""
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Libs / Includes

from flask import Blueprint, request, render_template, jsonify
from db_models import create_tables
from database import get_connection
# from auth import require_api_key, require_basic_auth   # Optional per-route security
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Blueprint Setup

sensors_bp = Blueprint("sensors", __name__, template_folder="templates/")
create_tables()  # Ensure DB tables exist
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Routes

@sensors_bp.route('/sensors')
# @require_basic_auth                           # Optional: Secure UI with Basic Auth
def index():
    """Render the sensor dashboard HTML page."""
    return render_template("sensors.html")

#-----------------------------------------------------------------------------#

@sensors_bp.route('/sensors/data')
def get_data():
    """Return latest 50 sensor entries as JSON."""
    conn = get_connection()
    c = conn.cursor()
    c.execute('SELECT sensor_type, value, timestamp FROM sensor_data ORDER BY id DESC LIMIT 50')
    rows = c.fetchall()
    conn.close()

    return jsonify(rows)

#-----------------------------------------------------------------------------#

@sensors_bp.route('/sensors/add', methods=['POST'])
# @require_api_key()                            # Optional: Secure write access
def add():
    """Insert a new sensor reading into the database."""
    sensor_type = request.form.get('sensor_type')
    value = request.form.get('value')

    if sensor_type and value:
        conn = get_connection()
        c = conn.cursor()
        c.execute('INSERT INTO sensor_data (sensor_type, value) VALUES (?, ?)',
                  (sensor_type, value))
        conn.commit()
        conn.close()

    return jsonify({"status": "ok"})

#-----------------------------------------------------------------------------#

@sensors_bp.route('/sensors/get', methods=['GET'])
# @require_api_key()                            # Optional: Secure write access
def get():
    """
    Return the last N values of a specific sensor as JSON.

    Query Parameters:
        sensor_type (str) : required, the sensor to query
        limit       (int) : optional, number of latest entries (default 10)

    Returns:
        JSON array of sensor readings in the form:
        [
            {"sensor_type": "bathroom_main", "value": 1.0, "timestamp": "..."},
            ...
        ]
    """
    sensor_type = request.args.get('sensor_type')
    limit = request.args.get('limit', 10, type=int)  # default 10 entries

    if not sensor_type:
        return jsonify({"error": "sensor_type parameter is required"}), 400

    conn = get_connection()
    c = conn.cursor()
    c.execute(
        'SELECT sensor_type, value, timestamp FROM sensor_data '
        'WHERE sensor_type = ? ORDER BY id DESC LIMIT ?',
        (sensor_type, limit)
    )
    rows = c.fetchall()
    conn.close()

    # Convert each row into a dict
    result = [{"sensor_type": r[0], "value": r[1], "timestamp": r[2]} for r in rows]

    return jsonify(result)
#-----------------------------------------------------------------------------#    