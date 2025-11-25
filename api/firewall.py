# firewall.py
import subprocess
import ipaddress
import socket
import re

def get_local_network_cidr():
    """Ermittelt IP + Subnetz und liefert CIDR (z.B. 192.168.178.0/24)."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("10.255.255.255", 1))
        local_ip = s.getsockname()[0]
    except:
        local_ip = "127.0.0.1"
    finally:
        s.close()

    # Standard-Subnetzmaske /24 – falls du willst, kann ich dynamisch auslesen.
    network = ipaddress.IPv4Network(local_ip + "/24", strict=False)
    return str(network)


def run(cmd):
    """Führt Shell-Kommandos aus (ohne Exception-Abbruch)."""
    print("[UFW]", cmd)
    subprocess.run(cmd, shell=True, stdout=subprocess.PIPE,
                   stderr=subprocess.PIPE)


def ufw_rule_exists(rule_regex):
    """Prüft ob eine Regel bereits existiert."""
    result = subprocess.run("sudo ufw status", shell=True, stdout=subprocess.PIPE)
    rules = result.stdout.decode()

    return re.search(rule_regex, rules) is not None


def setup_firewall(port=5000):
    subnet = get_local_network_cidr()
    print(f"Detected subnet: {subnet}")

    # 1. Standardpolicies NICHT anfassen (kein reset!)
    #    Optional: default incoming deny – nur falls nicht gesetzt
    if not ufw_rule_exists(r"Default:\s+deny\s+\(incoming\)"):
        run("sudo ufw default deny incoming")

    if not ufw_rule_exists(r"Default:\s+allow\s+\(outgoing\)"):
        run("sudo ufw default allow outgoing")

    # 2. Port 5000 für dein Subnetz erlauben
    rule_5000 = rf"{subnet}.*ALLOW IN.*{port}"
    if not ufw_rule_exists(rule_5000):
        run(f"sudo ufw allow from {subnet} to any port {port}")

    # 3. SSH Port 22 für dein LAN erlauben (falls noch nicht gesetzt)
    rule_ssh = rf"{subnet}.*ALLOW IN.*22"
    if not ufw_rule_exists(rule_ssh):
        run(f"sudo ufw allow from {subnet} to any port 22")

    # 4. Firewall aktivieren
    run("sudo ufw --force enable")

    print("Firewall configured without resetting.")


if __name__ == "__main__":
    setup_firewall()
