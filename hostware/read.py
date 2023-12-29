#!/bin/bash

import socket
import sys

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect(("192.168.1.99", 80))
    while True:
        sys.stdout.write(s.recv(100).decode('ascii', 'ignore'))
