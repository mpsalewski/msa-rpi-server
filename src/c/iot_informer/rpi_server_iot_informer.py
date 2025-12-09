#!/usr/bin/env python3
#-----------------------------------------------------------------------------#
"""
File:           write_i2c.py
Author(s):      Mika Paul Salewski
Created:        2025-12-09
Last Updated:   2025-12-09
Version:        2025.12.09

Title:
    I2C sensor writer for Raspberry Pi with server polling.

Short Description:
    Queries the latest value for 'bathroom_main' from a REST backend
    and sends it via I2C to an Arduino (or other microcontroller)
    acting as I2C slave.

License:
    CC BY-NC-SA 4.0
    See: https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en

Notes:
    • Uses smbus2 to write data to Arduino over I2C.
    • Polls the REST API every 10 seconds.
    • Sends data in CSV format: "sensor_type,value#"
    • Network or server errors are handled gracefully.
"""
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Libs / Includes

import smbus2 as smbus
import time
import requests
import sys
import os

# Allow relative import of config.py from /api
sys.path.append(os.path.join(os.path.dirname(__file__), "../../api"))
import config
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# I2C Configuration

I2C_BUS = 1                # Raspberry Pi bus index
I2C_ADDRESS = 0x08         # Must match Arduino/MCU address
bus = smbus.SMBus(I2C_BUS)
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# I2C Read Logic

def read_i2c_string_block(addr):
    """
    Reads 20-byte blocks repeatedly from the I2C device until a '#' terminator 
    is encountered. Returns the collected ASCII string.

    Example incoming format: "23.1,45.8#"
    """
    data_bytes = []

    while True:
        block = bus.read_i2c_block_data(I2C_ADDRESS, 0, 20)

        for b in block:
            if chr(b) == '#':
                return ''.join(chr(x) for x in data_bytes)

            data_bytes.append(b)
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# I2C Write Logic

def send_i2c_status(sensor_type, value):
    """
    Sends sensor value to Arduino via I2C.
    Format: "sensor_type,value#"
    """
    message = f"{sensor_type},{value}#"
    data_bytes = [ord(c) for c in message]
    
    try:
        bus.write_i2c_block_data(I2C_ADDRESS, 0, data_bytes)
        print(f"Sent to I2C -> {message}")
    except Exception as e:
        print(f"Error sending I2C data: {e}")
#-----------------------------------------------------------------------------#




#-----------------------------------------------------------------------------#
# Server Communication

def send_to_server(sensor_type, value):
    """
    Sends one sensor value to the backend using POST /sensors/add.
    Includes the API key header for authentication.
    """
    data = {"sensor_type": sensor_type, "value": value}
    headers = {"X-API-Key": config.API_KEY}

    try:
        r = requests.post(
            "http://localhost:5000/sensors/add",
            data=data,
            headers=headers,
            timeout=5
        )
        r.raise_for_status()
        print(f"Sent {sensor_type}: {value}")

    except requests.RequestException as e:
        print(f"Error sending {sensor_type}: {e}")

#-----------------------------------------------------------------------------#

def get_latest_bathroom_status(limit=1):
    """
    Queries the REST API for the latest 'bathroom_main' sensor values.
    Returns the most recent value (0.0 or 1.0) or None if an error occurs.
    """
    try:
        r = requests.get(
            f"http://localhost:5000/sensors/get?sensor_type=bathroom_main&limit={limit}",
            headers={"X-API-Key": config.API_KEY},
            timeout=5
        )
        r.raise_for_status()
        data = r.json()
        if data:
            # Take the first (latest) entry
            return float(data[0]["value"])
        return None
    except Exception as e:
        print(f"Error fetching bathroom_main status: {e}")
        return None
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Main Loop
def main(poll_interval=10):
    """
    Polls server for bathroom_main status every poll_interval seconds
    and sends the value to Arduino via I2C.
    """
    while True:
        status = get_latest_bathroom_status(limit=1)
        if status is not None:
            send_i2c_status("bathroom_main", status)
        time.sleep(poll_interval)




if False:
    def main(poll_interval=300):
        """
        Main read → parse → send loop.
        Default interval: 5 minutes.
        """
        while True:
            try:
                data_str = read_i2c_string_block(I2C_ADDRESS)

                if ',' in data_str:
                    temp, hum = data_str.split(',')
                    send_to_server("temperature_msa_room", temp)
                    send_to_server("humidity_msa_room", hum)
                else:
                    print("Invalid I2C data:", data_str)

            except Exception as e:
                print("Error reading I2C:", e)

            time.sleep(poll_interval)
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Entrypoint

if __name__ == "__main__":
    main()
#-----------------------------------------------------------------------------#
