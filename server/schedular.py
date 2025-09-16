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
import random
import functools
import os
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
        self._partitioning = False
        self.redundant = False
        self.node = None
        self.start_time = None
        self.or_waiting = []
        self.parameters = {}

    def __repr__(self):
        return '<{} at{} {}>'.format(
            self.name,
            self.remote_address,
            'idle' if self.node is None else '{}{}'.format(self.node.root, self.node.path())
        )

    @property
    def partitioning(self):
        return self._partitioning

    @partitioning.setter
    def partitioning(self, flag):
        self._partitioning = flag
        if not flag:
            self.n_partitions = None

    def started(self):
        return self.start_time is not None

    def runtime(self):
        if not self.started():
            return 0
        return time.time() - self.start_time

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
        self.start_time = time.time()
        if not self.node.started():
            self.node.start_time = time.time()

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
        self.start_time = time.time()
        if not node.started():
            node.start_time = time.time()
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

        self.partitioning = True
        self.n_partitions = n
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
                for n in self.node.root.all_children():
                    if str(n.path()) == header[constant.NODE]:
                        if n.status == framework.SolveStatus.unknown:
                            print(utils.bcolors.FAIL + ";illegal move from ", self.node.path(), " SOLVER POS: ", header + utils.bcolors.ENDC)
                            exit(1)
                        if n.status == framework.SolverStatus.sat and header[constant.REPORT] == framework.SolverStatus.unsat or \
                            n.status == framework.SolverStatus.unsat and header[constant.REPORT] == framework.SolverStatus.sat:
                            print("Inconsistent result ... ")
#                        else:
#                            return {}, b''
        ## this fixes logging but introduces unsoudness!!!
        ## if self.node.root.name != header[constant.NAME]:
        if self.node.root.name != header[constant.NAME] or str(self.node.path()) != header[constant.NODE]:
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
                for child in self.node.all_and_children():
                    if child.status == framework.SolveStatus.unknown:
                        child.status = framework.SolveStatus.unsat


        return header, payload

# Instance =============================================================================================================
class Instance(object):
    def __init__(self, name: str, smt: str):
        self.root = framework.parse(name, smt)
        self.start_time = None
        self.timeout = None
        self.sp = None

    def __repr__(self):
        return '{}({:.2f})'.format(repr(self.root), self.when_timeout)

    def started(self):
        return self.start_time is not None

    def runtime(self):
        if not self.started():
            return 0
        return time.time() - self.start_time

    @property
    def when_timeout(self):
        return self.timeout - self.runtime() if self.started() and self.timeout is not None else float('inf')

