#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import subprocess

__author__ = 'Matteo Marescotti'

if sys.version_info.major != 3 or sys.version_info.minor < 5:
    sys.stderr.write('Python 3.5 is needed!\n')
    sys.exit(-1)

version = int(subprocess.check_output(['git', 'rev-list', '--count', 'master']))

if __name__ == '__main__':
    print(version)
