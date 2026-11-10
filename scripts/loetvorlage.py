#!/usr/bin/env python3
"""1:1-Loetvorlage fuer die LED-Streifen der Wordclock (PDF, 2 x A4 hoch).

Aufruf im Projektordner:  python scripts/loetvorlage.py
Erzeugt docs/hardware/loetvorlage_start_oben_links.pdf und ..._rechts.pdf.

Die Geometrie stammt aus cad/Wordclock_30x30_v3.scad (Front 250 x 250 mm,
Buchstabenraster 11 x 10 a 20 mm ab x=15/y=35, LEDs bei +5/+15 mm je Zelle,
Minutenpunkte bei y=17 mit 10 mm Pitch). Ansicht = LED-Seite oben = Vorderansicht
der Uhr, NICHT gespiegelt. Keine externen Bibliotheken noetig.
"""
import os

# --- Geometrie (aus der SCAD-Datei) ------------------------------------------
FRONT = 250
COLS, ROWS, CELL = 11, 10, 20
GX, GY = 15, 35
STRIP_W = 10
LEDS_PER_ROW = 22
DOT_Y, DOT_PITCH = 17, 10
LETTERS = [
    "ESKISTLFÜNF", "ZEHNZWANZIG", "DREIVIERTEL", "VORFUNKNACH", "HALBAELFÜNF",
    "EINSXAMZWEI", "DREIPMJVIER", "SECHSNLACHT", "SIEBENZWÖLF", "ZEHNEUNKUHR",
]
GRID_L, GRID_R = GX, GX + COLS * CELL          # 15 .. 235
JOG = 6                                          # Ausladung der Wendeverbinder


def row_y(r):            # r = 0 ist die oberste Reihe
    return GY + (ROWS - r - 0.5) * CELL


def dot_x(k):
    return FRONT / 2 + (k - 1.5) * DOT_PITCH


# --- Farben -------------------------------------------------------------------
BLACK = (0.11, 0.14, 0.2)
GREY = (0.45, 0.5, 0.58)
LIGHT = (0.78, 0.81, 0.86)
STRIP = (0.93, 0.95, 0.98)
RED = (0.84, 0.16, 0.16)
ORANGE = (0.91, 0.35, 0.05)
GREEN = (0.17, 0.58, 0.28)
WHITE = (1, 1, 1)


# --- Minimaler PDF-Zeichner (Einheit: mm) --------------------------------------
def _w(ch):
    if ch in "iljI.,:;|!'": return 0.26
    if ch in "MW": return 0.88
    if ch == " ": return 0.28
    if ch.isdigit(): return 0.56
    if ch.isupper() or ch in "ÄÖÜ": return 0.68
    return 0.52


def text_width(s, size, bold=False):
    return sum(_w(c) for c in s) * size * (1.06 if bold else 1.0)


class Canvas:
    def __init__(self):
        self.ops = []

    def raw(self, s):
        self.ops.append(s.encode("latin-1") if isinstance(s, str) else s)

    def stroke(self, rgb, width=0.3, dash=None):
        self.raw("%.3f %.3f %.3f RG %.3f w %s 0 d" % (*rgb, width, "[%s]" % " ".join(map(str, dash)) if dash else "[]"))

    def fill(self, rgb):
        self.raw("%.3f %.3f %.3f rg" % rgb)

    def polyline(self, pts, rgb, width=0.3, dash=None):
        self.stroke(rgb, width, dash)
        self.raw("%.3f %.3f m " % pts[0] + " ".join("%.3f %.3f l" % p for p in pts[1:]) + " S")

    def rect(self, x, y, w, h, fill=None, stroke=None, width=0.3, dash=None):
        if fill: self.fill(fill)
        if stroke: self.stroke(stroke, width, dash)
        op = "B" if fill and stroke else ("f" if fill else "S")
        self.raw("%.3f %.3f %.3f %.3f re %s" % (x, y, w, h, op))

    def circle(self, cx, cy, r, fill=None, stroke=None, width=0.3):
        k = 0.5523 * r
        if fill: self.fill(fill)
        if stroke: self.stroke(stroke, width)
        p = "%.3f %.3f m " % (cx + r, cy)
        p += "%.3f %.3f %.3f %.3f %.3f %.3f c " % (cx + r, cy + k, cx + k, cy + r, cx, cy + r)
        p += "%.3f %.3f %.3f %.3f %.3f %.3f c " % (cx - k, cy + r, cx - r, cy + k, cx - r, cy)
        p += "%.3f %.3f %.3f %.3f %.3f %.3f c " % (cx - r, cy - k, cx - k, cy - r, cx, cy - r)
        p += "%.3f %.3f %.3f %.3f %.3f %.3f c " % (cx + k, cy - r, cx + r, cy - k, cx + r, cy)
        self.raw(p + ("B" if fill and stroke else ("f" if fill else "S")))

    def triangle(self, x, y, direction, size, rgb):
        d = 1 if direction > 0 else -1
        self.fill(rgb)
        self.raw("%.3f %.3f m %.3f %.3f l %.3f %.3f l f" % (
            x + d * size / 2, y, x - d * size / 2, y + size / 2, x - d * size / 2, y - size / 2))

    def text(self, x, y, s, size, rgb=BLACK, bold=False, align="l"):
        w = text_width(s, size, bold)
        if align == "c": x -= w / 2
        elif align == "r": x -= w
        b = s.encode("cp1252").replace(b"\\", b"\\\\").replace(b"(", b"\\(").replace(b")", b"\\)")
        self.fill(rgb)
        self.raw(b"BT /%s %.3f Tf %.3f %.3f Td (" % (b"F2" if bold else b"F1", size, x, y) + b + b") Tj ET")

    def push(self): self.raw("q")
    def pop(self): self.raw("Q")
    def translate(self, dx, dy): self.raw("1 0 0 1 %.3f %.3f cm" % (dx, dy))
    def clip(self, x, y, w, h): self.raw("%.3f %.3f %.3f %.3f re W n" % (x, y, w, h))

    def stream(self):
        return b"72 25.4 div 0 0 72 25.4 div 0 0 cm\n".replace(b"72 25.4 div", b"2.834646") + b"\n".join(self.ops)


