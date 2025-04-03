from version import version
import framework
import utils
import net
import config
import json
import logging
import traceback
import time
import constant
if config.visualize_tree:
    import re
    import graphviz
    from graphviz import nohtml

# LemmaServer ==========================================================================================================

class LemmaServer(net.Socket):
    def __init__(self, sock: net.Socket, listening: str):
        super().__init__(sock._sock)
        self.listening = listening
        self.init()

    def __repr__(self):
        global comment
        comment = "Lemma Server ON"
        return '<LemmaServer listening:{}>'.format(self.listening)

    def init(self):
        parameters = {}
        if config.enableLog:
            parameters.update({constant.LOG_MODE: 1})
        if config.max_memory:
            parameters.update({constant.MAX_MEMORY: config.max_memory})
        else:
            parameters.update({constant.MAX_MEMORY: 0})
        self.write(parameters, '')

    def reset(self, node):
        self.write({constant.NAME: node.root.name, constant.NODE: node.path(), constant.LEMMAS: '0'}, '')

    def read(self):
        return super().read()

# Solver ===============================================================================================================
class Solver(net.Socket):
    def __init__(self, sock: net.Socket, name: str):
        super().__init__(sock._sock)
        self.name = name
        self._reset()
        self.initialize()

    def initialize(self):
        if config.enableLog:
            self.write({constant.LOG_MODE: 1}, '')

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
            constant.COMMAND: constant.SOLVE,
            constant.NAME: self.node.root.name,
            constant.NODE: self.node.path(),
            constant.QUERY: query,

        })
        if config.max_memory:
            parameters.update({constant.MAX_MEMORY: config.max_memory})
        else:
            parameters.update({constant.MAX_MEMORY: 0})
        self.write(parameters, smt)
        self.started = time.time()
        if self.node.started is None:
            self.node.started = time.time()

    def incremental(self, node: framework.AndNode):
        smt, query = self.node.root.to_string(node, self.node)
        header = {constant.COMMAND: constant.INCREMENTAL,
                  constant.NAME: self.node.root.name,
                  constant.NODE: self.node.path(),
                  constant.NODE_: node.path(),
                  constant.QUERY: query,
                  }
        self.write(header, smt)
        self.or_waiting = []
        self.started = time.time()
        if node.started is None:
            node.started = time.time()
        self.node = node

    def stop(self):
        if self.node is None:
            raise ValueError('not solving anything')
        name = self.node.root.name
        path = self.node.path()
        self._reset()
        self.write({
            constant.COMMAND: constant.STOP,
            constant.NAME: name,
            constant.NODE: path
        }, '')

    def terminate(self):
        if self.node is None:
            raise ValueError('not solving anything')
        name = self.node.root.name
        path = self.node.path()
        self.write({
            constant.COMMAND: constant.TERMINATE,
            constant.NAME: name,
            constant.NODE: path
        }, '')

    def set_lemma_server(self, lemma_server: LemmaServer = None):
        self.write({
            constant.COMMAND: constant.LEMMAS,
            constant.LEMMA_AMOUNT: config.lemma_amount if config.lemma_amount else '1000',
            constant.LEMMAS: lemma_server.listening if lemma_server else '',
            constant.LEMMA_PUSH_MIN: config.lemmaPush_timeoutMin,
            constant.LEMMA_PULL_MIN: config.lemmaPull_timeoutMin,
            constant.LEMMA_PUSH_MAX: config.lemmaPush_timeoutMax,
            constant.LEMMA_PULL_MAX: config.lemmaPull_timeoutMax
        }, '')

    def ask_partitions(self, n, node: framework.AndNode = None):
        global estimate_partition_time
        config.partition_count += n
        if self.node is None:
            raise ValueError('not solving anything')
        self.write({
            constant.COMMAND: constant.PARTITION,
            constant.NAME: self.node.root.name,
            constant.NODE: self.node.path(),
            constant.QUERY: constant.CHECK_SAT,
            constant.PARTITIONS: n
        }, '')
        if isinstance(self.node, framework.SMT):
            estimate_partition_time = time.time()
        if not node:
            node = framework.OrNode(self.node)

        self.node.partitioning = True
        self.or_waiting.append(node)

    def read(self):
        header, payload = super().read()
        if constant.REPORT not in header:
            return {}, b''

        if self.node is None:
            return header, payload

        if constant.NAME not in header or constant.NODE not in header:
            return header, payload

        if config.debug:
            if str(self.node.path()) != header[constant.NODE] and header[constant.REPORT] == constant.PARTITIONS:
                for n in self.node.root.all():
                    if str(n.path()) == header[constant.NODE]:
                        if n.status == framework.SolveStatus.unknown:
                            print(utils.bcolors.FAIL + ";illegal move from ", self.node.path(), " SOLVER POS: ", header + utils.bcolors.ENDC)
                            exit(1)
                        if n.status == framework.SolverStatus.sat and header[constant.REPORT] == framework.SolverStatus.unsat or \
                            n.status == framework.SolverStatus.unsat and header[constant.REPORT] == framework.SolverStatus.sat:
                            print("Inconsistent result ... ")
