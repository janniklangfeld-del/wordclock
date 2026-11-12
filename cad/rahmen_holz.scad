/*
 WORDCLOCK – HOLZRAHMEN (Variante 2, gewaehlt am 12.11.2026)
 Buche-Leimholz 18 mm, als Streifen 50 mm breit zugeschnitten (BAUHAUS),
 45-Grad-Gehrung, geoelt. Tiefe 50 mm, weil die ESP32-Platine 22 mm hoch ist.
 (Die 40-mm-Leiste liess hinten nur 17,4 mm Platz.)
 Einheit: Millimeter. Nutzt die Uhr-Schichten aus Wordclock_30x30_v3.scad.

 Prinzip (Schnitt durch eine Seite):
   - Streifen hochkant: 18 mm sichtbare Rahmenbreite, 50 mm Tiefe.
   - Innen rundum ein schwarzer, gedruckter Anschlag (4 x 3 mm) buendig mit
     der Rahmenvorderkante. Die Front liegt von hinten dagegen und sitzt
     dadurch 3 mm zurueck (Schattenfuge, schwarz in schwarz).
   - Front, Diffusor, Lichtgitter, LED-Traeger von hinten einlegen,
     Andruckkloetze + Moosgummi druecken alles nach vorn. Kein Kleber im Sichtbereich.
   - Gedruckte Eckwinkel hinten in den Ecken stabilisieren die Gehrungen.
   - Rueckwand (3 mm Sperrholz) mit 8 Schrauben, abnehmbar.

 Koordinaten wie im Hauptmodell: Front bei x,y = 0..250,
 z = 0 Rueckseite LED-Traeger, Frontvorderseite z = 19.6.

 VOR DEM ZUSCHNITT: gedruckte Front zusammengelegt messen (front_size),
 Leiste nachmessen (holz_w / holz_d) und hier eintragen.
*/

use <Wordclock_30x30_v3.scad>

/* [Anzeige] */
part = "assembly"; // [assembly:Gesamt,explosion:Explosionsansicht,schnitt:Schnitt durch Seitenwand,rueckansicht:Rueckansicht,rahmen:Holzrahmen,anschlag_lang:Anschlag lang (Druck),anschlag_kurz:Anschlag kurz (Druck),eckwinkel:Eckwinkel (Druck),andruckklotz:Andruckklotz (Druck),rueckwand:Rueckwand,rueckwand_2d:Rueckwand 2D (DXF/SVG)]
show_uhr = true;

/* [Holzleiste (nachmessen!)] */
holz_w = 18;       // sichtbare Rahmenbreite (Leimholz 18 mm)
holz_d = 50;       // Tiefe (Streifenbreite)
elektronik_h = 22; // hoechstes Bauteil hinter dem LED-Traeger (ESP32-Platine)
spiel = 0.25;      // Luft je Seite zwischen Front und Holz

/* [Anschlag (schwarz drucken)] */
anschlag_b = 4;    // ragt so weit ueber die Front
anschlag_t = 3;    // Tiefe = Ruecksprung der Front
anschlag_rueck = 0; // Abstand Anschlag zur Rahmenvorderkante

/* [Rueckwand / Andruck] */
rueckwand_t = 3;
schaum_t = 3;
vorspannung = 1;
schraube_d = 3.2;  // Spax 3 x 16
kabel_d = 10;

/* [Uhr (aus Hauptmodell)] */
front_size = 250;
stack_h = 4 + 13 + 0.8 + 1.8;   // 19.6

$fn = 48;

// ---------- abgeleitete Masse ----------
innen   = front_size + 2*spiel;              // 250.5
aussen  = innen + 2*holz_w;                  // 290.5
fenster = innen - 2*anschlag_b;              // 242.5
z_front = stack_h;                           // Vorderseite Front
z_vorn  = z_front + anschlag_t + anschlag_rueck;  // Rahmenvorderkante
z_hint  = z_vorn - holz_d;                   // Rahmenrueckseite
luft    = -z_hint;                           // Platz hinter dem LED-Traeger
klotz_h = luft - schaum_t + vorspannung;
o       = -spiel - holz_w;                   // Aussenkante
i0      = -spiel;                            // Innenkante

