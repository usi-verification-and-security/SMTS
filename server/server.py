#!/usr/bin/env python
# -*- coding: utf-8 -*-

import optparse
import threading
import json
import framework
import net
import sys
import client
import logging
import traceback
import random
import pathlib
import time
import sqlite3

__author__ = 'Matteo Marescotti'


class Config:
    port = 3000
    portfolio_max = 0
    portfolio_min = 0
    partition_timeout = None
    partition_policy = [2, 2]
    solving_timeout = None
    lemma_amount = 1000
    log_level = logging.INFO

    @staticmethod
    def entrust(header, solver, solvers):
        pass


class Tree(framework.AndNode):
    def __init__(self, smtlib: str, name: str, timeout: int):
        super().__init__(smtlib, None)
        self.name = name
        self.started = None
        self.timeout = timeout
        self.last_partition = None

    @property
    def is_timeout(self):
        return (self.started + self.timeout) < time.time() if self.started else False


class LemmaServer(net.Socket):
    def __init__(self, sock: net.Socket, listening: str):
        super().__init__(sock._sock)
        self.listening = listening


class Solver(net.Socket):
    def __init__(self,
                 sock: net.Socket,
                 name: str,
                 *,
                 conn: sqlite3.Connection = None,
                 table_prefix: str = ''):
        super().__init__(sock._sock)
        self.name = name
        self.conn = conn
        self.table_prefix = table_prefix
        if self.conn:
            self.conn.cursor().execute("CREATE TABLE IF NOT EXISTS {}SolvingHistory ("
                                       "id INTEGER NOT NULL PRIMARY KEY, "
                                       "ts INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
                                       "name TEXT NOT NULL, "
                                       "node TEXT, "
                                       "event TEXT NOT NULL, "
                                       "solver TEXT, "
                                       "data TEXT"
                                       ");".format(self.table_prefix))
            self.conn.commit()
        self.node = None
        self.or_waiting = []

    def solve(self, node: framework.AndNode, header: dict):
        root = node.root
        if not isinstance(root, Tree):
            raise TypeError('node root of type Tree is expected')
        if self.node is not None:
            self.stop()
        header['command'] = 'solve'
        header['name'] = root.name
        header['node'] = node.path()
        self.write(header, node.smtlib_complete() + "\n(check-sat)")
        if not root.started:
            root.started = time.time()
        self.node = node
        self.db_log('+')

    def stop(self):
        if self.node is None:
            raise ValueError('not solving anything')
        self.write({
            'command': 'stop',
            'name': self.node.root.name,
            'node': self.node.path()
        }, '')
        self.db_log('-')
        self.node = None
        self.or_waiting = []

    def set_lemma_server(self, lemma_server: LemmaServer):
        self.write({
            'command': 'lemmas',
            'lemmas': lemma_server.listening
        }, '')

    def ask_partitions(self, n, node=None):
        if self.node is None:
            raise ValueError('not solving anything')
        self.write({
            'command': 'partition',
            'name': self.node.root.name,
            'node': self.node.path(),
            'partitions': n
        }, '')
        if not node:
            self.node.add_child()
            node = self.node.children[-1]
        self.or_waiting.append(node)
        self.db_log('OR', node.path())

    def read(self):
        header, payload = super().read()
        if self.node is None:
            return header, payload
        root = self.node.root
        if root.name != header['name']:
            header['error'] = 'wrong name "{}", expected "{}"'.format(
                header['name'],
                root.name
            )
            return header, payload
        if str(self.node.path()) != header['node']:
            header['error'] = 'wrong node "{}", expected "{}"'.format(
                header['node'],
                str(self.node.path())
            )
            return header, payload

        if 'error' in header:
            return header, payload

        if 'status' in header:
            status = framework.SolveStatus.__members__[header['status']]
            self.db_log('STATUS', {'status': status.name})
            self.node.status = status
            if root.status != framework.SolveStatus.unknown:
                self.db_log('SOLVED', {'status': root.status.name})
        elif 'partitions' in header and self.or_waiting:
            node = self.or_waiting.pop()
            try:
                for partition in payload.decode().split('\n'):
                    node.add_child('(assert {})'.format(partition))
                    self.db_log('AND', {"node": node.children[-1].path(), "smtlib": partition})
            except BaseException as ex:
                header['error'] = 'error reading partitions: {}'.format(ex)
                node.children.clear()
                self.ask_partitions(header['partitions'], node)
                return header, payload

        return header, payload

    def db_log(self, event, data=None):
        if not self.conn:
            return
        self.conn.cursor().execute("INSERT INTO {}SolvingHistory (name, node, event, solver, data) "
                                   "VALUES (?,?,?,?,?)".format(self.table_prefix), (
                                       self.node.root.name,
                                       str(self.node.path()) if self.node else None,
                                       event,
                                       str(self.remote_address),
                                       json.dumps(data) if data else None
                                   ))
        self.conn.commit()


