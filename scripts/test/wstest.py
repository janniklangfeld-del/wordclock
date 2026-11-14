"""Einfacher WebSocket-Client (ohne Zusatzpakete) fuer Tests gegen die Uhr: ws://HOST/ws"""
import base64, json, os, socket, struct, sys, threading, time

class WS:
    def __init__(self, host, pong=True, name="", hb=True):
        # hb: wie die Webseite alle 3 s {"t":"hb"} senden (ohne gilt das Handy nach 12 s als weg)
        self.name, self.pong, self.hb, self.msgs, self.frames, self.closed = name, pong, hb, [], 0, False
        t0 = time.time()
        self.s = socket.create_connection((host, 80), timeout=10)
        key = base64.b64encode(os.urandom(16)).decode()
        self.s.send(f"GET /ws HTTP/1.1\r\nHost: {host}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                    f"Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n".encode())
        buf = b""
        while b"\r\n\r\n" not in buf:
            c = self.s.recv(1024)
            if not c: raise IOError("Handshake: Verbindung zu")
            buf += c
        if b" 101 " not in buf.split(b"\r\n")[0]: raise IOError("Handshake: " + buf[:60].decode())
        self.rest = buf.split(b"\r\n\r\n", 1)[1]
        self.handshake = time.time() - t0
        self.s.settimeout(None)
        self.lock = threading.Lock()
        threading.Thread(target=self.reader, daemon=True).start()
        threading.Thread(target=self.beat, daemon=True).start()

    def beat(self):
        while not self.closed:
            time.sleep(3)
            if self.hb and not self.closed:
                try: self.text({"t": "hb"})
                except Exception: pass

    def _send(self, op, data):
        hdr = bytes([0x80 | op])
        n = len(data)
        hdr += bytes([0x80 | n]) if n < 126 else bytes([0x80 | 126]) + struct.pack(">H", n)
        m = os.urandom(4)
        with self.lock:
            self.s.send(hdr + m + bytes(b ^ m[i % 4] for i, b in enumerate(data)))

    def text(self, obj): self._send(1, json.dumps(obj).encode())
    def key(self, k, down): self._send(2, bytes([2, k, 1 if down else 0]))
    def close(self):
        try: self._send(8, b"\x03\xe8"); self.s.close()
        except Exception: pass

    def _recv(self, n):
        while len(self.rest) < n:
            c = self.s.recv(4096)
            if not c: raise IOError("zu")
            self.rest += c
        d, self.rest = self.rest[:n], self.rest[n:]
        return d

    def reader(self):
        try:
            while True:
                b0, b1 = self._recv(2)
                n = b1 & 0x7F
                if n == 126: n = struct.unpack(">H", self._recv(2))[0]
                elif n == 127: n = struct.unpack(">Q", self._recv(8))[0]
                d = self._recv(n)
                op = b0 & 0x0F
                if op == 9 and self.pong: self._send(10, d)
                elif op == 2: self.frames += 1
                elif op == 1: self.msgs.append((time.time(), json.loads(d)))
                elif op == 8: break
        except Exception:
            pass
        self.closed = True

    def wait(self, pred, timeout=10):
        t0 = time.time()
        while time.time() - t0 < timeout:
            for t, m in list(self.msgs):
                if pred(m): return m
            time.sleep(0.02)
        return None