echo(str("Aussen ", aussen, " | Innen ", innen, " | Fenster ", fenster,
         " | Rahmenbreite sichtbar ", holz_w, " + Anschlag ", anschlag_b,
         " | Front ", anschlag_t + anschlag_rueck, " mm zurueck | Platz Elektronik ", luft,
         " | Andruckklotz ", klotz_h, " | Gesamttiefe ", holz_d + rueckwand_t));
assert(luft >= elektronik_h + 2, str("Elektronik passt nicht: Platz ", luft, " mm, noetig ", elektronik_h + 2, " mm -> holz_d erhoehen"));
assert(15 - spiel - anschlag_b >= 5, "Anschlag reicht zu nah an die Buchstaben");

BUCHE   = [0.88,0.73,0.56];
FUGE    = [0.70,0.55,0.40];
SCHWARZ = [0.08,0.08,0.09];
PAPPEL  = [0.86,0.80,0.66];

// ---------- Holzrahmen mit Gehrung ----------
// Vier Leisten als Trapez (Gehrung), Laenge an der Aussenkante = aussen
module leiste_2d() { polygon([[0,0],[aussen,0],[aussen-holz_w,holz_w],[holz_w,holz_w]]); }
module rahmen() {
    color(BUCHE) translate([o,o,z_hint])
        for(k=[0:3]) translate([aussen/2,aussen/2,0]) rotate([0,0,90*k])
            translate([-aussen/2,-aussen/2,0]) linear_extrude(holz_d) leiste_2d();
    // Gehrungsfugen andeuten
    color(FUGE) for(c=[[o,o,45],[o+aussen,o,135],[o+aussen,o+aussen,225],[o,o+aussen,315]])
        translate([c[0],c[1],z_vorn]) rotate([0,0,c[2]])
            translate([0,-0.25,-0.01]) cube([holz_w*sqrt(2),0.5,0.03]);
}

// ---------- Anschlag (schwarz, 4 Teile stumpf) ----------
module anschlag_lang() { cube([innen, anschlag_b, anschlag_t]); }
module anschlag_kurz() { cube([innen-2*anschlag_b, anschlag_b, anschlag_t]); }
module anschlag() {
    color(SCHWARZ) translate([0,0,z_vorn-anschlag_rueck-anschlag_t]) {
        translate([i0,i0,0]) anschlag_lang();
        translate([i0,i0+innen-anschlag_b,0]) anschlag_lang();
        translate([i0+anschlag_b,i0+anschlag_b,0]) rotate([0,0,90]) anschlag_kurz();
        translate([i0+innen,i0+anschlag_b,0]) rotate([0,0,90]) anschlag_kurz();
    }
}

// ---------- Eckwinkel (Druck), hinten in die Ecken kleben ----------
ew_l = 35; ew_t = 5; ew_h = min(12, luft-2);
module eckwinkel() {
    difference() {
        union() {
            cube([ew_l,ew_t,ew_h]);
            cube([ew_t,ew_l,ew_h]);
            linear_extrude(ew_h) polygon([[ew_t,ew_t],[ew_t+12,ew_t],[ew_t,ew_t+12]]);
        }
        // Loecher fuer optionale Schrauben 3 x 12 (seitlich ins Holz)
        for(p=[[ew_l-9,-1,ew_h/2,[-90,0,0]],[-1,ew_l-9,ew_h/2,[0,90,0]]])
            translate([p[0],p[1],p[2]]) rotate(p[3]) cylinder(d=3.4,h=ew_t+2,center=false);
    }
}
module eckwinkel_alle() {
    color([0.2,0.2,0.22])
    for(c=[[i0,i0,0],[i0+innen,i0,90],[i0+innen,i0+innen,180],[i0,i0+innen,270]])
        translate([c[0],c[1],z_hint]) rotate([0,0,c[2]]) eckwinkel();
}