class ParallelizationServer(net.Server):
    def __init__(self,
                 config: Config,
                 *,
                 conn: sqlite3.Connection = None,
                 table_prefix: str = '',
                 logger: logging.Logger = None):
        super().__init__(port=config.port, timeout=1, logger=logger)
        self.config = config
        self.conn = conn
        self.table_prefix = table_prefix
        self.trees = set()
        self.current = None
        if self.conn:
            cursor = self.conn.cursor()
            cursor.execute("DROP TABLE IF EXISTS {}ServerLog;".format(table_prefix))
            cursor.execute("CREATE TABLE IF NOT EXISTS {}ServerLog ("
                           "id INTEGER NOT NULL PRIMARY KEY, "
                           "ts INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
                           "level TEXT NOT NULL,"
                           "message TEXT NOT NULL,"
                           "data TEXT"
                           ");".format(self.table_prefix))
            cursor.execute("DROP TABLE IF EXISTS {}SolvingHistory;".format(table_prefix))
            cursor.execute("VACUUM;")
            self.conn.commit()
        self.log(logging.INFO, 'server start')

    # TODO aggiungi che quando solvo qualcosa elimino le clausole.

    def handle_accept(self, sock):
        self.log(logging.DEBUG, 'new connection from {}'.format(sock.remote_address))

    def handle_message(self, sock, header, payload):
        self.log(logging.DEBUG, 'message from {}'.format(sock.remote_address),
                 {'header': header, 'payload': payload.decode()})
        if isinstance(sock, Solver):
            reported = False
            levels = ('info', 'warning', 'error')
            for report in levels + ('status',):
                if report in header:
                    level = report if report in levels else 'info'
                    self.log(getattr(logging, level.swapcase()), 'from solver "{}" at {}: {}'.format(
                        sock.name,
                        sock.remote_address,
                        header[report]
                    ), {'header': header, 'payload': payload.decode()})
                    reported = True
            if not reported:
                self.log(logging.INFO, 'message from solver {}'.format(
                    sock.remote_address
                ), {'header': header, 'payload': payload.decode()})
            self.entrust()
            return
        if 'command' in header:
            if header['command'] == 'solve':
                if 'name' not in header:
                    return
                self.log(logging.INFO, 'new instance "{}"'.format(
                    header['name']
                ), {'header': header})
                self.trees.add(
                    Tree(payload.decode().split('(check-sat)')[0],
                         header['name'],
                         self.config.solving_timeout)
                )
                self.entrust()
        elif 'solver' in header:
            solver = Solver(sock, header['solver'], conn=self.conn, table_prefix=self.table_prefix)
            self.log(logging.INFO, 'new solver "{}" at {}'.format(
                solver.name,
                solver.remote_address
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
            self.log(logging.INFO, 'new lemma server listening at {}'.format(
                header['lemmas']
            ), {'header': header, 'payload': payload.decode()})
            lemma_server = self.lemma_server
            if lemma_server:
                lemma_server.close()
            self._rlist.remove(sock)
            lemma_server = LemmaServer(sock, header["lemmas"])
            self._rlist.add(lemma_server)
            for solver in (solver for solver in self._rlist if isinstance(solver, Solver)):
                solver.set_lemma_server(lemma_server)
        elif 'eval' in header:
            response_payload = ''
            try:
                response_payload = str(eval(header['eval']))
            except:
                response_payload = str(traceback.format_exc())
            finally:
                sock.write({}, response_payload)

    def handle_close(self, sock):
        if isinstance(sock, Solver):
            self.log(logging.INFO, 'connection closed from solver "{}" at {}'.format(
                sock.name,
                sock.remote_address
            ))
        elif isinstance(sock, LemmaServer):
            self.log(logging.INFO, 'connection closed from lemma server listening at {}'.format(
                sock.listening
            ))

    def handle_timeout(self):
        self.entrust()

    def entrust(self):
        solving = self.current is not None
        # if the current tree is already solved or timed out: stop it
        if self.current and self.current.started and (
                        self.current.status != framework.SolveStatus.unknown or self.current.is_timeout
        ):
            self.log(
                logging.INFO,
                '{} instance "{}"'.format(
                    'solved' if self.current.status != framework.SolveStatus.unknown else 'timeout',
                    self.current.name
                )
            )
            for solver in {solver for solver in self._rlist
                           if isinstance(solver, Solver) and solver.node and solver.node.root == self.current}:
                solver.stop()
            self.current = None
        if not self.current:
            schedulables = [root for root in self.trees if
                            root.status == framework.SolveStatus.unknown and not root.is_timeout]
            if schedulables:
                self.current = schedulables[0]
                self.log(logging.INFO, 'solving instance "{}"'.format(self.current.name))
        if not self.current:
            if solving:
                self.log(logging.INFO, 'all done.')
            return

        assert isinstance(self.current, Tree)

        solvers = {solver for solver in self._rlist if isinstance(solver, Solver) and not solver.node}
        nodes = self.current.all()
        nodes.sort()

        def level_children(level):
            return self.config.partition_policy[level % len(self.config.partition_policy)]

        def leaves():
            return (node for node in nodes if len(node.children) == 0 and isinstance(node, framework.AndNode))

        def internals():
            return (node for node in nodes if len(node.children) > 0 and isinstance(node, framework.AndNode))

        # stop the solvers working on an already solved node of the whole tree, and add them to the list
        for node in nodes:
            if node.status != framework.SolveStatus.unknown:
                for solver in self.solvers(node):
                    solver.stop()
                    solvers.add(solver)

        # count the solvers required to fill all the leafs
        leaf_slots = 0
        for leaf in leaves():
            if self.config.portfolio_max <= 0:
                leaf_slots += 1
            else:
                leaf_slots += self.config.portfolio_max - len(self.solvers(leaf))

        # now stop leaf_slots solvers working on a non-leaf node, so it will be moved on a leaf afterwards
        # least depth nodes stopped first
        # if leaf_slots=-1 then all the solvers working on a internal node will be stopped
        for node in (node for node in internals() if len(node.children) == level_children(len(node.path()))):
            for solver in self.solvers(node):
                if leaf_slots == 0:
                    break
                if self.config.portfolio_min > 0 and len(self.solvers(node)) <= self.config.portfolio_min:
                    break
                leaf_slots -= 1
                solver.stop()
                solvers.add(solver)

        # spread the solvers among the unsolved nodes taking care of portfolio_max
        # first the leafs will be filled. inner nodes after and only if available solvers are left idle
        nodes.reverse()
        for selection in [leaves(), internals()]:
            while solvers:
                available = len(solvers)
                for node in selection:
                    if not solvers:
                        break
                    try:
                        # here i check that every node in the path to the root is already solved
                        # could happen that an upper level node is solved while one on its subtree is still unsolved
                        for _node in node.path(True):
                            if _node.status != framework.SolveStatus.unknown:
                                raise ValueError
                    except ValueError:
                        continue
                    if self.config.portfolio_max <= 0 or len(self.solvers(node)) < self.config.portfolio_max:
                        solver = solvers.pop()
                        header = {"lemmas": self.config.lemma_amount}
                        self.config.entrust(
                            header,
                            solver,
                            {solver for solver in self._rlist
                             if isinstance(solver, Solver) and solver.node and solver.node.root == node.root}
                        )
                        solver.solve(node, header)
                # break if no solver was assigned
                if len(solvers) == available:
                    break

        need_partition = (self.config.partition_timeout and self.current.started
                          and (
                              self.current.last_partition
                              if self.current.last_partition is not None else
                              self.current.started
                          ) + self.config.partition_timeout < time.time())

        # if there are still available solvers or need partition timeout: ask partitions
        if solvers or need_partition:
            self.current.last_partition = time.time()
            available = len(solvers)
            # for all the leafs with at least one solver
            for leaf in (leaf for leaf in leaves() if self.solvers(leaf)):
                children = level_children(leaf.level())
                if not need_partition:
                    if available <= 0:
                        break
                    available -= children
                for i in range(children):
                    solver = random.sample(self.solvers(leaf), 1)[0]
                    solver.ask_partitions(level_children(leaf.level() + 1))

    def solvers(self, node):
        return [solver for solver in self._rlist if isinstance(solver, Solver) and solver.node == node]

    @property
    def lemma_server(self):
        lemmas = [sock for sock in self._rlist if isinstance(sock, LemmaServer)]
        if lemmas:
            return lemmas[0]

    def log(self, level, message, data=None):
        super().log(level, message)
        if not self.conn or level < self.config.log_level:
            return
        self.conn.cursor().execute("INSERT INTO {}ServerLog (level, message, data) "
                                   "VALUES (?,?,?)".format(self.table_prefix), (
                                       logging.getLevelName(level),
                                       message,
                                       json.dumps(data) if data else None
                                   ))
        self.conn.commit()


if __name__ == '__main__':
    def config_config(option, opt_str, value, parser):
        path = pathlib.Path(value)
        sys.path.insert(0, str(path.parent.absolute()))

        try:
            module = __import__(path.stem)
        except ImportError as ex:
            logging.log(logging.ERROR, str(ex))
            sys.exit(1)

        config = getattr(parser.values, option.dest)

        for i in dir(module):
            if i[:1] == "_":
                continue
            setattr(config, i, getattr(module, i))


    def config_database(option, opt_str, value, parser):
        try:
            conn = sqlite3.connect(value)
        except BaseException as ex:
            logging.log(logging.ERROR, str(ex))
            sys.exit(1)
        setattr(parser.values, option.dest, conn)


    parser = optparse.OptionParser()
    parser.add_option('-c', '--config', dest='config', type='str',
                      action="callback", callback=config_config,
                      default=Config(), help='config file path')
    parser.add_option('-d', '--database', dest='db', type='str',
                      action="callback", callback=config_database,
                      default=None, help='sqlite3 database file path')

    options, args = parser.parse_args()

    logging.basicConfig(level=options.config.log_level, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    server = ParallelizationServer(config=options.config, conn=options.db, logger=logging.getLogger('server'))
    if hasattr(options.config, 'files'):
        def send_files(address, files):
            for path in files:
                try:
                    client.send_file(address, path)
                except:
                    pass


        thread = threading.Thread(target=send_files, args=(server.address, options.config.files))
        thread.start()

    try:
        server.run_forever()
    except KeyboardInterrupt:
        pass