#                        else:
#                            return {}, b''
        if self.node.root.name != header[constant.NAME]:
           return {}, b''

        del header[constant.NAME]

        if header[constant.REPORT] == constant.PARTITIONS and self.or_waiting:
            global estimate_partition_time
            node = self.or_waiting.pop()
            try:
                for partition in payload.decode().split('\0'):
                    if len(partition) == 0:
                        continue
                    child = framework.AndNode(node, '(assert {})'.format(partition))
                header['partition_recieved'] = node.parent
            except BaseException as ex:
                header[constant.REPORT] = 'error:(server) error reading partitions: {}'.format(traceback.format_exc())
                node.clear()
            else:
                header[constant.REPORT] = 'info:(server) received {} partitions'.format(len(node))
                if node.parent.status != framework.SolveStatus.unknown:
                    node.clear()
                if len(node) != config.partition_policy[1]:
                    config.partition_count = config.partition_count - config.partition_policy[1] + len(node)
            return header, payload

        if header[constant.REPORT] in framework.SolveStatus.__members__:
            status = framework.SolveStatus.__members__[header[constant.REPORT]]
            if self.or_waiting:
                self.or_waiting.pop()

            path = self.node.path(True)
            self.node.status = status
            path.reverse()
            if status == framework.SolveStatus.unsat:
                for child in self.node.all():
                    if child.status == framework.SolveStatus.unknown:
                        if isinstance(child, framework.AndNode):
                            child.status = framework.SolveStatus.unsat


        return header, payload

# Instance =============================================================================================================
class Instance(object):
    def __init__(self, name: str, smt: str):
        self.root = framework.parse(name, smt)
        self.started = None
        self.timeout = None
        self.sp = None

    def __repr__(self):
        return '{}({:.2f})'.format(repr(self.root), self.when_timeout)

    @property
    def when_timeout(self):
        return self.started + self.timeout - time.time() if self.started and self.timeout is not None else float('inf')

