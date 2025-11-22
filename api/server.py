
# install pip3 install flask
# source venv/bin/activate
# run: python3 server.py
#
# in browser: http://<RaspberryPi-IP>:5000
# add data: curl -d "value=23.5" -X POST http://<RaspberryPi-IP>:5000/add


# file: server.py (or your main module)
from flask import Flask, render_template, request
from routes.sensors import sensors_bp
import database
from datetime import datetime, timezone
import socket


# record process start time (used for uptime display)
START_TIME = datetime.now(timezone.utc)


app = Flask(__name__)


# init Database
database.init_db()


# register routes
app.register_blueprint(sensors_bp)


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
	app.run(host='0.0.0.0', port=5000)


