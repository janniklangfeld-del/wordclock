"""Fotos fuer die MJ Arcade auf die Uhr laden (bleiben nur im Flash der Uhr).

Die Fotos liegen lokal in private/fotos/ (nicht im Git, das Repo ist oeffentlich):
  gesicht.webp  Spiele-Knopf (quadratisch)
  bereit.webp   Spielhalle
  kuss.webp     Gewonnen
  muetze.webp   Game Over
  frech.webp    Verloren

Aufruf:  python scripts/fotos_hochladen.py [host] [name ...]   (Standard: wordclock.local, alle Fotos)
Das Admin-Passwort kommt aus der Benutzer-Umgebungsvariable WORDCLOCK_OTA_PASSWORD,
sonst wird es abgefragt. Alternativ in der Webseite unter
System -> Fotos fuer die Spielhalle einzeln hochladen.
"""
import base64
import getpass
import os
import sys
import urllib.request
import uuid

NAMES = ["gesicht", "bereit", "kuss", "muetze", "frech"]
host = sys.argv[1] if len(sys.argv) > 1 else "wordclock.local"
only = sys.argv[2:]
folder = os.path.join(os.path.dirname(__file__), "..", "private", "fotos")
def env_password():
    pw = os.environ.get("WORDCLOCK_OTA_PASSWORD", "")
    if not pw and sys.platform == "win32":  # frisch gesetzt -> steht nur in der Registry
        try:
            import winreg
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Environment") as k:
                pw = winreg.QueryValueEx(k, "WORDCLOCK_OTA_PASSWORD")[0]
        except OSError:
            pw = ""
    return pw


pw = env_password() or getpass.getpass("Admin-Passwort der Uhr: ")
auth = "Basic " + base64.b64encode(("admin:" + pw).encode()).decode()

for n in NAMES:
    if only and n not in only:
        continue
    path = os.path.join(folder, n + ".webp")
    if not os.path.exists(path):
        print(f"{n}: fehlt ({path})")
        continue
    data = open(path, "rb").read()
    b = uuid.uuid4().hex
    body = (f"--{b}\r\nContent-Disposition: form-data; name=\"f\"; filename=\"{n}.webp\"\r\n"
            f"Content-Type: image/webp\r\n\r\n").encode() + data + f"\r\n--{b}--\r\n".encode()
    req = urllib.request.Request(f"http://{host}/v1/photo?n={n}", data=body, method="POST",
                                 headers={"Content-Type": f"multipart/form-data; boundary={b}", "Authorization": auth})
    try:
        with urllib.request.urlopen(req, timeout=20) as r:
            print(f"{n}: ok ({len(data) // 1024} KB)")
    except Exception as e:  # noqa: BLE001
        print(f"{n}: Fehler {e}")
