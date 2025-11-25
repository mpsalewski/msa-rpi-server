# sensors.py
from flask import Blueprint, request, render_template, redirect, url_for, jsonify
from db_models import create_tables
from database import get_connection
# from auth import require_api_key, require_basic_auth #secure single routes


sensors_bp = Blueprint("sensors", __name__, template_folder="templates/")

create_tables()

@sensors_bp.route('/sensors')
#@require_basic_auth secure single routes
def index():
    return render_template("sensors.html")


@sensors_bp.route('/sensors/data')
def get_data():
    conn = get_connection()
    c = conn.cursor()
    c.execute('SELECT sensor_type, value, timestamp FROM sensor_data ORDER BY id DESC LIMIT 50')
    rows = c.fetchall()
    conn.close()

    return jsonify(rows)


@sensors_bp.route('/sensors/add', methods=['POST'])
#@require_api_key() #secure single routes 
def add():
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




