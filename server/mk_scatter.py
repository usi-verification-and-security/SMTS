#!/usr/bin/env python3

import sys
import pathlib
import optparse
import sql2times


def times2res(times_path):
    lst = times_path.open('r').readlines()
    h_out = {}
    for el in lst:
        try:
            rec = el.split()

            name = rec[0]
            res = rec[1]
            try:
                time = rec[2]
            except IndexError:
                print('no time: {} {}'.format(name, res), file=sys.stderr)
                time = -1
            if name in h_out:
                print("Duplicate result: %s" % name, file=sys.stderr)
            if res in ['unknown', 'indet']:
                time = -1  # time out
            h_out[name] = [res, float(time)]
        except:
            print(el, file=sys.stderr)
            raise
    return h_out


def db2res(db_path, attribute=None):
    benchmarks = sql2times.get_benchmarks(db_path)
    res = {}
    for benchmark in benchmarks:
        if not benchmark.ts_start:
            continue
        status = 'unknown'
        if benchmark.data and 'status' in benchmark.data:
            status = benchmark.data['status']
        t = benchmark.ts_end - benchmark.ts_start
        if t == 0:  # approximation problem!
            t = 1
        res[benchmark.name] = [status, t]
        if attribute and benchmark.data and attribute in benchmark.data:
            res[benchmark.name].append(benchmark.data[attribute])
    return res


