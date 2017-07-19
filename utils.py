#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from server import server, net, version, framework
import subprocess
import logging
import socket
import sys
import pathlib
import argparse

__author__ = 'Matteo Marescotti'


def install_gui():
    for cmd in (['npm', 'install'], ['npm', 'run', 'build']):
        if subprocess.Popen(cmd, cwd='gui', stdout=subprocess.DEVNULL).wait() != 0:
            logging.log(logging.ERROR, 'GUI: `{}` error'.format(' '.join(cmd)))


def run_gui(args):
    _, err = subprocess.Popen(['npm', 'start', '--'] + args,
                              stdout=subprocess.DEVNULL,
                              stderr=subprocess.PIPE,
                              cwd='gui').communicate()
    logging.log(logging.ERROR, 'GUI terminated: {}'.format(err))


def run_lemma_server(lemma_server, database, send_again):
    # searching for a better IP here because
    # this IP will be sent to the solvers
    # that perhaps are on another host, this 127.0.0.1 would not work
    ip = '127.0.0.1'
    try:
        ip = socket.gethostbyname(socket.gethostname())
    except:
        pass

    args = [lemma_server, '-s', ip + ':' + str(server.config.port)]
    if database:
        database = pathlib.Path(database)
        args += ['-d', database.parent / (database.stem + 'lemma.db')]
    if send_again:
        args += ['-a']
    try:
        lemma_server = subprocess.Popen(args)
    except BaseException as ex:
        print(ex)
    else:
        lemma_server.wait()


def run_solvers(*solvers):
    for path, n in solvers:
        if n:
            try:
                for _ in range(n):
                    subprocess.Popen([path, '-s127.0.0.1:' + str(server.config.port)])
            except BaseException as ex:
                logging.log(logging.ERROR, 'error `{}` while running `{}`'.format(ex, path))


def send_files(port, files):
    for path in files:
        try:
            send_file(('127.0.0.1', port), path)
        except:
            pass


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


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='=== SMTS version {} ==='.format(version))

    parser.add_argument('--version', action='version', version=str(version))
    mxg = parser.add_mutually_exclusive_group()
    mxg.add_argument('-g', dest='gui', metavar='DB_PATH', help='analyse database with GUI')
    mxg.add_argument('-s', dest='smt2', action='store_true', help='stdin smt2, stdout json')
    mxg.add_argument('-j', dest='json', action='store_true', help='stdin json, stdout smt2')

    args = parser.parse_args()

    if args.gui:
        try:
            install_gui()
            run_gui(['-d', str(pathlib.Path(args.gui).absolute())])
        except KeyboardInterrupt:
            sys.exit(0)

    if args.smt2:
        print(framework.smt2json(sys.stdin.read(), True))

    if args.json:
        print(framework.json2smt(sys.stdin.read(), True))
