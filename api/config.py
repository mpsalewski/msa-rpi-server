#!/usr/bin/env python3
#-----------------------------------------------------------------------------#
"""
File:           config.py
Author(s):      Mika Paul Salewski
Created:        2025-11-20
Last Updated:   2025-11-26
Version:        2025.11.20

Title:
    Environment variable configuration for the Raspberry Pi REST API backend.

Short Description:
    Loads secret values and configuration settings from a .env file using 
    python-dotenv. Provides a centralized location for authentication 
    credentials and server-related configuration constants.

License:
    CC BY-NC-SA 4.0
    See: https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en

Notes:
    • The .env file must exist in the project root.
    • Environment variables include API key, admin credentials, 
      and the HTTP authentication realm name.
    • Values are imported by auth.py and other modules at runtime.
"""
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Libs / Includes

import os
from dotenv import load_dotenv
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Load Environment Variables

# Load key-value pairs from .env file into process environment
load_dotenv()

# Retrieve environment variables used by the backend
API_KEY        = os.getenv("API_KEY")
ADMIN_USER     = os.getenv("ADMIN_USER")
ADMIN_PASSWORD = os.getenv("ADMIN_PASSWORD")
REALM_NAME     = os.getenv("REALM_NAME", "My-RPi-Server")
#-----------------------------------------------------------------------------#