# Parallelization_Server ===============================================================================================
class ParallelizationServer(net.Server):
    def __init__(self, logger: logging.Logger = None, port=None):
        super().__init__(port=port, timeout=0.1, logger=logger)
        self.config = config
        self.trees = dict()
        self.current = None
        self.idle_solvers = list()
        self.total_solvers = 0
        self.terminate = False
        self.counter = 75674531
        if self.config.enableLog:
            self.log(logging.INFO, 'server start. version {}'.format(version))
        if self.config.visualize_tree:
            global comment
            comment = 'Lemma Server OFF'
            self.v_tree = graphviz.Digraph('f',
                                           node_attr={'shape': 'record','width': '1%', 'height': '1%', 'fillcolor':'white'})
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
                     {constant.HEADER: header, constant.PAYLOAD: payload.decode()})
        if isinstance(sock, Solver):
            if not header:
                return
            if constant.REPORT in header:
                try:
                    level, message = header[constant.REPORT].split(':', 1)
                    level = logging._nameToLevel[level.upper()]
                    if level == logging.ERROR:
                        self.current.timeout = 0
                        print(";error ", message, self.current.root.name)
                        exit(1)

                except:
                    level = logging.INFO
                    message = header[constant.REPORT]

                if self.config.visualize_tree:
                    self.collectData_vTree(sock, message, header)
                if self.config.enableLog:
                    if message in framework.SolveStatus.__members__:
                        self.log(level, utils.bcolors.OKGREEN + '{}: {}'.format(sock, message + utils.bcolors.ENDC), {constant.HEADER: header, constant.PAYLOAD: payload.decode()})
                    else:
                        self.log(level, '{}: {}'.format(sock, message), {constant.HEADER: header, constant.PAYLOAD: payload.decode()})
            self.entrust(header)
            return
        if isinstance(sock, LemmaServer):
            if not header:
                return
            if constant.REPORT in header:
                level, message = header[constant.REPORT].split(':', 1)
                level = logging._nameToLevel[level.upper()]
                if level == logging.ERROR:
                    print(";error ", message)
                    exit(1)
        if constant.COMMAND in header:
            # if terminate command is found, close the server
            if header[constant.COMMAND] == constant.TERMINATE:
                if self.config.visualize_tree:
                    self.render_vTree(time.time() - self.current.started)
                self.log(logging.INFO, 'Termination command is received!')
                self.close()
            elif header[constant.COMMAND] == constant.SOLVE:
                if constant.NAME not in header:
                    return
                if self.config.enableLog:
                    self.log(logging.INFO, 'new instance "{}"'.format(
                        header[constant.NAME]
                    ), {constant.HEADER: header})
                try:
                    instance = Instance(header[constant.NAME], payload.decode())
                    instance.timeout = config.solving_timeout
                    self.trees[header[constant.NAME]] = instance
                    if isinstance(instance.root, framework.Fixedpoint) and config.fixedpoint_partition:
                        instance.root.partition()
                except:
                    self.log(logging.ERROR, 'cannot add instance: {}'.format(traceback.format_exc()))

                self.entrust()
        elif constant.SOLVER in header:
            solver = Solver(sock, header[constant.SOLVER])
            if self.config.enableLog:
                self.log(logging.INFO, 'new {}'.format(
                    solver,
                ), {constant.HEADER: header, constant.PAYLOAD: payload.decode()})
            self._rlist.remove(sock)
            self._rlist.add(solver)
            lemma_server = self.lemma_server
            if lemma_server:
                solver.set_lemma_server(lemma_server)
            self.entrust()
        elif constant.LEMMAS in header:
            if header[constant.LEMMAS][0] == ':':
                header[constant.LEMMAS] = sock.remote_address[0] + header[constant.LEMMAS]
            lemma_server = self.lemma_server
            if lemma_server:
                lemma_server.close()
            self._rlist.remove(sock)
            lemma_server = LemmaServer(sock, header[constant.LEMMAS])
            if self.config.enableLog:
                self.log(logging.INFO, 'new {}'.format(
                    lemma_server
                ), {constant.HEADER: header, constant.PAYLOAD: payload.decode()})
            self._rlist.add(lemma_server)
            for solver in (solver for solver in self._rlist if isinstance(solver, Solver)):
                solver.set_lemma_server(lemma_server)
        elif constant.EVAL in header:
            response_payload = ''
            try:
                if header[constant.EVAL]:
                    response_payload = str(eval(header[constant.EVAL]))
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
                    self.log(logging.WARNING, '{} had waiting or-nodes {}'.format(sock, sock.or_waiting))
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

    def entrust(self, header=None):
        p_node = None
        partition_recieved = False
        if header:
            if 'partition_recieved' in header:
                p_node = header['partition_recieved']
                partition_recieved = True
                del header['partition_recieved']
        solving = self.current

        if isinstance(self.current, Instance):
            # if not self.terminate:
            #     if sum(map(lambda solver: isinstance(solver, Solver), self._rlist)) != self.total_solvers and self.total_solvers != 0:
            #         print(';error, solvers are lost ', self.current.root.name)
            #         for solver in {solver for solver in self.solvers(False)}:
            #             solver.terminate()
            #         self.terminate = True
            #         return
            # elif sum(map(lambda solver: isinstance(solver, Solver), self._rlist)) == 0:
            #     self.close()
            #     exit(1)
            if self.current.root.status != framework.SolveStatus.unknown or self.current.when_timeout < 0:
                if self.config.visualize_tree:
                    self.render_vTree(time.time() - self.current.started)
                    self.node_dict.clear()
                    self.node_alias.clear()
                    self.node_alias['[]'] = ('[]')
                    self.rank = 0
                    import os
                    if os.path.exists(self.v_tree.filename):
                        os.remove(self.v_tree.filename)
                    else:
                        print("The file does not exist")
                    self.v_tree.clear()
                if self.current.root.status == framework.SolveStatus.unknown:
                    if self.current.root.partitioning:
                        if not self.current.root.childeren():
                            if not self.terminate:
                                print(';error,stuck',self.current.root.name)
                                for solver in {solver for solver in self.solvers(False)}:
                                    solver.terminate()
                                self.terminate = True
                                return

                    if config.spit_preference:
                        for sp in framework.SplitPreference.__members__.values():
                            del self.trees[self.current.root.name + sp.value]
                if not self.config.enableLog and self.current:
                    self.log(logging.INFO, '{}'.format(self.current.root.status.name),
                             self.current.root.name, round(time.time() - self.current.started, 2), header)
                    del self.trees[self.current.root.name]
                else:
                    # self.log(logging.INFO, self.current.root.status.name , self.current.root.name, round(time.time() - self.current.started, 2))
                    if self.current:
                        self.log(logging.INFO, '{} instance "{}" after {:.2f} seconds'.format(
                            'solved' if self.current.root.status != framework.SolveStatus.unknown else 'timeout', self.current.root.name,
                            time.time() - self.current.started))

                for solver in {solver for solver in self.solvers(False) if solver.node.root == self.current.root}:
                    solver.stop()
                if self.lemma_server:
                    self.lemma_server.reset(self.current.root)
                self.current = None
                # self.counter = 0
                self.idle_solvers.clear()
        if self.current is None:
            schedulables = [instance for instance in self.trees.values() if
                            instance.root.status == framework.SolveStatus.unknown and instance.when_timeout > 0]
            if schedulables:
                self.current = schedulables[0]

                if self.config.enableLog:
                    self.log(logging.INFO, 'solving instance "{}"'.format(self.current.root.name))
                self.total_solvers = len(self.solvers(False))
                for solver in self.solvers(None):
                    assert isinstance(solver, Solver)
                    parameters = {}
                    # if self.config.lemma_amount:
                    #     parameters[constant.LEMMAS] = self.config.lemma_amount
                    self.config.entrust(self.current.root, parameters, solver.name, self.solvers(self.current.root))
                    # self.seed_counter += 1
                    # if config.lemma_sharing:
                    #     parameters.update({
                    #         'lemma_push_min': self.seed_counter,
                    #         'lemma_pull_min': self.seed_counter*2,
                    #         constant.LEMMA_AMOUNT: config.lemma_amount
                    #     })
                    self.counter += 1
                    parameters['parameter.seed'] = self.counter
                    solver.solve(self.current.root, parameters)
                    if self.current.started is None:
                        self.current.started = time.time()
                    config.partition_count = 0
                return
        else:
            for solver in self.newly_joint_solver():
                self.total_solvers += 1
                parameters = {}
                self.config.entrust(self.current.root, parameters, solver.name, self.solvers(self.current.root))
                self.counter += 1
                parameters['parameter.seed'] = self.counter
                solver.solve(self.current.root, parameters)
                if self.current.started is None:
                    self.current.started = time.time()
        # if solving is not None and solving != self.current and self.lemma_server:
        #     self.lemma_server.reset(solving.root)
        if self.current is None:
            if solving is not None:
                if self.config.enableLog:
                    self.log(logging.INFO, 'all done.')
                if self.config.idle_quit:
                    if not any([type(socket) == net.Socket and socket is not self._sock for socket in self._rlist]):
                        self.close()
                        exit(0)
            return

        assert isinstance(self.current, Instance)
        if config.partition_timeout:

            nodes = self.current.root.all()
            nodes.sort(reverse=False)

            def all_active_nodes(sub_tree):
                return (node for node in sub_tree if (node.status == framework.SolveStatus.unknown and
                                                      isinstance(node, framework.AndNode) and not node.processed))

            def un_attempted_active_leaves():
                return (node for node in nodes if (len(node) == 0 and node.status == framework.SolveStatus.unknown and
                                                   node.started is None and isinstance(node, framework.AndNode) and not node.processed))

            movable_solvers = []
            solved_solvers = set()
            to_partition_node = None
            solver_partition = False

            for node in nodes:
                if isinstance(node, framework.AndNode) and not node.processed:
                    # if p_node and str(node.path()) == p_node and node.status == framework.SolveStatus.unknown:
                    #     p_node = node
                    # elif solved_moved_node and str(node.path()) == solved_moved_node:
                    #     node.status = framework.SolveStatus.__members__[header[constant.REPORT]]
                    #     print('       solved solver in forked -> ', solver)
                    #     return
                    if node.status == framework.SolveStatus.unsat:
                        if node.parent:
                            if node.parent.parent.status == framework.SolveStatus.unknown:
                                for solver in self.solvers(node.parent.parent):
                                    if solver in self.idle_solvers:
                                        self.idle_solvers.remove(solver)
                                    else:
                                        config.partition_count -= 1
                                    solved_solvers.add(solver)
                                node.parent.parent.processed = True
                        node.processed = True
                        for solver in self.solvers(node):
                            if solver in self.idle_solvers:
                                self.idle_solvers.remove(solver)
                            else:
                                config.partition_count -= 1
                            solved_solvers.add(solver)
                    elif config.node_timeout and (node.started and node.started + config.node_timeout <= time.time()):
                        if node.partitioning and (node._children[0])._children:
                            global estimate_partition_time
                            if node.assumed_timout:
                                for solver in self.solvers(node):
                                    if solver not in self.idle_solvers:
                                        self.idle_solvers.append(solver)
                            else:
                                for solver in self.solvers(node):
                                    if solver not in self.idle_solvers:
                                        self.idle_solvers.append(solver)
                                        config.partition_count -= 1
                            node.processed = True
                            node.is_timeout = True

                        elif not node.assumed_timout:
                            for solver in self.solvers(node):
                                if solver not in self.idle_solvers:
                                    self.idle_solvers.append(solver)
                                    config.partition_count -= 1
                            node.assumed_timout = True
                    # elif not config.node_timeout and len(node) == 0 and len(self.solvers(node)) == 1:
                    #     if round(time.time() - self.current.root.started) < 60:
                    #         config.node_timeout = 60
                    #     else:
                    #         config.node_timeout = round(time.time() - self.current.root.started + 5)
                    if not node.partitioning and not node.processed and len(node) == 0:
                        if to_partition_node is None:
                            to_partition_node = node
                            if not self.solvers(node) and node.assumed_timout:
                                solver_partition = True
            if solved_solvers:
                childs = self.current.root.childeren()
                while 0 != len(solved_solvers):
                    for child in childs:
                        try:
                            all_active_node = list(all_active_nodes(child.all()))
                            for ch in sorted(all_active_node, key=lambda leave: len(self.solvers(leave)), reverse=False):
                                if len(solved_solvers) != 0:
                                    s_solver = solved_solvers.pop()
                                    s_solver.incremental(ch)
                                    raise StopIteration

                        except StopIteration:
                            continue
            if partition_recieved:
                if config.portfolio_min:
                    for _node in p_node.path_to_node():
                        solvers = self.solvers(_node)

                        counter = len(solvers)
                        for solver in solvers:
                            if 0 <= self.config.portfolio_min < counter:
                                if solver not in self.idle_solvers:
                                    counter -= 1
                                    movable_solvers.append(solver)
                else:
                    movable_solvers = list(self.solvers(True))

                while 0 != len(movable_solvers):
                    for node in p_node.childeren():
                        if len(movable_solvers) == 0:
                            break
                        try:
                            for solver in movable_solvers:
                                movable_solvers.remove(solver)
                                solver.incremental(node)
                                raise StopIteration
                        except StopIteration:
                            continue

            if self.idle_solvers:
                for node in un_attempted_active_leaves():
                    if len(self.idle_solvers) == 0:
                        break
                    try:
                        for solver in self.idle_solvers:
                            if solver.node is not None and solver.node.is_ancestor(node):
                                self.idle_solvers.remove(solver)
                                solver.incremental(node)

                                raise StopIteration
                            elif len(solver.node) == 0 and solver.node.assumed_timout and not node.is_timeout and \
                                    not self.solvers(node) and solver.node is not node:
                                self.idle_solvers.remove(solver)
                                solver.incremental(node)

                                raise StopIteration

                    except StopIteration:
                        continue
                assert len(self.idle_solvers) <= self.total_solvers
                if solver_partition and to_partition_node is not None:
                    if self.idle_solvers:
                        self.idle_solvers.sort(key=lambda s:
                            str(to_partition_node.path())[1:len(str(s.node.path()))-1] == str(s.node.path())[1:len(str(s.node.path()))-1], reverse=True)
                        solver = self.idle_solvers[0]
                        if not solver.or_waiting:
                            solver.incremental(to_partition_node)
                            self.idle_solvers.remove(solver)

            if not isinstance(self.current.root, framework.SMT):
                return

            if to_partition_node is not None:
                if self.total_solvers > config.partition_count:
                    self.partition(to_partition_node)

    def partition(self, node: framework.AndNode, force=False):
        max_children = self.level_children(node.level)
        for i in range(max_children - len(node)):
            # solvers = list(self.solvers(node))
            # random.shuffle(solvers)
            for solver in self.solvers(node):
                if force or solver.started + self.config.partition_timeout <= time.time():
                    solver.ask_partitions(self.level_children(node.level + 1))
                    return True
        return False

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

    def newly_joint_solver(self):
        return {solver for solver in self._rlist
                if isinstance(solver, Solver) and (solver.node is None and solver.started is None)}

    @property
    def lemma_server(self) -> LemmaServer:
        lemmas = [sock for sock in self._rlist if isinstance(sock, LemmaServer)]
        if lemmas:
            return lemmas[0]

    def log(self, level, message, data=None, time=None, header=None):
        if config.enableLog:
            super().log(level, message)
        else:
            print(data, message, time)
    # Visual tree ======================================================================================================
    def render_vTree(self, solved_time):
        global comment
        self.v_tree.filename = self.current.root.name  + '.gv';
        # self.node_dict = sorted(self.node_dict.items(), key=self.node_dict.get(2))
        self.v_tree.attr(bgcolor='white',rankdir='UP', label='\n Solvers: ' + str(len(self.solvers(False)) ) + '    Portfolio: '
                                                             + str(self.config.portfolio_min) + '    Node Timeout: ' +
                                                             str(self.config.node_timeout) + '      Partition Policy ' +
                                                             str(self.config.partition_policy) + '      Elapsed Time: ' +str(round(solved_time))+
                                                             '   Partition Timout: '+str(self.config.partition_timeout )+ '\n'+comment
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
            self.v_tree.node(constant.NODE + key, nohtml('<f0> '+ str(self.node_dict[key][0]) + ' - ' + str(self.node_dict[key][7])+'|<f1> '
                                                         + rank_str + self.node_alias[key] + '|<f2>' +
                                                         str(self.node_dict[key][1]) + ' - ' + str(self.node_dict[key][6])),
                             color = error_color)

            self.v_tree.edge(constant.NODE+key+':f2', constant.NODE+key+':f2', color='white', label = total_solver)
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
                            self.v_tree.node(constant.NODE + part_node, nohtml('<f0> |<f1> ' + self.node_alias[part_node] + '|<f2>'), color='blue')
                        else:
                            self.v_tree.node(constant.NODE + part_node, nohtml('<f0> |<f1> ' + self.node_alias[part_node] + '|<f2>'))

                        self.v_tree.edge(constant.NODE+key+':f1', constant.NODE+part_node+':f1', color = color)
                    index += 1
        if lastSolvedNode != '':
            self.v_tree.node(constant.NODE + lastSolvedNode, nohtml('<f0> '+ str(self.node_dict[lastSolvedNode][0]) + ' - '
                                                                    + str(self.node_dict[lastSolvedNode][7])+'|<f1> '
                                                                    + "("+str(rank)+") " + self.node_alias[lastSolvedNode] + '|<f2>' +
                                                                    str(self.node_dict[lastSolvedNode][1]) + ' - ' + str(self.node_dict[lastSolvedNode][6])),
                             color ='#DFA500')
        self.v_tree.view()

    def collectData_vTree(self, sock, message, header):
        message_str = re.search(r"\[(.*)\]", message)
        if message_str:
            currentNode = message_str[0]
        else:
            currentNode = '[]'
        destNode = header
        if destNode:
            if constant.NODE_ in header:
                destNodePath = destNode[constant.NODE_]
            else:
                destNodePath = destNode[constant.NODE]
            if not destNodePath in self.node_dict:
                self.node_dict[destNodePath] = (0, 0, 0, 'white','white', [], 0, 0, list(), 0)
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
            if message == 'unsat' or message == 'sat':
                self.rank += 1
                box_color ='green'
                action = message
            elif message == 'start':
                edge_color = 'red'
                start += 1
                action = message
            else:
                if destNode:
                    action =constant.INCREMENTAL
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
                                            self.node_dict[destNodePath][9] + total
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
                                           self.node_dict[currentNode][9]- total
                                           )