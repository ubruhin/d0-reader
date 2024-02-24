#!/bin/sh

openocd -f "$(dirname "$0")/openocd.cfg" -c "program $@ verify reset exit"
