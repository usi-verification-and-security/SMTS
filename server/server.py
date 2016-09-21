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


# class SocketParallelizationTree(framework.ParallelizationTree):
#     def __init__(self, name, smtlib, config, conn=None, table_prefix=''):
#         self.name = name
#         self.root = framework.AndNode(smtlib.split('(check-sat)')[0])
#         self.config = config
#
#         if self._conn:
#             self._conn.cursor().execute("CREATE TABLE IF NOT EXISTS {}SolvingHistory ("
#                                         "id INTEGER NOT NULL PRIMARY KEY, "
#                                         "ts INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
#                                         "name TEXT NOT NULL, "
#                                         "event TEXT NOT NULL, "
#                                         "solver TEXT, "
#                                         "node TEXT, "
#                                         "data TEXT"
#                                         ");".format(self._table_prefix))
#             self._conn.commit()
#             self.db_log('CREATE')
#
#         self.solvers = set()
#         self.timeout = False
#
#     def solver_node(self, solver):
#         if solver not in self.solvers:
#             return
#         for path in self.reverse[sock]:
#             if len(path) > 1 and isinstance(path[-2], framework.AndNode) and sock in path[-2]['solvers']:
#                 return path[-2]
#
#     # if socket is new then it is added
#     # if node is none then auto assign
#     # if node is False remove solver
#     def assign_solvers(self, solvers=None, node=None):
#         if self.timeout:
#             return self.assign_solvers(self._solvers, False)
#         if solvers is None:
#             solvers = {solver for solver in self._solvers}
#         else:
#             if not isinstance(solvers, (list, set, tuple)):
#                 solvers = {solvers}
#             solvers = set(solvers)
#             self._solvers.update(solvers)
#         for solver in solvers:
#             current_node = self.solver_node(solver)
#             if current_node and node and node is not current_node:
#                 try:
#                     solver.stop()
#                 except BaseException as ex:
#                     self.db_log('ERROR',
#                                 solver,
#                                 current_node,
#                                 'exception during solver stop request: {}'.format(ex))
#                 current_node['solvers'].remove(solver)
#         if node is False:
#             # !!! check if the solver was asked to create partitions
#             self._solvers.difference_update(solvers)
#             return
#         if self.root['status'] != framework.SolveState.unknown:
#             return
#         if isinstance(node, framework.AndNode):
#             if node.observer is not self:
#                 raise ValueError
#             if 'started' not in node:
#                 node['started'] = time.time()
#             for solver in solvers:
#                 try:
#                     solver.solve(self, node)
#                 except BaseException as ex:
#                     self.db_log('ERROR',
#                                 solver,
#                                 node,
#                                 'exception during solver solve request:{}'.format(ex))
#                 else:
#                     node['solvers'].update(solvers)
#         elif node is None:
#             l = -1
#             while solvers:
#                 l += 1
#                 if l % 2:
#                     continue
#                 level = self.level(l)
#                 children = self._level_children(l)
#                 if not level:
#                     break
#                 for node in level:
#                     if node['active'] and 'started' in node:
#                         if time.time() - node['started'] > self._config.partition_timeout:
#                             node['active'] = False
#                     if not node['active'] and node['status'] == framework.SolveState.unknown and \
#                                     len(node['children']) < children and (
#                                     'partitions_asked' not in node or node['partitions_asked'] < children):
#                         if 'partitions_asked' not in node:
#                             node['partitions_asked'] = 0
#                         for i in range(children - node['partitions_asked']):
#                             if not node['solvers']:
#                                 if solvers:
#                                     self.assign_solvers(solvers.pop(), node)
#                                 break
#                             partition_solver = random.sample(node['solvers'], 1)[0]
#                             try:
#                                 partition_solver.ask_partitions(self._level_children(l + 1))
#                             except BaseException as ex:
#                                 self.db_log('ERROR',
#                                             partition_solver,
#                                             node,
#                                             'exception during solver ask for partitions: {}'.format(ex))
#                             else:
#                                 node['partitions_asked'] += 1
#                         for solver in node['solvers']:
#                             if not solver.or_nodes:
#                                 self.assign_solvers(solver)
#                     if not node['active'] and len(node['children']) == children and node['solvers']:
#                         _solvers = node['solvers']
#                         node['solvers'].clear()
#                         self.assign_solvers(_solvers)
#                     if node['active']:
#                         while solvers and \
#                                 (self._config.portfolio_max <= 0 or len(node['solvers']) < self._config.portfolio_max):
#                             self.assign_solvers(solvers.pop(), node)
#
#                             # if solvers:
#                             #     l = -1
#                             #     reserved = len(solvers)
#                             #     while True:
#                             #         l += 1
#                             #         if l % 2:
#                             #             continue
#                             #         level = self.level(l)
#                             #         if not level:
#                             #             break
#                             #         children = self._level_children(l)
#                             #
#                             #         for node in level:
#                             #             if reserved <= 0:
#                             #                 break
#                             #             if len(node['children']) < children:
#                             #                 for i in range(min(len(node['solvers']), children - len(node['children']))):
#                             #                     partition_solver = random.sample(node['solvers'], 1)[0]
#                             #                     try:
#                             #                         partition_solver.ask_partitions(self._level_children(l + 1))
#                             #                     except BaseException as ex:
#                             #                         self.db_log('ERROR',
#                             #                                     partition_solver,
#                             #                                     node,
#                             #                                     'exception during solver ask for partitions: {}'.format(ex))
#                             #                     else:
#                             #                         reserved -= 1
#                             #                     finally:
#                             #                         for solver in node['solvers'].difference(set(partition_solver)):
#                             #                             solver.stop()
#                             #                         node['solvers'].clear()
#                             #         if reserved <= 0:
#                             #             break
#         else:
#             raise ValueError
#
#     def db_log(self, event, solver=None, node=None, data=None):
#         if not self._conn:
#             return
#         self._conn.cursor().execute("INSERT INTO {}SolvingHistory (name, event, solver, node, data) "
#                                     "VALUES (?,?,?,?,?)".format(self._table_prefix), (
#                                         self.name,
#                                         event,
#                                         str(solver.remote_address) if solver else None,
#                                         str(self.node_path(node, keys=True)) if node else None,
#                                         json.dumps(data) if data else None
#                                     ))
#         self._conn.commit()
#
#     def _level_children(self, level):
#         if level < len(self._config.partition_policy):
#             return self._config.partition_policy[level]
#         elif len(self._config.partition_policy) > 1:
#             return self._config.partition_policy[-2]
#         elif len(self._config.partition_policy) > 0:
#             return self._config.partition_policy[-1]
#         else:
#             raise ValueError('invalid partition policy')


