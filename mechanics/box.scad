meter_pcb_distance = 12.0;
round_depth = 7.0;

standoff_diameter = 3.0;
standoff_distance = 2.0;
standoff_height = meter_pcb_distance - round_depth;

box_thickness = 2.0;
box_depth = meter_pcb_distance - round_depth + 1.6 + 3.0;

pcb_width = 36.4;
pcb_height = 34.4;
pcb_radius = 6.5;

eth_width = 16.0;

head_size = 12.85;
head_radius = 3.8;

$fn= $preview ? 32 : 64;

module rounded_box(w, h, r, height) {
    dx = w/2 - r;
    dy = h/2 - r;
    linear_extrude(height=height) {
        hull() {
            translate([dx, dy, 0]) circle(r);
            translate([dx, -dy, 0 ]) circle(r);
            translate([-dx, dy, 0]) circle(r);
            translate([-dx, -dy, 0]) circle(r);
        }
    }
}

module box() {
    difference() {
        rounded_box(pcb_width+2*box_thickness, pcb_height+2*box_thickness, pcb_radius+box_thickness, box_depth);
        translate([0, 0, box_thickness]) {
            rounded_box(pcb_width, pcb_height, pcb_radius, box_depth);
        }
        translate([0, 0, -box_depth/2]) rounded_box(head_size, head_size, head_radius, box_depth);
        translate([0, -pcb_width/2, meter_pcb_distance-round_depth+1.6+10]) {
            cube([eth_width, 2*box_thickness, 20], center=true);
        }
    }
}

translate([0, 0, round_depth]) {
    box();
    for(i=[1:4]) {
        rotate([0, 0, i*90]) {
            translate([pcb_width/2-standoff_distance, pcb_height/2-standoff_distance, 0]) {
                cylinder(d=standoff_diameter, h=standoff_height);
            }
        }
    }
}

if($preview) {
    color([0.5, 0.5, 0.5, 0.3]) {
        import("head.stl");
        translate([0, 0, meter_pcb_distance]) {
            import("d0-reader-pcb.stl");
        }
    }
}