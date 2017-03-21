import enum
import config

__author__ = 'Matteo Marescotti'


class SolveStatus(enum.Enum):
    unknown = 0
    sat = 1
    unsat = -1


class Node:
    def __init__(self, parent, smt):
        if parent and not isinstance(parent, Node):
            raise TypeError('Node expected')
        self._parent = parent
        if self._parent is not None:
            self._parent._children.append(self)
            self._root = self._parent._root
        else:
            self._root = self
        self.smt = smt
        self._children = []
        self._status = SolveStatus.unknown

    def __repr__(self):
        return '<{}:{}>'.format(
            self.path(),
            self.status.name
        )

    def __hash__(self):
        return id(self)

    def __lt__(self, other):
        return len(self.path()) < len(other.path())

    def __eq__(self, other):
        return self is other

    def __ne__(self, other):
        return self is not other

    def __getitem__(self, key):
        return self._children[key]

    def __iter__(self):
        return self._children.__iter__()

    def __len__(self):
        return self._children.__len__()

    def path(self, nodes=False):
        node = self
        path = []
        while node.parent:
            path.append(node.parent if nodes else node.parent._children.index(node))
            node = node.parent
        path.reverse()
        return path

    def is_ancestor(self, node):
        if self.root != node.root:
            raise ValueError('provided node from a different tree')
        selfp = self.path()
        nodep = node.path()
        if len(selfp) > len(nodep):
            return False
        for i in range(len(selfp)):
            if selfp[i] != nodep[i]:
                return False
        return True

    def all(self):
        nodes = [self]
        for node in self:
            nodes += node.all()
        return nodes

    def clear(self):
        self._children.clear()

    @property
    def parent(self):
        return self._parent

    @property
    def root(self):
        return self._root

    @property
    def level(self):
        return len(self.path())

    @property
    def status(self):
        return self._status

    @status.setter
    def status(self, status):
        self._set_status(status)

    def _set_status(self, status: SolveStatus):
        raise NotImplementedError


class AndNode(Node):
    def __init__(self, parent, smt):
        if not isinstance(parent, (OrNode, type(None))):
            raise TypeError
        super().__init__(parent, smt)

    def _set_status(self, status: SolveStatus):
        self._status = status
        if self.parent:
            if status == SolveStatus.sat or all([node.status == status for node in self.parent]):
                self.parent.status = status


class OrNode(Node):
    def __init__(self, parent, smt: str = ''):
        if not isinstance(parent, AndNode):
            raise TypeError
        super().__init__(parent, smt)

    def _set_status(self, status: SolveStatus):
        self._status = status
        self.parent.status = status


class Root(AndNode):
    def __init__(self, name: str, smt: str):
        super().__init__(None, smt)
        self.name = name

    def __repr__(self):
        return '<{} "{}":{}>'.format(
            self.__class__.__name__,
            self.name,
            self.status.name
        )

    def child(self, path: list):
        node = self
        for i in path:
            node = node[int(i)]
        return node

    def to_string(self, node: AndNode, start: AndNode = None):
        raise NotImplementedError


class SMT(Root):
    def __init__(self, name: str, smt: str):
        super().__init__(name, smt.split('(check-sat)')[0])

    def to_string(self, node: AndNode, start: AndNode = None):
        n_pop = 0
        if start is not None:
            while not start.is_ancestor(node):
                if isinstance(start, AndNode):
                    n_pop += 1
                start = start.parent
        smt = []

        while node is not start:
            if isinstance(node, AndNode):
                smt.append(node.smt)
                if node != self.root:
                    smt.append('(push 1)')
            node = node.parent
        smt += ['(pop 1)' for _ in range(n_pop)]
        smt.reverse()
        return '\n'.join(smt), '(check-sat)'


class Fixedpoint(Root):
    def __init__(self, name, fixedpoint, queries):
        self.fixedpoint = fixedpoint
        self.queries = queries
        s = self.fixedpoint.to_string([])
        super().__init__(name, s[s.index('(declare-rel '):])

    def partition(self):
        if len(self) > 0:
            raise ValueError('already partitioned')
        _f = config.z3().Fixedpoint(ctx=self.fixedpoint.ctx)
        query = self.queries[0]
        queries = []
        for rule in self.fixedpoint.get_rules():
            if config.z3().is_quantifier(rule):
                imp = rule.body()
                body = imp.arg(0)
                if imp.num_args() == 2:  # if is implies
                    head = imp.arg(1)
                    if config.z3().is_and(body):
                        for i in range(body.num_args()):  # app always before others
                            ch = body.arg(i)
                            if config.z3().is_app(ch) and ch.decl().kind() == config.z3().Z3_OP_UNINTERPRETED:
                                _f.register_relation(ch.decl())
                            else:
                                break
                    else:
                        _f.register_relation(body.decl())
                    if head.eq(query):
                        head = config.z3().Bool(query.sexpr() + str(len(queries)), self.fixedpoint.ctx)
                        queries.append(head)
                    _f.register_relation(head.decl())
                    _f.add_rule(head, body)
                    continue
            else:
                imp = rule
            _f.register_relation(imp.decl())
            _f.add_rule(rule)

        s = _f.to_string([])
        parent = OrNode(self, s[s.index('(declare-rel '):])
        if config.db():
            config.db().cursor().execute("INSERT INTO {}SolvingHistory (name, node, event, solver, data) "
                                         "VALUES (?,?,?,?,?)".format(config.table_prefix), (
                                             self.name,
                                             str(self.path()),
                                             'OR',
                                             '',
                                             {'node': parent.path(), 'smt': parent.smt}
                                         ))

        for query in queries:
            child = AndNode(parent, '(query ' + query.sexpr() + ')')
            if config.db():
                config.db().cursor().execute("INSERT INTO {}SolvingHistory (name, node, event, solver, data) "
                                             "VALUES (?,?,?,?,?)".format(config.table_prefix), (
                                                 self.name,
                                                 str(self.path()),
                                                 'AND',
                                                 '',
                                                 {'node': child.path(), 'smt': child.smt}
                                             ))

        if config.db():
            config.db().commit()

    def to_string(self, node: AndNode, start: AndNode = None):
        if node.root != self:
            raise ValueError
        if node is self:
            return self.smt, '(query ' + self.queries[0].sexpr() + ')'
        elif isinstance(node, AndNode):
            return node.parent.smt, node.smt


def parse(name: str, smt: str):
    context = config.z3().Context()
    fixedpoint = config.z3().Fixedpoint(ctx=context)
    queries = fixedpoint.parse_string(smt)
    if not queries:
        return SMT(name, smt)
    else:
        return Fixedpoint(name, fixedpoint, queries)
