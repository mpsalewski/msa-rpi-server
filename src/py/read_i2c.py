#!/usr/bin/env python3
#-----------------------------------------------------------------------------#
"""
File:           read_i2c.py
Author(s):      Mika Paul Salewski
Created:        2025-11-21
Last Updated:   2025-11-26
Version:        2025.11.21

Title:
    I2C sensor reader for Raspberry Pi with data forwarding to REST backend.

Short Description:
    Communicates with an Arduino (or other microcontroller) over I2C using 
    smbus2. Reads sensor data strings, parses temperature and humidity values, 
    and sends them to the Flask backend via POST requests.

License:
    CC BY-NC-SA 4.0
    See: https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en

Notes:
    • Uses smbus2 to read 20-byte blocks until a '#' terminator is found.
    • Expected data format: "temperature,humidity#"
    • Values are forwarded to the backend via /sensors/add using API key auth.
    • Poll interval defaults to 300 seconds (5 minutes).
    • NETWORK EFFECTS:
          - If the REST API is unreachable, data will be skipped but the loop 
            continues (non-fatal).
          - Malformed I2C packets are ignored gracefully.
          - I2C_DEVICE and /sensors/add URL must match your backend setup.

Pinout:    
    • SDA --> Pin 3 (GPIO2)
    • SCL --> Pin 5 (GPIO3)
    • connect both GND
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



#-----------------------------------------------------------------------------#
# Main Loop

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
