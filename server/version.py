#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pathlib
import sys
import subprocess
import os

__author__ = 'Matteo Marescotti'

if sys.version_info.major != 3 or sys.version_info.minor < 5:
    sys.stderr.write('Python 3.5 is needed!\n')
    sys.exit(-1)

##++ use it similarly as in opensmt; do not introduce dependance on Git during runtime
# version = int(subprocess.check_output([str(pathlib.Path(__file__).parent.parent / 'version.sh')]).strip())
version = 1

if __name__ == '__main__':
    print(version)
