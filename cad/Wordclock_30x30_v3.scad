/*
 WORDCLOCK 30x30 | Parametrisches Gesamtmodell | Version 3.0 – durchgehende Trennwaende an Segmentfugen
 Einheit: Millimeter. Fuer Bambu Lab P1S (256 x 256 x 256 mm).

 DESIGN:
 300 x 300 mm Aussenmass, 250 x 250 mm schwarze Front,
 11 x 10 deutsche Buchstaben, 4 Minutenpunkte MITTIG UNTEN.
 100 LEDs/m WS2812B, 2 LEDs pro Buchstabe (10 mm LED-Pitch).
 110 * 2 + 4 = 224 LEDs, 10 Streifenreihen a 22 LEDs.

 OpenSCAD: F5 Vorschau, F6 Render, F7 STL exportieren.
 'part' und 'quadrant' im Customizer aendern.

 Hinweis: Holz- bzw. Alurahmen ist nur geometrische VORSCHAU.
 Bohrungen, Stromverteiler, Boardhalter und exakte Falze duerfen erst
 nach Festlegung des realen Rahmenprofils final konstruiert werden.
 Der Diffusor kann durch Backpapier ersetzt werden.
*/

/* [Anzeige / Export] */
part = "assembly"; // [assembly:Gesamtuhr,front_all:Front voll,front_quarter:Front Viertel,grid_all:Lichtgitter voll,grid_quarter:Lichtgitter Viertel,diffuser_all:Diffusor voll,diffuser_quarter:Diffusor Viertel,back_all:Ruckplatte voll,back_quarter:Ruckplatte Viertel,frame_mockup:Rahmen nur Vorschau,led_positions:LED Positionsmarkierungen,test_front:Testfront 3x3,test_grid:Testgitter 3x3,test_diffuser:Testdiffusor 3x3]
quadrant = "TL"; // [TL:Oben links,TR:Oben rechts,BL:Unten links,BR:Unten rechts]
show_frame = true;
show_back = true;
show_diffuser = true;
show_grid = true;
show_leds = true;

/* [Abmessungen] */
outer_size = 300;
front_size = 250;
front_thickness = 1.8;
frame_depth = 36;
rear_thickness = 4;

/* [Buchstaben und Licht] */
cols = 11;
rows_n = 10;
cell_x = 20;
cell_y = 20;
grid_x = 15;
grid_y = 35;
wall = 1.5;
lightwell_height = 13;
strip_width = 10;
strip_channel_width = 11.4;
strip_channel_height = 3.2;
diffuser_thickness = 0.8;
letter_size = 13.0;
font_name = "DejaVu Sans:style=Bold";
stencil_bridge = 0.9;
minute_dot_diameter = 5;
minute_dot_pitch = 10;
minute_dot_y = 17;
minute_cell_w = 10;
minute_cell_h = 14;
minute_wall = 1.2;

/* [Toleranzen] */
cut_allowance = 0.02;
$fn = 48;

// Alle 10 Zeilen sind genau 11 Zeichen breit, inklusive Umlaute.
// Diese Matrix ist die Layoutvorlage fuer die spaetere LED-Zuordnung.
letters = [
 "ESKISTLFÜNF",
 "ZEHNZWANZIG",
 "DREIVIERTEL",
 "VORFUNKNACH",
 "HALBAELFÜNF",
 "EINSXAMZWEI",
 "DREIPMJVIER",
 "SECHSNLACHT",
 "SIEBENZWÖLF",
 "ZEHNEUNKUHR"
];

function grid_w() = cols*cell_x;
function grid_h() = rows_n*cell_y;
function letter_x(c) = grid_x + (c+0.5)*cell_x;
function letter_y(r) = grid_y + (rows_n-r-0.5)*cell_y;
function dot_x(k) = front_size/2 + (k-1.5)*minute_dot_pitch;
function is_counter(ch) = (ch=="A"||ch=="B"||ch=="D"||ch=="O"||ch=="P"||ch=="R"||ch=="Ö");

// Segmentfugen verlaufen AUSSERHALB der vollen gemeinsamen Trennwand.
// Wand an x=115 ist vollstaendig links (TL/BL), Wand an y=135
// vollstaendig unten (BL/BR); nicht doppelt, keine 0.75mm-Haelften.
// Minutenwand bei x=115 (1.2mm) ist ebenfalls vollstaendig links.
cut_x = grid_x + 5*cell_x + wall/2 + 0.15; // 115.75 mm; volle 1.5-mm-Wand links
cut_y = grid_y + 5*cell_y + wall/2 + 0.15; // 135.75 mm; volle 1.5-mm-Wand unten
function x0(q) = q=="TL" || q=="BL" ? 0 : cut_x;
function x1(q) = q=="TL" || q=="BL" ? cut_x : front_size;
function y0(q) = q=="BL" || q=="BR" ? 0 : cut_y;
function y1(q) = q=="BL" || q=="BR" ? cut_y : front_size;

