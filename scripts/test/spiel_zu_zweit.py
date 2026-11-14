# Test: zwei Handys spielen Pong, Seite laedt waehrenddessen, ein Handy faellt weg und kommt zurueck.
#   python scripts/test/spiel_zu_zweit.py <ip-der-uhr> scripts/test
# Testnamen TestA/TestB; das Spiel wird am Ende beendet (keine Statistik).
import sys, time, threading, urllib.request
sys.path.insert(0, sys.argv[2] if len(sys.argv) > 2 else ".")
from wstest import WS
H = sys.argv[1]
ph = lambda w: (lambda m: m.get("t") == "game" and m.get("ph"))
def phase_is(p): return lambda m: m.get("t") == "game" and m.get("ph") == p

A = WS(H, name="A"); B = WS(H, name="B")
print(f"Handshake A {A.handshake*1000:.0f} ms, B {B.handshake*1000:.0f} ms")
print("Settings A/B:", bool(A.wait(lambda m: m.get("t") == "settings", 3)), bool(B.wait(lambda m: m.get("t") == "settings", 3)))
time.sleep(1.5); print("Live-Bilder in 1,5 s: A", A.frames, "B", B.frames)

A.text({"t": "g.open", "game": "pong", "mode": "2p", "name": "TestA", "id": "devA"})
print("Lobby:", bool(A.wait(phase_is("lobby"), 3)))
B.text({"t": "g.join", "slot": 1, "name": "TestB", "id": "devB"})
time.sleep(0.5)
for w in (A, B): w.text({"t": "frames", "on": False}); w.text({"t": "g.ready", "on": True})
print("Countdown:", bool(A.wait(phase_is("count"), 5)))
print("Spiel laeuft:", bool(A.wait(phase_is("play"), 6)))

# Seite + REST waehrend des Spiels
stop = False; times = []
def poll():
    while not stop:
        t0 = time.time()
        try: urllib.request.urlopen(f"http://{H}/", timeout=10).read(); times.append(time.time() - t0)
        except Exception as e: times.append(-1)
        time.sleep(0.3)
th = threading.Thread(target=poll, daemon=True); th.start()
n0 = len(A.msgs); t0 = time.time()
try:
 for i in range(40):
    A.key(0 if i % 2 else 1, True); B.key(1 if i % 2 else 0, True); time.sleep(0.05)
    A.key(0 if i % 2 else 1, False); B.key(1 if i % 2 else 0, False); time.sleep(0.05)
except Exception as e:
    print("FEHLER beim Senden nach", i, "Runden:", e, "A zu", A.closed, "B zu", B.closed)
dt = time.time() - t0
stop = True; th.join()
ok = [t for t in times if t >= 0]
print(f"Spielnachrichten an A in {dt:.1f} s: {len(A.msgs)-n0}; Seitenabrufe waehrend Spiel: {len(ok)}/{len(times)} ok, "
      f"max {max(ok)*1000:.0f} ms, Mittel {sum(ok)/len(ok)*1000:.0f} ms")

# B "sperrt das Handy": antwortet nicht mehr auf Pings, sendet nichts
B.pong = B.hb = False; t0 = time.time()
m = A.wait(lambda m: m.get("t") == "game" and m.get("ph") == "pause" and time.time() > t0, 20)
print(f"Pause nach Wegfall von B: {m and m.get('why')} nach {time.time()-t0:.1f} s")
B.close()
A.msgs.clear()
B2 = WS(H, name="B2")
print(f"B neu verbunden: Handshake {B2.handshake*1000:.0f} ms")
B2.text({"t": "frames", "on": False})
B2.text({"t": "g.join", "slot": 1, "name": "TestB", "id": "devB"})
time.sleep(0.5)
for w in (A, B2): w.text({"t": "g.ready", "on": True})
print("Weiter nach Rueckkehr:", bool(A.wait(lambda m: m.get("t") == "game" and m.get("ph") in ("count", "play") and m is not None and time.time() - t0 > 0, 8)))
A.text({"t": "g.quit"})
print("Beendet:", bool(A.wait(phase_is("idle"), 5)))
A.close(); B2.close()
