# read_i2c.py
import smbus2 as smbus
import time

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
				


while True:
	try:
		data_str = read_i2c_string_block(I2C_ADDRESS)

		if ',' in data_str:
			temp, hum = data_str.split(',')
			print(f"Temperature: {temp} °C, Humidity: {hum} %")
		else:
			print("invalid data:", data_str)
	except Exception as e:
		print("Error reading I2C:", e)
	time.sleep(5)  # 5 minutes


