#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from server import server, version
import client
import argparse
import logging
import threading
import socket
import pathlib
import subprocess
import sys

__author__ = 'Matteo Marescotti'

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='=== SMTS version {} ==='.format(version))

    parser.add_argument('--version', action='version', version=str(version))
    parser.add_argument('-c', dest='config_path', type=lambda value: server.config.extend(value),
                        help='config file path')
    parser.add_argument('-d', dest='db_path', help='sqlite3 database file path')
    parser.add_argument('-g', dest='gui', action='store_true', help='run gui')
    parser.add_argument('-l', dest='lemma', action='store_true', help='enable lemma sharing')
    parser.add_argument('-D', dest='lemmaDB', action='store_true', help='store lemmas in database')
    parser.add_argument('-a', dest='lemmaAgain', action='store_true', help='send lemmas again to solver')
    parser.add_argument('-o', dest='opensmt', type=int, metavar='N', help='run N opensmt2 solvers')
    parser.add_argument('-z', dest='z3spacer', type=int, metavar='N', help='run N z3spacer solvers')

    args = parser.parse_args()

    if args.db_path:
        server.config.db_path = args.db_path
    server.config.db()

    logging.basicConfig(level=server.config.log_level, format='%(asctime)s\t%(levelname)s\t%(message)s')

    ps = server.ParallelizationServer(logging.getLogger('server'))

    if server.config.files_path:
        def send_files(address, files):
            for path in files:
                try:
                    client.send_file(address, path)
                except:
                    pass


        files_thread = threading.Thread(target=send_files, args=(ps.address, server.config.files_path))
        files_thread.daemon = True
        files_thread.start()

    if args.lemma:
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


        lemma_thread = threading.Thread(target=run_lemma_server, args=(
            server.config.build_path + '/lemma_server',
            server.config.db_path if args.lemmaDB else None,
            args.lemmaAgain
        ))
        lemma_thread.daemon = True
        lemma_thread.start()

    if args.opensmt or args.z3spacer:
        def run_solvers(*solvers):
            for path, n in solvers:
                try:
                    for _ in range(n):
                        subprocess.Popen([path, '-s127.0.0.1:' + str(server.config.port)])
                except BaseException as ex:
                    print(ex)


        solvers_thread = threading.Thread(target=run_solvers, args=(
            (server.config.build_path + '/solver_opensmt', args.opensmt),
            (server.config.build_path + '/solver_z3spacer', args.z3spacer)
        ))
        solvers_thread.daemon = True
        solvers_thread.start()

    if args.gui:
        def run_gui():
            subprocess.Popen(['npm', 'run', 'all', '--', '-s', str(server.config.port)], cwd='gui')


        gui_thread = threading.Thread(target=run_gui)
        gui_thread.daemon = True
        gui_thread.start()

    try:
        ps.run_forever()
    except KeyboardInterrupt:
        sys.exit(0)
