# auth.py
from functools import wraps
from flask import request, abort, g, make_response, current_app
import os
from config import API_KEY, ADMIN_USER, ADMIN_PASSWORD, REALM_NAME
import secrets
from itsdangerous import URLSafeSerializer, BadSignature


# Serializer für Cookie-Sessions
#serializer = URLSafeSerializer(CHATROOM_PASSWORD)

def _unauthorized_response(realm=REALM_NAME):
    resp = make_response("Unauthorized", 401)
    resp.headers["WWW-Authenticate"] = f'Basic realm="{realm}"'
    return resp

# ----------------------------
# Basic HTTP auth (Admin)
# ----------------------------
def require_basic_auth(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        auth = request.authorization
        if not auth:
            return _unauthorized_response()

        # sichere, zeitkonstante Vergleiche
        username_ok = secrets.compare_digest(auth.username or "", ADMIN_USER or "")
        password_ok = secrets.compare_digest(auth.password or "", ADMIN_PASSWORD or "")
        if not (username_ok and password_ok):
            return _unauthorized_response()
        # optional: setze g.user
        g.user = auth.username
        return func(*args, **kwargs)
    return wrapper

# ----------------------------
# API Key for services
# ----------------------------
def check_api_key_value(x_api_key: str | None):
    if not x_api_key:
        abort(401, description="Missing API key")
    if not (API_KEY and secrets.compare_digest(x_api_key, API_KEY)):
        abort(403, description="Invalid API key")

def require_api_key(header_name="X-API-Key"):
    """Decorator factory: @require_api_key()"""
    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            # header names are case-insensitive in Flask
            x_api_key = request.headers.get(header_name)
            check_api_key_value(x_api_key)
            return func(*args, **kwargs)
        return wrapper
    return decorator

# ----------------------------
# Cookie session helpers
# ----------------------------
"""
def create_session_cookie(data: dict) -> str:
    return serializer.dumps(data)

def verify_session_cookie(token: str):
    if not token:
        abort(403, "Not logged in")
    try:
        data = serializer.loads(token)
    except BadSignature:
        abort(403, "Invalid token")
    return data
"""


# ----------------------------
# Optional: global protection (alles schützen)
# ----------------------------
def require_auth_everywhere(app, exempt_paths=None):
    """Registriere app.before_request um ALLE Pfade zu schützen.
       Exempt_paths = list of paths allowed without auth (e.g. '/', '/health').
       Auth logic: Accept either valid API-Key OR valid Basic Auth.
    """
    exempt_paths = exempt_paths or ["/", "/health"]

    @app.before_request
    def _global_auth():
        if request.path in exempt_paths:
            return None

        # 1) API Key accepted
        x_api = request.headers.get("X-API-Key")
        if x_api and API_KEY and secrets.compare_digest(x_api, API_KEY):
            return None

        # 2) Basic Auth accepted
        auth = request.authorization
        if auth and secrets.compare_digest(auth.username or "", ADMIN_USER or "") and \
                secrets.compare_digest(auth.password or "", ADMIN_PASSWORD or ""):
            g.user = auth.username
            return None

        return _unauthorized_response()
