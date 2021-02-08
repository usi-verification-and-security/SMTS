#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from version import version
import net
import utils
import sys
import pathlib
import argparse
import readline

__author__ = 'Matteo Marescotti'

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='=== SMTS version {} ==='.format(version))

    parser.add_argument('--version', action='version', version=str(version))
    parser.add_argument('host_port', metavar='[host:]port', nargs=1, help='default host=127.0.0.1')
    parser.add_argument('files', metavar='file', nargs='*', help='SMT files to submit. CLI mode if empty')
    parser.add_argument('-terminate',metavar='q', help='Terminating SMTS Server from client By q')

    args = parser.parse_args()

    components = args.host_port[0].split(':', maxsplit=1)
    if len(components) == 1:
        components = ['127.0.0.1'] + components
    try:
        address = (components[0], int(components[1]))
    except:
        print('invalid host:port', file=sys.stderr)
        sys.exit(1)

    try:
        socket = net.Socket()
        socket.connect(address)
        # Terminate server from client
        if args.terminate is not None and args.terminate[0] == 'q':
            utils.send_command(args.terminate, socket)
        elif args.files:
            for path in args.files:
                try:
                    utils.send_file(path, socket)
                except FileNotFoundError:
                    print('File not found: {}'.format(path), file=sys.stderr)
        else:
            history_file = pathlib.Path.home() / '.smts_client_history'
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
        socket.close()
    except ConnectionError as ex:
        print(ex, file=sys.stderr)
        sys.exit(1)
