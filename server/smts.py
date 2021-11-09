#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from version import version
import server as server
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
    lg.add_argument('-l', dest='lemma_sharing', action='store_true', help='enable lemma sharing')
    lg.add_argument('-D', dest='lemma_db', action='store_true', help='store lemmas in database')
    lg.add_argument('-r', dest='lemma_resend', action='store_true', help='send same lemmas multiple times to solver')
    sg = parser.add_argument_group('solvers')
    sg.add_argument('-o', dest='opensmt', type=int, metavar='N', help='run N opensmt2 solvers')
    sg.add_argument('-z', dest='z3spacer', type=int, metavar='N', help='run N z3spacer solvers')
    sg.add_argument('-s', dest='sally', type=int, metavar='N', help='run N sally solvers')

    args = parser.parse_args()

    if args.db_path:
        server.config.db_path = args.db_path
    server.config.db()

    if args.gui:
        server.config.gui = args.gui
    if args.lemma_sharing:
        server.config.lemma_sharing = args.lemma_sharing
    if args.lemma_db:
        server.config.lemma_db_path = args.lemma_db
    if args.lemma_resend:
        server.config.lemma_resend = args.lemma_resend
    if args.opensmt:
        server.config.opensmt = args.opensmt
    if args.z3spacer:
        server.config.z3spacer = args.z3spacer
    if args.sally:
        server.config.sally = args.sally

    if args.list:
        for attr_name in dir(server.config):
            if attr_name.startswith('_'):
                continue
            attr = getattr(server.config, attr_name)
            if type(attr) not in [list, dict, str, int, bool, type(None)]:
                continue
            if server.config.enableLog:
                print('{} = {}'.format(attr_name, repr(getattr(server.config, attr_name))))
        sys.exit(0)

    ps = server.ParallelizationServer(logging.getLogger('server'))
    if server.config.gui:
        if not server.config.db_path:
            logging.error('GUI requires a database. please specify one with -d')
            sys.exit(-1)
        utils.gui_install()
        gui_thread = threading.Thread(target=utils.gui_start, args=(['-s', str(server.config.port)],))
        gui_thread.daemon = True
        gui_thread.start()

    if server.config.files_path:
        files_thread = threading.Thread(target=utils.send_files,
                                        args=(server.config.files_path, ('127.0.0.1', server.config.port)))
        files_thread.daemon = True
        files_thread.start()

    if server.config.lemma_sharing:
        # done in separate thread because gethostbyname could take time
        lemma_thread = threading.Thread(target=utils.run_lemma_server, args=(
            server.config.build_path + '/lemma_server',
            server.config.db_path if server.config.lemma_db_path else None,
            server.config.lemma_resend
        ))
        lemma_thread.daemon = True
        lemma_thread.start()

    if server.config.opensmt or server.config.z3spacer or server.config.sally:
        utils.run_solvers(
            (server.config.build_path + '/solver_opensmt', server.config.opensmt),
            (server.config.build_path + '/solver_z3spacer', server.config.z3spacer),
            (server.config.build_path + '/solver_sally', server.config.sally)
        )

    try:
        ps.run_forever()
    except KeyboardInterrupt:
        sys.exit(0)
