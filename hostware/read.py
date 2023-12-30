#!/bin/bash

import socket
import sys
import re

def parse_smartmeter_data(data, param, rtype):
    m = re.search(re.escape(param) + '\\(([^\\*\\)]*)\\*?(.*)\\)', data)
    if m is None:
        return None, None
    return rtype(m.group(1)), m.group(2)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect(("192.168.1.51", 1337))
    current_data = ''
    while True:
        data = s.recv(100).decode()
        sys.stdout.write(data)
        current_data += data
        if 'ERRORS' in current_data:
            power, unit = parse_smartmeter_data(current_data, '1.7.0', float)
            errors, _ = parse_smartmeter_data(current_data, 'ERRORS', int)
            sys.stdout.write("Power: {} {} ({} errors)\r\n".format(power, unit, errors))
            current_data = ""
