#!/usr/bin/env python3
#-----------------------------------------------------------------------------#
"""
File: send_data.py
Author(s):      Mika Paul Salewski
Created:        2025-11-26
Last Updated:   2025-11-26
Version:        2025.11.26

Title:
    Test data sender for Raspberry Pi REST API backend.

Short Description:
    This module provides a simple interface to send test sensor data
    to the Flask backend. It uses HTTP POST requests with an API key
    for authentication.

License:
    CC BY-NC-SA 4.0
    See: https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en

Usage Instructions:
    Run the script directly:
        $ python3 src/py/send_data.py

    The script will send example sensor data to the local API server.
"""
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Libs / Includes

import requests
import sys
import os

# Add /api folder to path to allow relative import of config.py
sys.path.append(os.path.join(os.path.dirname(__file__), "../../api"))
import config  # Contains config.API_KEY for authentication
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Constants

# API endpoint for sending sensor data
SERVER = "http://localhost:5000/sensors/add"
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Function Defs

def send(sensor_type: str, value):
    """
    Send sensor data to the backend API.

    Args:
        sensor_type (str): Type of the sensor (e.g., 'temperature', 'humidity').
        value: The value to send (can be int, float, or str).

    Behavior:
        Sends a POST request to the backend with API key authentication.
        Prints the HTTP status code and server response.
    """
    # Construct data payload
    data = {
        "sensor_type": sensor_type,
        "value": value
    }
    
    # Include API key in request headers
    headers = {
        "X-API-Key": config.API_KEY
    }
    
    # Send POST request
    try:
        response = requests.post(SERVER, data=data, headers=headers)
        print(f"[INFO] Sent {sensor_type}={value}")
        print(f"[INFO] Status: {response.status_code}")
        print(f"[INFO] Response: {response.text}")
    except requests.RequestException as e:
        print(f"[ERROR] Failed to send data: {e}")
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Main / Example Usage

if __name__ == "__main__":
    # Send example sensor readings
    #send("temperature", 23.8)
    #send("room_entrance", 1)
    
    #send("apartment_traffic", 0.0)
    #send("apartment_traffic", 1.0)    

    send("bathroom_main", 0.0)    
    #send("bathroom_main", 1.0)    
#-----------------------------------------------------------------------------#
