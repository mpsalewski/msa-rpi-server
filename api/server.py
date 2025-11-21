
# install pip3 install flask
# source venv/bin/activate
# run: python3 server.py
#
# in browser: http://<RaspberryPi-IP>:5000
# add data: curl -d "value=23.5" -X POST http://<RaspberryPi-IP>:5000/add





from flask import Flask 
from routes.sensors import sensors_bp
import database

app = Flask(__name__)


# init Dabatbase
database.init_db()

# register routes 
app.register_blueprint(sensors_bp)#, url_prefix='/')

@app.route('/')
def index():
	return "server is running"
	
if __name__ == '__main__':
	app.run(host='0.0.0.0', port=5000)



# Old First Steps
if False:
	from flask import Flask, render_template_string, request


	app = Flask(__name__)

	# example data list
	data = []

	# start page 
	@app.route('/')
	def index():
			return render_template_string('''
					<h1>Sensor Testdaten</h1>
					<ul>
					{% for d in data %}
						<li>{{ d }}</li>
					{% endfor %}
					</ul>
					''', data=data)
					
	# endpoint for test data 
	@app.route('/add', methods=['POST'])
	def add():
		value = request.form.get('value')
		if value:
			data.append(value)
			return f"Datapoint {value} added"
		return "no data received"
		
	if __name__ == '__main__':
		app.run(host='0.0.0.0', port=5000)