// Z=0 ist die Rueckseite. Alle Teile sind in der Gesamtdarstellung korrekt gestapelt.
back_z = 0;
grid_z = rear_thickness;
diffuser_z = grid_z + lightwell_height;
front_z = diffuser_z + diffuser_thickness;

// 3D-Buchstaben-Loch, mit feinem Steg fuer eingeschlossene Innenflaechen.
// Echte Locher ohne Steg wuerden bei z.B. O, A, B herausfallen.
module letter_cut(ch, x, y, thickness) {
    translate([x,y,-0.25])
    difference() {
        linear_extrude(height=thickness+0.5)
            text(ch, size=letter_size, font=font_name,
                 halign="center", valign="center");
        if (is_counter(ch))
            translate([-stencil_bridge/2,-letter_size/2-2,-0.2])
                cube([stencil_bridge,letter_size+4,thickness+1]);
    }
}

module front_full() {
    difference() {
        cube([front_size,front_size,front_thickness]);
        for(r=[0:rows_n-1]) for(c=[0:cols-1])
            letter_cut(letters[r][c],letter_x(c),letter_y(r),front_thickness);
        for(k=[0:3])
            translate([dot_x(k),minute_dot_y,-0.25])
                cylinder(d=minute_dot_diameter,h=front_thickness+0.5);
    }
}

// Ein vollstaendiges Lichtgitter; offene Kabeldurchfuehrung NUR an der
// Rueckseite. Vorne bleibt eine 9.8 mm hohe lichtdichte Wand stehen.
module grid_full() {
    union() {
        difference() {
            union() {
                for(c=[0:cols])
                    translate([grid_x+c*cell_x-wall/2,grid_y-wall/2,0])
                        cube([wall,grid_h()+wall,lightwell_height]);
                for(r=[0:rows_n])
                    translate([grid_x-wall/2,grid_y+r*cell_y-wall/2,0])
                        cube([grid_w()+wall,wall,lightwell_height]);
            }
            // Der LED-Streifen liegt horizontal. Die Aussparungen liegen
            // mittig zu jeder Zelle, unterhalb der Kammerwand.
            for(c=[0:cols]) for(r=[0:rows_n-1])
                translate([grid_x+c*cell_x-wall/2-0.1,
                           grid_y+(r+0.5)*cell_y-strip_channel_width/2,-0.1])
                    cube([wall+0.2,strip_channel_width,strip_channel_height+0.1]);
        }
        // Minutenpunkte als 4er-Modul fuer einen durchgehenden LED-Streifen.
        // Vorne sind die 4 Punkte optisch getrennt; hinten/unten laeuft das
        // 10 mm breite LED-Band horizontal durch. Zwischen den Punkten bleiben
        // Trennwaende stehen, jeweils mit einer unteren Aussparung fuer das Band.
        minute_module();
    }
}



// Minutenmodul: vier getrennte Lichtkammern bei 10 mm Pitch,
// kompatibel mit einem geraden Streifenabschnitt aus 4 LEDs.
module minute_module() {
    left = dot_x(0) - minute_cell_w/2;
    bottom = minute_dot_y - minute_cell_h/2;
    top = bottom + minute_cell_h;

    difference() {
        union() {
            // Vier rechteckige, oben offene Kammern mit lichtdichten Waenden.
            translate([left-minute_wall/2,bottom-minute_wall/2,0])
                cube([4*minute_cell_w+minute_wall,
                      minute_cell_h+minute_wall,lightwell_height]);
            // Verbindung zum Hauptgitter an beiden Seiten, ohne einen
            // einzelnen LED-Streifen oder Leuchtpunkt zu behindern.
            for(x=[left-minute_wall/2,
                   left+4*minute_cell_w-minute_wall/2])
                translate([x,top+minute_wall/2-0.02,0])
                    cube([minute_wall,grid_y-top-minute_wall/2+0.77,lightwell_height]);
        }
        // Jede der vier LED-Kammern wird nach vorne vollstaendig geoeffnet.
        for(k=[0:3])
            translate([dot_x(k)-(minute_cell_w-minute_wall)/2,
                       minute_dot_y-(minute_cell_h-minute_wall)/2,-0.1])
                cube([minute_cell_w-minute_wall,
                      minute_cell_h-minute_wall,lightwell_height+0.2]);
        // Nur rueckseitig: ein horizontaler Kanal verbindet die Kammern
        // fuer EINEN 4-LED-Streifen (10 mm breit, LED-Pitch 10 mm).
        // Die obersten 9.8 mm der Trennwaende bleiben stehen.
        translate([left-minute_wall/2-0.1,
                   minute_dot_y-strip_channel_width/2,-0.1])
            cube([4*minute_cell_w+minute_wall+0.2,
                  strip_channel_width,strip_channel_height+0.1]);
    }
}

