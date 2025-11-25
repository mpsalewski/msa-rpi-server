# read_i2c.py
import smbus2 as smbus
import time
import requests
import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), "../../api"))
import config

# ---------------- I2C SECTION ----------------
I2C_BUS = 1          # On Raspberry Pi 2, I2C bus 1
I2C_ADDRESS = 0x08   # Must match Arduino I2C_ADDRESS
bus = smbus.SMBus(I2C_BUS)
# --------------------------------------------


def read_i2c_string_block(addr):
	data_bytes = []
	while True:
		block = bus.read_i2c_block_data(I2C_ADDRESS, 0, 20)				
		for b in block:
			if chr(b) == '#':
				return ''.join([chr(x) for x in data_bytes])
			data_bytes.append(b)
				

def send_to_server(sensor_type, value):
    data = {"sensor_type": sensor_type, "value": value}
    headers = {"X-API-Key": config.API_KEY}
    try:
        #r = requests.post(config.SERVER_URL, data=data, headers=headers, timeout=5)
        r = requests.post("http://localhost:5000/sensors/add", data=data, headers=headers, timeout=5)
        r.raise_for_status()
        print(f"Sent {sensor_type}: {value}")
    except requests.RequestException as e:
        print(f"Error sending {sensor_type}: {e}")


def main(poll_interval=10):
    while True:
        try:
            data_str = read_i2c_string_block()
            if ',' in data_str:
                temp, hum = data_str.split(',')
                send_to_server("temperature", temp)
                send_to_server("humidity", hum)
            else:
                print("Invalid I2C data:", data_str)
        except Exception as e:
            print("Error reading I2C:", e)
        time.sleep(poll_interval)

if __name__ == "__main__":
    main()



