#!/bin/sh

openocd -f openocd.cfg -c "program $@ verify reset exit"
