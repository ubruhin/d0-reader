round_diameter = 32.0;
round_depth = 10.0;
head_size = 16.0;
head_radius = 2.0;
box_thickness = 2.0;
led_diameter = 4.3;
led_distance = 6.5;
magnet_diameter = 5.0;
magnet_radius = 12.0;

$fn= $preview ? 32 : 64;

head_delta = head_size/2 - head_radius;

module led_hole(sgn) {
    translate([sgn*led_distance/2, 0, 0]) {
        cylinder(h=3*round_depth, d=led_diameter, center=true);
    }
}

module magnet_hole(angle) {
    dx = magnet_radius * sin(angle);
    dy = magnet_radius * cos(angle);
    translate([dx, dy, 0]) {
        cylinder(h=3*round_depth, d=magnet_diameter, center=true);
    }
}

difference() {
    union() {
        cylinder(h=round_depth, d=round_diameter);
        translate([0, 0, round_depth+box_thickness/2]) {
            translate([head_delta, head_delta, 0]) cylinder(h=box_thickness, r=head_radius, center=true);
            translate([head_delta, -head_delta, 0]) cylinder(h=box_thickness, r=head_radius, center=true);
            translate([-head_delta, head_delta, 0]) cylinder(h=box_thickness, r=head_radius, center=true);
            translate([-head_delta, -head_delta, 0]) cylinder(h=box_thickness, r=head_radius, center=true);
            cube([head_size, 2*head_delta, box_thickness], center=true);
            cube([2*head_delta, head_size, box_thickness], center=true);
        }
    }
    led_hole(1);
    led_hole(-1);
    magnet_hole(0);
    magnet_hole(120);
    magnet_hole(240);
}
