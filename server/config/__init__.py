import sys
import pathlib
import sqlite3
import importlib.util

from config.default import *


def z3():
    if hasattr(z3, 'z3'):
        return z3.z3
    z3.z3 = _import(z3_path, 'z3')
    return z3()


def db():
    if hasattr(db, 'db'):
        return db.db
    if db_path is None:
        return None
    db.db = sqlite3.connect(str(db_path.resolve()))
    return db()


def _import(path, name=None):
    path = pathlib.Path(path).resolve()
    if name and path.is_dir():
        path /= name
    if path.suffix != '.py':
        path = pathlib.Path(str(path) + '.py')
    sys.path.insert(0, str(path.parent))
    spec = importlib.util.spec_from_file_location(path.stem, str(path))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    sys.path.pop(0)
    return module


def extend(path):
    module = _import(path)
    for attr in dir(module):
        if attr[:1] == '_':
            continue
        globals()[attr] = getattr(module, attr)


_config_path = pathlib.Path(pathlib.Path(__file__).parent / 'config.py')
if _config_path.exists():
    extend(_config_path)
