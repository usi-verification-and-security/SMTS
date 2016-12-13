# !/usr/bin/env python
# -*- coding: utf-8 -*-

import enum
import sqlite3
import hashlib
import json

__author__ = 'Matteo Marescotti'


class SolveStatus(enum.Enum):
    unknown = 0
    sat = 1
    unsat = -1


class Node:
    def __init__(self, parent=None):
        if parent and not isinstance(parent, Node):
            raise TypeError('Node expected')
        self._parent = parent
        if self.parent:
            self.parent.children.append(self)
        self.children = []
        self._status = SolveStatus.unknown
        self._root = self.child([])

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
        return id(self) == id(other)

    def __ne__(self, other):
        return id(self) != id(other)

    # @staticmethod
    # def db_setup(conn, table_prefix=''):
    #     cursor = conn.cursor()
    #     cursor.execute("CREATE TABLE IF NOT EXISTS {}Node ("
    #                    "nid INTEGER PRIMARY KEY, "
    #                    "name TEXT NOT NULL, "
    #                    "path TEXT NOT NULL, "
    #                    "status TEXT NOT NULL, "
    #                    "data TEXT "
    #                    ");".format(table_prefix))
    #     conn.commit()

    # def db_dump(self, conn, name, table_prefix=''):
    #     if type(conn) is sqlite3.Connection:  # cursor initiated only once for all recursive calls
    #         cursor = conn.cursor()
    #     else:
    #         cursor = conn
    #
    #     cursor.execute("INSERT INTO {}Node VALUES (NULL,?,?,?,?);".format(table_prefix), (
    #         name,
    #         str(self.path()),
    #         self.status.name,
    #         self.db_data()
    #     ))
    #     for child in self.children:
    #         child.db_dump(cursor, name, table_prefix=table_prefix)
    #     if type(conn) is sqlite3.Connection:
    #         conn.commit()
    #
    # def db_load(self, conn, name, table_prefix=''):
    #     if type(conn) is sqlite3.Connection:  # cursor initiated only once for all recursive calls
    #         cursor = conn.cursor()
    #     else:
    #         cursor = conn
    #
    #     try:
    #         row = cursor.execute("SELECT * FROM {}Node WHERE name=? and path=?".format(table_prefix), (
    #             name,
    #             str(self.path())
    #         )).fetchall()[0]
    #     except:
    #         raise ValueError('node {}{} not found'.format(name, str(self.path())))
    #
    #     self._status = SolveStatus.__members__[row[3]]
    #     self.db_data(row[4])
    #
    #     while True:
    #         self.add_child()
    #         try:
    #             self.children[-1].db_load(cursor, name, table_prefix)
    #         except ValueError:
    #             break
    #
    #     if type(conn) is sqlite3.Connection:
    #         conn.commit()

    def path(self, nodes=False):
        node = self
        path = []
        while node.parent:
            path.append(node.parent if nodes else node.parent.children.index(node))
            node = node.parent
        path.reverse()
        return path

    def child(self, path):
        node = self
        while node.parent:
            node = node.parent
        for i in path:
            node = node.children[int(i)]
        return node

    def is_ancestor(self, node):
        selfp = self.path()
        nodep = node.path()
        if len(selfp) > len(nodep):
            return False
        for i in range(len(selfp)):
            if selfp[i] != nodep[i]:
                return False
        return True

    @property
    def parent(self):
        return self._parent

    @property
    def root(self):
        return self._root

    def level(self):
        return len(self.path())

    def all(self):
        nodes = [self]
        for node in self.children:
            nodes += node.all()
        return nodes

    @property
    def status(self):
        return self._status

    @status.setter
    def status(self, status):
        self._set_status(status)

    def _set_status(self, status: SolveStatus):
        raise NotImplementedError

        # def db_data(self, data=None):
        #     raise NotImplementedError


class AndNode(Node):
    def __init__(self, smtlib: str, query: str, parent=None):
        if not isinstance(parent, (OrNode, type(None))):
            raise TypeError
        super().__init__(parent)
        self._smtlib = smtlib
        self.query = query

    def smtlib(self, threshold=None):
        node = self
        root = node.root
        smtlibs = []
        while node is not threshold:
            if isinstance(node, AndNode):
                smtlibs.append(node._smtlib)
                if node is not root:
                    smtlibs.append('(push 1)')
            node = node.parent
        smtlibs.reverse()
        return '\n'.join(smtlibs)

    def _set_status(self, status: SolveStatus):
        self._status = status
        if self.parent:
            if status == SolveStatus.sat or all([node.status == status for node in self.parent.children]):
                self.parent.status = status

    # def db_data(self, data=None):
    #     if data:
    #         self.smtlib = data
    #     else:
    #         return self.smtlib


class OrNode(Node):
    def __init__(self, parent):
        if not isinstance(parent, AndNode):
            raise TypeError
        super().__init__(parent)

    def _set_status(self, status: SolveStatus):
        self._status = status
        self.parent.status = status

    # def db_data(self, data=None):
    #     return