module diffuser_full() {
    cube([front_size,front_size,diffuser_thickness]);
}

module back_full() {
    cube([front_size,front_size,rear_thickness]);
}

// Crop geometries by quadrant, then translate to origin for easy printing.
// Nur die GITTER-Viertel sind auf 115.75 / 135.75 mm optimiert.
// Front-/Diffusor-Viertel werden derzeit hier ebenfalls so geteilt;
// ihre Fugen sind daher NICHT identisch mit frueheren v1/v2-STLs.
// Die geteilten Gitter sind bewusst ohne Befestigungsloecher;
// diese haengen vom noch nicht gewaelten Rahmen ab.
module crop_to_quadrant(q, height) {
    translate([-x0(q),-y0(q),0])
        intersection() {
            children();
            translate([x0(q),y0(q),-0.05])
                cube([x1(q)-x0(q),y1(q)-y0(q),height+0.1]);
        }
}

// Rahmenmodell nur als Anzeige. Bei Eckprofilen aus Holz/Alu die
// tatsaechliche Kontur erst nach Auswahl neu abgleichen.
module preview_frame() {
    color([0.76,0.60,0.41])
        difference(){
            translate([-(outer_size-front_size)/2,-(outer_size-front_size)/2,front_z+front_thickness-frame_depth])
                cube([outer_size,outer_size,frame_depth]);
            translate([0,0,front_z+front_thickness-frame_depth-0.1])
                cube([front_size,front_size,frame_depth+0.2]);
        }
}

// Angedeutete 5x5 mm LED-Koerper; zeigen die zwei LEDs pro Feld.
module led_positions() {
    for(r=[0:rows_n-1]) for(c=[0:cols-1])
        for(i=[0:1])
            translate([grid_x+c*cell_x+5+i*10,
                       grid_y+(r+0.5)*cell_y,grid_z+1.8])
                color([1,0.69,0.20])
                    cube([5,5,1],center=true);
    for(k=[0:3])
        translate([dot_x(k),minute_dot_y,grid_z+1.8])
            color([1,0.69,0.20]) cube([5,5,1],center=true);
}

module assembly() {
    if(show_frame) preview_frame();
    if(show_back) color([0.72,0.57,0.40]) back_full();
    if(show_leds) led_positions();
    if(show_grid) color([0.10,0.11,0.12])
        translate([0,0,grid_z]) grid_full();
    if(show_diffuser) color([0.96,0.96,0.93,0.55])
        translate([0,0,diffuser_z]) diffuser_full();
    color([0.045,0.05,0.06])
        translate([0,0,front_z]) front_full();
}

// Passprobe: die gleichen 20 mm Kammern und 13 mm Hoehe, 3 Spalten x 3 Zeilen.
module test_grid() {
    intersection() {
        translate([-grid_x,-grid_y,0]) grid_full();
        cube([60,60,lightwell_height]);
    }
}
module test_front() {
    difference() {
        cube([60,60,front_thickness]);
        for(r=[0:2]) for(c=[0:2])
            letter_cut(letters[r][c],(c+0.5)*cell_x,(2-r+0.5)*cell_y,front_thickness);
    }
}
module test_diffuser() { cube([60,60,diffuser_thickness]); }

if(part=="assembly") assembly();
else if(part=="front_all") front_full();
else if(part=="front_quarter") crop_to_quadrant(quadrant,front_thickness) front_full();
else if(part=="grid_all") grid_full();
else if(part=="grid_quarter") crop_to_quadrant(quadrant,lightwell_height) grid_full();
else if(part=="diffuser_all") diffuser_full();
else if(part=="diffuser_quarter") crop_to_quadrant(quadrant,diffuser_thickness) diffuser_full();
else if(part=="back_all") back_full();
else if(part=="back_quarter") crop_to_quadrant(quadrant,rear_thickness) back_full();
else if(part=="frame_mockup") preview_frame();
else if(part=="led_positions") led_positions();
else if(part=="test_front") test_front();
else if(part=="test_grid") test_grid();
else if(part=="test_diffuser") test_diffuser();
