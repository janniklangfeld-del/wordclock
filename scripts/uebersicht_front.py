"""Erzeugt docs/hardware/uebersicht_front_querschnitt.svg:
Buchstabenfront mit Massen und Querschnitt der eingebauten Uhr (Holzrahmen).
Masse aus cad/Wordclock_30x30_v3.scad und cad/rahmen_holz.scad.
Aufruf: python scripts/uebersicht_front.py
"""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "docs", "hardware", "uebersicht_front_querschnitt.svg")

LETTERS = ["ESKISTLFÜNF", "ZEHNZWANZIG", "DREIVIERTEL", "VORFUNKNACH", "HALBAELFÜNF",
           "EINSXAMZWEI", "DREIPMJVIER", "SECHSNLACHT", "SIEBENZWÖLF", "ZEHNEUNKUHR"]
FRONT, CELL, GX, GTOP, LETTER = 250, 20, 15, 15, 13
DOT_D, DOT_PITCH, DOT_Y = 5, 10, 17          # DOT_Y ueber Unterkante
# Stapel / Rahmen (z = 0 Rueckseite LED-Traeger)
TRAEGER, GITTER, DIFF, FRONT_T = 4, 13, 0.8, 1.8
Z_DIFF, Z_FRONT, Z_FV = TRAEGER + GITTER, TRAEGER + GITTER + DIFF, TRAEGER + GITTER + DIFF + FRONT_T  # 17 / 17.8 / 19.6
ANS_B, ANS_T, HOLZ_W, HOLZ_D, SPIEL, RW = 4, 3, 18, 50, 0.25, 3
Z_VORN = Z_FV + ANS_T            # 22.6
Z_HINT = Z_VORN - HOLZ_D         # -27.4
WALL = 1.5

o = []
def add(s): o.append(s)
def f(v): return f"{v:.2f}".rstrip("0").rstrip(".")
def de(v): return f(v).replace(".", ",")

def hdim(x1, x2, y, label, ext_from=None, cls="m", up=True):
    if ext_from is not None:
        for x in (x1, x2):
            add(f'<line class="ext" x1="{f(x)}" y1="{f(ext_from)}" x2="{f(x)}" y2="{f(y + (-1.5 if up else 1.5))}"/>')
    add(f'<line class="dim" x1="{f(x1)}" y1="{f(y)}" x2="{f(x2)}" y2="{f(y)}" marker-start="url(#a)" marker-end="url(#a)"/>')
    add(f'<text x="{f((x1 + x2) / 2)}" y="{f(y - 1.2 if up else y + 3.8)}" text-anchor="middle" class="{cls}">{label}</text>')

def vdim(y1, y2, x, label, ext_from=None, cls="m", left=False):
    if ext_from is not None:
        for y in (y1, y2):
            add(f'<line class="ext" x1="{f(ext_from)}" y1="{f(y)}" x2="{f(x + (-1.5 if left else 1.5))}" y2="{f(y)}"/>')
    add(f'<line class="dim" x1="{f(x)}" y1="{f(y1)}" x2="{f(x)}" y2="{f(y2)}" marker-start="url(#a)" marker-end="url(#a)"/>')
    tx, ty = x + (-1.6 if left else 1.6), (y1 + y2) / 2
    add(f'<text x="{f(tx)}" y="{f(ty + 1.2)}" text-anchor="{"end" if left else "start"}" class="{cls}">{label}</text>')

add('<svg xmlns="http://www.w3.org/2000/svg" width="297mm" height="210mm" viewBox="0 0 297 210" font-family="Arial, Helvetica, sans-serif">')
add('''<defs>
<marker id="a" viewBox="0 0 10 10" refX="10" refY="5" markerWidth="3" markerHeight="3" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" fill="#222"/></marker>
<pattern id="hz" width="1.6" height="1.6" patternUnits="userSpaceOnUse" patternTransform="rotate(45)"><rect width="1.6" height="1.6" fill="#e3c39a"/><line x1="0" y1="0" x2="0" y2="1.6" stroke="#b08a5c" stroke-width=".35"/></pattern>
<pattern id="hr" width="1.6" height="1.6" patternUnits="userSpaceOnUse" patternTransform="rotate(-45)"><rect width="1.6" height="1.6" fill="#efe4c8"/><line x1="0" y1="0" x2="0" y2="1.6" stroke="#b9a678" stroke-width=".3"/></pattern>
<linearGradient id="glow" x1="0" y1="1" x2="0" y2="0"><stop offset="0" stop-color="#ffd27a" stop-opacity=".75"/><stop offset="1" stop-color="#ffd27a" stop-opacity=".1"/></linearGradient>
</defs>
<style>
.dim{stroke:#222;stroke-width:.25;fill:none}.ext{stroke:#777;stroke-width:.18;fill:none}
.lead{stroke:#555;stroke-width:.2;fill:none}
.m{font-size:3.4px;font-weight:700;fill:#222}.t{font-size:3.2px;fill:#222}.s{font-size:2.7px;fill:#555}
.h{font-size:4.6px;font-weight:700;fill:#222}.lb{font-size:2.8px;fill:#222}.lbs{font-size:2.4px;fill:#666}
</style>
<rect width="297" height="210" fill="#fff"/>''')
add('<text x="14" y="13" font-size="7" font-weight="700" fill="#222">Wordclock – Buchstabenfront und Querschnitt</text>')
add('<text x="14" y="19" class="s">Maße in mm · Stand 12.11.2026 · Quelle cad/Wordclock_30x30_v3.scad + cad/rahmen_holz.scad</text>')

