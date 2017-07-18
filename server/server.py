#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from version import version
import framework
import net
import client
import config
import argparse
import threading
import socket
import json
import sys
import logging
import pathlib
import subprocess
import traceback
import random
import time

__author__ = 'Matteo Marescotti'


class LemmaServer(net.Socket):
    def __init__(self, sock: net.Socket, listening: str):
        super().__init__(sock._sock)
        self.listening = listening

    def __repr__(self):
        return '<LemmaServer listening:{}>'.format(self.listening)

    def reset(self, node):
        self.write({'name': node.root.name, 'node': node.path(), 'lemmas': '0'}, '')


class Solver(net.Socket):
    def __init__(self, sock: net.Socket, name: str):
        super().__init__(sock._sock)
        self.name = name
        self.node = None
        self.started = None
        self.or_waiting = []

    def __repr__(self):
        return '<{} at{} {}>'.format(
            self.name,
            self.remote_address,
            'idle' if self.node is None else '{}{}'.format(self.node.root, self.node.path())
        )

    def solve(self, node: framework.AndNode, header: dict):
        if self.node is not None:
            self.stop()
        self.node = node
        smt, query = self.node.root.to_string(self.node)
        header.update({
            'command': 'solve',
            'name': self.node.root.name,
            'node': self.node.path(),
            'query': query,
        })
        self.write(header, smt)
        self.started = time.time()
        self._db_log('+')

    def incremental(self, node: framework.AndNode):
        smt, query = self.node.root.to_string(node, self.node)
        header = {'command': 'incremental',
                  'name': self.node.root.name,
                  'node': self.node.path(),
                  'node_': node.path(),
                  'query': query,
                  }
        self.write(header, smt)
        self._db_log('-')
        self.started = time.time()
        self.node = node
        self._db_log('+')

    def stop(self):
        if self.node is None:
            raise ValueError('not solving anything')
        self.write({
            'command': 'stop',
            'name': self.node.root.name,
            'node': self.node.path()
        }, '')
        self._db_log('-')
        self.node = None
        self.or_waiting = []

    def set_lemma_server(self, lemma_server: LemmaServer = None):
        self.write({
            'command': 'lemmas',
            'lemmas': lemma_server.listening if lemma_server else ''
        }, '')

    def ask_partitions(self, n, node: framework.AndNode = None):
        if self.node is None:
            raise ValueError('not solving anything')
        self.write({
            'command': 'partition',
            'name': self.node.root.name,
            'node': self.node.path(),
            'partitions': n
        }, '')
        if not node:
            node = framework.OrNode(self.node)
        self.or_waiting.append(node)
        self._db_log('OR', {'node': str(node.path())})

    def read(self):
        header, payload = super().read()
        if 'report' not in header:
            return {}, b''
        if header['report'] == 'partitions' and self.or_waiting:
            for node in self.or_waiting:
                if self.node.root.child(json.loads(header['node'])) is node.parent:
                    self.or_waiting.remove(node)
                    try:
                        for partition in payload.decode().split('\0'):
                            if len(partition) == 0:
                                continue
                            child = framework.AndNode(node, '(assert {})'.format(partition))
                            self._db_log('AND', {'node': str(child.path()), 'smt': child.smt})
                    except BaseException as ex:
                        header['report'] = 'error:(server) error reading partitions: {}'.format(traceback.format_exc())
                        node.clear()
                        # ask them again?
                    else:
                        header['report'] = 'info:(server) received {} partitions'.format(len(node))
                        if len(node) == 1:
                            node.clear()
                    return header, payload

        if self.node is None:
            return header, payload

        if self.node.root.name != header['name'] or str(self.node.path()) != header['node']:
            return {}, b''

        if header['report'] in framework.SolveStatus.__members__:
            status = framework.SolveStatus.__members__[header['report']]
            self._db_log('STATUS', header)
            self.node.status = status
            path = self.node.path(True)
            path.reverse()
            for node in path:
                if node.status != framework.SolveStatus.unknown and isinstance(node, framework.AndNode):
                    self._db_log('SOLVED', {'status': node.status.name, 'node': str(node.path())})
                else:
                    break
            if status == framework.SolveStatus.unknown:
                self.stop()

        return header, payload

    def _db_log(self, event: str, data: dict = None):
        if not config.db():
            return
        config.db().cursor().execute("INSERT INTO {}SolvingHistory (name, node, event, solver, data) "
                                     "VALUES (?,?,?,?,?)".format(config.table_prefix), (
                                         self.node.root.name,
                                         str(self.node.path()),
                                         event,
                                         str(self.remote_address),
                                         json.dumps(data) if data else None
                                     ))
        config.db().commit()


