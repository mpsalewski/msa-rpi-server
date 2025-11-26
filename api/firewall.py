#!/usr/bin/env python3
#-----------------------------------------------------------------------------#
"""
File:           firewall.py
Author(s):      Mika Paul Salewski
Created:        2025-11-20
Last Updated:   2025-11-26
Version:        2025.11.20

Title:
    Local UFW firewall wrapper for the Raspberry Pi REST API backend.

Short Description:
    Provides helper functions to automatically configure the UFW firewall 
    at server startup. Allows inbound traffic only from the local network 
    and only for permitted ports (such as the API server port and SSH).

License:
    CC BY-NC-SA 4.0
    See: https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en

Notes:
    • This module configures UFW *without resetting* existing rules.
    • Allows inbound API requests only from the detected LAN subnet.
    • Ensures SSH access is allowed only from the LAN, enhancing security.
    • Outgoing traffic is set to allow by default, incoming to deny.
    • setup_firewall() is called by server.py during application startup.
    • Requires UFW to be installed and the script to run with sudo rights.
"""
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Libs / Includes

import subprocess
import ipaddress
import socket
import re
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Function Defs
def get_local_network_cidr():
    """
    Determine the IPv4 address of the Raspberry Pi and return the 
    corresponding /24 subnet in CIDR notation.

    Example:
        '192.168.178.0/24'

    Notes:
        • Subnet mask is assumed to be /24 — can be expanded in the future 
          to detect real netmask dynamically.
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("10.255.255.255", 1))    # "Fake" outbound connect
        local_ip = s.getsockname()[0]
    except:
        local_ip = "127.0.0.1"
    finally:
        s.close()

    network = ipaddress.IPv4Network(local_ip + "/24", strict=False)
    return str(network)

#-----------------------------------------------------------------------------#

def run(cmd):
    """
    Execute a shell command via subprocess without raising exceptions.
    Used mainly for calling UFW commands.
    """
    print("[UFW]", cmd)
    subprocess.run(cmd, shell=True, stdout=subprocess.PIPE,
                   stderr=subprocess.PIPE)

#-----------------------------------------------------------------------------#

def ufw_rule_exists(rule_regex):
    """
    Check whether a UFW rule already exists by searching the output of
    'ufw status' using a regular expression.

    Returns:
        True  — rule exists
        False — rule does not exist
    """
    result = subprocess.run("sudo ufw status", shell=True, stdout=subprocess.PIPE)
    rules = result.stdout.decode()

    return re.search(rule_regex, rules) is not None

#-----------------------------------------------------------------------------#

def setup_firewall(port=5000):
    """
    Automatically configure UFW to protect the Raspberry Pi backend.

    Steps performed:
        1. Ensure default policies:
            - incoming: deny
            - outgoing: allow
        2. Allow the API server port (default: 5000) for the local subnet only.
        3. Allow SSH port 22 for the local subnet only.
        4. Enable UFW without resetting existing rules.

    Args:
        port (int): The API port that should be accessible in the LAN.
    """
    subnet = get_local_network_cidr()
    print(f"Detected subnet: {subnet}")

    # 1. Default policies (incoming deny / outgoing allow)
    if not ufw_rule_exists(r"Default:\s+deny\s+\(incoming\)"):
        run("sudo ufw default deny incoming")

    if not ufw_rule_exists(r"Default:\s+allow\s+\(outgoing\)"):
        run("sudo ufw default allow outgoing")

    # 2. Allow API port (e.g. 5000) from local subnet
    rule_5000 = rf"{subnet}.*ALLOW IN.*{port}"
    if not ufw_rule_exists(rule_5000):
        run(f"sudo ufw allow from {subnet} to any port {port}")

    # 3. Allow SSH access from the LAN only
    rule_ssh = rf"{subnet}.*ALLOW IN.*22"
    if not ufw_rule_exists(rule_ssh):
        run(f"sudo ufw allow from {subnet} to any port 22")

    # 4. Activate UFW safely (no reset)
    run("sudo ufw --force enable")

    print("Firewall configured without resetting existing rules.")
#-----------------------------------------------------------------------------#



#-----------------------------------------------------------------------------#
# Main (manual execution)

if __name__ == "__main__":
    setup_firewall()
#-----------------------------------------------------------------------------#