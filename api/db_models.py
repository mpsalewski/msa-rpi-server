#!/usr/bin/env python3
#-----------------------------------------------------------------------------#
"""
File:           db_models.py
Author(s):      Mika Paul Salewski
Created:        2025-11-20
Last Updated:   2025-11-26
Version:        2025.11.20

Title:
    Database model definitions for the Raspberry Pi REST API backend.

Short Description:
    Defines the SQLite database tables used by the backend. Table creation 
    is performed during server startup via database.init_db().

License:
    CC BY-NC-SA 4.0
    See: https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en

Notes:
    • Only minimal schema logic is implemented here.
    • Additional models or migrations can be added as the project grows.
    • Uses SQLite's built-in CURRENT_TIMESTAMP for automatic entry timestamps.
"""
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Libs / Includes

from database import get_connection
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Function Defs

def create_tables():
    """
    Create all required database tables if they do not exist.
    Currently includes:
        • sensor_data — stores sensor values, types, and timestamp.
    """
    conn = get_connection()
    cursor = conn.cursor()

    cursor.execute('''
        CREATE TABLE IF NOT EXISTS sensor_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sensor_type TEXT,
            value TEXT,
            timestamp DATETIME DEFAULT (datetime('now','localtime'))
        )
    ''')

    conn.commit()
    conn.close()
#-----------------------------------------------------------------------------#
