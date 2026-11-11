/*
 WORDCLOCK – RAHMEN VARIANTE 1
 Holzkasten + weisse Kunststoff-L-Profile (Bauhaus) + Rueckwand.
 Einheit: Millimeter. Nutzt die Uhr-Schichten aus Wordclock_30x30_v3.scad.

 Prinzip (Schnitt durch eine Seitenwand):
   - Der vordere Profilschenkel deckt die Holzkante ab und ragt
     'ueberdeckung' mm ueber den Rand der schwarzen Front. Er ist damit
     gleichzeitig der vordere Anschlag fuer die Front – keine Fraese,
     keine Anschlagleiste noetig.
   - Front, Diffusor, Lichtgitter und LED-Traeger werden von hinten
     eingelegt. Andruckkloetze + Moosgummi zwischen LED-Traeger und
     Rueckwand druecken alles nach vorn. Kein Kleber im Sichtbereich.
   - Holzkasten stumpf verleimt; die Ecken vorn verdeckt das Profil.

 Koordinaten wie im Hauptmodell: Front liegt bei x,y = 0..250,
 z = 0 ist die Rueckseite des LED-Traegers, Frontvorderseite bei z = 19.6.

 VOR DEM ZUSCHNITT: echte Profilmasse (a, b, Wandstaerke) und die
 zusammengelegte gedruckte Front nachmessen und hier eintragen.
*/

use <Wordclock_30x30_v3.scad>

/* [Anzeige] */
part = "assembly"; // [assembly:Gesamt,schnitt:Schnitt durch Seitenwand,explosion:Explosionsansicht,rueckansicht:Rueckansicht,holzkasten:Holzkasten,profile:L-Profile,rueckwand:Rueckwand,rueckwand_2d:Rueckwand 2D (DXF/SVG),andruckklotz:Andruckklotz (Druck)]
show_uhr = true;

/* [L-Profil (nachmessen!)] */
profil_a = 25;     // vorderer Schenkel (sichtbare Rahmenbreite)
profil_b = 25;     // seitlicher Schenkel
profil_t = 1.5;    // Wandstaerke

/* [Holzkasten] */
holz_w = 20;       // Leistenstaerke (sichtbar unter dem Profil)
holz_d = 40;       // Leistentiefe = Bautiefe des Kastens
spiel = 0.25;      // Luft je Seite zwischen Front und Holz

/* [Rueckwand / Andruck] */
rueckwand_t = 3;   // HDF oder Sperrholz
schaum_t = 3;      // Moosgummi auf den Andruckkloetzen
vorspannung = 1;   // so viel wird der Schaum zusammengedrueckt
schraube_d = 3.2;  // Spax 3 x 16
kabel_d = 10;      // Loch fuer Netzteilkabel

/* [Uhr (aus Hauptmodell, nicht aendern)] */
front_size = 250;
stack_h = 4 + 13 + 0.8 + 1.8;   // LED-Traeger + Gitter + Diffusor + Front = 19.6

$fn = 48;

// ---------- abgeleitete Masse ----------
innen      = front_size + 2*spiel;          // 250.5
kasten     = innen + 2*holz_w;              // 290.5
aussen     = kasten + 2*profil_t;           // 293.5
fenster    = aussen - 2*profil_a;           // 243.5
ueberdeckung = (front_size - fenster)/2;    // 3.25
z_front    = stack_h;                       // Vorderseite Front
z_kasten0  = z_front - holz_d;              // Rueckseite Holzkasten
luft       = -z_kasten0;                    // Raum hinter LED-Traeger (Elektronik)
klotz_h    = luft - schaum_t + vorspannung;
o          = -spiel - holz_w;               // x/y der Kasten-Aussenkante

echo(str("Aussenmass ", aussen, " mm | Kasten ", kasten, " mm | Lichte Weite ", innen,
         " mm | Fenster ", fenster, " mm | Ueberdeckung Front ", ueberdeckung,
         " mm | Raum hinten ", luft, " mm | Andruckklotz ", klotz_h, " mm"));
assert(ueberdeckung >= 1, "Profil deckt die Fuge zur Front nicht ab: holz_w kleiner waehlen");
assert(ueberdeckung <= 10, "Profil ragt zu weit ueber die Front (Buchstaben ab 15 mm)");
assert(profil_b <= holz_d, "Seitlicher Schenkel laenger als der Kasten tief");

// ---------- Teile ----------
WEISS = [0.96,0.96,0.94];
HOLZ  = [0.80,0.66,0.47];
HDF   = [0.62,0.50,0.38];