if __name__ == '__main__':
    parser = optparse.OptionParser(usage='usage: %prog [options] x-res y-res x-label y-label output')
    parser.add_option('-m', dest='min', type='float',
                      default=0.01, help='chart minimum')
    parser.add_option('-M', dest='max', type='float',
                      default=1000.0, help='char maximum')
    parser.add_option('-p', dest='attribute', type='str',
                      default=None, help='plot data attribute')

    options, args = parser.parse_args()

    if len(args) != 5:
        parser.print_help()
        sys.exit(-1)

    path_x = pathlib.Path(args[0]).resolve()
    if path_x.suffix == '.db':
        x_res = db2res(path_x, options.attribute)
    else:
        x_res = times2res(path_x)

    path_y = pathlib.Path(args[1]).resolve()
    if path_y.suffix == '.db':
        y_res = db2res(path_y, options.attribute)
    else:
        y_res = times2res(path_y)

    max_x = max(map(lambda x: x_res[x][1], x_res))
    max_y = max(map(lambda x: y_res[x][1], y_res))

    max_all = max(max_x, max_y)

    bnd = 2 * options.max

    speedups = []
    x_total = 0
    y_total = 0

    print("Not in y:", file=sys.stderr)
    for k in x_res.keys():
        if k not in y_res:
            print("  %s" % k, file=sys.stderr)
            continue
        if k in y_res and x_res[k][1] >= 0 and y_res[k][1] > 0:
            if x_res[k][0] != 'unknown' and y_res[k][0] != 'unknown':
                speedups.append(float(x_res[k][1]) / float(y_res[k][1]))
                x_total += float(x_res[k][1])
                y_total += float(y_res[k][1])
    print(file=sys.stderr)

    print("Not in x:", file=sys.stderr)
    for k in y_res.keys():
        if k not in x_res:
            print("  %s" % k, file=sys.stderr)
    print(file=sys.stderr)

    if speedups:
        speedup = sum(speedups) / len(speedups)
    else:
        print(file=sys.stderr)
        print("No instances in common!", file=sys.stderr)
        sys.exit(-1)

    # print("Not solved for x:", file=sys.stderr)
    for k in x_res:
        if x_res[k][0] not in ('sat', 'unsat'):
            # print("  %s" % k, file=sys.stderr)
            x_res[k][1] = bnd
    # print(file=sys.stderr)

    # print("Not solved for y:", file=sys.stderr)
    for k in y_res:
        if y_res[k][0] not in ('sat', 'unsat'):
            # print("  %s" % k, file=sys.stderr)
            y_res[k][1] = bnd
    # print(file=sys.stderr)

    solved_x = len([x for x in x_res.keys() if x_res[x][1] != bnd and x in y_res])
    solved_y = len([y for y in y_res.keys() if y_res[y][1] != bnd and y in x_res])


    def postProc(h, bnd):
        for k in h:
            if h[k][1] == -1:
                h[k][1] = bnd


    postProc(x_res, bnd)
    postProc(y_res, bnd)

    print('---', file=sys.stderr)

    print('#!/usr/bin/env gnuplot')
    #    print('set term epslatex standalone header "\\\\input{macros.tex}"')
    if args[4].endswith('.ps'):
        print('set term postscript')
    elif args[4].endswith('.tex'):
        print('set term epslatex standalone color')
    elif args[4].endswith('.svg'):
        print('set term svg')
    print('set output "%s"' % args[4])
    print('set size square')
    print('set xlabel "%s"' % args[2])
    print('set ylabel "%s"' % args[3])
    print('set logscale x')
    print('set logscale y')
    print('set key right bottom')
    print('set xrange [%f:%f]' % (options.min, bnd))
    print('set yrange [%f:%f]' % (options.min, bnd))
    print('set pointsize 1.5')
    print('set arrow from graph 0, first %f to %f,%f nohead' % (options.max, options.max, options.max))
    print('set arrow from %f, graph 0 to %f,%f nohead' % (options.max, options.max, options.max))
    print('set arrow from %f, graph 0 to graph 1.05, graph -.05 backhead' % bnd)
    print('set label "timeout" at graph 1.05, graph -0.06')
    print('set label "sp tot x/y %.02f" at graph -0.3,1' % (x_total / float(y_total)))
    print('set label "sp x/y %.02f" at graph -0.3,0.95' % speedup)
    print('set label "solved x %d" at graph -0.3,0.9' % solved_x)
    print('set label "solved y %d" at graph -0.3,0.85' % solved_y)
    solved_both = len(
        [y for y in y_res if
         y in [x for x in x_res.keys() if x_res[x][1] != bnd and x in y_res] and y_res[y][1] != bnd])
    print('set label "solved both %d" at graph -0.3,0.8' % solved_both)
    print('set label "solved x+y %d" at graph -0.3,0.75' % (solved_x + solved_y - solved_both))
    if options.attribute:
        n_x = 0
        n_y = 0
        for res in (x_res, y_res):
            for name in res:
                if len(res[name]) == 3 and res[name][2]:
                    if res is x_res:
                        if name not in y_res:
                            continue
                        print('set label "%s" at %.02f, graph %0.2f font ",8"' %
                              (res[name][2],
                               x_res[name][1],
                               n_x * 0.01 + 0.01)
                              )
                        if name in y_res:
                            print('set arrow from %.02f,graph 0 to %.02f,%.02f nohead lt 2 lw 0.5' %
                                  (x_res[name][1],
                                   x_res[name][1],
                                   y_res[name][1])
                                  )
                        n_x += 1
                    else:
                        if name not in x_res:
                            continue
                        print('set label "%s" at graph 0.03, first %0.2f font ",8"' %
                              (res[name][2],
                               y_res[name][1])
                              )
                        print('set arrow from graph 0, first %.02f to %.02f,%.02f nohead lt 2 lw 0.5' %
                              (y_res[name][1],
                               x_res[name][1],
                               y_res[name][1])
                              )
    print('plot x title "" lc "black", '
          '"-" title "" with point pointtype 2 lc "black", '
          '"-" title "" with points pointtype 4 lc "black", '
          '"-" title "" with points pointtype 3 lc "black", '
          '"-" title "" with points pointtype 5 lc 1')
    # print('plot x title "", "-" title "SAT" with point pointtype 2, "-" title "UNSAT" with points pointtype 4, "-" title "Unknown" with points pointtype 3')

    sat_strings = []
    unsat_strings = []
    ukn_strings = []
    fail_strings = []
    for name in x_res:
        if name in y_res:
            if (x_res[name][0] == 'sat' and y_res[name][0] == 'unsat') or \
                    (x_res[name][0] == 'unsat' and y_res[name][0] == 'sat'):
                print("Oops: %s %s %s" % (name, x_res[name], y_res[name]), file=sys.stderr)
                fail_strings.append("%.02f %.02f # %s" % (x_res[name][1], y_res[name][1], name))
            elif (x_res[name][0] == 'sat') or (y_res[name][0] == 'sat'):
                sat_strings.append("%.02f %.02f # %s" % (x_res[name][1], y_res[name][1], name))
            elif (x_res[name][0] == 'unsat') or (y_res[name][0] == 'unsat'):
                unsat_strings.append("%.02f %.02f # %s" % (x_res[name][1], y_res[name][1], name))
            else:
                ukn_strings.append("%.02f %.02f # %s" % (x_res[name][1], y_res[name][1], name))

    if sat_strings:
        print("\n".join(sat_strings))
    print("e")
    if unsat_strings:
        print("\n".join(unsat_strings))
    print("e")
    if ukn_strings:
        print("\n".join(ukn_strings))
    print("e")
    if fail_strings:
        print("\n".join(fail_strings))
    print("e")
