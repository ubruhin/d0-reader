meter_pcb_distance = 12.0;
round_diameter = 32.0;
round_depth = 7.0;
head_size = 13.0;
head_radius = 4.0;
box_thickness = 2.0;

separator_width = 1.3;
separator_length = 12.0;

led_diameter = 5.0;
led_distance = 6.5;

magnet_count = 4;
magnet_rotation = 45;
magnet_diameter = 6.3;
magnet_diameter_top = 6.8;
magnet_radius = 11.0;

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
    translate([dx, dy, round_depth]) {
        cylinder(h=3, d=magnet_diameter_top, center=true);
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
        translate([0, 0, meter_pcb_distance/2]) {
            rotate([0, 0, -12]) {
                cube([separator_width, separator_length, meter_pcb_distance], center=true);
            }
        }
    }
    led_hole(1);
    led_hole(-1);
    for(i = [1:magnet_count]) {
        magnet_hole(magnet_rotation+i*360/magnet_count);
    }
}

if($preview) {
    translate([0, 0, meter_pcb_distance]) {
        color([0.5, 0.5, 0.5, 0.3]) import("d0-reader-pcb.stl");
    }
}
