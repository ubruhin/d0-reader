meter_pcb_distance = 12.0;
round_depth = 7.0;

box_thickness = 2.0;
box_depth = meter_pcb_distance - round_depth + 1.6 + 3.0;

cover_depth = 16.0;
cover_overlap = 3.0;

pcb_width = 36.4;
pcb_height = 34.4;
pcb_radius = 6.5;

eth_length = 12.9;
eth_width = 16.0;

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

module cover() {
    difference() {
        rounded_box(pcb_width+2*box_thickness, pcb_height+2*box_thickness, pcb_radius+box_thickness, cover_depth);
        translate([0, 0, -0.01]) difference() {
            rounded_box(pcb_width+2*box_thickness+0.01, pcb_height+2*box_thickness+0.01, pcb_radius+box_thickness+0.01, cover_overlap);
            rounded_box(pcb_width, pcb_height, pcb_radius, cover_overlap);
        }
        translate([0, 0, -box_thickness]) {
            rounded_box(pcb_width-box_thickness, pcb_height-box_thickness, pcb_radius-box_thickness/2, cover_depth);
        }
        translate([0, eth_length/2-pcb_height/2-box_thickness/2-0.01, meter_pcb_distance-round_depth+1.6+10]) {
            cube([eth_width, eth_length+box_thickness, 40], center=true);
        }
        cube([eth_width, pcb_height, 28], center=true);
    }
}

translate([0, 0, round_depth+box_depth-cover_overlap]) {
    cover();
    for(sgn=[-1,1]) {
        l = eth_length+box_thickness;
        h = cover_depth;
        translate([sgn*(eth_width+box_thickness)/2, -pcb_height/2+l/2, cover_depth-h/2]) {
            cube([box_thickness, l, h], center=true);
        }
    }
}

if($preview) {
    color([0.5, 0.5, 0.5, 0.3]) {
        import("head.stl");
        import("box.stl");
        translate([0, 0, meter_pcb_distance]) {
            import("d0-reader-pcb.stl");
        }
    }
}