class Solver(net.Socket):
    def __init__(self, sock, solver, *, conn=None, table_prefix=''):
        super().__init__(sock._sock)
        self.solver = solver
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
        self.name = None
        self.node = None
        self.or_waiting = []

    def solve(self, name, node):
        assert (isinstance(node, framework.AndNode))
        self.write({
            'command': 'solve',
            'name': name,
            'node': node.path()
        }, node.smtlib_complete() + "\n(check-sat)\n")
        self.name = name
        self.node = node
        self.db_log('+')

    def stop(self):
        if self.node is None:
            raise ValueError('not solving anything')
        self.write({
            'command': 'stop',
            'name': self.name,
            'node': self.node.path()
        }, '')
        self.db_log('-')
        self.name = self.node = None
        self.or_waiting = []

    def ask_partitions(self, n, node=None):
        if self.node is None:
            raise ValueError('not solving anything')
        self.write({
            'command': 'partition',
            'name': self.name,
            'node': self.node.path(),
            'partitions': n
        }, '')
        if not node:
            self.node.add_child()
            node = self.node.children[-1]
        self.or_waiting.append(node)
        self.db_log('OR', node.path())

    def set_lemma_server(self, address):
        self.write({
            'command': 'lemmas',
            'lemmas': address
        }, '')

    def read(self):
        header, payload = super().read()
        if self.name is None or self.node is None:
            return header, payload
        if self.name != header['name']:
            header['error'] = 'wrong name "{}", expected "{}"'.format(
                header['name'],
                self.name
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
            if self.node.child([]).status != framework.SolveStatus.unknown:
                self.db_log('SOLVED', {'status': self.node.child([]).status.name})
        elif 'partitions' in header and self.or_waiting:
            node = self.or_waiting.pop()
            try:
                for partition in payload.decode().split('\n'):
                    node.add_child('(assert {})'.format(partition))
                    self.db_log('AND', node.children[-1].path())
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
                                   "VALUES (?,?,?,?,?)".format(self._table_prefix), (
                                       self.name,
                                       str(self.node.path()) if self.node else None,
                                       event,
                                       self.remote_address,
                                       json.dumps(data) if data else None
                                   ))
        self.conn.commit()


