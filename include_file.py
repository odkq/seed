#!/usr/bin/env python3
import sys
import json

with open(sys.argv[2], 'w') as fo:
    fo.write('const char * ' + sys.argv[1].replace('.', '_') + '=' +
             json.dumps(open(sys.argv[1], 'r').read()) + ';')
