#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from server import version, server
import utils
import argparse
import logging
import threading
import sys

__author__ = 'Matteo Marescotti'

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='=== SMTS version {} ==='.format(version))

    parser.add_argument('--version', action='version', version=str(version))
    parser.add_argument('-c', dest='config_path', nargs='+', type=lambda value: server.config.extend(value),
                        help='config files path. following file updates previous one, and so on ...')
    parser.add_argument('-d', dest='db_path', help='sqlite3 database file path')
    parser.add_argument('-g', dest='gui', action='store_true', help='run gui in live mode')
    lg = parser.add_argument_group('lemma sharing')
    lg.add_argument('-l', dest='lemma', action='store_true', help='enable lemma sharing')
    lg.add_argument('-D', dest='lemmaDB', action='store_true', help='store lemmas in database')
    lg.add_argument('-a', dest='lemmaAgain', action='store_true', help='send lemmas again to solver')
    sg = parser.add_argument_group('solvers')
    sg.add_argument('-o', dest='opensmt', type=int, metavar='N', help='run N opensmt2 solvers')
    sg.add_argument('-z', dest='z3spacer', type=int, metavar='N', help='run N z3spacer solvers')

    args = parser.parse_args()

    if args.db_path:
        server.config.db_path = args.db_path
    server.config.db()

    logging.basicConfig(level=server.config.log_level, format='%(asctime)s\t%(levelname)s\t%(message)s')

    ps = server.ParallelizationServer(logging.getLogger('server'))

    if args.gui:
        utils.install_gui()
        gui_thread = threading.Thread(target=utils.run_gui, args=(['-s', str(server.config.port)],))
        gui_thread.daemon = True
        gui_thread.start()

    if server.config.files_path:
        files_thread = threading.Thread(target=utils.send_files, args=(server.config.port, server.config.files_path))
        files_thread.daemon = True
        files_thread.start()

    if args.lemma:
        lemma_thread = threading.Thread(target=utils.run_lemma_server, args=(
            server.config.build_path + '/lemma_server',
            server.config.db_path if args.lemmaDB else None,
            args.lemmaAgain
        ))
        lemma_thread.daemon = True
        lemma_thread.start()

    if args.opensmt or args.z3spacer:
        solvers_thread = threading.Thread(target=utils.run_solvers, args=(
            (server.config.build_path + '/solver_opensmt', args.opensmt),
            (server.config.build_path + '/solver_z3spacer', args.z3spacer)
        ))
        solvers_thread.daemon = True
        solvers_thread.start()

    try:
        ps.run_forever()
    except KeyboardInterrupt:
        sys.exit(0)
