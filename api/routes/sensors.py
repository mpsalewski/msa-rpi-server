from flask import Blueprint, request, render_template_string 
from db_models import create_tables
from database import get_connection

sensors_bp = Blueprint("sensors", __name__)

# save tables
create_tables()


@sensors_bp.route('/sensors')
def index():
	conn = get_connection()
	c = conn.cursor()
	c.execute('SELECT sensor_type, value, timestamp FROM sensor_data ORDER BY id DESC')
	rows = c.fetchall()
	conn.close()
	return render_template_string('''
			<h1>Sensor Data</h1>
			<form action="/sensors/add" method="post">
				Type: <input type="text" name="sensor_type" placeholder="temperature"><br>
				Value: <input type="text" name="value"><br>
				<input type="submit" value="add">
			</form>
			<ul>
			{% for sensor_type, value, timestamp in rows %}
				<li>{{ timestamp }} | {{ sensor_type }}: {{ value }}</li>
			{% endfor %}
			</ul>
		''', rows=rows)
		
@sensors_bp.route('/sensors/add', methods=['POST'])
def add():
	sensor_type = request.form.get('sensor_type')
	value = request.form.get('value')
	if sensor_type and value:
		conn = get_connection()
		c = conn.cursor()
		c.execute('INSERT INTO sensor_data (sensor_type, value) VALUES (?,?)', (sensor_type, value))
		conn.commit()
		conn.close()
	return "added datapoint <a href='/sensors'>Sensors</a> <a href='/sensors'>Home</a>"
