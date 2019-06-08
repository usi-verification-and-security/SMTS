import sys
import pathlib
import sqlite3
import importlib.util

from .default import *

__author__ = 'Matteo Marescotti'


def db():
    if hasattr(db, 'db'):
        return db.db
    if db_path is None:
        return None
    db.db = sqlite3.connect(db_path)
    cursor = db.db.cursor()
    cursor.execute("DROP TABLE IF EXISTS {}ServerLog;".format(table_prefix))
    cursor.execute("CREATE TABLE IF NOT EXISTS {}ServerLog ("
                   "id INTEGER NOT NULL PRIMARY KEY, "
                   "ts INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
                   "level TEXT NOT NULL,"
                   "message TEXT NOT NULL,"
                   "data TEXT"
                   ");".format(table_prefix))
    cursor.execute("DROP TABLE IF EXISTS {}SolvingHistory;".format(table_prefix))
    cursor.execute("CREATE TABLE IF NOT EXISTS {}SolvingHistory ("
                   "id INTEGER NOT NULL PRIMARY KEY, "
                   "ts INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
                   "name TEXT NOT NULL, "
                   "node TEXT, "
                   "event TEXT NOT NULL, "
                   "solver TEXT, "
                   "data TEXT"
                   ");".format(table_prefix))
    db.db.commit()
    return db()


def _import(path, name=None):
    if path:
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
    else:
        module = importlib.import_module(name)
    return module


def extend(path):
    path = pathlib.Path(path).resolve()
    module = _import(path)
    for attr_name in dir(module):
        if attr_name.startswith('_'):
            continue
        attr = getattr(module, attr_name)
        if isinstance(attr, dict):
            globals()[attr_name].update(attr)
        else:
            globals()[attr_name] = attr
            if attr_name.endswith('_path'):
                if isinstance(attr, str) and not attr.startswith('/'):
                    globals()[attr_name] = str(path.parent / attr)
                elif isinstance(attr, list):
                    globals()[attr_name] = list(map(lambda i: i if i.startswith('/') else str(path.parent / i), attr))


def entrust(node, header: dict, solver_name, solvers: set):
    for key in parameters:
        try:
            solver, parameter = key.split('.', 1)
            if solver != solver_name:
                raise ValueError
        except:
            continue
        if callable(parameters[key]):
            header['parameter.{}'.format(parameter)] = parameters[key]()
        else:
            header['parameter.{}'.format(parameter)] = parameters[key]
    if solver_name == "Spacer":
        if len(solvers) % 4 == 0:
            header["-p"] = "def"
            header["parameter.fp.spacer.push_pob"] = "true"
            header["parameter.fp.spacer.reset_pob_queue"] = "false"
        if len(solvers) % 4 == 1:
            header["-p"] = "ic3"
            header["parameter.fp.spacer.push_pob"] = "true"
        if len(solvers) % 4 == 2:
            header["parameter.fp.spacer.gpdr"] = "true"
            header["-p"] = "newgpdr"
        if len(solvers) % 4 == 3:
            header["-p"] = "gpdr"

    if solver_name == "SALLY":
        if len(solvers) % 3 == 0:
            header['parameter.opensmt2-itp-lra'] = "0"
        elif len(solvers) % 3 == 1:
            header['parameter.opensmt2-itp-lra'] = "2"
        elif len(solvers) % 3 == 2:
            header['parameter.opensmt2-itp-lra'] = "4"
        if (len(solvers)/2) % 3 == 0:
            header['parameter.opensmt2-itp-bool'] = "0"
        elif (len(solvers)/2) % 3 == 1:
            header['parameter.opensmt2-itp-bool'] = "1"
        elif (len(solvers)/2) % 3 == 2:
            header['parameter.opensmt2-itp-bool'] = "2"
        if (len(solvers)/4) % 3 == 0:
            header['parameter.pdkind-induction-max'] = "0"
        elif (len(solvers)/4) % 3 == 1:
            header['parameter.pdkind-induction-max'] = "1"
        elif (len(solvers)/4) % 3 == 2:
            header['parameter.pdkind-induction-max'] = "2"



extend(pathlib.Path(pathlib.Path(__file__).parent / 'default.py'))
try:
    extend(pathlib.Path(pathlib.Path(__file__).parent / 'config.py'))
except:
    pass