class SvgCanvas:
    """Gleiche Zeichenbefehle wie Canvas, aber als SVG (Vorschau fuer die Doku)."""

    def __init__(self, x0, y0, w, h):
        self.x0, self.y0, self.w, self.h = x0, y0, w, h
        self.els = []

    def _p(self, x, y):
        return x - self.x0, self.y0 + self.h - y

    @staticmethod
    def _c(rgb):
        return "rgb(%d,%d,%d)" % tuple(round(v * 255) for v in rgb)

    def _s(self, rgb, width, dash):
        s = 'stroke="%s" stroke-width="%.2f"' % (self._c(rgb), width)
        return s + (' stroke-dasharray="%s"' % " ".join(map(str, dash)) if dash else "")

    def polyline(self, pts, rgb, width=0.3, dash=None):
        p = " ".join("%.2f,%.2f" % self._p(*q) for q in pts)
        self.els.append('<polyline points="%s" fill="none" %s/>' % (p, self._s(rgb, width, dash)))

    def rect(self, x, y, w, h, fill=None, stroke=None, width=0.3, dash=None):
        px, py = self._p(x, y + h)
        self.els.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="%s" %s/>' % (
            px, py, w, h, self._c(fill) if fill else "none", self._s(stroke, width, dash) if stroke else ""))

    def circle(self, cx, cy, r, fill=None, stroke=None, width=0.3):
        px, py = self._p(cx, cy)
        self.els.append('<circle cx="%.2f" cy="%.2f" r="%.2f" fill="%s" %s/>' % (
            px, py, r, self._c(fill) if fill else "none", self._s(stroke, width, None) if stroke else ""))

    def triangle(self, x, y, direction, size, rgb):
        d = 1 if direction > 0 else -1
        pts = [(x + d * size / 2, y), (x - d * size / 2, y + size / 2), (x - d * size / 2, y - size / 2)]
        self.els.append('<polygon points="%s" fill="%s"/>' % (
            " ".join("%.2f,%.2f" % self._p(*q) for q in pts), self._c(rgb)))

    def text(self, x, y, s, size, rgb=BLACK, bold=False, align="l"):
        px, py = self._p(x, y)
        anchor = {"l": "start", "c": "middle", "r": "end"}[align]
        s = s.replace("&", "&amp;").replace("<", "&lt;").replace('"', "&quot;")
        self.els.append('<text x="%.2f" y="%.2f" font-size="%.2f" fill="%s" text-anchor="%s"%s>%s</text>' % (
            px, py, size, self._c(rgb), anchor, ' font-weight="700"' if bold else "", s))

    def svg(self):
        return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%dmm" height="%dmm" '
                'font-family="Helvetica, Arial, sans-serif">\n<rect width="100%%" height="100%%" fill="#fff"/>\n'
                % (self.w, self.h, self.w, self.h) + "\n".join(self.els) + "\n</svg>\n")