# ---------------- Front ----------------
S, FX, FY = 0.5, 28, 40
P = lambda v: v * S
add(f'<text x="14" y="29" class="h">Buchstabenfront 250 × 250 (Ansicht von vorn, M 1:2)</text>')
add(f'<rect x="{FX}" y="{FY}" width="{P(FRONT)}" height="{P(FRONT)}" fill="#1c1c1f" rx=".4"/>')
fs = LETTER / 0.72 * S   # Versalhoehe ~ 0,72 x Schriftgroesse
for r, row in enumerate(LETTERS):
    for c, ch in enumerate(row):
        cx = FX + P(GX + (c + 0.5) * CELL)
        cy = FY + P(GTOP + (r + 0.5) * CELL)
        add(f'<text x="{f(cx)}" y="{f(cy + P(LETTER) / 2)}" text-anchor="middle" font-size="{f(fs)}" font-weight="700" fill="#efe9dc">{ch}</text>')
for k in range(4):
    cx = FX + P(FRONT / 2 + (k - 1.5) * DOT_PITCH)
    add(f'<circle cx="{f(cx)}" cy="{f(FY + P(FRONT - DOT_Y))}" r="{f(P(DOT_D / 2))}" fill="#efe9dc"/>')
# Druckteil-Fugen und sichtbarer Bereich
cut_x, cut_y = 115.75, FRONT - 135.75
add(f'<line x1="{f(FX + P(cut_x))}" y1="{FY}" x2="{f(FX + P(cut_x))}" y2="{f(FY + P(FRONT))}" stroke="#e0565b" stroke-width=".3" stroke-dasharray="1.5 1"/>')
add(f'<line x1="{FX}" y1="{f(FY + P(cut_y))}" x2="{f(FX + P(FRONT))}" y2="{f(FY + P(cut_y))}" stroke="#e0565b" stroke-width=".3" stroke-dasharray="1.5 1"/>')
inset = ANS_B - SPIEL
add(f'<rect x="{f(FX + P(inset))}" y="{f(FY + P(inset))}" width="{f(P(FRONT - 2 * inset))}" height="{f(P(FRONT - 2 * inset))}" fill="none" stroke="#5aa0e0" stroke-width=".3" stroke-dasharray="1 .8"/>')
# Masse
R, B = FX + P(FRONT), FY + P(FRONT)
hdim(FX, R, FY - 6, "250", ext_from=FY - 0.5)
vdim(FY, B, FX - 7, "250", ext_from=FX - 0.5, left=True)
vdim(FY, FY + P(GTOP), R + 5, "15", ext_from=R + 0.5, cls="t")
vdim(FY + P(GTOP), FY + P(GTOP + 200), R + 5, "200 (10 × 20)", ext_from=R + 0.5, cls="t")
vdim(FY + P(GTOP + 200), B, R + 5, "35", ext_from=R + 0.5, cls="t")
hdim(FX, FX + P(GX), B + 6, "15", ext_from=B + 0.5, cls="t", up=False)
hdim(FX + P(GX), FX + P(GX + 220), B + 6, "220 (11 × 20)", ext_from=B + 0.5, cls="t", up=False)
hdim(FX + P(GX + 220), R, B + 6, "15", ext_from=B + 0.5, cls="t", up=False)
# Legende Front
ly = B + 15
add(f'<text x="14" y="{f(ly)}" class="t">• Feld 20 × 20 je Buchstabe, Schrift 13 hoch (DejaVu Sans Bold, Schablonen-Stege), 2 LEDs pro Feld</text>')
add(f'<text x="14" y="{f(ly + 4.6)}" class="t">• 4 Minutenpunkte Ø 5, Abstand 10, Mitte 17 über Unterkante · Front 1,8 dick, schwarz gedruckt</text>')
add(f'<text x="14" y="{f(ly + 9.2)}" class="t"><tspan fill="#e0565b">- - -</tspan> Fugen der 4 Druckteile (x 115,75 / y 135,75 von unten)   <tspan fill="#5aa0e0">- - -</tspan> sichtbar im Rahmen: 242,5 × 242,5</text>')
add(f'<text x="14" y="{f(ly + 13.8)}" class="t">• Mit Holzrahmen außen 286,5 × 286,5, Gesamttiefe 53</text>')