class Instance(object):
    def __init__(self, name: str, smt: str):
        self.root = framework.parse(name, smt)
        self.started = None
        self.timeout = None

    def __repr__(self):
        return '{}({:.2f})'.format(repr(self.root), self.when_timeout)

    @property
    def when_timeout(self):
        return self.started + self.timeout - time.time() if self.started and self.timeout else float('inf')


class ParallelizationServer(net.Server):
    def __init__(self, logger: logging.Logger = None):
        super().__init__(port=config.port, timeout=1, logger=logger)
        self.config = config
        self.trees = []
        self.current = None
        self.log(logging.INFO, 'server start. version {}'.format(version))

    def handle_accept(self, sock):
        self.log(logging.DEBUG, 'new connection from {}'.format(sock.remote_address))

    def handle_message(self, sock, header, payload):
        self.log(logging.DEBUG, 'message from {}'.format(sock.remote_address),
                 {'header': header, 'payload': payload.decode()})
        if isinstance(sock, Solver):
            if not header:
                return
            if 'report' in header:
                try:
                    level, message = header['report'].split(':', 1)
                    level = logging._nameToLevel[level.upper()]
                except:
                    level = logging.INFO
                    message = header['report']
                self.log(level, '{}: {}'.format(sock, message), {'header': header, 'payload': payload.decode()})
            self.entrust()
            return
        if 'command' in header:
            if header['command'] == 'solve':
                if 'name' not in header:
                    return
                self.log(logging.INFO, 'new instance "{}"'.format(
                    header['name']
                ), {'header': header})
                try:
                    instance = Instance(header["name"], payload.decode())
                    if isinstance(instance.root, framework.Fixedpoint) and config.fixedpoint_partition:
                        instance.root.partition()
                except:
                    self.log(logging.ERROR, 'cannot add instance: {}'.format(traceback.format_exc()))
                else:
                    instance.timeout = config.solving_timeout
                    self.trees.append(instance)
                self.entrust()
        elif 'solver' in header:
            solver = Solver(sock, header['solver'])
            self.log(logging.INFO, 'new {}'.format(
                solver,
            ), {'header': header, 'payload': payload.decode()})
            self._rlist.remove(sock)
            self._rlist.add(solver)
            lemma_server = self.lemma_server
            if lemma_server:
                solver.set_lemma_server(lemma_server)
            self.entrust()
        elif 'lemmas' in header:
            if header['lemmas'][0] == ':':
                header['lemmas'] = sock.remote_address[0] + header['lemmas']
            lemma_server = self.lemma_server
            if lemma_server:
                lemma_server.close()
            self._rlist.remove(sock)
            lemma_server = LemmaServer(sock, header["lemmas"])
            self.log(logging.INFO, 'new {}'.format(
                lemma_server
            ), {'header': header, 'payload': payload.decode()})
            self._rlist.add(lemma_server)
            for solver in (solver for solver in self._rlist if isinstance(solver, Solver)):
                solver.set_lemma_server(lemma_server)
        elif 'eval' in header:
            response_payload = ''
            try:
                if header['eval']:
                    response_payload = str(eval(header['eval']))
            except:
                response_payload = str(traceback.format_exc())
            finally:
                sock.write({}, response_payload)

    def handle_close(self, sock):
        self.log(logging.DEBUG, 'connection closed by {}'.format(
            sock
        ))
        if isinstance(sock, Solver):
            if sock.or_waiting:
                self.log(logging.WARNING, '{} had waiting or-nodes {}'.format(
                    sock,
                    sock.or_waiting
                ))
                # todo: manage what to do now
        if isinstance(sock, LemmaServer):
            for solver in self.solvers(False):
                solver.set_lemma_server()

    def handle_timeout(self):
        self.entrust()

    def entrust(self):
        solving = self.current
        # if the current tree is already solved or timed out: stop it
        if isinstance(self.current, Instance):
            if self.current.root.status != framework.SolveStatus.unknown or self.current.when_timeout < 0:
                self.log(
                    logging.INFO,
                    '{} instance "{}" after {:.2f} seconds'.format(
                        'solved' if self.current.root.status != framework.SolveStatus.unknown else 'timeout',
                        self.current.root.name,
                        time.time() - self.current.started
                    )
                )
                for solver in {solver for solver in self.solvers(True) if solver.node.root == self.current.root}:
                    solver.stop()
                self.current = None
        if self.current is None:
            schedulables = [instance for instance in self.trees if
                            instance.root.status == framework.SolveStatus.unknown and instance.when_timeout > 0]
            if schedulables:
                self.current = schedulables[0]
                self.log(logging.INFO, 'solving instance "{}"'.format(self.current.root.name))
        if solving is not None and solving != self.current and self.lemma_server:
            self.lemma_server.reset(solving.root)
        if self.current is None:
            if solving is not None:
                self.log(logging.INFO, 'all done.')
            return

        assert isinstance(self.current, Instance)

        idle_solvers = self.solvers(None)
        nodes = self.current.root.all()
        nodes.sort()

        def level_children(level):
            return self.config.partition_policy[level % len(self.config.partition_policy)]

        def leaves():
            return (node for node in nodes if len(node) == 0 and isinstance(node, framework.AndNode))

        def internals():
            return (node for node in nodes if len(node) > 0 and isinstance(node, framework.AndNode))

        # stop the solvers working on an already solved node of the whole tree, and add them to the list
        for node in nodes:
            if node.status != framework.SolveStatus.unknown:
                for solver in self.solvers(node):
                    idle_solvers.add(solver)

        # spread the solvers among the unsolved nodes taking care of portfolio_max
        # first the leafs will be filled. inner nodes after and only if available solvers are left idle
        # nodes.reverse()
        for selection in [leaves, internals]:
            available = -1
            while available != len(idle_solvers):
                available = len(idle_solvers)
                for node in selection():
                    if node.status != framework.SolveStatus.unknown:
                        continue
                    try:
                        # here i check whether some node in the path to the root is already solved
                        # could happen that an upper level node is solved while one on its subtree is still unsolved
                        for _node in node.path(True):
                            if _node.status != framework.SolveStatus.unknown:
                                raise StopIteration
                    except StopIteration:
                        continue
                    if 0 < self.config.portfolio_max <= len(self.solvers(node)):
                        continue
                    # now node needs to be solved.
                    # try to search for a solver...
                    if not idle_solvers:
                        for _node in (node for node in internals() if
                                      len(node) == level_children(len(node.path()))):
                            # here I check that every or-node child has some partitions, that is
                            # every child is completed. if not then I'll not use any solver working on that node.
                            try:
                                for child in _node:
                                    if len(child) == 0:
                                        raise StopIteration
                            except StopIteration:
                                continue
                            solvers = self.solvers(_node)
                            if 0 <= self.config.portfolio_min < len(solvers):
                                try:
                                    # I can choose only one solver if it's solving for more than partition_timeout
                                    for solver in self.solvers(_node):
                                        if self.config.partition_timeout and solver.started + self.config.partition_timeout > time.time():
                                            continue
                                        idle_solvers.add(solver)
                                        raise StopIteration
                                except StopIteration:
                                    break

                    # if there are still no solvers available
                    if not idle_solvers:
                        continue

                    if isinstance(self.current.root, framework.SMT):
                        if self.config.incremental > 0:
                            try:
                                # I first search for a solver which is solving an ancestor of node
                                # so that incremental solving would work better
                                for solver in idle_solvers:
                                    if not solver.node or solver.node is node:
                                        continue
                                    if solver.node.is_ancestor(node):
                                        idle_solvers.remove(solver)
                                        solver.incremental(node)
                                        raise StopIteration
                            except StopIteration:
                                continue
                        if self.config.incremental > 1:
                            try:
                                # try to use incremental on another already solving solver
                                for solver in idle_solvers:
                                    if solver.node and solver.node is not node:
                                        idle_solvers.remove(solver)
                                        solver.incremental(node)
                                        raise StopIteration
                            except StopIteration:
                                continue
                    solver = idle_solvers.pop()
                    assert isinstance(solver, Solver)
                    if solver.node is node:
                        continue
                    header = {}
                    if self.config.lemma_amount:
                        header["lemmas"] = self.config.lemma_amount
                    self.config.entrust(node, header, solver.name, self.solvers(node))
                    solver.solve(node, header)
                    if self.current.started is None:
                        self.current.started = time.time()

        # only standard instances can partition
        if not isinstance(self.current.root, framework.SMT):
            return

        # if need partition: ask partitions
        if self.config.partition_timeout or idle_solvers:
            # for all the leafs with at least one solver
            for leaf in (leaf for leaf in leaves() if self.solvers(leaf)):
                max_children = level_children(leaf.level)
                for i in range(max_children - len(leaf)):
                    solvers = list(self.solvers(leaf))
                    random.shuffle(solvers)
                    for solver in solvers:
                        # ask the solver to partition if timeout or if needed because idle solvers
                        if idle_solvers or solver.started + self.config.partition_timeout <= time.time():
                            solver.ask_partitions(level_children(leaf.level + 1))
                            break

    # node = False : return all solvers
    # node = True  : return all non idle solvers
    # node = None  : return all idle solvers
    def solvers(self, node):
        return {solver for solver in self._rlist
                if isinstance(solver, Solver) and (
                    node is False or
                    (node is True and solver.node is not None) or
                    solver.node == node
                )}

    @property
    def lemma_server(self) -> LemmaServer:
        lemmas = [sock for sock in self._rlist if isinstance(sock, LemmaServer)]
        if lemmas:
            return lemmas[0]

    def log(self, level, message, data=None):
        super().log(level, message)
        if not config.db() or level < self.config.log_level:
            return
        config.db().cursor().execute("INSERT INTO {}ServerLog (level, message, data) "
                                     "VALUES (?,?,?)".format(config.table_prefix), (
                                         logging.getLevelName(level),
                                         message,
                                         json.dumps(data) if data else None
                                     ))
        config.db().commit()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='=== SMTS version {} ==='.format(version))

    parser.add_argument('--version', action='version', version=str(version))
    parser.add_argument('-c', dest='config_path', type=lambda value: config.extend(value), help='config file path')
    parser.add_argument('-d', dest='db_path', help='sqlite3 database file path')
    parser.add_argument('-l', dest='lemma', action='store_true', help='enable lemma sharing')
    parser.add_argument('-D', dest='lemmaDB', action='store_true', help='store lemmas in database')
    parser.add_argument('-a', dest='lemmaAgain', action='store_true', help='send lemmas again to solver')
    parser.add_argument('-o', dest='opensmt', type=int, metavar='N', help='run N opensmt2 solvers')
    parser.add_argument('-z', dest='z3spacer', type=int, metavar='N', help='run N z3spacer solvers')

    args = parser.parse_args()

    if args.db_path:
        config.db_path = args.db_path
    config.db()

    logging.basicConfig(level=config.log_level, format='%(asctime)s\t%(levelname)s\t%(message)s')

    server = ParallelizationServer(logging.getLogger('server'))

    if config.files_path:
        def send_files(address, files):
            for path in files:
                try:
                    client.send_file(address, path)
                except:
                    pass


        files_thread = threading.Thread(target=send_files, args=(server.address, config.files_path))
        files_thread.daemon = True
        files_thread.start()

    if args.lemma:
        def run_lemma_server(lemma_server, database, send_again):
            # searching for a better IP because this IP will be sent to the solvers connecting to the server
            # that perhaps are on another host
            ip = '127.0.0.1'
            try:
                ip = socket.gethostbyname(socket.gethostname())
            except:
                pass

            args = [lemma_server, '-s', ip + ':' + str(config.port)]
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
            config.build_path + '/lemma_server',
            config.db_path if args.lemmaDB else None,
            args.lemmaAgain
        ))
        lemma_thread.daemon = True
        lemma_thread.start()

    if args.opensmt or args.z3spacer:
        def run_solvers(*solvers):
            for path, n in solvers:
                try:
                    for _ in range(n):
                        subprocess.Popen([path, '-s127.0.0.1:' + str(config.port)])
                except BaseException as ex:
                    print(ex)


        solvers_thread = threading.Thread(target=run_solvers, args=(
            (config.build_path + '/solver_opensmt', args.opensmt),
            (config.build_path + '/solver_z3spacer', args.z3spacer)
        ))
        solvers_thread.daemon = True
        solvers_thread.start()

    try:
        server.run_forever()
    except KeyboardInterrupt:
        sys.exit(0)
