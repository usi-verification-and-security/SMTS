#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sqlite3
import sys
import json
import traceback
import pathlib


class Benchmark:
    def __init__(self, name):
        self.name = name
        self.ts_start = None
        self.ts_end = None
        self.data = None

    def __repr__(self):
        return '<{},{},{}>'.format(self.name, self.ts_start, self.ts_end)


def get_benchmarks(db_path):
    path_sql = db_path.resolve()
    if not path_sql.is_file():
        raise ValueError(str(db_path) + ' is not a file')
    conn = sqlite3.connect(str(db_path))
    c = conn.cursor()
    benchmarks = {Benchmark(name[0]) for name in c.execute('SELECT DISTINCT(name) FROM SolvingHistory;')}
    for benchmark in benchmarks:
        ts_start = c.execute('SELECT min(ts) '
                             'FROM SolvingHistory '
                             'WHERE name = ? AND event = "+";', (benchmark.name,)).fetchone()[0]
        if (ts_start is None):
            continue
        benchmark.ts_start = ts_start
        row_solved = c.execute('SELECT ts, data FROM SolvingHistory WHERE id = (SELECT min(id) '
                               'FROM SolvingHistory '
                               'WHERE name = ? AND event = "SOLVED");', (benchmark.name,)).fetchone()
        if not row_solved:
            next_started = c.execute('SELECT ts, data FROM SolvingHistory WHERE id = (SELECT min(id) '
                                     'FROM SolvingHistory '
                                     'WHERE name != ? AND ts > ? AND event = "+");',
                                     (benchmark.name, ts_start)).fetchone()

            if next_started:
                row_solved = (next_started[0], None)
            else:
                row_solved = (c.execute('SELECT max(ts) FROM SolvingHistory;').fetchone()[0], None)
        else:
            # if there is STATUS event on root I use that record instead because
            # it contains more data (statistics of the solver)
            # otherwise if STATUS comes from SOLVED of deeper node, then I just use data from STATUS
            # which is just the status itself.
            row_root_status = c.execute('SELECT ts, data FROM SolvingHistory WHERE id = (SELECT min(id) '
                                        'FROM SolvingHistory '
                                        'WHERE name = ? AND event = "STATUS" AND node="[]");',
                                        (benchmark.name,)).fetchone()
            if row_root_status:
                try:
                    json_solved = json.loads(row_solved[1])
                except:
                    json_solved = {}
                try:
                    json_root_status = json.loads(row_root_status[1])
                except:
                    json_root_status = {}
                json_root_status.update(json_solved)
                row_solved = (row_root_status[0], json.dumps(json_root_status))

        benchmark.ts_end = row_solved[0]
        benchmark.data = json.loads(row_solved[1]) if row_solved[1] else None
    return benchmarks


def main():
    for arg in sys.argv[1:]:
        try:
            path = pathlib.Path(arg)
            benchmarks = get_benchmarks(path)
        except BaseException as ex:
            print(traceback.format_exc())
            continue
        total_time = 0
        with open(str(path.parent / (path.stem + '.times')), 'w') as file_times:
            for benchmark in benchmarks:
                if not benchmark.ts_start:
                    print('not started: ' + benchmark.name)
                    continue
                file_times.write(
                    '{} {} {}\n'.format(
                        benchmark.name,
                        benchmark.data['status'] if benchmark.data and 'status' in benchmark.data else 'unknown',
                        benchmark.ts_end - benchmark.ts_start))
                total_time += benchmark.ts_end - benchmark.ts_start
        print('TOTAL for ' + arg + ': {}'.format(total_time))


if __name__ == '__main__':
    main()
