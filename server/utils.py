#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from version import version
import server
import net
import framework
import subprocess
import logging
import socket
import sys
import pathlib
import argparse
import os
import threading

__author__ = 'Matteo Marescotti'

logging.basicConfig(level=server.config.log_level, format='%(asctime)s\t%(levelname)s\t%(message)s')


def run_log(args, **kwargs):
    def log(fd, level):
        for line in os.fdopen(fd):
            logging.log(level, line.strip())

    stdout_out, stdout_in = os.pipe()
    stderr_out, stderr_in = os.pipe()

    if 'stdout' not in kwargs:
        kwargs['stdout'] = stdout_in
    if 'stderr' not in kwargs:
        kwargs['stderr'] = stderr_in

    threads = []
    for log_args in ((stdout_out, logging.INFO), (stderr_out, logging.ERROR)):
        thread = threading.Thread(target=log, args=log_args)
        thread.daemon = True
        thread.start()
        threads.append(thread)

    subprocess.run(args, **kwargs)
    os.close(stdout_in)
    os.close(stderr_in)
    for thread in threads:
        thread.join()


def gui_install():
    run_log(['npm', 'install'], cwd='../gui', stdout=subprocess.DEVNULL)


def gui_start(args):
    run_log(['npm', 'start', '--silent', '--'] + args, cwd='../gui')


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
        args += ['-d', str(database.parent / (database.stem + '.lemma.db'))]
    if send_again:
        args += ['-a']
    try:
        return subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except BaseException as ex:
        print(ex)


def run_solvers(*solvers):
    ps = []
    for path, n in solvers:
        if n:
            for _ in range(n):
                try:
                    ps.append(subprocess.Popen([path, '-s127.0.0.1:' + str(server.config.port)],
                                               stdout=subprocess.DEVNULL,
                                               stderr=subprocess.DEVNULL))
                except BaseException as ex:
                    logging.log(logging.ERROR, 'error "{}" while running "{}"'.format(ex, path))
    return ps


def send_files(files, address):
    socket = net.Socket()
    socket.connect(address)
    for path in files:
        try:
            send_file(path, socket)
        except:
            pass
    socket.close()


def send_file(path, socket):
    path = pathlib.Path(path)
    path.resolve()
    if path.suffix == '.bz2':
        import bz2
        with bz2.open(str(path)) as file:
            content = file.read().decode()
        name = pathlib.Path(path.stem).stem
    else:
        with open(str(path), 'r') as file:
            content = file.read()
        name = path.stem
    socket.write({
        'command': 'solve',
        'name': name
    }, content)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='=== SMTS version {} ==='.format(version))

    parser.add_argument('--version', action='version', version=str(version))
    mxg = parser.add_mutually_exclusive_group()
    mxg.add_argument('-g', dest='gui', metavar='DB_PATH', help='analyse database with GUI')
    mxg.add_argument('-s', dest='smt2', metavar='FILE', nargs='?', const=True, help='smt to json. stdin if empty')
    mxg.add_argument('-j', dest='json', metavar='FILE', nargs='?', const=True, help='json to smt. stdin if empty')

    args = parser.parse_args()

    if args.gui:
        try:
            gui_install()
            gui_start(['-d', str(pathlib.Path(args.gui).absolute())])
        except KeyboardInterrupt:
            sys.exit(0)

    if args.smt2:
        if args.smt2 is True:
            file = sys.stdin
        else:
            file = open(args.smt2, 'r')
        print(framework.smt2json(file.read(), True))

    if args.json:
        if args.json is True:
            file = sys.stdin
        else:
            file = open(args.json, 'r')
        print(framework.json2smt(file.read(), True))
