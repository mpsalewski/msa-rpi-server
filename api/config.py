# config.py
import os
from dotenv import load_dotenv

# Load environment variables from .env
load_dotenv()

# get env vars
API_KEY = os.getenv("API_KEY")
ADMIN_USER = os.getenv("ADMIN_USER")
ADMIN_PASSWORD = os.getenv("ADMIN_PASSWORD")
REALM_NAME = os.getenv("REALM_NAME", "My-RPi-Server")
