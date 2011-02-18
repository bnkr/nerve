#!/bin/sh
g++ -c -o /tmp/dwarf.o dwarfreader.c || exit 1
g++ -o /tmp/dwarf -lelf /tmp/dwarf.o /usr/lib/libdwarf.a && /tmp/dwarf
