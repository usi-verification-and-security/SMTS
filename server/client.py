#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import net
import sys
import bz2
import pathlib
import readline

__author__ = 'Matteo Marescotti'


def send_file(address, path):
    path = pathlib.Path(path).resolve()
    if path.suffix == '.bz2':
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
    if len(sys.argv) < 2:
        print('Usage: {} [host=127.0.0.1:]<port> [file [...]]'.format(sys.argv[0]), file=sys.stderr)
        sys.exit(1)
    else:
        if ':' not in sys.argv[1]:
            sys.argv[1] = '127.0.0.1:' + sys.argv[1]
        components = sys.argv[1].split(':')
        address = (components[0], int(components[1]))

    if len(sys.argv) > 2:
        for path in sys.argv[2:]:
            try:
                send_file(address, path)
            except FileNotFoundError:
                print('File not found: {}'.format(path), file=sys.stderr)
            except ConnectionError:
                print('Connection error.', file=sys.stderr)
                sys.exit(1)
    else:
        terminal(address)
