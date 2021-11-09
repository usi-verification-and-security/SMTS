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
import re
import graphviz
from graphviz import nohtml
__author__ = 'Matteo Marescotti'

class LemmaServer(net.Socket):
    def __init__(self, sock: net.Socket, listening: str):
        super().__init__(sock._sock)
        self.listening = listening

    def __repr__(self):
        global comment
        comment = "Lemma Server ON"
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
        global totalN_partitions
        totalN_partitions = 0
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
            'lemma_push_min': config.lemmaPush_timeoutMin,
            'lemma_pull_min': config.lemmaPull_timeoutMin,
            'lemma_push_max': config.lemmaPush_timeoutMax,
            'lemma_pull_max': config.lemmaPull_timeoutMax,
            'lemma_amount': config.lemma_amount,
            'colorMode': '1' if config.clientLogColorMode else '0'
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
        # print('Partition Numbers: ',n,'\n')
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
        global totalN_partitions
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
                totalN_partitions += len(node)
                # if len(node) == 1:
                #     node.clear()
            return header, payload

        if header['report'] in framework.SolveStatus.__members__:
            status = framework.SolveStatus.__members__[header['report']]
            self._db_log('STATUS', header)
            self.node.status = status
            path = self.node.path(True)
            path.reverse()
            print("path . . ..........", path)
            if status == framework.SolveStatus.unsat:
                totalN_partitions -= 1
                for child in self.node.all():
                    print(child.path(),len(child.path()))
                    if child.status == framework.SolveStatus.unknown:
                        child.status = framework.SolveStatus.unsat
                        if len(child.path()) % 2 == 0:
                            totalN_partitions -= 1

            # print("path . . ..........", path)
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
        if self.config.enableLog:
            self.log(logging.INFO, 'server start. version {}'.format(version))
        if self.config.visualize_tree:
            global comment
            comment = 'Lemma Server OFF'
            self.v_tree = graphviz.Digraph('f',
                node_attr={'shape': 'record','width': '.8%', 'height': '1.2%', 'fillcolor':'white'})
            # self.v_tree.graph_attr.setdefault('height', '1300px');
            self.node_dict = dict()
            self.node_alias = dict()
            self.node_alias['[]'] = ('[]')
            self.rank = 0

    def handle_accept(self, sock):
        if self.config.enableLog:
            self.log(logging.DEBUG, 'new connection from {}'.format(sock.remote_address))

    def handle_message(self, sock, header, payload):
        if self.config.enableLog:
            self.log(logging.DEBUG, 'message from {}'.format(sock.remote_address),
                     {'header': header, 'payload': payload.decode()})
        if isinstance(sock, Solver):
            if not header:
                return
            if 'report' in header:
                try:
                    level, message = header['report'].split(':', 1)
                    level = logging._nameToLevel[level.upper()]
                    #if level == logging.ERROR: # if a solver sends an error then the instance is skipped
                    #   self.current.timeout = 0
                except:
                    level = logging.INFO
                    message = header['report']
                if self.config.visualize_tree:
                    self.collectData_vTree(sock, message)
                if self.config.enableLog:
                    self.log(level, '{}: {}'.format(sock, message), {'header': header, 'payload': payload.decode()})
            self.entrust()
            return
        #         Second to execute
        if 'command' in header:
            # if terminate command is found, close the server
            if header['command'] == 'terminate':
                if self.config.visualize_tree:
                    self.render_vTree(0)
                self.log(logging.INFO, 'Termination command is received!')
                self.close()
            elif header['command'] == 'solve':
                print(header)
                if 'name' not in header:
                    return
                if self.config.enableLog:
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
                    # print("self.trees ..... ",self.trees)
                    # print("\n")
                self.entrust()
        #         First to execute
        elif 'solver' in header:
            solver = Solver(sock, header['solver'])
            if self.config.enableLog:
                self.log(logging.INFO, 'new {}'.format(
                    solver,
                ), {'header': header, 'payload': payload.decode()})
            self._rlist.remove(sock)
            self._rlist.add(solver)
            # print("self._rlist..... ", self._rlist)
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
            if self.config.enableLog:
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
        if self.config.enableLog:
            self.log(logging.DEBUG, 'connection closed by {}'.format(
                sock
            ))
        if isinstance(sock, Solver):
            if sock.or_waiting:
                if self.config.enableLog:
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
        global totalN_partitions
        # print(self.current)
        solving = self.current
        # if the current tree is already solved or timed out: stop it
        if isinstance(self.current, Instance):
            if self.current.root.status != framework.SolveStatus.unknown or self.current.when_timeout < 0:
                if self.config.visualize_tree:
                    self.render_vTree(time.time() - self.current.started)
                    # self.node_dict.clear()
                    # self.node_alias.clear()
                    # self.node_alias['[]'] = ('[]')
                    # self.rank = 0
                    # self.v_tree.clear()
                    # for key in self.node_dict:
                    #     print('\n node..',key, '   value... ',self.node_dict[key])
                    import os
                    if os.path.exists(self.v_tree.filename):
                        os.remove(self.v_tree.filename)
                    else:
                        print("The file does not exist")
                if not self.config.enableLog:
                    self.log(logging.INFO, '{}'.format(self.current.root.status.name))
                else:
                    self.log(
                        logging.INFO,
                        '{} instance "{}" after {:.2f} seconds'.format(
                            'solved' if self.current.root.status != framework.SolveStatus.unknown else 'timeout',
                            self.current.root.name,
                            time.time() - self.current.started))
                for solver in {solver for solver in self.solvers(True) if solver.node.root == self.current.root}:
                    solver.stop()
                self.current = None
        if self.current is None:
            schedulables = [instance for instance in self.trees if
                            instance.root.status == framework.SolveStatus.unknown and instance.when_timeout > 0]
            if schedulables:
                self.current = schedulables[0]
                if self.config.enableLog:
                    self.log(logging.INFO, 'solving instance "{}"'.format(self.current.root.name))
            # print(schedulables)
        if solving is not None and solving != self.current and self.lemma_server:
            self.lemma_server.reset(solving.root)
        if self.current is None:
            if solving is not None:
                if self.config.enableLog:
                    self.log(logging.INFO, 'all done.')
                if self.config.idle_quit:
                    if not any([type(socket) == net.Socket and socket is not self._sock for socket in self._rlist]):
                        self.close()
            return

        assert isinstance(self.current, Instance)

        idle_solvers = self.solvers(None)
        # for s in idle_solvers:
        #     print("\n idle at the first place -> ",s)
        nodes = self.current.root.all()

        nodes.sort()

        def leaves():
            return (node for node in nodes if len(node) == 0 and isinstance(node, framework.AndNode))

        def internals():
            return (node for node in nodes if len(node) > 0 and isinstance(node, framework.AndNode))
        #
        # def isAns(solvedNode):
        #     for n in nodes:
        #         if n.status != framework.SolveStatus.unknown:
        #             continue
        #         if solvedNode.is_ancestor(n):
        #             n.status = framework.SolveStatus.unsat
        def isBaseNode(destNode):
            # target, result = current.split(':', 1)
            # if result[0:len(result) - 1] == "unsat":
            #     return False
            return not re.search(r"\[(([0-9])+ *,* [0-9]*)*\]", destNode)
        # stop the solvers working on an already solved node of the whole tree, and add them to the list
        # print("\n")
        # print(nodes)
        # for node in nodes:
        #     if node.status != framework.SolveStatus.unknown:
        #         isAns(node)
                # if self.nNode != len(node):
                #     print("\n solved node -> ", node)
                # self.nNode +=1
        for node in nodes:
            if node.status != framework.SolveStatus.unknown:
                for solver in self.solvers(node):
                    # print(" solver found result in node -> ", solver)
                    # self.nPartitions -= 1
                    idle_solvers.add(solver)
        # spread the solvers among the unsolved nodes taking care of portfolio_max
        # first the leafs will be filled. inner nodes after and only if available solvers are left idle
        # nodes.reverse()
        for selection in [leaves, internals]:
            available = -1
            while available != len(idle_solvers):
                # print("\n","Idles .... ", len(idle_solvers))
                # print("Working .... ", self.solvers(True),"\n")
                # print("ALL .... ", self.solvers(False),"\n")
                available = len(idle_solvers)
                # for node in selection():
                #     print(" solver in selection node ........ ", self.solvers(node))
                #     print(" selection node ..... ", node)
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
                            # print("len of solvers...",len(solvers))
                            # if totalN_partitions + 1 > 0:
                            self.config.portfolio_min = (len(self.solvers(False))) / round((totalN_partitions + 1))

                            if 0 <= self.config.portfolio_min < len(solvers) or self.config.portfolio_min <= 1:
                                # print("     self.config.portfolio_min->    ",self.config.portfolio_min, len(solvers))
                                try:
                                    # I can choose only one solver if it's solving for more than partition_timeout
                                    # print("node -> ",_node)
                                    for solver in self.solvers(_node):
                                        if self.config.portfolio_min > 1:
                                            if self.config.partition_timeout and solver.started + self.config.partition_timeout > time.time():
                                                # print(" partition_timeout reachedt -> ",solver)
                                                continue
                                            idle_solvers.add(solver)
                                        else:
                                            if self.config.solver_timeout and solver.started + self.config.solver_timeout > time.time():
                                                continue
                                            idle_solvers.add(solver)

                                        # print(" \n  add to idles -> ",solver)

                                        # for s in idle_solvers:
                                        #     print("  idle after checking for timout -> ",s)
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
                                # print("idle_solvers",idle_solvers)
                                for solver in idle_solvers:
                                    if solver.node is None or solver.node is node:
                                        continue
                                    if solver.node.is_ancestor(node):
                                        # self.idle_solvers_map[solver] = True
                                        idle_solvers.remove(solver)
                                        solver.incremental(node)
                                #     else:
                                #         self.idle_solvers_map[solver] = False
                                # for key in self.idle_solvers_map:
                                #     print("                 key - > ",key, self.idle_solvers_map[key])
                                #     if self.idle_solvers_map[key]:
                                #         idle_solvers.remove(key)
                                        raise StopIteration
                            except StopIteration:
                                continue
                        if self.config.incremental > 1:
                            try:
                                # try to use incremental on another already solving solver
                                for solver in idle_solvers:

                                    # if solver.node is not None and isBaseNode(format(node)):
                                    #     # print("             inc > 1 ........ ", node.level)
                                    #     print("\n            masoud > 1 ........ ", solver)
                                    #     print("             masoud > 1 ........ ", solver.node)
                                    #     print("             masoud > 1 ........ ", node)
                                    #
                                    #     continue
                                    if solver.node is not None and solver.node is not node:
                                    # if self.config.portfolio_min >= len(self.solvers(solver.node)):
                                        #     idle_solvers.remove(solver)
                                        #     raise StopIteration
                                        # self.idle_solvers_map[solver] = True
                                        idle_solvers.remove(solver)
                                        solver.incremental(node)
                                #     else:
                                #         self.idle_solvers_map[solver] = False
                                # for key in self.idle_solvers_map:
                                #     print("                 key > 1 - > ",key, self.idle_solvers_map[key])
                                #     if self.idle_solvers_map[key]:
                                #         idle_solvers.remove(key)

                                        raise StopIteration
                            except StopIteration:
                                continue
                    # self.idle_solvers_map.clear()
                    solver = idle_solvers.pop()
                    assert isinstance(solver, Solver)
                    if solver.node is node:
                        continue
                    parameters = {}
                    if self.config.lemma_amount:
                        parameters["lemmas"] = self.config.lemma_amount
                    self.config.entrust(node, parameters, solver.name, self.solvers(node))
                    # print("Write solve to the solvers")
                    solver.solve(node, parameters)
                    if self.current.started is None:
                        self.current.started = time.time()
                    return

        # only standard instances can partition
        if not isinstance(self.current.root, framework.SMT):
            return

        # if need partition: ask partitions
        # print("self.nPartitions" , totalN_partitions)
        # if (len(self.solvers(False)) - self.config.portfolio_min) >= totalN_partitions:
        if (self.config.partition_timeout or idle_solvers):
            # for all the leafs with at least one solver
            for leaf in (leaf for leaf in leaves() if self.solvers(leaf)):
                self.partition(leaf, bool(idle_solvers))

    def partition(self, node: framework.AndNode, force=False):
        global totalN_partitions
        max_children = self.level_children(node.level)
        for i in range(max_children - len(node)):
            solvers = list(self.solvers(node))
            random.shuffle(solvers)
            for solver in solvers:
                # ask the solver to partition if timeout or if needed because idle solvers
                if force or solver.started + self.config.partition_timeout <= time.time():
                    # totalN_partitions += self.level_children(node.level + 1)
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
        if config.enableLog:
            super().log(level, message)
        else:
            print(message)
        if not config.db() or level < self.config.log_level:
            return
        config.db().cursor().execute("INSERT INTO {}ServerLog (level, message, data) "
                                     "VALUES (?,?,?)".format(config.table_prefix), (
                                         logging.getLevelName(level),
                                         message,
                                         json.dumps(data) if data else None
                                     ))
        config.db().commit()

    def render_vTree(self, solved_time):
        global comment
        self.v_tree.filename = self.current.root.name  + '.gv';
        # self.node_dict = sorted(self.node_dict.items(), key=self.node_dict.get(2))
        self.v_tree.attr(bgcolor='white',rankdir='UP', label='\n Solvers: ' + str(len(self.solvers(False)) ) + '    Portfolio: '
                                                + str(self.config.portfolio_min) + '     Solver_timeout: ' +
                                                str(self.config.solver_timeout) + '      Partition_policy ' +
                                                str(self.config.partition_policy) + '      solved_time: ' +str(round(solved_time))+
            '   partition_timout: '+str(self.config.partition_timeout )+ '\n'+comment
                         , fontcolor = 'black')
        self.v_tree.comment = "Test"
        lastSolvedNode = ''
        rank = 0
        for key in self.node_dict:
            rank_str = ''
            total_solver = ''
            # print('node..',key, '   value... ',self.node_dict[key])
            # for destNode in [ self.node_dict[k1] for k2 in self.node_dict[k1] ]:
            #     push =+ destNode[0]
            #     pop =+ destNode[1]
            error_color = self.node_dict[key][3]
            if self.node_dict[key][3] == 'green':
                if self.node_dict[key][9] > 0:
                    error_color = "red"
                if self.node_dict[key][2] > rank:
                    rank = self.node_dict[key][2]
                    lastSolvedNode = key
                rank_str = '(' + str(self.node_dict[key][2]) + ') '
            # else:
            #     self.node_dict[key][3] ='white'
            total_solver = '(' + str(self.node_dict[key][9]) + ')'

            if self.node_dict[key][9] < 0:
                error_color = "red"
            self.v_tree.node('node' + key, nohtml('<f0> '+ str(self.node_dict[key][0]) + ' - ' + str(self.node_dict[key][7])+'|<f1> '
                                                  + rank_str + self.node_alias[key] + '|<f2>' +
                                                  str(self.node_dict[key][1]) + ' - ' + str(self.node_dict[key][6])),
                             color = error_color)

            self.v_tree.edge('node'+key+':f2', 'node'+key+':f2', color='white',label=total_solver)
            if self.node_dict[key][5]:
                index = 0
                for partition in self.node_dict[key][5]:
                    for x in range(partition):
                        color = 'black'
                        if index % 2 == 0:
                            # color = 'paleturquoise'
                            color = 'blue'
                        part_node_alias = '['+str(index)+', '+str(x)+']'
                        if key == '[]':
                            part_node = '['+str(index)+', '+str(x)+']'
                        else:
                            part_node = key[0:len(key)-1]+', '+str(index)+', '+str(x)+']'
                        self.node_alias[part_node] = part_node_alias
                        if part_node not in self.node_dict:
                            self.v_tree.node('node' + part_node, nohtml('<f0> |<f1> ' + self.node_alias[part_node] + '|<f2>'), color = 'blue')
                        else:
                            self.v_tree.node('node' + part_node, nohtml('<f0> |<f1> ' + self.node_alias[part_node] + '|<f2>'))

                        self.v_tree.edge('node'+key+':f1', 'node'+part_node+':f1', color = color)
                    index += 1
        if lastSolvedNode != '':
            self.v_tree.node('node' + lastSolvedNode, nohtml('<f0> '+ str(self.node_dict[lastSolvedNode][0]) + ' - '
                                                  + str(self.node_dict[lastSolvedNode][7])+'|<f1> '
                                                  + "("+str(rank)+") " + self.node_alias[lastSolvedNode] + '|<f2>' +
                                                  str(self.node_dict[lastSolvedNode][1]) + ' - ' + str(self.node_dict[lastSolvedNode][6])),
                                                  color ='#61971A')
        self.v_tree.view()

    def collectData_vTree(self, sock, message):
        solver_IP = re.search(r"\['[0-9]+.[0-9]+.[0-9].[0-9]', *\d+\]", format(sock))
        message_str = re.search(r"\[(.*)\]", message)
        if message_str:
            currentNode = message_str[0]
        else:
            currentNode = '[]'
        destNode = re.search(r"\[(([0-9])+ *,* [0-9]*)*\]", format(sock))
        if destNode:
            destNodePath = destNode[0]
            if not destNodePath in self.node_dict:
                self.node_dict[destNodePath] = (0, 0, 0, 'white','white', [], 0, 0, list(), 0)
            # if not currentNode in self.node_dict.get(destNodePath, {}):
            #     (push,  pop_SameBranch,  ,action, box_color,  edge_color,  partition, pop_DifferentBranch, start)
        if not currentNode in self.node_dict:
            self.node_dict[currentNode] = (0, 0, 0, 'white','white', [], 0, 0, list(), 0)
        start = 0
        push = 0
        pop_SameBranch = 0
        pop_DifferentBranch = 0
        action = ''
        box_color = ''
        edge_color = ''
        partition = 0

        p_str = re.search(r"received [0-9]+ partitions", message)
        if not p_str:
            if message == 'unsat':
                self.rank += 1
                box_color ='green'
                action = message
            elif message == 'sat':
                box_color ='paleturquoise'
                action = message
            elif message == 'start':
                edge_color = 'red'
                start += 1
                action = message
            else:
                if destNode:
                    action ='incremental'
                    prefix = currentNode[1:len(currentNode)-1]
                    full = destNodePath[1:len(destNodePath)-1]
                    if prefix != full and prefix == full[0:len(prefix)]:
                        push += 1
                    else:
                        if prefix != full and full == prefix[0:len(full)]:
                            pop_SameBranch += 1
                        elif prefix != full:
                            pop_DifferentBranch += 1
        else:
            action ='parition'
            pn = re.search(r"[0-9]+", p_str[0])
            if pn:
                partition = int(pn[0])
                edge_color = 'blue'
        # print('push . . . .', push)
        # print('start . . . .', start)
        # print('pop_same . . . .', pop_SameBranch)
        # print('pop_diff . . . .', pop_DifferentBranch)
        solver_action = dict()
        # for sa in self.node_dict[destNodePath][8]:
        #     if not solver_IP in sa:
        # solver_action[solver_IP[0]] = (action, round(time.time()))
        if destNode:
            total = start+push+pop_SameBranch+pop_DifferentBranch
            self.node_dict[destNodePath] = (self.node_dict[destNodePath][0] + start,
                                            self.node_dict[destNodePath][1] + pop_DifferentBranch,
                                            self.rank,
                                            box_color,
                                            edge_color,
                                            self.node_dict[destNodePath][5],
                                            self.node_dict[destNodePath][6] + pop_SameBranch,
                                            self.node_dict[destNodePath][7] + push,
                                            self.node_dict[destNodePath][8],
                                            self.node_dict[destNodePath][9] +total
                                            )
            # self.node_dict[destNodePath][8].append(solver_action)
            if partition != 0:
                self.node_dict[destNodePath][5].append(partition)
            if start != 0:
                total = 0
            self.node_dict[currentNode] = (self.node_dict[currentNode][0],
                                            self.node_dict[currentNode][1],
                                            self.node_dict[currentNode][2],
                                            self.node_dict[currentNode][3],
                                            self.node_dict[currentNode][4],
                                            self.node_dict[currentNode][5],
                                            self.node_dict[currentNode][6],
                                            self.node_dict[currentNode][7],
                                            self.node_dict[currentNode][8],
                                            self.node_dict[currentNode][9]-total
                                            )
            # for c in self.node_dict:
            #     if c==currentNode or c==destNodePath:
            #         print(c,self.node_dict[c])
            # print('par list', self.node_dict[destNodePath][5])