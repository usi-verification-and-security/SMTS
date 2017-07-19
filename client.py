#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from server.version import version
from server import net
import sys
import pathlib
import argparse
import readline

__author__ = 'Matteo Marescotti'


def send_file(address, path):
    path = pathlib.Path(path).resolve()
    if path.suffix == '.bz2':
        import bz2
        with bz2.open(str(path)) as file:
            content = file.read().decode()
        name = pathlib.Path(path.stem).stem
    else:
        with open(str(path), 'r') as file:
            content = file.read()
        name = path.stem
    socket = net.Socket()
    socket.connect(address)
    socket.write({
        'command': 'solve',
        'name': name
    }, content)


def terminal(address):
    history_file = pathlib.Path.home() / '.smts_client_history'
    socket = net.Socket()
    socket.connect(address)
    if sys.stdin.isatty() and history_file.is_file():
        readline.read_history_file(str(history_file.resolve()))
    while True:
        try:
            if sys.stdin.isatty():
                line = input('{}:{}> '.format(*socket.remote_address))
                readline.write_history_file(str(history_file))
            else:
                line = input()
        except (KeyboardInterrupt, EOFError):
            print()
            break
        socket.write({'eval': line.strip()}, '')
        header, message = socket.read()
        print(message.decode())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='=== SMTS version {} ==='.format(version))

    parser.add_argument('--version', action='version', version=str(version))
    parser.add_argument('host_port', metavar='[host:]port', nargs=1, help='default host=127.0.0.1')
    parser.add_argument('files', metavar='file', nargs='*', help='SMT files to submit. CLI mode if empty')

    args = parser.parse_args()

    components = args.host_port[0].split(':', maxsplit=1)
    if len(components) == 1:
        components = ['127.0.0.1'] + components
    try:
        address = (components[0], int(components[1]))
    except:
        print('invalid host:port', file=sys.stderr)
        sys.exit(-1)

    try:
        if args.files:
            for path in args.files:
                try:
                    send_file(address, path)
                except FileNotFoundError:
                    print('File not found: {}'.format(path), file=sys.stderr)
        else:
            terminal(address)
    except ConnectionError as ex:
        print(ex)