# ---------------- Querschnitt ----------------
K, SX, SY = 1.6, 192, 46
U0, U1 = -SPIEL - HOLZ_W, 26.75
X = lambda u: SX + (u - U0) * K
Y = lambda z: SY + (Z_VORN - z) * K
def box(u1, u2, z1, z2, fill, stroke="#333", sw=.25, extra=""):
    add(f'<rect x="{f(X(u1))}" y="{f(Y(z2))}" width="{f((u2 - u1) * K)}" height="{f((z2 - z1) * K)}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}" {extra}/>')

add(f'<text x="172" y="29" class="h">Querschnitt eingebaut (linker Rand, M 1,6:1)</text>')
add(f'<text x="{f(X(U0))}" y="{f(SY - 3)}" class="lbs">▲ vorn (Betrachter)</text>')
add(f'<text x="{f(X(U0))}" y="{f(Y(Z_HINT - RW) + 5)}" class="lbs">▼ Wand</text>')
# Elektronikraum
box(-SPIEL, U1, Z_HINT, 0, "#f6f7f9", stroke="none")
add(f'<text x="{f(X(12))}" y="{f(Y(-12))}" text-anchor="middle" class="lbs">Elektronikraum</text>')
add(f'<text x="{f(X(12))}" y="{f(Y(-12) + 3)}" text-anchor="middle" class="lbs">(ESP32, Kabel)</text>')
# Rahmen, Rueckwand, Schraube
box(U0, -SPIEL, Z_HINT, Z_VORN, "url(#hz)", stroke="#7a5530", sw=.35)
box(U0, U1, Z_HINT - RW, Z_HINT, "url(#hr)", stroke="#9a8a62")
sxu = U0 + HOLZ_W / 2
add(f'<rect x="{f(X(sxu) - 1.4)}" y="{f(Y(Z_HINT - RW + 16))}" width="2.8" height="{f(16 * K)}" fill="#9aa0a8" stroke="#555" stroke-width=".2"/>')
add(f'<rect x="{f(X(sxu) - 2.6)}" y="{f(Y(Z_HINT - RW) - 0.6)}" width="5.2" height="1.2" fill="#9aa0a8" stroke="#555" stroke-width=".2"/>')
# Eckwinkel
box(-SPIEL, -SPIEL + 5, Z_HINT, Z_HINT + 12, "#3a3b40")
# LED-Traeger, Streifen, LED, Lichtgitter, Diffusor, Front, Anschlag
box(0, U1, 0, TRAEGER, "#c9a77f", stroke="#7a5530")
box(14.25, 15.75, TRAEGER + 3.2, Z_DIFF, "#2b2c30")
box(13, U1, TRAEGER, TRAEGER + 0.4, "#fafafa", stroke="#888", sw=.15)
box(17.5, 22.5, TRAEGER + 0.4, TRAEGER + 2, "#f2f2f2", stroke="#666", sw=.2)
add(f'<rect x="{f(X(15.75))}" y="{f(Y(Z_DIFF))}" width="{f((U1 - 15.75) * K)}" height="{f((Z_DIFF - TRAEGER - 2) * K)}" fill="url(#glow)"/>')
box(0, U1, Z_DIFF, Z_FRONT, "#ffffff", stroke="#999", sw=.2)
box(0, 20.5, Z_FRONT, Z_FV, "#1c1c1f")
add(f'<text x="{f(X(23.6))}" y="{f(Y(Z_FV) - 1)}" text-anchor="middle" class="lbs">Buchstabe</text>')
box(-SPIEL, ANS_B - SPIEL, Z_FV, Z_VORN, "#111")
# Bruchkante rechts
xb = X(U1)
add(f'<path d="M{f(xb)} {f(SY - 1)} l1 3 l-2 3 l2 3 l-1 3" class="lead"/>')
add(f'<line x1="{f(xb)}" y1="{f(SY)}" x2="{f(xb)}" y2="{f(Y(Z_HINT - RW))}" stroke="#999" stroke-width=".2" stroke-dasharray="1 1"/>')

