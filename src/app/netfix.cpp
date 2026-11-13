// Fix fuer einen Fehler im ESP32-Arduino-Core (2.0.x): WiFiClient::connected() prueft die
// Verbindung mit recv(fd, &x, 0, MSG_DONTWAIT). Liegen Daten an, liefert lwIP 0 und setzt
// errno NICHT -> connected() wertet einen alten errno-Wert aus. Hat kurz vorher irgendeine
// andere Verbindung einen Fehler gemeldet (ECONNRESET, ENOTCONN – z. B. ein Handy, das die
// Seite schliesst oder gesperrt wird), gilt eine gesunde Verbindung als getrennt:
//   - ein neuer Mitspieler wird mitten im WebSocket-Handshake stumm abgewiesen
//     ("Keine Live-Verbindung zur Uhr"),
//   - ein laufender Spieler fliegt raus.
// Abhilfe: Der Linker leitet lwip_recv auf diese Funktion um (-Wl,--wrap=lwip_recv in
// platformio.ini). Bei Laenge 0 wird errno vorher geloescht; lwIP setzt es nur bei echtem
// Fehler/Ende (ENOTCONN), sonst melden wir "lebt" (EWOULDBLOCK).
#include <errno.h>
#include <sys/types.h>

extern "C" ssize_t __real_lwip_recv(int s, void* mem, size_t len, int flags);

extern "C" ssize_t __wrap_lwip_recv(int s, void* mem, size_t len, int flags) {
  if (len) return __real_lwip_recv(s, mem, len, flags);
  errno = 0;
  const ssize_t r = __real_lwip_recv(s, mem, len, flags);
  if (r == 0 && errno == 0) errno = EWOULDBLOCK;  // Daten liegen an -> Verbindung lebt
  return r;
}
