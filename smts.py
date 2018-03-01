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
                        help='config files path. following files update previous ones')
    parser.add_argument('-L', dest='list', action='store_true', help='list config parameters and exit')
    parser.add_argument('-d', dest='db_path', help='sqlite3 database file path')
    parser.add_argument('-g', dest='gui', action='store_true', help='run GUI in live mode')
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

    if args.list:
        for attr_name in dir(server.config):
            if attr_name.startswith('_'):
                continue
            attr = getattr(server.config, attr_name)
            if type(attr) not in [list, dict, str, int, bool]:
                continue
            print('{}: {}'.format(attr_name, getattr(server.config, attr_name)))
        sys.exit(0)

    ps = server.ParallelizationServer(logging.getLogger('server'))

    if args.gui:
        if not server.config.db_path:
            logging.error('GUI requires a database. please specify one with -d')
            sys.exit(-1)
        utils.gui_install()
        gui_thread = threading.Thread(target=utils.gui_start, args=(['-s', str(server.config.port)],))
        gui_thread.daemon = True
        gui_thread.start()

    if server.config.files_path:
        files_thread = threading.Thread(target=utils.send_files, args=(server.config.port, server.config.files_path))
        files_thread.daemon = True
        files_thread.start()

    if args.lemma:
        # done in separate thread because gethostbyname could take time
        lemma_thread = threading.Thread(target=utils.run_lemma_server, args=(
            server.config.build_path + '/lemma_server',
            server.config.db_path if args.lemmaDB else None,
            args.lemmaAgain
        ))
        lemma_thread.daemon = True
        lemma_thread.start()

    if args.opensmt or args.z3spacer:
        utils.run_solvers(
            (server.config.build_path + '/solver_opensmt', args.opensmt),
            (server.config.build_path + '/solver_z3spacer', args.z3spacer)
        )

    try:
        ps.run_forever()
    except KeyboardInterrupt:
        sys.exit(0)