# Hoehenmasse links
xa, xb2 = X(U0) - 4, X(U0) - 11
vdim(Y(Z_VORN), Y(Z_FV), xa, "3", ext_from=X(U0) - 0.5, cls="t", left=True)
vdim(Y(Z_FV), Y(0), xa, "19,6", cls="t", left=True)
vdim(Y(0), Y(Z_HINT), xa, "27,4", cls="t", left=True)
vdim(Y(Z_HINT), Y(Z_HINT - RW), xa, "3", ext_from=X(U0) - 0.5, cls="t", left=True)
for z in (Z_FV, 0):
    add(f'<line class="ext" x1="{f(X(0))}" y1="{f(Y(z))}" x2="{f(xa - 1.5)}" y2="{f(Y(z))}"/>')
vdim(Y(Z_VORN), Y(Z_HINT - RW), xb2, "53", cls="m", left=True)
# Breitenmass Rahmen
hdim(X(U0), X(-SPIEL), SY - 6.5, "18", cls="t")
hdim(X(-SPIEL), X(ANS_B - SPIEL), SY - 6.5, "4", cls="t")

# Beschriftungen rechts
LX = 268
labels = [
    (ANS_B / 2 - SPIEL, (Z_FV + Z_VORN) / 2, 39, "Anschlag 4 × 3", "schwarz, Front 3 zurück"),
    (8, (Z_FRONT + Z_FV) / 2, 49, "Front 1,8", "Buchstaben offen"),
    (8, (Z_DIFF + Z_FRONT) / 2, 56, "Diffusor 0,8", "oder Backpapier"),
    (15, 12, 64, "Lichtgitter 13", "Wände 1,5, unten 3,2 frei"),
    (20, TRAEGER + 1.2, 72, "LED-Streifen", "WS2812B, ca. 2 hoch"),
    (8, TRAEGER / 2, 80, "LED-Träger 4", "Streifen aufgeklebt"),
    (2, Z_HINT + 6, 106, "Eckwinkel 12", "gedruckt, hält die Gehrung"),
    (-SPIEL - 9, Z_HINT + 8, 115, "Spax 3 × 16", "Rückwand, 8 Stück"),
    (10, Z_HINT - RW / 2, 125, "Rückwand 3", "Pappel-Sperrholz"),
    (-SPIEL - 14, -3, 94, "Buche 18 × 50", "45° Gehrung"),
]
for u, z, ty, a, b in labels:
    add(f'<polyline class="lead" points="{f(X(u))},{f(Y(z))} {f(LX - 2)},{f(ty - 1)} {f(LX - 0.5)},{f(ty - 1)}"/>')
    add(f'<circle cx="{f(X(u))}" cy="{f(Y(z))}" r=".45" fill="#555"/>')
    add(f'<text x="{LX}" y="{f(ty)}" class="lb" font-weight="700">{a}</text>')
    add(f'<text x="{LX}" y="{f(ty + 3)}" class="lbs">{b}</text>')

# Tabelle Tiefen
ty = 146
add(f'<text x="172" y="{ty}" class="h" font-size="4">Tiefe ab Rahmen-Vorderkante</text>')
rows = [("Front vorn", ANS_T), ("Front hinten / Diffusor", Z_VORN - Z_FRONT), ("Lichtgitter vorn", Z_VORN - Z_DIFF),
        ("LED-Träger vorn (LEDs)", Z_VORN - TRAEGER), ("LED-Träger hinten", Z_VORN), ("Rahmen hinten", HOLZ_D),
        ("Rückwand hinten = Gesamttiefe", HOLZ_D + RW)]
for i, (n, v) in enumerate(rows):
    yy = ty + 6 + i * 4.4
    add(f'<text x="174" y="{f(yy)}" class="t">{n}</text>')
    add(f'<text x="232" y="{f(yy)}" text-anchor="end" class="t" font-weight="700">{de(v)}</text>')
add(f'<text x="238" y="{ty + 6}" class="s">Lichtweg Träger → Diffusor: 13</text>')
add(f'<text x="238" y="{ty + 10.4}" class="s">Elektronik: 27,4 frei,</text>')
add(f'<text x="238" y="{ty + 13.6}" class="s">ESP32-Platine 22 hoch</text>')
add(f'<text x="238" y="{ty + 18}" class="s">8 Andruckklötze 25,4 +</text>')
add(f'<text x="238" y="{ty + 21.2}" class="s">3 Moosgummi drücken den</text>')
add(f'<text x="238" y="{ty + 24.4}" class="s">Stapel nach vorn (nicht</text>')
add(f'<text x="238" y="{ty + 27.6}" class="s">über die Platine setzen)</text>')
add(f'<text x="238" y="{ty + 32}" class="s">Luft Front ↔ Holz: 0,25 je Seite</text>')
add('</svg>')

with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
    fh.write("\n".join(o))
print(OUT)
