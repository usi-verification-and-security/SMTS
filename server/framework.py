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


class AndNode(Node):
    def __init__(self, smtlib, query, parent=None):
        if not isinstance(parent, (OrNode, type(None))):
            raise TypeError
        super().__init__(parent)
        self.smtlib = smtlib
        self.query = query

    def _set_status(self, status: SolveStatus):
        self._status = status
        if self.parent:
            if status == SolveStatus.sat or all([node.status == status for node in self.parent.children]):
                self.parent.status = status


class OrNode(Node):
    def __init__(self, parent):
        if not isinstance(parent, AndNode):
            raise TypeError
        super().__init__(parent)

    def _set_status(self, status: SolveStatus):
        self._status = status
        self.parent.status = status
