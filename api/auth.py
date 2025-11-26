#!/usr/bin/env python3
#-----------------------------------------------------------------------------#
"""
File:           auth.py
Author(s):      Mika Paul Salewski
Created:        2025-11-20
Last Updated:   2025-11-26
Version:        2025.11.20

Title:
    Authentication module for the Raspberry Pi REST API backend.

Short Description:
    Provides Basic Authentication for admin access, API-Key validation 
    for service-to-service communication, and a global authentication 
    wrapper that protects all endpoints except explicitly exempted paths.

License:
    CC BY-NC-SA 4.0
    See: https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en

Notes:
    • This module is imported by server.py.
    • Authentication is applied globally using require_auth_everywhere().
    • The module supports:
        – Basic HTTP Authentication
        – API-Key Authentication
        – Optional cookie session utilities (disabled by default)
"""
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Libs / Includes

from functools import wraps
from flask import request, abort, g, make_response, current_app
import secrets
import os

from config import API_KEY, ADMIN_USER, ADMIN_PASSWORD, REALM_NAME
# from itsdangerous import URLSafeSerializer, BadSignature   # (optional)
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Setup & Utility Functions

def _unauthorized_response(realm=REALM_NAME):
    """
    Return a proper HTTP 401 Unauthorized response including the
    WWW-Authenticate header required by Basic Authentication clients.
    """
    resp = make_response("Unauthorized", 401)
    resp.headers["WWW-Authenticate"] = f'Basic realm="{realm}"'
    return resp

#-----------------------------------------------------------------------------#
# Basic Authentication (Admin Access)
def require_basic_auth(func):
    """
    Decorator: Enforce Basic HTTP authentication for admin endpoints.
    Uses constant-time comparison for credentials.
    """
    @wraps(func)
    def wrapper(*args, **kwargs):
        auth = request.authorization
        if not auth:
            return _unauthorized_response()

        
        username_ok = secrets.compare_digest(auth.username or "", ADMIN_USER or "")
        password_ok = secrets.compare_digest(auth.password or "", ADMIN_PASSWORD or "")
        if not (username_ok and password_ok):
            return _unauthorized_response()

         # Optionally expose authenticated user to the application context
        g.user = auth.username
        return func(*args, **kwargs)

    return wrapper

#-----------------------------------------------------------------------------#
# API Key for services
def check_api_key_value(x_api_key: str | None):
    """
    Validate that an API key is provided and matches the expected value.
    On failure, abort with appropriate HTTP status code.
    """
    if not x_api_key:
        abort(401, description="Missing API key")

    if not (API_KEY and secrets.compare_digest(x_api_key, API_KEY)):
        abort(403, description="Invalid API key")

#-----------------------------------------------------------------------------#

def require_api_key(header_name="X-API-Key"):
    """
    Decorator factory: Protect an endpoint using an API key.
    Header name is configurable (default: X-API-Key).
    """
    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            # header names are case-insensitive in Flask
            x_api_key = request.headers.get(header_name)
            check_api_key_value(x_api_key)
            return func(*args, **kwargs)
        return wrapper
    return decorator

#-----------------------------------------------------------------------------#
# Cookie Session Helpers (Disabled — Optional Feature)
"""
# Example for optional session-token authentication using a serializer.

serializer = URLSafeSerializer(CHATROOM_PASSWORD)

def create_session_cookie(data: dict) -> str:
    return serializer.dumps(data)

#-----------------------------------------------------------------------------#

def verify_session_cookie(token: str):
    if not token:
        abort(403, "Not logged in")
    try:
        return serializer.loads(token)
    except BadSignature:
        abort(403, "Invalid token")
"""

#-----------------------------------------------------------------------------#
# Global Authentication Wrapper
def require_auth_everywhere(app, exempt_paths=None):
    """
    Attach a before_request hook that protects the entire Flask application.
    A request is accepted if it contains either:
        • a valid API key, OR
        • valid Basic Authentication credentials.

    exempt_paths : list[str]
        A list of URL paths that do NOT require authentication.
        Example: ['/', '/health']
    """
    exempt_paths = exempt_paths or ["/", "/health"]

    @app.before_request
    def _global_auth():
        # Skip authentication for exempted endpoints
        if request.path in exempt_paths:
            return None

        # 1) Accept valid API-Key
        x_api = request.headers.get("X-API-Key")
        if x_api and API_KEY and secrets.compare_digest(x_api, API_KEY):
            return None

        # 2) Accept valid Basic Auth
        auth = request.authorization
        if auth and secrets.compare_digest(auth.username or "", ADMIN_USER or "") and \
                secrets.compare_digest(auth.password or "", ADMIN_PASSWORD or ""):
            g.user = auth.username
            return None

        # Otherwise reject request
        return _unauthorized_response()
