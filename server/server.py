from version import version
import framework
import utils
import net
import config
import json
import logging
import os
import threading
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
        self._reset()

    def _reset(self):
        self.node = None
        self.started = None
        self.or_waiting = []
        self.parameters = {}

    def __repr__(self):
        return '<{} at{} {}>'.format(
            self.name,
            self.remote_address,
            'idle' if self.node is None else '{}{}'.format(self.node.root, self.node.path())
        )

    def solve(self, node: framework.AndNode, parameters: dict):
        if self.node is not None:
            self.stop()
        self.node = node
        self.parameters = parameters.copy()
        smt, query = self.node.root.to_string(self.node)
        parameters.update({
            'max_memory': config.max_memory,
            'command': 'solve',
            'name': self.node.root.name,
            'node': self.node.path(),
            'query': query,
        })
        self.write(parameters, smt)
        self.started = time.time()
        self._db_log('+', self.parameters)

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
        self.or_waiting = []
        self.started = time.time()
        self.node = node
        self._db_log('+', self.parameters)

    def stop(self):
        if self.node is None:
            raise ValueError('not solving anything')
        name = self.node.root.name
        path = self.node.path()
        self._db_log('-')
        self._reset()
        self.write({
            'command': 'stop',
            'name': name,
            'node': path
        }, '')

    def set_lemma_server(self, lemma_server: LemmaServer = None):
        self.write({
            'command': 'lemmas',
            'lemmas': lemma_server.listening if lemma_server else ''
        }, '')

    def make_pipe(self, name):
        pipename = str(utils.TempFile())
        try:
            os.mkfifo(pipename)
            return pipename
        except:
            return ''

    def ask_cnf_clauses(self, node):
        pipename = self.make_pipe(node.root.name + str(hash(node)))
        if pipename:
            if (node == self.node):
                self.write({
                    'name': node.root.name,
                    'node': node.path(),
                    'command': 'cnf-clauses',
                    'pipename': pipename
                }, '')
            else:
                smt, query = node.root.to_string(node)
                stop = 'false'
                if self.node is None:
                    self.solve(node, {})
                    stop = 'true'
                self.write({
                    'name': node.root.name,
                    'node': node.path(),
                    'command': 'cnf-clauses',
                    'pipename': pipename,  # used here later in read()
                    'query': query,
                    'stop': stop  # used here later in read()
                }, smt)
        return pipename

    def ask_cnf_learnts(self):
        pipename = self.make_pipe(self.node.root.name + str(hash(self.node)))
        if pipename:
            self.write({
                'name': self.node.root.name,
                'node': self.node.path(),
                'command': 'cnf-learnts',
                'pipename': pipename
            }, '')
        return pipename
            
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
        self._db_log('OR', {'node': str(node.path()), 'solver': str(self.remote_address)})

    def read(self):
        header, payload = super().read()
        if 'report' not in header:
            return {}, b''

        if self.node is None:
            return header, payload

        if 'name' not in header or 'node' not in header:
            return header, payload

        if self.node.root.name != header['name'] or str(self.node.path()) != header['node']:
            return {}, b''

        # Handle CNF request
        if header['report'].startswith('cnf'):

            # Thread to avoid blocking main process while waiting for pipe read
            # (the pipe can't be open until someone opens it for reading)
            def write_pipe(header, payload):
                pipename = header["pipename"]
                pipe = open(pipename, 'w')
                if pipe:
                    # Make CNF JSON compatible
                    cnf = utils.smt2json(payload.decode(), True)
                    # Write CNF in pipe
                    pipe.write(cnf)
                    pipe.flush()
                    pipe.close()
            # Start thread
            threading.Thread(target=write_pipe, args=(header, payload)).start()

            # Stop solver if it has only being started to retrieve CNF
            if "stop" in header and header['stop'] == 'true':
                self.stop()
                    
        del header['name']
        del header['node']

        if header['report'] == 'partitions' and self.or_waiting:
            node = self.or_waiting.pop()
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

        if header['report'] in framework.SolveStatus.__members__:
            status = framework.SolveStatus.__members__[header['report']]
            self._db_log('STATUS', header)
            self.node.status = status
            path = self.node.path(True)
            path.reverse()
            for node in path:
                if node.status != framework.SolveStatus.unknown:
                    if isinstance(node, framework.AndNode):
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
                                         json.dumps(self.remote_address),
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
        return self.started + self.timeout - time.time() if self.started and self.timeout is not None else float('inf')


class ParallelizationServer(net.Server):
    def __init__(self, logger: logging.Logger = None):
        super().__init__(port=config.port, timeout=0.1, logger=logger)
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
                    if level == logging.ERROR: # if a solver sends an error then the instance is skipped
                        self.current.timeout = 0
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
                self.log(logging.WARNING, '{} had waiting or-nodes {}'.format( # todo: manage what to do now
                    sock,
                    sock.or_waiting
                ))
            try:
                sock.stop()
            except:
                pass
        if isinstance(sock, LemmaServer):
            for solver in self.solvers(False):
                solver.set_lemma_server()

    def handle_timeout(self):
        self.entrust()

    def level_children(self, level):
        return self.config.partition_policy[level % len(self.config.partition_policy)]

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
                if self.config.idle_quit:
                    if not any([type(socket) == net.Socket and socket is not self._sock for socket in self._rlist]):
                        self.close()
            return

        assert isinstance(self.current, Instance)

        idle_solvers = self.solvers(None)
        nodes = self.current.root.all()
        nodes.sort()

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
                                      len(node) == self.level_children(len(node.path()))):
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
                    parameters = {}
                    if self.config.lemma_amount:
                        parameters["lemmas"] = self.config.lemma_amount
                    self.config.entrust(node, parameters, solver.name, self.solvers(node))
                    solver.solve(node, parameters)
                    if self.current.started is None:
                        self.current.started = time.time()
                    return

        # only standard instances can partition
        if not isinstance(self.current.root, framework.SMT):
            return

        # if need partition: ask partitions
        if self.config.partition_timeout or idle_solvers:
            # for all the leafs with at least one solver
            for leaf in (leaf for leaf in leaves() if self.solvers(leaf)):
                self.partition(leaf, bool(idle_solvers))
                            
    def partition(self, node: framework.AndNode, force=False):
        max_children = self.level_children(node.level)
        for i in range(max_children - len(node)):
            solvers = list(self.solvers(node))
            random.shuffle(solvers)
            for solver in solvers:
                # ask the solver to partition if timeout or if needed because idle solvers
                if force or solver.started + self.config.partition_timeout <= time.time():
                    solver.ask_partitions(self.level_children(node.level + 1))
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

    def get_cnf_clauses(self, instanceName, nodePath):
        # Select node
        instances = [instance for instance in self.trees if instance.root.name == instanceName]
        if not instances:
            return
        node = instances[0].root.child(nodePath)

        # Node already has a solver
        solvers = self.solvers(node)
        if solvers:
            return solvers.pop().ask_cnf_clauses(node)
        
        # Node doesn't have a solver, take random solver
        solvers = self.solvers(False)
        if solvers:
            return solvers.pop().ask_cnf_clauses(node)

        # No solver available
        return ''

    def get_cnf_learnts(self, instanceName, solverAddress): 
        # Get solver with matching address
        solvers = [solver for solver in self.solvers(True) if solver.node.root.name == instanceName]
        if solverAddress:
            for solver in solvers:
                if solver.remote_address == solverAddress:
                    return solver.ask_cnf_learnts()

        # No non-idle solver with matching address
        return ''
       
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
