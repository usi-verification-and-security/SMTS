#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from version import version
import utils
import net
import config
import random
import argparse
import time
import sys
import logging
import os
import signal
import multiprocessing
import subprocess

__author__ = 'Matteo Marescotti'


def solve(w_sat, w_unsat, w_timeout, max):
    status = random.choices(['sat', 'unsat', None], [w_sat, w_unsat, w_timeout])[0]
    delay = random.uniform(0, max) if status else float('inf')
    if delay != float('inf'):
        logging.info('reporting {} in {:.2f} seconds'.format(status, delay))
        time.sleep(delay)
    else:
        logging.info('going timeout')
        signal.pause()
    return status


def solver(address, w_sat, w_unsat, w_timeout, max):
    def solve_process(sock, name, node):
        sys.stderr = open(os.devnull, 'w')
        status = solve(w_sat, w_unsat, w_timeout, max)
        sock.write({'name': name, 'node': node, 'report': status}, '')

    p = name = node = None
    sock = net.Socket()
    sock.connect(address)
    sock.write({'solver': 'dummy solver'}, '')
    while True:
        try:
            header, message = sock.read()
        except ConnectionAbortedError:
            break
        if p and header['command'] == 'stop':
            p.terminate()
            p = name = node = query = smt = None
        elif header['command'] in ['solve', 'incremental']:
            if p:
                p.terminate()
            name = header['name']
            node = header['node'] if 'node_' not in header else header['node_']
            query = header['query'] if 'query' in header else ''
            smt = message if header['command'] == 'solve' else smt + message
            logging.info('solving {}'.format(name + node))
            p = multiprocessing.Process(target=solve_process, args=(sock, name, node))
            p.daemon = True
            p.start()
        elif header['command'] == 'partition':
            logging.info('creating {} partitions'.format(header['partitions']))
            message = '\0'.join(['true' for _ in range(int(header['partitions']))])
            sock.write({'name': name, 'node': node, 'report': 'partitions'}, message)
        elif header['command'] == 'cnf-clauses':
            # run solver_opensmt with -d
            # and send back the clauses
            if 'query' in header:
                query = header['query']
                smt = message
            filename_smt = str(utils.TempFile())
            open(filename_smt, 'w').write('{}{}'.format(smt.decode() if smt else '', query if query else ''))
            subprocess.Popen([config.build_path + '/solver_opensmt', '-d', filename_smt]).wait()
            filename_cnf = filename_smt + '.cnf'
            logging.info('sending CNF {} of {}'.format(filename_cnf, filename_smt))
            header["report"] = header["command"]
            header.pop('command')
            sock.write(header, open(filename_cnf).read())
        else:
            print(header, message)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='=== SMTS version {} ==='.format(version),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    parser.add_argument('--version', action='version', version=str(version))
    parser.add_argument('-S', dest='w_sat', type=float, default=0, help='sat weight')
    parser.add_argument('-U', dest='w_unsat', type=float, default=60, help='unsat weight')
    parser.add_argument('-t', dest='w_timeout', type=float, default=40, help='timeout weight')
    parser.add_argument('-m', dest='max', type=int, default=60, help='maximum solving time')
    parser.add_argument('-n', dest='n', type=int, default=1, help='number of solvers')
    parser.add_argument('-s', dest='host_port', metavar='host:port', type=str, default=None,
                        help='maximum solving time in seconds')

    args = parser.parse_args()

    components = args.host_port.split(':', maxsplit=1)
    try:
        address = (components[0], int(components[1]))
    except:
        print('invalid host:port', file=sys.stderr)
        sys.exit(1)


    def solver_process():
        sys.stderr = open(os.devnull, 'w')
        solver(address, args.w_sat, args.w_unsat, args.w_timeout, args.max)


    try:
        for _ in range(args.n):
            p = multiprocessing.Process(target=solver_process)
            p.start()
        signal.pause()
    except KeyboardInterrupt:
        pass
