pcb_width = 28.0;
pcb_height = 31.0;
pcb_offset = 4.5;
standoff_height = 4.0;
box_depth = 15.0;
box_thickness = 2.0;
head_size = 16.0;
pwr_width = 9.0;
pwr_offset = 6.258;
eth_width = 16.0;
eth_offset = -9.315;

$fn= $preview ? 32 : 64;

w = pcb_width+box_thickness*2;
h = pcb_height+box_thickness*2;
d = box_thickness + standoff_height + 1.5 + box_depth + box_thickness;
connectors_z = box_thickness + standoff_height + 1.5 + d/2;
union() {
    difference() {
        translate([-w/2, -pcb_offset-h/2, 0]) cube([w, h, d]);
        translate([-pcb_width/2, -pcb_offset-pcb_height/2, box_thickness]) cube([pcb_width, pcb_height, d]);
        cube([head_size, head_size, 3*box_thickness], center=true);
        translate([-w/2, pwr_offset, connectors_z]) cube([3*box_thickness, pwr_width, d], center=true);
        translate([-w/2, eth_offset, connectors_z]) cube([3*box_thickness, eth_width, d], center=true);
    }
    translate([7-pcb_width/2, -pcb_offset+pcb_height/2, 0]) cylinder(h=box_thickness+standoff_height, r=1.5);
    translate([pcb_width/2, -pcb_offset+pcb_height/2, 0]) cylinder(h=box_thickness+standoff_height, r=1.5);
    translate([-pcb_width/2, -pcb_offset-pcb_height/2, 0]) cylinder(h=box_thickness+standoff_height, r=2.0);
    translate([pcb_width/2, -pcb_offset-pcb_height/2, 0]) cylinder(h=box_thickness+standoff_height, r=1.5);
}