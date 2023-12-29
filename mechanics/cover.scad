pcb_width = 28.0;
pcb_height = 31.0;
pcb_offset = 4.5;
box_depth = 15.0;
box_thickness = 2.0;
pwr_width = 9.0;
pwr_offset = 6.258;
pwr_height = 11.0;
eth_width = 16.0;
eth_offset = -9.315;
eth_height = 14.5;

$fn= $preview ? 32 : 64;

w = pcb_width+box_thickness*2;
h = pcb_height+box_thickness*2;
pwr_z = box_depth - pwr_height;
eth_z = box_depth - eth_height;
union() {
    translate([-w/2, -pcb_offset-h/2, 0]) cube([w, h, box_thickness]);
    translate([-pcb_width/2, -pcb_offset-pcb_height/2, box_thickness]) cube([pcb_width, pcb_height, box_thickness]);
    translate([(w-box_thickness)/2, pwr_offset, box_thickness+pwr_z/2]) cube([box_thickness, pwr_width, pwr_z], center=true);
    //translate([(w-box_thickness)/2, eth_offset, box_thickness+eth_z/2]) cube([box_thickness, eth_width, eth_z], center=true);
}