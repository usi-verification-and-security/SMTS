#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from version import version
import schedular as schedular
import utils
import argparse
import logging
import threading
import sys

__author__ = 'Matteo Marescotti'

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='=== SMTS version {} ==='.format(version))

    parser.add_argument('--version', action='version', version=str(version))
    parser.add_argument('-c', dest='config_path', nargs='+', type=lambda value: schedular.config.extend(value),
                        help='config files path. following files update previous ones')
    parser.add_argument('-L', dest='list', action='store_true', help='list config parameters and exit')
    parser.add_argument('-d', dest='db_path', help='sqlite3 database file path')
    parser.add_argument('-fp', dest='file_paths', help='file paths')
    parser.add_argument('-g', dest='gui', action='store_true', help='run GUI in live mode')
    parser.add_argument('-Pf', dest='print_filename', action='store_true', help='print filename in front of the result')
    parser.add_argument('-Pt', dest='print_runtime', action='store_true', help='print elapsed runtime')
    parser.add_argument('-el', dest='log_mode', action='store_true', help='enable logging system-widely')
    pg = parser.add_argument_group('partitioning')
    pg.add_argument('-pt', dest='partition_timeout', type=int, metavar='N', help='partition timeout')
    pg.add_argument('-nt', dest='node_timeout', type=int, metavar='N', help='node timeout')
    pg.add_argument('-p', dest='partitioning', action='store_true', help='enable partitioning')
    lg = parser.add_argument_group('lemma sharing')
    lg.add_argument('-l', dest='lemma_sharing', action='store_true', help='enable lemma sharing')
    lg.add_argument('-D', dest='lemma_db', action='store_true', help='store lemmas in database')
    lg.add_argument('-r', dest='lemma_resend', action='store_true', help='send same lemmas multiple times to solver')
    sg = parser.add_argument_group('solvers')
    sg.add_argument('-o', dest='opensmt', type=int, metavar='N', help='run N opensmt2 solvers')
    sg.add_argument('-z', dest='z3spacer', type=int, metavar='N', help='run N z3spacer solvers')
    sg.add_argument('-s', dest='sally', type=int, metavar='N', help='run N sally solvers')
    sg.add_argument('-pn', dest='port', type=int, metavar='N', help='port number')

    args = parser.parse_args()
    port = schedular.config.port
    if args.port:
        port = args.port
    if args.db_path:
        schedular.config.db_path = args.db_path
    schedular.config.db()

    if args.partition_timeout:
        schedular.config.partition_timeout = args.partition_timeout
    if args.node_timeout:
        schedular.config.node_timeout = args.node_timeout
    if not args.partitioning:
        schedular.config.partition_timeout = None
    if args.print_filename:
        schedular.config.printFilename = args.print_filename
    if args.print_runtime:
        schedular.config.printRuntime = args.print_runtime
    if args.log_mode:
        schedular.config.enableLog = args.log_mode
    if args.gui:
        schedular.config.gui = args.gui
    # if args.lemma_sharing:
    #     schedular.config.lemma_sharing = args.lemma_sharing
    if args.lemma_db:
        schedular.config.lemma_db_path = args.lemma_db
    if args.lemma_resend:
        schedular.config.lemma_resend = args.lemma_resend
    if args.opensmt:
        schedular.config.opensmt = args.opensmt
    if args.z3spacer:
        schedular.config.z3spacer = args.z3spacer
    if args.sally:
        schedular.config.sally = args.sally

    if args.list:
        for attr_name in dir(schedular.config):
            if attr_name.startswith('_'):
                continue
            attr = getattr(schedular.config, attr_name)
            if type(attr) not in [list, dict, str, int, bool, type(None)]:
                continue
            if schedular.config.enableLog:
                print('{} = {}'.format(attr_name, repr(getattr(schedular.config, attr_name))))
        sys.exit(0)
    ps = schedular.ParallelizationServer(logging.getLogger('schedular'), port)
    if schedular.config.gui:
        if not schedular.config.db_path:
            logging.error('GUI requires a database. please specify one with -d')
            sys.exit(-1)
        utils.gui_install()
        gui_thread = threading.Thread(target=utils.gui_start, args=(['-s', str(port)],))
        gui_thread.daemon = True
        gui_thread.start()

    # done in separate thread because gethostbyname could take time
    if args.lemma_sharing:
        lemma_thread = threading.Thread(target=utils.run_lemma_server, args=(
            schedular.config.build_path + '/lemma_server',
            schedular.config.db_path if schedular.config.lemma_db_path else None,
            schedular.config.lemma_resend,
            port,
            schedular.config.enableLog
        ))
        lemma_thread.daemon = True
        lemma_thread.start()

    if schedular.config.opensmt or schedular.config.z3spacer or schedular.config.sally:
        utils.run_solvers(
            (schedular.config.build_path + '/solver_opensmt', schedular.config.opensmt, port),
            (schedular.config.build_path + '/solver_z3spacer', schedular.config.z3spacer, port),
            (schedular.config.build_path + '/solver_sally', schedular.config.sally, port)
        )

    if args.file_paths:
        files_thread = threading.Thread(target=utils.send_files,
                                        args=(args.file_paths, ('127.0.0.1', port)))
        files_thread.daemon = True
        files_thread.start()
    try:
        ps.run_forever(schedular.config.enableLog)
    except KeyboardInterrupt:
        sys.exit(0)
