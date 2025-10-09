#!/usr/bin/env python3

import sys

# Default timeout
def_to = 1200

usage = """
%s -- create scatter plot gnuplot scripts using tex driver
Usage: %s <x-res> <y-res> <x-label> <y-label> <div> <subdiv> <output> [<timeout>]
<x-res> and <y-res> are the result files for horizontal
and vertical axis.  The results files should consist of
lines of the form

<name> <result> <time>

where <name> identifies uniquely a formula, <result> is
indet, sat or unsat, and <time> is a
floating point number.
<x-label> and <y-label> are the axis labels, and <output>
is a tex file where gnuplot will place its output.  The
script assumes %d seconds time out
"""

if __name__ == '__main__':
    if len(sys.argv) < 8 or len(sys.argv) > 9:
        print(usage % (sys.argv[0], sys.argv[0], def_to))
        sys.exit(1)

    to = def_to if len(sys.argv) == 8 else float(sys.argv[8])

    output = sys.argv[7]

    division = sys.argv[5].replace("_", "\\\\_")
    subdivision = sys.argv[6].replace("_", "\\\\_")

    x_l = open(sys.argv[1], 'r').readlines()
    y_l = open(sys.argv[2], 'r').readlines()

    use_log = True
    # use_log = False

    minTime = 1e6

    epsTime = 4e-3

    def getRes(lst):
        global minTime

        h_out = {}
        for el in lst:
            rec = el.split()
            name = rec[0]
            res = rec[1]
            time = -1
            if len(rec) > 2:
                time = float(rec[2])
                if time == 0:
                    time = epsTime
                minTime = min(minTime, time)
            if name in h_out:
                print("Duplicate result: %s" % name)
                sys.exit(1)
            if (res in ['unknown', 'indet']) and time < to:
                time = -2 # mem out
            elif (res in ['unknown', 'indet']):
                time = -1 # time out
            elif (res not in ['sat', 'unsat']):
                print("Unrecognized result: %s" % res)
                sys.exit(1)
            assert time != 0
            h_out[name] = [res, time]
        return h_out

    try:
        x_res = getRes(x_l)
        y_res = getRes(y_l)
    except UnboundLocalError as e:
        print(e)
        print("Problem: %s, %s" % (x_l, y_l))
        sys.exit(1)

    assert minTime >= epsTime

    max_x = max(map(lambda x: x_res[x][1], x_res))
    max_y = max(map(lambda x: y_res[x][1], y_res))

    max_all = max(max_x, max_y)

    low = 0

    if (use_log):
        bnd = 1.5*to
        bnd2 = 2*to
    else:
        bnd = 1.01*to
        bnd2 = 1.02*to

    speedups = []
    x_total = 0
    y_total = 0
    sat_speedups = []
    unsat_speedups = []
    sat_x_total = 0
    sat_y_total = 0
    unsat_x_total = 0
    unsat_y_total = 0

    for k in x_res.keys():
        if k not in y_res:
            print("!! Not in y: %s" % k, file=sys.stderr)
        elif x_res[k][1] >= 0:
            if y_res[k][1] < 0:
                print("unknown y-only (%s): %s" % (x_res[k][0], k), file=sys.stderr)
            else:
                assert x_res[k][1] > 0
                assert y_res[k][1] > 0
                speedups.append(x_res[k][1]/float(y_res[k][1]))
                x_total += float(x_res[k][1])
                y_total += float(y_res[k][1])
                if x_res[k][0] == 'sat' and y_res[k][0] == 'sat':
                    sat_speedups.append(x_res[k][1]/float(y_res[k][1]))
                    sat_x_total += float(x_res[k][1])
                    sat_y_total += float(y_res[k][1])
                elif x_res[k][0] == 'unsat' and y_res[k][0] == 'unsat':
                    unsat_speedups.append(x_res[k][1]/float(y_res[k][1]))
                    unsat_x_total += float(x_res[k][1])
                    unsat_y_total += float(y_res[k][1])

    for k in y_res.keys():
        if k not in x_res:
            print("!! Not in x: %s" % k, file=sys.stderr)
        elif y_res[k][1] >= 0 and x_res[k][1] < 0:
            print("unknown x-only (%s): %s" % (y_res[k][0], k), file=sys.stderr)

    x_par2 = 0
    y_par2 = 0
    sat_x_par2 = 0
    sat_y_par2 = 0
    unsat_x_par2 = 0
    unsat_y_par2 = 0

    for k in x_res.keys():
        if k not in y_res:
            continue
        if x_res[k][1] < 0:
            x_par2 += float(2*to)
        else:
            x_par2 += float(x_res[k][1])
            if x_res[k][0] == 'sat':
                sat_x_par2 += float(x_res[k][1])
                if y_res[k][1] < 0:
                    sat_y_par2 += float(2*to)
            elif x_res[k][0] == 'unsat':
                unsat_x_par2 += float(x_res[k][1])
                if y_res[k][1] < 0:
                    unsat_y_par2 += float(2*to)
        if y_res[k][1] < 0:
            y_par2 += float(2*to)
        else:
            y_par2 += float(y_res[k][1])
            if y_res[k][0] == 'sat':
                sat_y_par2 += float(y_res[k][1])
                if x_res[k][1] < 0:
                    sat_x_par2 += float(2*to)
            elif y_res[k][0] == 'unsat':
                unsat_y_par2 += float(y_res[k][1])
                if x_res[k][1] < 0:
                    unsat_x_par2 += float(2*to)

    # print('PAR-2: x:{} y:{}'.format(x_par2, y_par2), file=sys.stderr)

    speedup = sum(speedups)/len(speedups)
    if sat_speedups:
      sat_speedup = sum(sat_speedups)/len(sat_speedups)
    if unsat_speedups:
      unsat_speedup = sum(unsat_speedups)/len(unsat_speedups)

    # print("Not solved for x", file=sys.stderr)
    # for k in x_res:
    #     e = x_res[k]
    #     if e[1] == -1:
    #         print("%s timeout" % k, file=sys.stderr)
    #     elif e[1] == -2:
    #         print("%s memout" % k, file=sys.stderr)

    # print("Not solved for y", file=sys.stderr)
    # for k in y_res:
    #     e = y_res[k]
    #     if e[1] == -1:
    #         print("%s timeout" % k, file=sys.stderr)
    #     elif e[1] == -2:
    #         print("%s memout" % k, file=sys.stderr)

    # print(len(x_res), file=sys.stderr)
    # print(len(y_res), file=sys.stderr)


    solved_x = len(list(filter(lambda x: x_res[x][1] >= 0, x_res.keys())))
    solved_y = len(list(filter(lambda x: y_res[x][1] >= 0, y_res.keys())))
    sat_solved_x = len(list(filter(lambda x: x_res[x][1] >= 0 and x_res[x][0] == 'sat', x_res.keys())))
    sat_solved_y = len(list(filter(lambda x: y_res[x][1] >= 0 and y_res[x][0] == 'sat', y_res.keys())))
    unsat_solved_x = len(list(filter(lambda x: x_res[x][1] >= 0 and x_res[x][0] == 'unsat', x_res.keys())))
    unsat_solved_y = len(list(filter(lambda x: y_res[x][1] >= 0 and y_res[x][0] == 'unsat', y_res.keys())))


    def postProc(h, bnd):
        for k in h:
            if h[k][1] == -1:
                h[k][1] = bnd
            if h[k][1] == -2:
                h[k][1] = bnd2

    postProc(x_res, bnd)
    postProc(y_res, bnd)

    print('#!/usr/bin/env gnuplot')
