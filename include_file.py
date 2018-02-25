#!/usr/bin/env python3
# Quick and dirty way to include a file as a static C buffer,
# properly escaping \ and " characters
import sys
import json

with open(sys.argv[2], 'w') as fo:
    fo.write('/* this is generated code by include_file.py. Do not edit */\n')
    fo.write('const char * ' + sys.argv[1].replace('.', '_') + '=' +
             json.dumps(open(sys.argv[1], 'r').read()) + ';')
