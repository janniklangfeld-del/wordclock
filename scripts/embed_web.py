# PlatformIO pre-script: packt web/index.html per gzip in src/app/web_index.h.
#
# Die Webseite ist eine einzige Datei (HTML + CSS + JS, keine externen Quellen),
# damit sie auch ohne Internet (z. B. im Einrichtungsportal) vollstaendig laedt.
# Die Header-Datei wird bei jedem Build neu erzeugt und ist nicht im Git.
import gzip
import os

Import("env")  # noqa: F821

project = env["PROJECT_DIR"]  # noqa: F821
src = os.path.join(project, "web", "index.html")
dst = os.path.join(project, "src", "app", "web_index.h")

with open(src, "rb") as f:
    raw = f.read()
packed = gzip.compress(raw, compresslevel=9, mtime=0)

lines = []
for i in range(0, len(packed), 20):
    lines.append("  " + ", ".join("0x%02x" % b for b in packed[i:i + 20]) + ",")
header = (
    "// AUTOMATISCH ERZEUGT aus web/index.html durch scripts/embed_web.py – nicht bearbeiten.\n"
    "#pragma once\n#include <Arduino.h>\n\n"
    "const size_t WEB_INDEX_GZ_LEN = %d;\n"
    "const uint8_t WEB_INDEX_GZ[] PROGMEM = {\n%s\n};\n" % (len(packed), "\n".join(lines))
)

old = None
if os.path.exists(dst):
    with open(dst, "r", encoding="utf-8") as f:
        old = f.read()
if old != header:
    with open(dst, "w", encoding="utf-8", newline="\n") as f:
        f.write(header)
print("[embed_web.py] index.html %d Bytes -> gzip %d Bytes" % (len(raw), len(packed)))
