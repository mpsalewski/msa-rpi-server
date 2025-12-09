#!/usr/bin/env python3
#-----------------------------------------------------------------------------#
"""
File: server.py
Author(s):      Mika Paul Salewski
Created:        2025-11-20
Last Updated:   2025-11-26
Version:        2025.11.20

Title:
    Main server module for the Raspberry Pi REST API backend.

Short Description:
    This file initializes the Flask application, registers blueprints, 
    sets up the firewall, loads the database, and provides a small 
    web interface to display server uptime and network information.

License:
    CC BY-NC-SA 4.0
    See: https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en

Usage Instructions:
    Run development server:
        $ python3 api/server.py

    Run production server (planned):
        $ gunicorn --bind 0.0.0.0:5000 server:app \
                   --access-logfile - --error-logfile -

Accessing the API:
    In a browser:
        http://<RaspberryPi-IP>:5000

    Example request using API key:
        curl -X POST http://<RPI-IP>:5000/sensors/add \
            -H "X-API-Key: <strong-api-key-32+>" \
            -d "sensor_type=temperature" \
            -d "value=23.5"

    Example request using Basic Auth:
        curl -X POST http://<RPI-IP>:5000/sensors/add \
            -u admin:<strong-password-or-hash> \
            -d "sensor_type=temperature" \
            -d "value=20.4"

Notes:
    • This is the main entry point of the backend service.
    • For detailed information see submodules and README.md:
        https://github.com/mpsalewski/msa-rpi-server/
"""
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Libs / Includes

from flask import Flask, render_template, request
from routes.sensors import sensors_bp
import database
from datetime import datetime, timezone
import socket
import subprocess
import os
from auth import require_api_key, require_basic_auth, require_auth_everywhere
import firewall
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Setup

# Set up local firewall for security (blocks unwanted traffic)
firewall.setup_firewall(port=5000)


# Record process start time (used for uptime display on homepage)
START_TIME = datetime.now(timezone.utc)


app = Flask(__name__)

# Protect ALL endpoints with authentication
# Only '/' is publicly accessible
require_auth_everywhere(app, exempt_paths=['/'])


# Initialize database connection and tables
database.init_db()


# Register blueprints (routes are organized in modules)
app.register_blueprint(sensors_bp)


# Optionally start external I2C data-reading script
#i2c_script = os.path.join(os.path.dirname(__file__), "../src/py/read_i2c.py")
#subprocess.Popen(["python3", i2c_script])

# autostart i2c connection to arduino
rpi_server_iot_informer = os.path.join(os.path.dirname(__file__), "../src/c/iot_informer/rpi_server_iot_informer.py")
subprocess.Popen(["python3", rpi_server_iot_informer])
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Function Defs 

def get_local_ip():
	"""
	Return the local IPv4 address of the machine.
	A UDP "fake connect" trick is used to determine the outbound IP.
	"""
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	try:
		s.connect(("10.255.255.255", 1)) # Will not actually send data
		ip = s.getsockname()[0]
	except Exception:
		ip = "127.0.0.1"	# Fallback
	finally:
		s.close()

	return ip

#-----------------------------------------------------------------------------#

def format_timedelta(delta):
	"""Return a human friendly string for a timedelta object."""
	seconds = int(delta.total_seconds())
	days, seconds = divmod(seconds, 86400)
	hours, seconds = divmod(seconds, 3600)
	minutes, seconds = divmod(seconds, 60)
	parts = []
	
	if days:
		parts.append(f"{days}d")
	
	if hours:
		parts.append(f"{hours}h")
	
	if minutes:
		parts.append(f"{minutes}m")
	
	parts.append(f"{seconds}s")
	return ' '.join(parts)

#-----------------------------------------------------------------------------#

@app.route('/')
def index():
	"""Render the quiet server homepage and show uptime/info."""
	now = datetime.now(timezone.utc)
	uptime = now - START_TIME

	uptime_str = format_timedelta(uptime)
	# created date is fixed as requested
	created_on = '21.11.2025'
	owner = 'Mika Paul Salewski'
	pi_ip = get_local_ip()

	return render_template(
		'server_homepage.html', 
		uptime=uptime_str, 
		started_at=START_TIME.isoformat(), 
		created_on=created_on, 
		owner=owner,
		pi_ip=pi_ip
		)
#-----------------------------------------------------------------------------#
	 


#-----------------------------------------------------------------------------#
if __name__ == '__main__':
	# 0.0.0.0 = Allow connections from all devices in the local network
	app.run(host='0.0.0.0', port=5000, debug=False)
#-----------------------------------------------------------------------------#

