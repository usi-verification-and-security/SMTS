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

    @property
    def status(self):
        return self.data['status'] if self.data and 'status' in self.data else 'unknown'

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

        row_solved = c.execute('select ts, data FROM SolvingHistory where id = (select min(id) '
                               'from SolvingHistory '
                               'where name = ? and event = "STATUS" and node = "[]");', (benchmark.name,)).fetchone()
        if row_solved:
            benchmark.data = json.loads(row_solved[1])
            benchmark.data['status'] = benchmark.data['report']
        else:
            row_solved = c.execute('select ts, data FROM SolvingHistory where id = (select max(id) '
                               'from SolvingHistory '
                               'where name = ? and event = "SOLVED");', (benchmark.name,)).fetchone()
            if row_solved:
                data = json.loads(row_solved[1])
                if data['node'] != '[]':
                    row_solved = None
                else:
                    benchmark.data = data

        if not row_solved:
            next_started = c.execute('SELECT ts, data FROM SolvingHistory WHERE id = (SELECT min(id) '
                                     'FROM SolvingHistory '
                                     'WHERE name != ? AND ts > ? AND event = "+");',
                                     (benchmark.name, ts_start)).fetchone()

            if next_started:
                row_solved = (next_started[0], None)
            else:
                row_solved = (c.execute('SELECT max(ts) FROM SolvingHistory;').fetchone()[0], None)

        benchmark.ts_end = row_solved[0]
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
                        benchmark.status,
                        benchmark.ts_end - benchmark.ts_start))
                total_time += benchmark.ts_end - benchmark.ts_start
        print('TOTAL for ' + arg + ': {}'.format(total_time))


if __name__ == '__main__':
    main()