// 2 lange (oben/unten, durchgehend) + 2 kurze (links/rechts, dazwischen)
module holzkasten() {
    color(HOLZ) translate([o,o,z_kasten0]) difference() {
        cube([kasten,kasten,holz_d]);
        translate([holz_w,holz_w,-1]) cube([innen,innen,holz_d+2]);
    }
    // Stossfugen andeuten
    color([0.55,0.42,0.28]) for(y=[o+holz_w-0.2, o+kasten-holz_w-0.2])
        for(x=[o-0.01, o+kasten-holz_w+0.01])
            translate([x,y,z_kasten0]) cube([holz_w,0.4,holz_d+0.02]);
}

// L-Profile als ein Gehrungsrahmen: vorderer Ring + seitlicher Mantel
module profile() {
    color(WEISS) translate([o-profil_t,o-profil_t,0]) {
        translate([0,0,z_front]) difference() {
            cube([aussen,aussen,profil_t]);
            translate([profil_a,profil_a,-1]) cube([fenster,fenster,profil_t+2]);
        }
        translate([0,0,z_front+profil_t-profil_b]) difference() {
            cube([aussen,aussen,profil_b]);
            translate([profil_t,profil_t,-1]) cube([kasten,kasten,profil_b+2]);
        }
    }
    // Gehrungsfugen
    color([0.75,0.75,0.72]) for(c=[[o-profil_t,o-profil_t,45],[o-profil_t+aussen,o-profil_t,135],
                                   [o-profil_t+aussen,o-profil_t+aussen,225],[o-profil_t,o-profil_t+aussen,315]])
        translate([c[0],c[1],z_front+profil_t]) rotate([0,0,c[2]])
            translate([0,-0.2,-0.01]) cube([profil_a*sqrt(2),0.4,0.03]);
}

function schrauben() = [
    for(p=[0.15,0.5,0.85]) [o+holz_w/2, o+p*kasten],
    for(p=[0.15,0.5,0.85]) [o+kasten-holz_w/2, o+p*kasten],
    for(p=[0.5]) [o+p*kasten, o+holz_w/2],
    for(p=[0.5]) [o+p*kasten, o+kasten-holz_w/2]
];

module rueckwand_2d() {
    difference() {
        translate([o,o]) square([kasten,kasten]);
        for(s=schrauben()) translate(s) circle(d=schraube_d);
        // Kabel: von vorn gesehen unten rechts, von hinten unten links
        translate([o+kasten-55, o+holz_w+20]) circle(d=kabel_d);
        // Lueftung unten und oben
        for(y=[o+holz_w+8, o+kasten-holz_w-12]) for(i=[0:7])
            translate([front_size/2-70+i*20-6, y]) hull() {
                circle(d=4); translate([12,0]) circle(d=4);
            }
        // Schluesselloch-Aufhaenger oben mittig
        translate([front_size/2, o+kasten-45]) {
            circle(d=9);
            translate([-2.25,0]) square([4.5,12]);
        }
    }
}

module rueckwand() {
    color(HDF) translate([0,0,z_kasten0-rueckwand_t])
        linear_extrude(rueckwand_t) rueckwand_2d();
}

// Andruckklotz zum Drucken: 15 x 15 mm, Schaum oben aufkleben
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
        color([0.25,0.35,0.85]) translate([0,0,z_kasten0]) cube([15,15,schaum_t-vorspannung]);
    }
}

// Elektronikzone nur als Platzhalter
module elektronik() {
    color([0.1,0.45,0.25]) translate([front_size/2-14, o+holz_w+12, z_kasten0]) cube([28,52,13]);
    color([0.9,0.6,0.1]) translate([front_size/2+30, o+holz_w+12, z_kasten0]) cube([40,20,18]);
}

module uhr() {
    translate([0,0,0])          color([0.72,0.57,0.40]) back_full();
    translate([0,0,4])          color([0.10,0.11,0.12]) grid_full();
    translate([0,0,17])         color([0.96,0.96,0.93]) diffuser_full();
    translate([0,0,17.8])       color([0.045,0.05,0.06]) front_full();
}

module assembly(ex=0) {
    if(show_uhr) translate([0,0,ex*1.2]) uhr();
    translate([0,0,ex*2.6]) profile();
    holzkasten();
    translate([0,0,-ex*0.6]) andruck();
    translate([0,0,-ex*0.8]) elektronik();
    translate([0,0,-ex*1.4]) rueckwand();
}

if(part=="assembly") assembly();
else if(part=="explosion") assembly(ex=60);
else if(part=="schnitt") intersection() {
    assembly();
    translate([o-20,100,z_kasten0-20]) cube([aussen+40,50,100]);
}
else if(part=="rueckansicht") rotate([0,180,0]) assembly();
else if(part=="holzkasten") holzkasten();
else if(part=="profile") profile();
else if(part=="rueckwand") rueckwand();
else if(part=="rueckwand_2d") rueckwand_2d();
else if(part=="andruckklotz") andruckklotz();
