#!/bin/bash

import socket
import sys
import re

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect(("192.168.1.51", 80))
    current_data = ""
    while True:
        data = s.recv(100).decode('ascii', 'ignore')
        sys.stdout.write(data)
        current_data += data
        if current_data.endswith('!\r\n'):
            m = re.search("1\\.7\\.0\\(([0-9\\.]+).*\\)", current_data, re.M)
            if m:
                sys.stdout.write("Power: {} W\r\n".format(float(m.group(1)) * 1000))
            current_data = ""
