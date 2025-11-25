# server.py
#
# install pip3 install flask
# pip install python-dotenv itsdangerous Flask-Limiter flask-cors gunicorn
# optional: flask-httpauth, pyjwt, flask-wtf (CSRF), fail2ban ist Systempaket
#
#
# generate safe tokens: 
"""
python3 - <<'PY'
import secrets
print(secrets.token_urlsafe(32))
PY
"""
#
#
"""
Firewall
sudo apt install ufw
sudo ufw default deny incoming
sudo ufw default allow outgoing

sudo ufw allow from 192.168.178.0/24 to any port 5000
sudo ufw allow from 192.168.178.0/24 to any port 22   # SSH
sudo ufw enable
sudo ufw status
→ Damit kann NICHTS aus dem Internet deinen Server erreichen.
→ Nur dein Heimnetz hat Zugriff.


later, when external access is wanted see:
~/Desktop/rpi_external_access.txt 
"""
#
#
# source venv/bin/activate
# run: python3 server.py
#
# in browser: http://<RaspberryPi-IP>:5000
# add data: curl -d "value=23.5" -X POST http://<RaspberryPi-IP>:5000/add 
# --> does not work anymore; use: 
"""
curl -X POST http://<R-Pi-IP>:5000/sensors/add \
     -H "X-API-Key: <starker-api-key-32+>" \
     -d "sensor_type=temperature" \
     -d "value=23.5"
"""
# or
"""
curl -X POST http://<R-Pi-IP>:5000/sensors/add \
     -u admin:<starkes-passwort-oder-better-hash> \
     -d "sensor_type=temperature" \
     -d "value=20.4"
""" 


# file: server.py (or your main module)
from flask import Flask, render_template, request
from routes.sensors import sensors_bp
import database
from datetime import datetime, timezone
import socket
import subprocess
import os
from auth import require_api_key, require_basic_auth, require_auth_everywhere
import firewall
firewall.setup_firewall(port=5000)



# record process start time (used for uptime display)
START_TIME = datetime.now(timezone.utc)


app = Flask(__name__)

# Falls du ALLE Endpoints schützen willst:
require_auth_everywhere(app, exempt_paths=['/'])  # dann musst du API-Key oder Basic bei jedem Request senden


# init Database
database.init_db()


# register routes
app.register_blueprint(sensors_bp)


# start i2c script
i2c_script = os.path.join(os.path.dirname(__file__), "../src/py/read_i2c.py")
subprocess.Popen(["python3", i2c_script])


# get local ip for homepage display
def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # Trick: Verbindung zu nicht existierendem Ziel, nur um lokale IP zu bekommen
        s.connect(("10.255.255.255", 1))
        ip = s.getsockname()[0]
    except Exception:
        ip = "127.0.0.1"
    finally:
        s.close()
    return ip




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

	 

if __name__ == '__main__':
	# listening on 0.0.0.0 allows other devices on the same local network to connect
	app.run(host='0.0.0.0', port=5000, debug=False)