#    print('set term epslatex standalone color size 8, 4')
    # print('set term pngcairo')
    print('set term svg dynamic fname "Arvo"')

    print('set output "%s"' % output)
    print('set size square')
    print('set size 0.8, 0.8')
    print('set title "%s %s"' % (division, subdivision))
    print('set xlabel "%s" offset 0,0.25' % sys.argv[3])
    print('set ylabel "%s" offset 1.5,0' % sys.argv[4])
    if (use_log):
        print('set logscale x')
        print('set logscale y')
    print('set key right bottom')
    print('set xrange [%f:%f]' % (low, bnd2))
    print('set yrange [%f:%f]' % (low, bnd2))
    print('set pointsize 0.7')
    print('set arrow from graph 0, first %f to %f,%f nohead' % (to, to, to))
    print('set arrow from %f, graph 0 to %f,%f nohead' % (to, to, to))
    print('set arrow from graph 0, first %f to %f,%f nohead' % (bnd, bnd, bnd))
    print('set arrow from %f, graph 0 to %f,%f nohead' % (bnd, bnd, bnd))
    print('set arrow from %f, graph 0 to graph .98, graph -.12 backhead lt 2' % bnd)
    print('set label "t/o" at graph .95, graph -0.14')
    print('set arrow from %f, graph 0 to graph 1.05, graph -.08 backhead lt 2' % bnd2)
    print('set label "m/o" at graph 1.05, graph -0.1')

    sat_point_str = 'point pointtype 2 lc "green"'
    unsat_point_str = 'point pointtype 4 lc "red"'
    unknown_point_str = 'point pointtype 3 lc "blue"'
    unsound_point_str = 'point pointtype 5 lc "magenta"'

    label_xpos = 1.05
    label_xsatpos = 1.1
    label_ypos = 0.8
    label_ypos_step = 0.075
    # print('set label "avg speedup x/y: %.02f" at graph %f,%f' % (speedup, label_xpos, label_ypos))
    # label_ypos -= label_ypos_step
    # if sat_speedups:
    #     print('set label "   - sat speedup x/y: %.02f" at graph %f,%f' % (sat_speedup, label_xpos, label_ypos))
    # label_ypos -= label_ypos_step
    # if unsat_speedups:
    #     print('set label "   - unsat speedup x/y: %.02f" at graph %f,%f' % (unsat_speedup, label_xpos, label_ypos))
    # label_ypos -= label_ypos_step
    # print('set label "total time ratio x/y: %.02f" at graph %f,%f' % (x_total/float(y_total), label_xpos, label_ypos))
    # label_ypos -= label_ypos_step
    # if sat_speedups:
    #     print('set label "   - sat ratio x/y: %.02f" at graph %f,%f' % (sat_x_total/float(sat_y_total), label_xpos, label_ypos))
    # label_ypos -= label_ypos_step
    # if unsat_speedups:
    #     print('set label "   - unsat ratio x/y: %.02f" at graph %f,%f' % (unsat_x_total/float(unsat_y_total), label_xpos, label_ypos))
    # label_ypos -= label_ypos_step
    print('set label "solved x: %d" at graph %f,%f' % (solved_x, label_xpos, label_ypos))
    label_ypos -= label_ypos_step
    print('set label "" at graph %f,%f %s' % (label_xsatpos, label_ypos, sat_point_str))
    print('set label "   solved x: %d" at graph %f,%f' % (sat_solved_x, label_xsatpos, label_ypos))
    label_ypos -= label_ypos_step
    print('set label "" at graph %f,%f %s' % (label_xsatpos, label_ypos, unsat_point_str))
    print('set label "   solved x: %d" at graph %f,%f' % (unsat_solved_x, label_xsatpos, label_ypos))
    label_ypos -= label_ypos_step
    print('set label "solved y: %d" at graph %f,%f' % (solved_y, label_xpos, label_ypos))
    label_ypos -= label_ypos_step
    print('set label "" at graph %f,%f %s' % (label_xsatpos, label_ypos, sat_point_str))
    print('set label "   solved y: %d" at graph %f,%f' % (sat_solved_y, label_xsatpos, label_ypos))
    label_ypos -= label_ypos_step
    print('set label "" at graph %f,%f %s' % (label_xsatpos, label_ypos, unsat_point_str))
    print('set label "   solved y: %d" at graph %f,%f' % (unsat_solved_y, label_xsatpos, label_ypos))
    label_ypos -= label_ypos_step
    label_ypos -= label_ypos_step
    print('set label "PAR-2 ratio x/y: %.02f" at graph %f,%f' % (x_par2/float(y_par2), label_xpos, label_ypos))
    label_ypos -= label_ypos_step
    if sat_speedups:
        print('set label "" at graph %f,%f %s' % (label_xsatpos, label_ypos, sat_point_str))
        print('set label "   ratio x/y: %.02f" at graph %f,%f' % (sat_x_par2/float(sat_y_par2), label_xsatpos, label_ypos))
    label_ypos -= label_ypos_step
    if unsat_speedups:
        print('set label "" at graph %f,%f %s' % (label_xsatpos, label_ypos, unsat_point_str))
        print('set label "   ratio x/y: %.02f" at graph %f,%f' % (unsat_x_par2/float(unsat_y_par2), label_xsatpos, label_ypos))
    label_ypos -= label_ypos_step

    print('plot x title "" lc "black", "-" title "" with %s, "-" title "" with %s, "-" title "" with %s, "-" title "" with %s' % (sat_point_str, unsat_point_str, unknown_point_str, unsound_point_str))

    sat_strings = []
    unsat_strings = []
    ukn_strings = []
    fail_strings = []
    for name in x_res:
        if name in y_res:
            if (x_res[name][0] == 'sat' and \
                y_res[name][0] == 'unsat') or \
               (x_res[name][0] == 'unsat' and \
                y_res[name][0] == 'sat'):
                print("!! Oops: %s %s %s" %
                        (name, x_res[name], y_res[name]), file=sys.stderr)
                fail_strings.append("%.02f %.02f # %s" % (x_res[name][1], y_res[name][1], name))

            elif (x_res[name][0] == 'sat') or (y_res[name][0] == 'sat'):
                sat_strings.append("%.02f %.02f # %s" % (x_res[name][1], y_res[name][1], name))
            elif (x_res[name][0] == 'unsat') or (y_res[name][0] == 'unsat'):
                unsat_strings.append("%.02f %.02f # %s" % (x_res[name][1], y_res[name][1], name))
            else:
                ukn_strings.append("%.02f %.02f # %s" % (x_res[name][1], y_res[name][1], name))

    print("\n".join(sat_strings))
    print("e")
    print("\n".join(unsat_strings))
    print("e")
    print("\n".join(ukn_strings))
    print("e")
    print("\n".join(fail_strings))
    print("e")
