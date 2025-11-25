# curl -d "sensor_type=room_entrance&value=1" -X POST http://127.0.0.1:5000/sensors/add
# curl -d "sensor_type=temperature&value=23.5" -X POST http://127.0.0.1:5000/sensors/add



import requests 

SERVER = "http://localhost:5000/sensors/add"

def send(sensor_type, value):
	data = {
	"sensor_type": sensor_type,
	"value": value
	}
	
	r = requests.post(SERVER, data=data)
	print("Status: ", r.status_code)
	print("Response: ", r.text)
	
	
# example 
send("temperature", 22.8)
send("room_entrance", 1)
	
	 

