#!/usr/bin/env python3
#-----------------------------------------------------------------------------#
"""
File:           db_schemas.py
Author(s):      Mika Paul Salewski
Created:        2025-11-20
Last Updated:   2025-11-26
Version:        2025.11.20

Title:
    Database schema definitions for the Raspberry Pi REST API backend.

Short Description:
    Contains Python dictionary-based schema definitions used for validating
    or describing database table fields. These schemas ensure consistent
    structure across modules interacting with the database.

License:
    CC BY-NC-SA 4.0
    See: https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en

Notes:
    • This module supports db_models.py and parts of the sensor API.
    • Schemas may be expanded in future versions for validation or ORM-like use.
"""
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Schema Definitions

# Schema describing expected fields for sensor entries
sensor_schema = {
    "sensor_type": str,   # Type/category of the sensor (e.g., "temperature")
    "value": str          # Raw sensor value stored as string
}
#-----------------------------------------------------------------------------#