def write_pdf(path, pages):
    objs = [None,
            b"<< /Type /Catalog /Pages 2 0 R >>",
            None,
            b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>",
            b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>"]
    kids = []
    for c in pages:
        data = c.stream()
        objs.append(b"<< /Length %d >>\nstream\n" % len(data) + data + b"\nendstream")
        content_id = len(objs) - 1
        objs.append(b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595.276 841.890] "
                    b"/Resources << /Font << /F1 3 0 R /F2 4 0 R >> >> /Contents %d 0 R >>" % content_id)
        kids.append(len(objs) - 1)
    objs[2] = b"<< /Type /Pages /Kids [%s] /Count %d >>" % (b" ".join(b"%d 0 R" % k for k in kids), len(kids))
    out = bytearray(b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n")
    offsets = []
    for i in range(1, len(objs)):
        offsets.append(len(out))
        out += b"%d 0 obj\n" % i + objs[i] + b"\nendobj\n"
    xref = len(out)
    out += b"xref\n0 %d\n0000000000 65535 f \n" % len(objs)
    out += b"".join(b"%010d 00000 n \n" % o for o in offsets)
    out += b"trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%d\n%%%%EOF\n" % (len(objs), xref)
    with open(path, "wb") as f:
        f.write(out)


# --- Vorlage zeichnen (Koordinaten = Front, mm, y nach oben) -------------------
def draw_template(c, start_left):
    def row_dir(r):  # +1 = von links nach rechts
        return (1 if r % 2 == 0 else -1) * (1 if start_left else -1)

    def end_side_right(r):
        return row_dir(r) > 0

    # Frontumriss und Buchstabenraster
    c.rect(0, 0, FRONT, FRONT, stroke=GREY, width=0.4)
    for col in range(COLS + 1):
        c.polyline([(GX + col * CELL, GY), (GX + col * CELL, GY + ROWS * CELL)], LIGHT, 0.2)
    for r in range(ROWS + 1):
        c.polyline([(GRID_L, GY + r * CELL), (GRID_R, GY + r * CELL)], LIGHT, 0.2)

    for r in range(ROWS):
        y = row_y(r)
        d = row_dir(r)
        # Streifen + Schnittlinien
        c.rect(GRID_L, y - STRIP_W / 2, GRID_R - GRID_L, STRIP_W, fill=STRIP, stroke=GREY, width=0.25)
        for x in (GRID_L, GRID_R):
            c.polyline([(x, y - STRIP_W / 2 - 1.5), (x, y + STRIP_W / 2 + 1.5)], RED, 0.35, dash=(1.2, 0.8))
        for col in range(COLS):
            cx0 = GX + col * CELL
            c.text(cx0 + CELL / 2, y + 6.2, LETTERS[r][col], 3.0, GREY, bold=True, align="c")
            for i in range(2):
                lx = cx0 + 5 + i * 10
                pos = col * 2 + i
                idx = r * LEDS_PER_ROW + (pos if d > 0 else LEDS_PER_ROW - 1 - pos)
                c.rect(lx - 2.5, y - 2.5, 5, 5, fill=WHITE, stroke=BLACK, width=0.25)
                c.text(lx, y - 8.6, str(idx), 1.7, RED if idx == 0 else GREY, bold=idx == 0, align="c")
            if col > 0:
                c.triangle(cx0, y, d, 2.2, ORANGE)
        # Reihenbeschriftung beidseitig
        c.text(-1.5, y - 0.9, "Reihe %d" % (r + 1), 2.4, BLACK, bold=True, align="r")
        c.text(FRONT + 1.5, y - 0.9, "Reihe %d" % (r + 1), 2.4, BLACK, bold=True)

    # Wendeverbinder zwischen den Reihen
    for r in range(ROWS - 1):
        y1, y2 = row_y(r), row_y(r + 1)
        if end_side_right(r):
            pts = [(GRID_R, y1), (GRID_R + JOG, y1), (GRID_R + JOG, y2), (GRID_R, y2)]
        else:
            pts = [(GRID_L, y1), (GRID_L - JOG, y1), (GRID_L - JOG, y2), (GRID_L, y2)]
        c.polyline(pts, BLACK, 0.7)

    # Minutenpunkte (4er-Stueck) + Verbinder von Reihe 10
    mx0, mx1 = dot_x(0) - 5, dot_x(3) + 5
    c.rect(mx0, DOT_Y - STRIP_W / 2, mx1 - mx0, STRIP_W, fill=STRIP, stroke=GREY, width=0.25)
    for x in (mx0, mx1):
        c.polyline([(x, DOT_Y - STRIP_W / 2 - 1.5), (x, DOT_Y + STRIP_W / 2 + 1.5)], RED, 0.35, dash=(1.2, 0.8))
    from_left = not end_side_right(ROWS - 1)
    for k in range(4):
        idx = 220 + (k if from_left else 3 - k)
        c.circle(dot_x(k), DOT_Y, 2.5, fill=WHITE, stroke=BLACK, width=0.25)
        c.text(dot_x(k), DOT_Y - 8.6, str(idx), 1.7, GREY, align="c")
    for k in (1, 2, 3):
        c.triangle(dot_x(k) - 5, DOT_Y, 1 if from_left else -1, 2.2, ORANGE)
    c.text(FRONT / 2, DOT_Y + 7.5, "Minutenpunkte (4 LEDs am Stück)", 2.4, BLACK, bold=True, align="c")
    y10 = row_y(ROWS - 1)
    if from_left:
        mpts = [(GRID_L, y10), (GRID_L - JOG, y10), (GRID_L - JOG, DOT_Y), (mx0, DOT_Y)]
        e3 = (GRID_L - JOG, (y10 + DOT_Y) / 2)
    else:
        mpts = [(GRID_R, y10), (GRID_R + JOG, y10), (GRID_R + JOG, DOT_Y), (mx1, DOT_Y)]
        e3 = (GRID_R + JOG, (y10 + DOT_Y) / 2)
    c.polyline(mpts, BLACK, 0.7)

    # Dateneingang + Einspeisepunkte
    y0 = row_y(0)
    sx = GRID_L if start_left else GRID_R
    c.polyline([(sx, y0 + STRIP_W / 2), (sx, y0 + 12)], GREEN, 0.8)
    c.text(sx + (1.5 if start_left else -1.5), y0 + 10, "Daten rein (DIN) + LED 0", 2.4, GREEN, bold=True,
           align="l" if start_left else "r")
    e1 = (sx - 3.6 if start_left else sx + 3.6, y0)
    r5_right = end_side_right(4)
    e2 = (GRID_R + JOG if r5_right else GRID_L - JOG, (row_y(4) + row_y(5)) / 2)
    for (x, y), label in ((e1, "E1"), (e2, "E2"), (e3, "E3")):
        c.circle(x, y, 3.0, fill=ORANGE)
        c.text(x, y - 0.95, label, 2.5, WHITE, bold=True, align="c")

    # Klebelinie fuer die beiden Blaetter
    c.polyline([(FRONT / 2, 0), (FRONT / 2, FRONT)], GREY, 0.2, dash=(2, 1.5))
    for y in (5, 135, 245):
        c.polyline([(FRONT / 2 - 4, y), (FRONT / 2 + 4, y)], BLACK, 0.3)
        c.polyline([(FRONT / 2, y - 4), (FRONT / 2, y + 4)], BLACK, 0.3)


def build(start_left, path):
    corner = "links" if start_left else "rechts"
    tiles = [(-12, "Blatt 1 von 2 (linke Hälfte)"), (113, "Blatt 2 von 2 (rechte Hälfte)")]
    tile_w, margin_x, base_y = 149, (210 - 149) / 2, 27
    pages = []
    for x0, label in tiles:
        c = Canvas()
        c.push()
        c.translate(margin_x - x0, base_y)
        c.clip(x0, -2, tile_w, FRONT + 4)
        draw_template(c, start_left)
        c.pop()
        c.text(105, 289, "Lötvorlage Wordclock 1:1 – Start oben %s" % corner, 4.6, BLACK, bold=True, align="c")
        c.text(105, 283.5, label + " · Blick auf die LED-Seite = Vorderansicht der Uhr (nicht gespiegelt)",
               2.6, GREY, align="c")
        # Kontrollmass 100 mm
        c.rect(20, 19, 100, 2.5, stroke=BLACK, width=0.3)
        for i in range(11):
            c.polyline([(20 + i * 10, 19), (20 + i * 10, 23 if i % 5 == 0 else 21.5)], BLACK, 0.3)
        c.text(124, 19.4, "100 mm", 2.6, BLACK, bold=True)
        c.text(20, 14.5, "Kontrolle: Der Balken muss genau 100 mm lang sein. Sonst mit \"Tatsächliche Größe\" "
                         "(100 %) drucken.", 2.5, BLACK)
        c.text(20, 10, "Rot gestrichelt = Schnittlinie · Dreieck = Datenrichtung (Pfeil auf dem Streifen) · "
                         "Schwarze Linie = Verbinder +5 V / Daten / GND", 2.3, GREY)
        c.text(20, 6.5, "E1–E3 = zusätzliche Einspeisung (nur +5 V und GND) · Blätter an der Klebelinie mit den "
                        "Kreuzen deckungsgleich übereinanderlegen", 2.3, GREY)
        pages.append(c)
    write_pdf(path, pages)
    print("geschrieben:", path)
    # Vorschau der ganzen Vorlage (nicht zum Drucken)
    s = SvgCanvas(-14, -4, FRONT + 28, FRONT + 8)
    draw_template(s, start_left)
    svg_path = path[:-4] + "_vorschau.svg"
    with open(svg_path, "w", encoding="utf-8") as f:
        f.write(s.svg())
    print("geschrieben:", svg_path)


if __name__ == "__main__":
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out = os.path.join(root, "docs", "hardware")
    build(True, os.path.join(out, "loetvorlage_start_oben_links.pdf"))
    build(False, os.path.join(out, "loetvorlage_start_oben_rechts.pdf"))