class ParallelizationServer(net.Server):
    def __init__(self, config, *, conn=None, table_prefix='', logger=None):
        super().__init__(port=config.port, timeout=1, logger=logger)
        self.config = config
        self.conn = conn
        self.table_prefix = table_prefix
        self.trees = {}
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
                           ");".format(self._table_prefix))
            cursor.execute("DROP TABLE IF EXISTS {}SolvingHistory;".format(table_prefix))
            cursor.execute("VACUUM;")
            self.conn.commit()
        self.log(logging.INFO, 'server start')
        self.lemmas = None

    # TODO aggiungi che quando solvo qualcosa elimino le clausole.

    def handle_accept(self, sock):
        self.log(logging.INFO, 'new connection from {}'.format(sock.remote_address))

    def handle_message(self, sock, header, payload):
        if isinstance(sock, Solver):
            if 'error' in header:
                self.log(logging.ERROR, 'error message from solver {}. {}'.format(
                    sock.remote_address,
                    header['error']
                ), {'header': header, 'payload': payload.decode()})
            else:
                self.log(logging.INFO, 'message from solver {}'.format(
                    sock.remote_address
                ), {'header': header, 'payload': payload.decode()})
            self.entrust()
            return
        self.log(logging.INFO, 'message from {}'.format(sock.remote_address))
        if 'command' in header:
            if header['command'] == 'solve':
                if 'name' not in header:
                    return
                self.log(logging.INFO, 'new instance "{}"'.format(
                    header['name']
                ), {'header': header, 'payload': payload.decode()})
                smtlib = payload.decode().split('(check-sat)')[0]
                self.trees[framework.AndNode(smtlib)] = {
                    'name': header['name'],
                    'started': None,
                    'timeout': self.config.solving_timeout,
                    'last_partition': None
                }
                self.entrust()
        elif 'solver' in header:
            solver = Solver(sock, header['solver'])
            self.log(logging.INFO, 'new solver "{}" at {}'.format(
                solver.solver,
                solver.remote_address
            ), {'header': header, 'payload': payload.decode()})
            self._rlist.remove(sock)
            self._rlist.add(solver)
            if self.lemmas:
                solver.set_lemma_server(self.lemmas[1])
            self.entrust()
        elif 'lemmas' in header:
            if header['lemmas'][0] == ':':
                header['lemmas'] = sock.remote_address[0] + header['lemmas']
            self.log(logging.INFO, 'new lemma server listening at {}'.format(
                header['lemmas']
            ), {'header': header, 'payload': payload.decode()})
            if self.lemmas:
                self.lemmas[0].close()
            self.lemmas = (sock, header["lemmas"])
            for solver in (solver for solver in self._rlist if isinstance(solver, Solver)):
                solver.set_lemma_server(self.lemmas[1])
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
                sock.solver,
                sock.remote_address
            ))

    def handle_timeout(self):
        self.entrust()

    def entrust(self):
        # if the current tree is already solved or timed out: stop it
        if self.current and self.trees[self.current]['started'] and (
                        self.current.status != framework.SolveStatus.unknown
                or self.trees[self.current]['started'] + self.trees[self.current]['timeout'] < time.time()
        ):
            for solver in {solver for solver in self._rlist
                           if isinstance(solver, Solver) and solver.name == self.trees[self.current]['name']}:
                solver.stop()
            self.current = None
        if not self.current:
            schedulables = [node for node in self.trees if
                            node.status == framework.SolveStatus.unknown and (
                                not self.trees[node]['started'] or
                                self.trees[node]['started'] + self.trees[node]['timeout'] > time.time()
                            )]
            if schedulables:
                self.current = schedulables[0]
        if not self.current:
            self.log(logging.INFO, 'all done!')
            return

        solvers = {solver for solver in self._rlist if isinstance(solver, Solver) and not solver.name}
        nodes = self.current.all()
        nodes.sort()
        leaves = lambda: (node for node in nodes if len(node.children) == 0 and isinstance(node, framework.AndNode))
        internals = lambda: (node for node in nodes if len(node.children) > 0 and isinstance(node, framework.AndNode))

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
        for node in (node for node in internals() if len(node.children) == self._level_children(len(node.path()))):
            for solver in self.solvers(node):
                if leaf_slots == 0:
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
                    if node.status != framework.SolveStatus.unknown:
                        continue
                    if not solvers:
                        break
                    if self.config.portfolio_max <= 0 or len(self.solvers(node)) < self.config.portfolio_max:
                        solvers.pop().solve(self.trees[self.current]['name'], node)
                        if not self.trees[self.current]['started']:
                            self.trees[self.current]['started'] = time.time()
                # break if no solver was assigned
                if len(solvers) == available:
                    break

        need_partition = (self.config.partition_timeout and self.trees[self.current]['started']
                          and (
                              self.trees[self.current]['last_partition']
                              if self.trees[self.current]['last_partition'] else
                              self.trees[self.current]['started']
                          ) + self.config.partition_timeout < time.time())

        # if there are still available solvers or need partition timeout: ask partitions
        if solvers or need_partition:
            available = len(solvers)
            # for all the leafs with at least one solver
            for leaf in (leaf for leaf in leaves() if self.solvers(leaf)):
                children = self._level_children(leaf.level())
                if not need_partition:
                    if available <= 0:
                        break
                    available -= children
                for i in range(children):
                    solver = random.sample(self.solvers(leaf), 1)[0]
                    solver.ask_partitions(self._level_children(leaf.level() + 1))

    def _level_children(self, level):
        if level < len(self.config.partition_policy):
            return self.config.partition_policy[level]
        elif len(self.config.partition_policy) > 1:
            return self.config.partition_policy[-2]
        elif len(self.config.partition_policy) > 0:
            return self.config.partition_policy[-1]
        else:
            raise ValueError('invalid partition policy')

    def solvers(self, node):
        return [solver for solver in self._rlist if isinstance(solver, Solver) and solver.node == node]

    def log(self, level, message, data=None):
        super().log(level, message)
        if not self.conn:
            return
        self.conn.cursor().execute("INSERT INTO {}ServerLog (level, message, data) "
                                   "VALUES (?,?,?)".format(self.table_prefix), (
                                       logging.getLevelName(level),
                                       message,
                                       json.dumps(data) if data else None
                                   ))
        self.conn.commit()


if __name__ == '__main__':
    class Config:
        port = 3000
        portfolio_max = 0
        partition_timeout = None
        partition_policy = [2, 2]
        solving_timeout = None


    def config_config(option, opt_str, value, parser):
        path = pathlib.Path(value)
        sys.path.insert(0, str(path.parent.absolute()))
        try:
            setattr(parser.values, option.dest, __import__(path.stem))
        except ImportError as ex:
            logging.log(logging.ERROR, str(ex))
            sys.exit(1)


    def config_database(option, opt_str, value, parser):
        try:
            conn = sqlite3.connect(value)
        except BaseException as ex:
            logging.log(logging.ERROR, str(ex))
            sys.exit(1)
        setattr(parser.values, option.dest, conn)


    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    parser = optparse.OptionParser()
    parser.add_option('-c', '--config', dest='config', type='str',
                      action="callback", callback=config_config,
                      default=Config(), help='config file path')
    parser.add_option('-d', '--database', dest='db', type='str',
                      action="callback", callback=config_database,
                      default=None, help='sqlite3 database file path')

    options, args = parser.parse_args()

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
