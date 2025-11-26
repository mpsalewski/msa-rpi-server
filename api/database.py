#!/usr/bin/env python3
#-----------------------------------------------------------------------------#
"""
File:           database.py
Author(s):      Mika Paul Salewski
Created:        2025-11-20
Last Updated:   2025-11-26
Version:        2025.11.20

Title:
    SQLite database helper module for the Raspberry Pi REST API backend.

Short Description:
    Provides a central interface for obtaining database connections and 
    initializing all required tables used by the backend service.

License:
    CC BY-NC-SA 4.0
    See: https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en

Notes:
    • The database file is stored locally as server.db.
    • All table creation logic is delegated to db_models.py.
    • Used by server.py during the startup process.
"""
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Libs / Includes

import sqlite3
import os
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Setup

# Absolute path to the SQLite database file (located in project root)
DB_FILE = "server.db"
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Function Defs

def get_connection():
    """
    Return a thread-safe SQLite3 connection.

    check_same_thread=False:
        Allows database access from different threads. 
        Required because Flask routes may run in separate threads.
    """
    conn = sqlite3.connect(DB_FILE, check_same_thread=False)
    return conn

#-----------------------------------------------------------------------------#

def init_db():
    """
    Initialize the database and create all required tables.
    The actual table definitions live in db_models.py.
    """
    from db_models import create_tables
    create_tables()

#-----------------------------------------------------------------------------#