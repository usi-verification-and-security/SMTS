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
from time import sleep

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
        # self.partitioning = False
        self.or_waiting = []
        self.parameters = {}

    def __repr__(self):
        return '<{} at{} {}>'.format(
            self.name,
            self.remote_address,
            'idle' if self.node is None else '{}{}'.format(self.node.root, self.node.path())
        )

    def solve(self, node: framework.AndNode, parameters: dict, sp_value=None):
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

        })
        if config.lemma_sharing:
            parameters.update({
                'lemma_push_min': config.lemmaPush_timeoutMin,
                'lemma_pull_min': config.lemmaPull_timeoutMin,
                'lemma_push_max': config.lemmaPush_timeoutMax,
                'lemma_pull_max': config.lemmaPull_timeoutMax,
                'lemma_amount': config.lemma_amount,
                'colorMode': '1' if config.clientLogColorMode else '0'
            })
        if sp_value:
            parameters.update({
                'split-preference': sp_value,
            })
        if config.enableLog:
            parameters.update({
                'enableLog': '1',
            })
        # print('     sp-param',parameters)
        self.write(parameters, smt)
        self.started = time.time()
        if self.node.started is None:
            self.node.started = time.time()
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
        # self.or_waiting = []
        self.started = time.time()
        if node.started is None:
            node.started = time.time()
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
        global totalN_partitions
        global estimate_partition_time
        totalN_partitions += n
        if self.node is None:
            raise ValueError('not solving anything')
        if config.enableLog:
            print("             Partition emited from ",node,self)
        self.write({
            'command': 'partition',
            'name': self.node.root.name,
            'node': self.node.path(),
            'partitions': n
        }, '')
        if isinstance(self.node, framework.SMT):
            estimate_partition_time = time.time()
        if not node:
            node = framework.OrNode(self.node)

        self.node.partitioning = True
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

        if self.node.root.name != header['name']:
            # print("                 event ... ",header['node'],self.node.root.name,header['name'],str(self.node.path()))
             return {}, b'', False
        # if self.node.root.name != header['name'] or str(self.node.path()) != header['node']:
        #     return {}, b''
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
        # del header['node']
        # print(header['parameter.seed'])
        # if header['node'][1:len(header['node'])-1] != ', '.join(map(str, self.node.path())):
        #     print("   Solver has left . . ..........", header['node'])
        # if self.node.root.status != framework.SolveStatus.unknown:
        #     return {}, b''
        if header['report'] == 'partitions' and self.or_waiting:
            global estimate_partition_time
            node = self.or_waiting.pop()
            # self.partitioning = False
            try:
                # if isinstance(self.node, framework.SMT):
                #     config.node_timeout += round(time.time() - estimate_partition_time)
                for partition in payload.decode().split('\0'):
                    if len(partition) == 0:
                        continue
                    child = framework.AndNode(node, '(assert {})'.format(partition),  config.node_timeout)
                    self._db_log('AND', {'node': str(child.path()), 'smt': child.smt})
                header['partition_recieved'] = True
            except BaseException as ex:
                header['report'] = 'error:(server) error reading partitions: {}'.format(traceback.format_exc())
                node.clear()
                # self.ask_partitions(config.partition_policy[self.node.level % len(config.partition_policy)])
                # self.partitioning = True
            else:
                header['report'] = 'info:(server) received {} partitions'.format(len(node))
                if not config.conflict:
                    config.conflict = header['conflict']
                if not config.status_info:
                    config.status_info = header['statusinf']
                if node.parent.status != framework.SolveStatus.unknown:
                    node.clear()
                if len(node) != config.partition_policy[1]:
                    totalN_partitions = totalN_partitions - config.partition_policy[1] + len(node)
            return header, payload

        if header['report'] in framework.SolveStatus.__members__:
            status = framework.SolveStatus.__members__[header['report']]
            if header['node'] != str(self.node.path()):
                if self.or_waiting:
                    node = self.or_waiting.pop()
                    node.parent.status = status
                    if status == framework.SolveStatus.unsat:
                        for child in node.all():
                            if child.status == framework.SolveStatus.unknown:
                                if isinstance(child, framework.AndNode):
                                    child.status = framework.SolveStatus.unsat
            if self.node.status != framework.SolveStatus.unknown:
                return header, payload
            if not config.conflict:
                config.conflict = header['conflict']
            if not config.status_info:
                config.status_info = header['statusinf']
            self._db_log('STATUS', header)
            path = self.node.path(True)
            self.node.status = status
            path.reverse()
            # print("path . . ..........", path)
            if status == framework.SolveStatus.unsat:
                for child in self.node.all():
                    # print('solved node - > ',child.path())
                    if child.status == framework.SolveStatus.unknown:
                        if isinstance(child, framework.AndNode):
                            child.status = framework.SolveStatus.unsat

            for node in path:
                # print("node . . ..........", node)
                if node.status != framework.SolveStatus.unknown:
                    if isinstance(node, framework.AndNode):
                        self._db_log('SOLVED', {'status': node.status.name, 'node': str(node.path())})
                else:
                    break
            if status != framework.SolveStatus.unknown:
                if self.node == self.node.root:
                    config.level_info = 'root'
                else:
                    config.level_info = 'child'
            else:
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
        self.sp = None

    def __repr__(self):
        return '{}({:.2f})'.format(repr(self.root), self.when_timeout)

    @property
    def when_timeout(self):
        return self.started + self.timeout - time.time() if self.started and self.timeout is not None else float('inf')