// ---------- Rueckwand ----------
function schrauben() = [
    for(p=[0.15,0.5,0.85]) [o+holz_w/2, o+p*aussen],
    for(p=[0.15,0.5,0.85]) [o+aussen-holz_w/2, o+p*aussen],
    [o+aussen/2, o+holz_w/2], [o+aussen/2, o+aussen-holz_w/2]
];
module rueckwand_2d() {
    difference() {
        translate([o,o]) square([aussen,aussen]);
        for(s=schrauben()) translate(s) circle(d=schraube_d);
        // Kabel: von vorn gesehen unten rechts, von hinten unten links
        translate([o+aussen-55, i0+20]) circle(d=kabel_d);
        // Lueftung unten und oben
        for(y=[i0+8, i0+innen-12]) for(i=[0:7])
            translate([front_size/2-70+i*20-6, y]) hull() {
                circle(d=4); translate([12,0]) circle(d=4);
            }
        // Schluesselloch oben mittig
        translate([front_size/2, i0+innen-75]) {
            circle(d=9);
            translate([-2.25,0]) square([4.5,12]);
        }
    }
}
module rueckwand() {
    color(PAPPEL) translate([0,0,z_hint-rueckwand_t]) linear_extrude(rueckwand_t) rueckwand_2d();
}

// ---------- Andruck ----------
module andruckklotz() {
    hull() {
        translate([1,1,0]) cube([13,13,0.01]);
        translate([0,0,1]) cube([15,15,klotz_h-1]);
    }
}
function klotz_pos() = [for(x=[45,125,205]) for(y=[45,125,205]) if(!(x==125&&y==125)) [x,y]];
module andruck() {
    for(p=klotz_pos()) translate([p[0]-7.5,p[1]-7.5,0]) {
        color([0.2,0.2,0.22]) translate([0,0,-klotz_h]) andruckklotz();
        color([0.25,0.35,0.85]) translate([0,0,z_hint]) cube([15,15,schaum_t-vorspannung]);
    }
}

module elektronik() {
    color([0.1,0.45,0.25]) translate([front_size/2-30, 60, z_hint]) cube([60,50,elektronik_h]);
    color([0.9,0.6,0.1]) translate([front_size/2+40, 60, z_hint]) cube([40,20,10]);
}

module uhr() {
    color([0.72,0.57,0.40]) back_full();
    translate([0,0,4])    color([0.10,0.11,0.12]) grid_full();
    translate([0,0,17])   color([0.96,0.96,0.93]) diffuser_full();
    translate([0,0,17.8]) color([0.045,0.05,0.06]) front_full();
}

module assembly(ex=0) {
    translate([0,0,ex*3.0]) rahmen();
    translate([0,0,ex*2.4]) anschlag();
    if(show_uhr) translate([0,0,ex*1.4]) uhr();
    translate([0,0,ex*3.0]) eckwinkel_alle();
    translate([0,0,-ex*0.4]) andruck();
    translate([0,0,-ex*0.8]) elektronik();
    translate([0,0,-ex*1.4]) rueckwand();
}

if(part=="assembly") assembly();
else if(part=="explosion") assembly(ex=60);
else if(part=="schnitt") intersection() {
    assembly();
    translate([o-20,100,z_hint-20]) cube([aussen+40,50,100]);
}
else if(part=="rueckansicht") rotate([0,180,0]) assembly();
else if(part=="rahmen") rahmen();
else if(part=="anschlag_lang") anschlag_lang();
else if(part=="anschlag_kurz") anschlag_kurz();
else if(part=="eckwinkel") eckwinkel();
else if(part=="andruckklotz") andruckklotz();
else if(part=="rueckwand") rueckwand();
else if(part=="rueckwand_2d") rueckwand_2d();