# Parallelization_Server ===============================================================================================
class ParallelizationServer(net.Server):
    def __init__(self, logger: logging.Logger = None, port=None):
        super().__init__(port=port, timeout=0.1, logger=logger)
        self.trees = dict()
        self.current = None
        self.movable_solvers = list()
        self.total_solvers = 0
        self.terminate = False
        self.counter = 75674531
        if config.enableLog:
            self.log(logging.INFO, 'server start. version {}'.format(version))
        if config.visualize_tree:
            global comment
            comment = 'Lemma Server OFF'
            self.v_tree = graphviz.Digraph('f',
                                           node_attr={'shape': 'record','width': '1%', 'height': '1%', 'fillcolor':'white'})
            self.node_dict = dict()
            self.node_alias = dict()
            self.node_alias['[]'] = ('[]')
            self.rank = 0

    def handle_accept(self, sock):
        if config.enableLog:
            self.log(logging.DEBUG, 'new connection from {}'.format(sock.remote_address))

    def handle_message(self, sock, header, payload):
        if config.enableLog:
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

                if config.visualize_tree:
                    self.collectData_vTree(sock, message, header)
                if config.enableLog:
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
                if config.visualize_tree:
                    self.render_vTree(self.current.runtime())
                self.log(logging.INFO, 'Termination command is received!')
                self.close()
            elif header[constant.COMMAND] == constant.SOLVE:
                if constant.NAME not in header:
                    return
                if config.enableLog:
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
            if config.enableLog:
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
            if config.enableLog:
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
        if config.enableLog:
            self.log(logging.DEBUG, 'connection closed by {}'.format(
                sock
            ))
        if isinstance(sock, Solver):
            if sock.or_waiting:
                if config.enableLog:
                    self.log(logging.WARNING, '{} had waiting or-nodes {}'.format(sock, sock.or_waiting))
            try:
                sock.stop()
            except:
                pass
        if isinstance(sock, LemmaServer):
            for solver in self.all_solvers():
                solver.set_lemma_server()

    def handle_timeout(self):
        self.entrust()

    def level_children(self, level):
        return config.partition_policy[level % len(config.partition_policy)]

    def entrust(self, header=None):
        partition_received = False
        if header:
            if 'partition_recieved' in header:
                p_node = header['partition_recieved']
                partition_received = True
                del header['partition_recieved']
        solving = self.current

        if isinstance(self.current, Instance):
            # if not self.terminate:
            #     if sum(map(lambda solver: isinstance(solver, Solver), self._rlist)) != self.total_solvers and self.total_solvers != 0:
            #         print(';error, solvers are lost ', self.current.root.name)
            #         for solver in {solver for solver in self.all_solvers()}:
            #             solver.terminate()
            #         self.terminate = True
            #         return
            # elif sum(map(lambda solver: isinstance(solver, Solver), self._rlist)) == 0:
            #     self.close()
            #     exit(1)
            if self.current.root.status != framework.SolveStatus.unknown or self.current.when_timeout < 0:
                if config.visualize_tree:
                    self.render_vTree(self.current.runtime())
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
                    if any(solver.partitioning for solver in self.solvers_at(self.current.root)):
                        if not self.current.root.remaining_and_children():
                            if not self.terminate:
                                print(';error,stuck',self.current.root.name)
                                for solver in {solver for solver in self.all_solvers()}:
                                    solver.terminate()
                                self.terminate = True
                                return

                    if config.spit_preference:
                        for sp in framework.SplitPreference.__members__.values():
                            del self.trees[self.current.root.name + sp.value]
                ## This prints out the overall result
                filename = self.current.root.name
                runtime = round(self.current.runtime(), 2)
                if not config.enableLog and self.current:
                    self.log(logging.INFO, '{}'.format(self.current.root.status.name),
                             filename if config.printFilename else None,
                             runtime if config.printRuntime else None,
                             header)
                    del self.trees[self.current.root.name]
                else:
                    # self.log(logging.INFO, self.current.root.status.name , self.current.root.name, round(self.current.runtime(), 2))
                    if self.current:
                        self.log(logging.INFO, '{} instance "{}" after {:.2f} seconds'.format(
                            'solved' if self.current.root.status != framework.SolveStatus.unknown else 'timeout', filename,
                            runtime))

                for solver in {solver for solver in self.all_solvers() if solver.node.root == self.current.root}:
                    solver.stop()
                if self.lemma_server:
                    self.lemma_server.reset(self.current.root)
                self.current = None
                # self.counter = 0
                self.movable_solvers.clear()
        if self.current is None:
            schedulables = [instance for instance in self.trees.values() if
                            instance.root.status == framework.SolveStatus.unknown and instance.when_timeout > 0]
            if schedulables:
                self.current = schedulables[0]

                if config.enableLog:
                    self.log(logging.INFO, 'solving instance "{}"'.format(self.current.root.name))
                self.total_solvers = len(self.all_solvers())
                for solver in self.new_solvers():
                    assert isinstance(solver, Solver)
                    self.run_solver_at_root(solver)
                ##+ initialize again to the same value as in default config after moving to a new instance
                ## > should be removed from config
                # config.partition_count = 0
                return
        else:
            for solver in self.newly_joined_solvers():
                self.total_solvers += 1
                self.run_solver_at_root(solver)
        # if solving is not None and solving != self.current and self.lemma_server:
        #     self.lemma_server.reset(solving.root)
        if self.current is None:
            if solving is not None:
                if config.enableLog:
                    self.log(logging.INFO, 'all done.')
                if config.idle_quit:
                    if not any([type(socket) == net.Socket and socket is not self._sock for socket in self._rlist]):
                        self.close()
                        exit(0)
            return

        assert isinstance(self.current, Instance)
        if config.partitioning:
            assert len(self.movable_solvers) == 0

            for solver in self.placed_solvers():
                solver.redundant = False

            solved_solvers = []
            for node in self.get_nodes():
                assert isinstance(node, framework.AndNode)
                assert not node.solved
                assert not partition_received or node != p_node or len([solver for solver in self.solvers_at(node) if solver.partitioning]) == 1
                assert node.status != framework.SolveStatus.sat
                if node.status != framework.SolveStatus.unsat:
                    continue

                ##+ also implement partition tree pruning?
                node.solved = True
                ## .. otherwise, it has already been decremented
                if node.counted:
                    config.partition_count -= 1
                    node.counted = False
                for solver in self.solvers_at(node):
                    if solver.partitioning:
                        if not (partition_received and node == p_node):
                            assert solver.n_partitions > 0
                            config.partition_count -= solver.n_partitions
                        solver.partitioning = False
                    assert solver not in self.movable_solvers
                    assert solver not in solved_solvers
                    solved_solvers.append(solver)
                    node.total_runtime += solver.runtime()

            partition_node_candidate = None
            unsolved_nodes = self.get_nodes()
            for node in unsolved_nodes:
                assert not node.solved
                assert node.status == framework.SolveStatus.unknown

                if not any(solver.partitioning for solver in self.solvers_at(node)) and len(node) == 0:
                    if partition_node_candidate is None:
                        partition_node_candidate = node

            if config.node_timeout:
                for solver in self.placed_solvers():
                    node = solver.node
                    if node.solved:
                        continue
                    assert solver not in solved_solvers

                    assert solver.started()
                    if solver.runtime() < config.node_timeout:
                        continue

                    if solver.partitioning and not (partition_received and node == p_node):
                        continue

                    self.add_movable_solver(solver)
                    if not solver.partitioning:
                        ## usually, solver.runtime() > config.node_timeout
                        node.total_runtime += config.node_timeout
                    else:
                        node.total_runtime += solver.runtime()

                    ## at a certain point, do not block the expansion any more and decrement
                    if config.n_timeouts_to_count_partition and node.counted and node.n_timeouts() >= config.n_timeouts_to_count_partition:
                        config.partition_count -= 1
                        node.counted = False

            if partition_received and not p_node.solved:
                assert len([solver for solver in self.solvers_at(p_node) if solver.partitioning]) == 1
                for solver in self.solvers_at(p_node):
                    if solver.partitioning:
                        solver.partitioning = False
                        break

            redundant_solvers = []
            ## allow redundant re-placement only when the tree has changed
            ##+ should only be useful if portfolio_max is not configured or tree is small
            if partition_received or solved_solvers or (not config.redundant_only_if_tree_changed and self.movable_solvers):
                for node in unsolved_nodes:
                    n_stay = 0
                    ##++ config by redundant_max
                    min_stay = config.portfolio_min
                    assert min_stay > 0
                    ## keep solvers that already run for some time
                    solvers = sorted(self.solvers_at(node), key=lambda solver: solver.runtime(), reverse=True)
                    for solver in solvers:
                        if solver.partitioning:
                            assert not (partition_received and node == p_node)
                            n_stay += 1
                            continue
                        ## i.e. just timeouted
                        if solver in self.movable_solvers:
                            continue
                        if n_stay < min_stay:
                            n_stay += 1
                            continue
                        assert solver not in redundant_solvers
                        redundant_solvers.append(solver)
                        solver.redundant = True

            if solved_solvers:
                self.movable_solvers = solved_solvers + self.movable_solvers
                solved_solvers.clear()

            if redundant_solvers:
                self.movable_solvers += redundant_solvers
                redundant_solvers.clear()

            assert all(not solver.partitioning for solver in self.movable_solvers)
            assert not partition_received or all(not solver.partitioning for solver in self.placed_solvers() if solver.node == p_node)

            will_partition = self.will_partition(partition_node_candidate, partition_received)

            assert len(self.movable_solvers) <= self.total_solvers
            if self.movable_solvers:
                assert config.portfolio_min >= 1
                ##+ minPortfolio disregarded
                n = len(self.movable_solvers)
                nodes = self.get_nodes_to_solve(n)
                if will_partition:
                    assert partition_node_candidate is not None
                    assert not any(solver.partitioning for solver in self.solvers_at(partition_node_candidate))
                    if partition_node_candidate in nodes:
                        nodes.remove(partition_node_candidate)
                        nodes.insert(0, partition_node_candidate)
                    ## only if there is not already any solver that will stay there
                    elif not any(solver not in self.movable_solvers for solver in self.solvers_at(partition_node_candidate)):
                        nodes.pop()
                        nodes.insert(0, partition_node_candidate)
                assert len(nodes) == n
                for node in nodes:
                    self.map_solver_to_node(node)
            assert len(self.movable_solvers) == 0

            if not isinstance(self.current.root, framework.SMT):
                return

            if will_partition:
                self.partition(partition_node_candidate)

    def run_solver_at_root(self, solver: Solver):
        root = self.current.root

        parameters = {}
        # if config.lemma_amount:
        #     parameters[constant.LEMMAS] = config.lemma_amount
        config.entrust(root, parameters, solver.name, self.solvers_at(root))
        # self.seed_counter += 1
        # if config.lemma_sharing:
        #     parameters.update({
        #         'lemma_push_min': self.seed_counter,
        #         'lemma_pull_min': self.seed_counter*2,
        #         constant.LEMMA_AMOUNT: config.lemma_amount
        #     })
        self.counter += 1
        parameters['parameter.seed'] = self.counter
        solver.solve(root, parameters)
        if not self.current.started():
            self.current.start_time = time.time()

        if not config.partitioning:
            return

        if any(s.partitioning for s in self.solvers_at(root)) or len(root) > 0:
            return
        if self.will_partition(root):
            self.partition(root)

    def add_movable_solver(self, solver: Solver):
        assert solver not in self.movable_solvers
        self.movable_solvers.append(solver)

    ##++ shuffle
    def get_nodes(self, reverse=False, unsolved=True):
        nodes = self.current.root.all_and_children()
        if unsolved:
            nodes = [n for n in nodes if not n.solved]
        nodes.sort(reverse=reverse)
        return nodes

    ## Morpeach
    def get_nodes_to_solve(self, n: int):
        assert n == len(self.movable_solvers)
        nodes = self.get_nodes(reverse=True)
        ret_nodes = []

        n_skipped = 0
        n_to_skip = 0
        for node in nodes:
            assert not hasattr(node, 'n_skipped')
            node.n_skipped = 0
            assert not hasattr(node, 'n_to_skip')
            node.n_to_skip = len([solver for solver in self.solvers_at(node) if solver not in self.movable_solvers])
            n_to_skip += node.n_to_skip
        assert n_to_skip == len(self.placed_solvers()) - n

        max_touts = 0
        while True:
            for node in nodes:
                assert not node.solved
                assert node.status == framework.SolveStatus.unknown
                if node.n_timeouts() > max_touts:
                    continue

                ##+ implement portfolio_max by skipping nodes (if enough available)
                assert node.n_skipped <= node.n_to_skip
                if node.n_skipped < node.n_to_skip:
                    node.n_skipped += 1
                    n_skipped += 1
                    continue

                ret_nodes.append(node)
                if len(ret_nodes) == n:
                    assert n_skipped <= n_to_skip
                    for nd in nodes:
                        del nd.n_skipped
                        del nd.n_to_skip
                    return ret_nodes
            max_touts += 1

    ##+ n to n mapping would be more efficient and avoid unnecessary moves
    def get_solver_for_node(self, node: framework.AndNode):
        assert self.movable_solvers
        assert all(solver.node is not None and not solver.partitioning for solver in self.movable_solvers)

        node_path = node.path()
        node_path_str = str(node_path)

        def cmp(s1: Solver, s2: Solver):
            n1 = s1.node
            n2 = s2.node
            path1 = str(n1.path())
            path2 = str(n2.path())

            prefix1 = os.path.commonprefix([node_path_str, path1])
            prefix1_len = len(prefix1)
            prefix2 = os.path.commonprefix([node_path_str, path2])
            prefix2_len = len(prefix2)
            if prefix1_len < prefix2_len:
                return 1
            if prefix1_len > prefix2_len:
                return -1

            path1_len = len(path1)
            path2_len = len(path2)
            if path1_len < path2_len:
                return -1
            if path1_len > path2_len:
                return 1

            if s1.runtime() < s2.runtime():
                return 1
            if s1.runtime() > s2.runtime():
                return -1

            return 0

        sorted_solvers = sorted(self.movable_solvers, key=functools.cmp_to_key(cmp))
        return sorted_solvers[0]

    def map_solver_to_node(self, node: framework.AndNode):
        solver = self.get_solver_for_node(node)
        assert solver in self.movable_solvers
        self.movable_solvers.remove(solver)
        assert not solver.partitioning
        if solver.redundant:
            ##+ sometimes do not interrupt even if not redundant?
            if solver.node == node:
                return
            solver.node.total_runtime += solver.runtime()
        solver.incremental(node)

    def will_partition(self, node: framework.AndNode, partition_received: bool = False):
        if node is None:
            return False

        if not self.movable_solvers:
            if node != self.current.root:
                return False

        assert partition_received or any(not solver.redundant for solver in self.placed_solvers())

        ##+ minPortfolio disregarded
        n = self.total_solvers
        tree_size = config.partition_count

        if tree_size < n:
            return True

        if partition_received:
            return False

        k = 2
        if tree_size >= k*n:
            return False

        if any(not nd.started() for nd in self.get_nodes()):
            return False

        p = config.partition_policy[1]

        r = 0
        while r == 0:
            r = random.random()
        ##++ parametrization with b is missing
        return r < 1/p

    def partition(self, node: framework.AndNode):
        solvers = sorted(list(self.solvers_at(node)), key=lambda solver: solver.runtime())
        assert solvers
        assert not any(solver.partitioning for solver in solvers)
        solvers[0].ask_partitions(self.level_children(node.level + 1))


    def all_solvers(self):
        return {solver for solver in self._rlist
                if isinstance(solver, Solver)}

    def solvers_at(self, node: framework.AndNode):
        return {solver for solver in self._rlist
                if isinstance(solver, Solver) and solver.node == node}

    def placed_solvers(self):
        return {solver for solver in self._rlist
                if isinstance(solver, Solver) and solver.node is not None}

    def new_solvers(self):
        return {solver for solver in self._rlist
                if isinstance(solver, Solver) and solver.node is None}

    def newly_joined_solvers(self):
        return {solver for solver in self._rlist
                if isinstance(solver, Solver) and (solver.node is None and not solver.started())}

    @property
    def lemma_server(self) -> LemmaServer:
        lemmas = [sock for sock in self._rlist if isinstance(sock, LemmaServer)]
        if lemmas:
            return lemmas[0]

    def log(self, level, message, data=None, time=None, header=None):
        if config.enableLog:
            super().log(level, message)
        else:
            print(*[arg for arg in (data, message, time) if arg is not None])
    # Visual tree ======================================================================================================
    def render_vTree(self, solved_time):
        global comment
        self.v_tree.filename = self.current.root.name  + '.gv';
        # self.node_dict = sorted(self.node_dict.items(), key=self.node_dict.get(2))
        self.v_tree.attr(bgcolor='white',rankdir='UP', label='\n Solvers: ' + str(len(self.all_solvers()) ) + '    Portfolio: '
                                                             + str(config.portfolio_min) + '    Node Timeout: ' +
                                                             str(config.node_timeout) + '      Partition Policy ' +
                                                             str(config.partition_policy) + '      Elapsed Time: ' +str(round(solved_time))+
                                                             '\n'+comment
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