class ParallelizationServer(net.Server):
    def __init__(self, logger: logging.Logger = None, port=None):
        super().__init__(port=port, timeout=0.1, logger=logger)
        self.config = config
        self.trees = dict()
        self.current = None
        self.idle_solvers = set()
        # self.solved_solvers = set()
        self.total_solvers = 0
        self.idles = 0
        self.seed_counter = config.lemmaPull_timeoutMax
        self.tcounter = 0
        if self.config.enableLog:
            self.log(logging.INFO, 'server start. version {}'.format(version))
        if self.config.visualize_tree:
            global comment
            comment = 'Lemma Server OFF'
            self.v_tree = graphviz.Digraph('f',
                                           node_attr={'shape': 'record','width': '1%', 'height': '1%', 'fillcolor':'white'})
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
                    # p_str = re.search(r"received [0-9]+ partitions", message)
                    # if not p_str:
                    #     sock.partitioning = False
                    if level == logging.ERROR: # if a solver sends an error then the instance is skipped
                        self.current.timeout = 0
                        print(message,self.current.root.name, self.current.sp)
                        self.close()
                        exit(0)
                        # self.counter += 1
                        # if self.counter == self.total_solvers:
                        #     self.current.timeout = 0
                        #     print('solver sends an error!', self.current.root.name)
                        #     self.close()
                except:
                    level = logging.INFO
                    message = header['report']
                # if header['report'] == 'info:start':
                #     if self.current.started is None:
                #         self.current.started = time.time()
                if self.config.visualize_tree:
                    self.collectData_vTree(sock, message, header)
                if self.config.enableLog:
                    p_str = re.search(r"received [0-9]+ partitions", header['report'])
                    if p_str:
                        self.log(level, '{}: {}'.format(header['node'], message), {'header': header, 'payload': payload.decode()})
                    else:
                        self.log(level, '{}: {}'.format(sock, message), {'header': header, 'payload': payload.decode()})
            self.entrust(header)
            return
        if 'command' in header:
            # if terminate command is found, close the server
            if header['command'] == 'terminate':
                if self.config.visualize_tree:
                    self.render_vTree(0)
                # self.log(logging.INFO, 'Termination command is received!')
                self.close()
            elif header['command'] == 'solve':
                # print(header)
                if 'name' not in header:
                    return
                if self.config.enableLog:
                    self.log(logging.INFO, 'new instance "{}"'.format(
                        header['name']
                    ), {'header': header})
                try:
                    if config.spit_preference:
                        for sp in framework.SplitPreference.__members__.values():
                            instance = Instance(header["name"], payload.decode())
                            instance.timeout = config.solving_timeout
                            instance.sp = sp.value
                            self.trees[header["name"]+sp.value] = instance
                        instance = Instance(header["name"], payload.decode())
                        instance.timeout = config.solving_timeout
                        instance.sp = 'portfolio'
                        self.trees[header["name"]+instance.sp] = instance

                    else:
                        instance = Instance(header["name"], payload.decode())
                        instance.timeout = config.solving_timeout
                        self.trees[header["name"]] = instance
                    if isinstance(instance.root, framework.Fixedpoint) and config.fixedpoint_partition:
                        instance.root.partition()
                except:
                    self.log(logging.ERROR, 'cannot add instance: {}'.format(traceback.format_exc()))

                self.entrust()
        elif 'solver' in header:
            solver = Solver(sock, header['solver'])
            if self.config.enableLog:
                self.log(logging.INFO, 'new {}'.format(
                    solver,
                ), {'header': header, 'payload': payload.decode()})
            self._rlist.remove(sock)
            self._rlist.add(solver)
            # self.total_solvers += 1
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

    def entrust(self, header=None):
        global totalN_partitions
        p_node = None
        # solved_moved_node = None
        partition_recieved = False
        if header:
            if 'partition_recieved' in header:
                p_node = header['node']
                partition_recieved = True
                del header['partition_recieved']
            # elif 'solved-moved' in header:
            #     solved_moved_node = header['node']
            #     del header['solved-moved']
        solving = self.current

        # def active_solvers():
        #     s_counter = 0
        #     for s in self._rlist:
        #         if isinstance(s, Solver):
        #             s_counter += 1
        #     return s_counter
        # if the current tree is already solved or timed out: stop it
        if isinstance(self.current, Instance):
            if sum(map(lambda s : isinstance(s, Solver), self._rlist)) != self.total_solvers:
                print(':error,solvers are lost',self.current.root.name,self.current.sp)
                self.close()
                exit(0)
            if self.current.root.status != framework.SolveStatus.unknown or self.current.when_timeout < 0:
                if self.config.visualize_tree:
                    self.render_vTree(time.time() - self.current.started)
                    self.node_dict.clear()
                    self.node_alias.clear()
                    self.node_alias['[]'] = ('[]')
                    self.rank = 0
                    # for key in self.node_dict:
                    #     print('\n node..',key, '   value... ',self.node_dict[key])
                    import os
                    if os.path.exists(self.v_tree.filename):
                        os.remove(self.v_tree.filename)
                    else:
                        print("The file does not exist")
                    # self.v_tree.clear()
                if not self.config.enableLog and self.config.spit_preference:
                    # print(len(self._rlist)-1,self.total_solvers)
                    if self.current.root.status == framework.SolveStatus.unknown:
                        if self.current.root.partitioning:
                            if not self.current.root.childeren():
                                print(':error,stuck',self.current.root.name, self.current.sp)
                                # if not any([type(socket) == net.Socket and socket is not self._sock for socket in self._rlist]):
                                self.close()
                                exit(0)
                        # self.tcounter += 1
                        # if self.tcounter == 4:
                        #     self.log(logging.INFO, '{}'.format(self.current.root.status.name),
                        #              self.current.root.name, round(time.time() - self.current.started, 3), config.conflict, self.current.sp)
                        #     self.close()
                        #     exit(0)
                        # for key in self.trees:
                        #     if key.startswith(self.current.root.name):
                        #         self.tcounter += 1

                        # del self.trees[self.current.root.name + 'portfolio']
                        # for sp in framework.SplitPreference.__members__.values():
                        #     del self.trees[self.current.root.name + sp.value]
                    self.log(logging.INFO, '{}'.format(self.current.root.status.name),
                             self.current.root.name, round(time.time() - self.current.started, 3), config.conflict, self.current.sp)
                    del self.trees[self.current.root.name + self.current.sp]
                else:
                    del self.trees[self.current.root.name]
                    self.log(
                        logging.INFO,
                        '{} instance "{}" after {:.2f} seconds'.format(
                            'solved' if self.current.root.status != framework.SolveStatus.unknown else 'timeout',
                            self.current.root.name,
                            time.time() - self.current.started))

                for solver in {solver for solver in self.solvers(True) if solver.node.root == self.current.root}:
                    solver.stop()
                self.current = None
                self.counter = 0
                self.idles = 0
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
                    # print(" Start - > ",solver)
                    assert isinstance(solver, Solver)
                    parameters = {}
                    if self.config.lemma_amount:
                        parameters["lemmas"] = self.config.lemma_amount
                    self.config.entrust(self.current.root, parameters, solver.name, self.solvers(self.current.root))
                    self.seed_counter += 1
                    # if config.lemma_sharing:
                    #     parameters.update({
                    #         'lemma_push_min': self.seed_counter,
                    #         'lemma_pull_min': self.seed_counter*2,
                    #         'lemma_amount': config.lemma_amount,
                    #         'colorMode': '1' if config.clientLogColorMode else '0'
                    #     })
                    solver.solve(self.current.root, parameters, self.current.sp)
                    if self.current.started is None:
                        self.current.started = time.time()
                return
                # print(schedulables)

        if solving is not None and solving != self.current and self.lemma_server:
            self.lemma_server.reset(solving.root)
        if self.current is None:
            if solving is not None:
                if self.config.enableLog:
                    self.log(logging.INFO, 'all done.')
                if self.config.idle_quit:
                    # if not any([type(socket) == net.Socket and socket is not self._sock for socket in self._rlist]):
                    self.close()
                    exit(0)
            return

        assert isinstance(self.current, Instance)
        if config.partition_timeout:

            nodes = self.current.root.all()
            # print(nodes)
            # nodes.sort(reverse=True)
            nodes.sort(reverse=False)

            def all_active_nodes(sub_tree):
                return (node for node in sub_tree if (node.status == framework.SolveStatus.unknown and
                                                   isinstance(node, framework.AndNode) and not node.processed))

            def all_active_leaves():
                return (node for node in nodes if (node.status == framework.SolveStatus.unknown and
                                                   isinstance(node, framework.AndNode) and not node.processed))

            def all_active_internals():
                return (node for node in nodes if (len(node) > 0 and node.status == framework.SolveStatus.unknown and isinstance(node, framework.AndNode)
                                                   and not node.processed) or (isinstance(node, framework.SMT) and not node.processed))

            def all_current_active_nodes():
                return (node for node in nodes if (node.status  == framework.SolveStatus.unknown and not isinstance(node, framework.OrNode) and
                                                   not node.processed and node._started is not None))
            def getPartitionedNode(p_node):
                for node in nodes:
                    if str(node.path()) == p_node:
                        return node
                return None;

            def assumed_timout_leaves():
                return (node for node in nodes if len(node) == 0 and node.status == framework.SolveStatus.unknown and not self.solvers(node) and
                        isinstance(node, framework.AndNode) and node.assumed_timout and not node.processed)

            def un_attempted_active_leaves():
                return (node for node in nodes if (len(node) == 0 and node.status == framework.SolveStatus.unknown and
                                                   node.started is None and isinstance(node, framework.AndNode) and not node.processed))

            def attempted_notPartitioned_leaves():
                return (node for node in nodes if len(node) == 0 and not node.partitioning
                        and node.status == framework.SolveStatus.unknown and isinstance(node, framework.AndNode) )
            #
            movable_solvers = []
            solved_solvers = set()
            to_partition_node = None
            solver_partition = False
            # for node in nodes:
            #     if isinstance(node, framework.AndNode) and node.started is not None and not node.processed:
            #         if node.status == framework.SolveStatus.unknown and not node.is_timeout and (node.started + config.node_timeout <= time.time()):
            #             node.assumed_timout = True
            #             self.idles += 1
            #             if self.idles == config.partition_policy[1]:
            #                 break
            # if not config.node_timeout and totalN_partitions != 0 and self.total_solvers/totalN_partitions <= 1:
            #     config.node_timeout = self.current.root.started + 5
            #     print("             node_timeout is set",self.total_solvers,totalN_partitions)
            #     return

            for node in nodes:
                if isinstance(node, framework.AndNode) and not node.processed:
                    if p_node and str(node.path()) == p_node and node.status == framework.SolveStatus.unknown:
                        p_node = node
                    # elif solved_moved_node and str(node.path()) == solved_moved_node:
                    #     node.status = framework.SolveStatus.__members__[header['report']]
                    #     print('       solved solver in forked -> ', solver)
                    #     return
                    if node.status == framework.SolveStatus.unsat:
                        if node.parent:
                            if node.parent.parent.status == framework.SolveStatus.unknown:
                                # print('       solved solver has unknown parent -> ')
                                for solver in self.solvers(node.parent.parent):
                                    # print('       solved solver parent-> ', solver)
                                    if solver in self.idle_solvers:
                                        self.idle_solvers.remove(solver)
                                        # if solver.node is not self.current.root:
                                        #     solver.incremental(self.current.root)
                                    else:
                                        totalN_partitions -= 1
                                    # self.solved_solvers.add(solver)
                                    solved_solvers.add(solver)
                                node.parent.parent.processed = True
                                        # print('       solved solver was in idle -> ', solver)
                                    # else:
                                    #     # print('       solved solver parent-> ', solver)
                                    #     totalN_partitions -= 1
                                    #     solved_solvers.add(solver)
                                    #     if solver.node is not self.current.root:
                                    #         # solver.incremental(self.current.root)
                                    #         node.parent.parent.processed = True
                        node.processed = True
                        for solver in self.solvers(node):
                            # print('       solved solver -> ', solver)
                            if solver in self.idle_solvers:
                                self.idle_solvers.remove(solver)
                                # if solver.node is not self.current.root:
                                # solver.incremental(self.current.root)
                                # print('       solved solver was in idle -> ', solver)
                            else:
                                totalN_partitions -= 1
                            # self.solved_solvers.add(solver)
                            solved_solvers.add(solver)
                                # if solver.node is not self.current.root:
                                #     solver.incremental(self.current.root)
                                # print('       solved solver -> ', solver)
                    elif config.node_timeout and not node.is_timeout and (node.started and node.started + config.node_timeout <= time.time()):
                        if node.partitioning and (node._children[0])._children:
                            # print("               timout node has partition . . . ",node)
                            global estimate_partition_time
                            # for ch in node.childeren():
                            if node.assumed_timout:
                                # node.assumed_timout = False
                                for solver in self.solvers(node):
                                    # if solver.or_waiting:
                                    #     return
                                    # print('              node has been assumed_timout - done partition -> ',self.idles,node.path() )
                                    self.idle_solvers.add(solver)
                                    # print("     idle .. ",self.idle_solvers)
                                    # config.node_timeout += estimate_partition_time+1
                            else:
                                for solver in self.solvers(node):
                                    # if solver.or_waiting:
                                    #     return
                                    # print('              Timeout solver - has partition -> ',self.idles,node.path() )
                                    self.idle_solvers.add(solver)
                                    totalN_partitions -= 1
                            node.processed = True
                            node.is_timeout = True
                                # config.node_timeout += estimate_partition_time+1
                                # if self.idles >= config.partition_policy[1]:
                                #     print('    now partition TO ',self.idles)
                                #     break
                            # break

                        elif not node.assumed_timout:
                            # self.idles += 1
                            for solver in self.solvers(node):
                                # print('    Timeout solvers n-p ',len(self.idle_solvers) ,solver)
                                # if solver.or_waiting:
                                #     return
                                totalN_partitions -= 1
                                self.idle_solvers.add(solver)
                                # print('    Timeout solvers n-p ATO ',node.path())
                                node.assumed_timout = True
                    elif not config.node_timeout and len(node) == 0 and len(self.solvers(node)) == 1:
                        config.node_timeout = time.time() - self.current.root.started + 45
                        print("             node_timeout is set", config.node_timeout)
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
                                    print(" Solved -> Move from: ",s_solver, " to : ",ch.path())
                                    s_solver.incremental(ch)
                                    raise StopIteration

                        except StopIteration:
                            continue
                # for ch in childs:
                #     if len(solved_solvers) != 0 and ch.status == framework.SolveStatus.unknown:
                #         s_solver = solved_solvers.pop()
                #         if s_solver.node is not ch:
                #             s_solver.incremental(ch)
            if partition_recieved:
                if config.portfolio_min:
                    # if totalN_partitions > 0:
                    # self.config.portfolio_min = math.ceil(self.total_solvers / (totalN_partitions + 1))-1
                    # print("             self.config.portfolio_min",self.config.portfolio_min)
                    # if self.config.portfolio_min <= 1 and not config.node_timeout:
                    #     print("             node_timeout is set")
                    #     config.node_timeout = 30
                    # for _node in all_current_active_nodes():
                    solvers = self.solvers(p_node)
                    # solvers = sorted(solvers,key = lambda s: s.node, reverse=True)
                    # self.config.portfolio_min = (len(self.solvers(False))) / round((totalN_partitions + 1))
                    # print(" self.config.portfolio_min...",self.config.portfolio_min)
                    # try:
                    # print(" internal", _node,len(solvers))
                    counter = len(solvers)
                    # print("     len of the solvers", self.total_solvers ,totalN_partitions,len(solvers), self.config.portfolio_min)
                    for solver in solvers:
                        if 0 <= self.config.portfolio_min < counter:
                            if solver not in self.idle_solvers:
                                counter -= 1
                                # print("             Redundent_Solver",solver)
                                movable_solvers.append(solver)
                else:
                    movable_solvers = list(self.solvers(True))
            # if self.solved_solvers:
            #     for node in un_attempted_active_leaves():
            #         try:
            #             for solver in self.solved_solvers:
            #                 if solver.node.is_ancestor(node):
            #                     self.solved_solvers.remove(solver)
            #                     solver.incremental(node)
            #                     raise StopIteration
            #         except StopIteration:
            #             continue
            #             # solved_solver = self.solved_solvers.pop()
            #             # solved_solver.incremental(node)
                while 0 != len(movable_solvers):
                    for node in p_node.childeren():
                        if len(movable_solvers) == 0:
                            break
                        # for node in selection():
                        # print("     Movables , Dest_Node - >" ,len(movable_solvers),node.path())
                        # if there are still no solvers available
                        try:
                            for solver in movable_solvers:
                                movable_solvers.remove(solver)
                                solver.incremental(node)
                                raise StopIteration
                        except StopIteration:
                            continue

            # if solved_solvers:
            #     sorted(map(lambda n : isinstance(n, framework.AndNode) and n.status == framework.SolveStatus.unknown
            #     and not n.processed, nodes), key=lambda leave: leave.level, reverse=True)
            #     all_active_leaves_l = list(all_active_leaves())
            #     all_active_leaves_l = sorted(all_active_leaves_l,  key=lambda leave: len(self.solvers(leave)), reverse=False)
            #     all_active_leaves_l =  sorted(all_active_leaves_l,  key=lambda leave: leave.level, reverse=True)
            #     level = all_active_leaves_l[0].level
            #     while 0 != len(solved_solvers):
            #         # for selection in [un_attempted_active_leaves]:
            #         #     if len(solved_solvers) == 0:
            #         #         break
            #         for node in all_active_leaves_l:
            #             # print("     Move solved to Dest_Node - >" ,len(solved_solvers),node.path())
            #             if node.level != level:
            #                 continue
            #             if len(solved_solvers) == 0:
            #                 break
            #             if isinstance(self.current.root, framework.SMT):
            #                 try:
            #                     # print("solved_solvers",len(solved_solvers))
            #                     for solver in solved_solvers:
            #                             # print("                  > 0 ...     Stayed Here -> " ,solver, node.path())
            #                         solved_solvers.remove(solver)
            #                         solver.incremental(node)
            #                         raise StopIteration
            #
            #                 except StopIteration:
            #                     continue

            # attempted_notPartitioned = list(attempted_notPartitioned_leaves())
            # attempted_notPartitioned = sorted(attempted_notPartitioned, reverse=False )
            if self.idle_solvers:
                # should_continue = True
                # un_attempted_active_leaves_l = list(un_attempted_active_leaves())
                # un_attempted_active_leaves_l = sorted(un_attempted_active_leaves_l, key = lambda leave: leave,reverse=False)
                # print("")
                for node in un_attempted_active_leaves():
                    if len(self.idle_solvers) == 0:
                        break
                    # should_continue = False
                    # print("     idle_solvers , Dest_Node - >" ,len(self.idle_solvers),node)
                    try:
                        for solver in self.idle_solvers:
                            if solver.node.is_ancestor(node):
                                self.idle_solvers.remove(solver)
                                # print("                  > 0 ...     go ansetor -> " ,solver.node.path(), node.path())
                                solver.incremental(node)

                                raise StopIteration
                            elif len(solver.node) == 0 and solver.node.assumed_timout and not node.is_timeout and\
                                   not self.solvers(node) and solver.node is not node:
                                self.idle_solvers.remove(solver)
                                # print("                  > 0 ...     backtrack -> " ,solver.node.path(), node.path())
                                solver.incremental(node)

                                raise StopIteration

                    except StopIteration:
                        continue
                # assumed_timout_leaves_l = list(assumed_timout_leaves())
                # assumed_timout_leaves_l = sorted(assumed_timout_leaves_l, key = lambda leave: leave, reverse=True)
                # print("")
                if solver_partition:
                    # print("   go to_partition_node", to_partition_node)
                    if self.idle_solvers:
                        idles = sorted(self.idle_solvers,  key = lambda s:
                        str(to_partition_node.path())[1:len(str(s.node.path()))-1] == str(s.node.path())[1:len(str(s.node.path()))-1], reverse=True)
                        solver = idles.pop()
                        print("   stuck in partitioning", solver.node.path(), to_partition_node.path())
                        solver.incremental(to_partition_node)
                    # for node in attempted_notPartitioned_leaves():
                    #     if len(self.idle_solvers) == 0:
                    #         break
                    #     # should_continue = False
                    #     if self.solvers(node):
                    #         continue
                    #
                    #     if isinstance(self.current.root, framework.SMT):
                    #         if self.config.incremental > 0:
                    #             try:
                    #                 for solver in self.idle_solvers:
                    #
                    #                     if solver.node is None or solver.node is node:
                    #                         # if node.assumed_timout:
                    #                         # config.node_timeout += estimate_partition_time+1
                    #                         # print("                  > 0 ...     next node -> " ,solver, node.path())
                    #                         raise StopIteration
                    #                     if solver.node.is_ancestor(node):
                    #
                    #                         # self.movable_solvers_map[solver] = True
                    #                         self.idle_solvers.remove(solver)
                    #                         # print("                  > 0 ...     go ansetor -assumed -> " ,solver, node.path())
                    #                         solver.incremental(node)
                    #                         # if node.assumed_timout:
                    #                         # node.started = time.time()
                    #
                    #
                    #                         raise StopIteration
                    #             except StopIteration:
                    #                 continue
                    #         # print("                 dar oumad - > " ,node)
                    #         if self.config.incremental > 1:
                    #             try:
                    #                 # try to use incremental on another already solving solver
                    #                 for solver in self.idle_solvers:
                    #                     # if node.assumed_timout and self.solvers(node):
                    #                     #     print("             1 > idle_solvers  ..   go next node -> " ,solver, node.path())
                    #                     #     raise StopIteration
                    #                     if solver.node is not None and solver.node is not node:
                    #
                    #                         # if node.assumed_timout:
                    #                         # node.started = time.time()
                    #                         # print("             1 > idle_solvers  ..   Move - assumed -> " ,solver, node.path())
                    #                         # config.node_timeout += estimate_partition_time+1
                    #                         self.idle_solvers.remove(solver)
                    #                         solver.incremental(node)
                    #                         # node.portfolio -= 1
                    #                         # sleep(2)
                    #                         # break
                    #                         raise StopIteration
                    #
                    #             except StopIteration:
                    #                 continue

            # only standard instances can partition
            if not isinstance(self.current.root, framework.SMT):
                return
            # if need partition: ask partitions

            # for leaf in attempted_notPartitioned:

            if to_partition_node is not None:
                # if len(to_partition_node.path()) == 2:
                #     return
                if self.total_solvers > totalN_partitions + 1:
                    self.partition(to_partition_node)
                        # config.solver_partition = True
                # else:
                #     config.solver_partition = True
                #     break


    def partition(self, node: framework.AndNode, force=False):
        max_children = self.level_children(node.level)
        for i in range(max_children - len(node)):
            solvers = list(self.solvers(node))
            random.shuffle(solvers)
            for solver in solvers:
                # ask the solver to partition if timeout or if needed because idle solvers
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

    def log(self, level, message, data=None, time=None, conflict=None, sp=None):
        if config.enableLog:
            super().log(level, message)
        else:
            if config.spit_preference:
                res = 'Failed'
                if config.status_info == '':
                    config.status_info = None
                    res = None
                elif message == 'unknown':
                    res = 'Timout'
                    config.level_info = None
                elif config.status_info == message or config.status_info == 'unknown':
                    res = 'Passed'
                if conflict == '':
                    conflict = None
                print(data, message, time, conflict, sp, config.status_info, res, config.level_info)
                config.level_info = None
                config.status_info = None
                config.conflict = None
            else:
                print(data, message, time)
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
                                                             + str(self.config.portfolio_min) + '     node_timeout: ' +
                                                             str(self.config.node_timeout) + '      Partition_policy ' +
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

            self.v_tree.edge('node'+key+':f2', 'node'+key+':f2', color='white', label = total_solver)
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
                            self.v_tree.node('node' + part_node, nohtml('<f0> |<f1> ' + self.node_alias[part_node] + '|<f2>'), color='blue')
                        else:
                            self.v_tree.node('node' + part_node, nohtml('<f0> |<f1> ' + self.node_alias[part_node] + '|<f2>'))

                        self.v_tree.edge('node'+key+':f1', 'node'+part_node+':f1', color = color)
                    index += 1
        if lastSolvedNode != '':
            self.v_tree.node('node' + lastSolvedNode, nohtml('<f0> '+ str(self.node_dict[lastSolvedNode][0]) + ' - '
                                                             + str(self.node_dict[lastSolvedNode][7])+'|<f1> '
                                                             + "("+str(rank)+") " + self.node_alias[lastSolvedNode] + '|<f2>' +
                                                             str(self.node_dict[lastSolvedNode][1]) + ' - ' + str(self.node_dict[lastSolvedNode][6])),
                             color ='#11971A')
        self.v_tree.view()

    def collectData_vTree(self, sock, message, header):
        solver_IP = re.search(r"\['[0-9]+.[0-9]+.[0-9].[0-9]', *\d+\]", format(sock))
        message_str = re.search(r"\[(.*)\]", message)
        if message_str:
            currentNode = message_str[0]
        else:
            currentNode = '[]'
        destNode = header
        # destNode = re.search(r"\[(([0-9])+ *,* [0-9]*)*\]", format(sock))
        if destNode:
            destNodePath = destNode['node']
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
            # for c in self.node_dict:
            #     if c==currentNode or c==destNodePath:
            #         print(c,self.node_dict[c])
            # print('par list', self.node_dict[destNodePath][5])