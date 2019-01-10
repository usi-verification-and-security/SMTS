#!/usr/bin/env python3

import sys
import pathlib
import optparse
import sql2times
import mk_scatter

if __name__ == '__main__':
    parser = optparse.OptionParser(usage='usage: %prog [options] file [...] label [...]')
    parser.add_option('-o', dest='output', type='string',
                      default=None, help='output file')
    parser.add_option('-b', dest='bottom', action='store_true',
                      default=False, help='put legend on bottom right')

    options, args = parser.parse_args()

    if options.output is None:
        parser.error('output file not given')

    files = set()
    all_res = []
    for i in range(int(len(args) / 2)):
        path = pathlib.Path(args[i]).resolve()

        if path.suffix == '.times':
            res = mk_scatter.times2res(path)
        else:
            res = mk_scatter.db2res(path)

        if i == 0:
            files = set(res.keys())

        values = [[k, res[k][0], res[k][1]] for k in res if k in files]
        files.intersection_update(set([i[0] for i in values]))
        all_res.append([args[i + int(len(args) / 2)], values])

    for item in all_res:
        item[1] = sorted([[i[1], i[2], i[0]] for i in item[1] if i[0] in files], key=lambda i: i[1])

    print('#!/usr/bin/env gnuplot')
    if options.output.endswith('.ps'):
        print('set term postscript eps enhanced color')
    elif options.output.endswith('.tex'):
        print('set term epslatex standalone color')
    elif options.output.endswith('.svg'):
        print('set term svg')
    print('set output "%s"' % options.output)
    print('set xlabel "\\\\# solved instances"')
    print('set ylabel "runtime (sec.)"')
    print('#set logscale y')
    print('set key out horiz %s' % ('bottom right' if options.bottom else 'top left'))
    print('#set key font ",8"')
    print('#set key spacing 3')
    print('#set xrange [0:]')
    print('#set yrange [0:]')
    plot_c = []
    for item in all_res:
        plot_c.append('"-" title "%s" with linespoints' % item[0])  # with linespoints

    print('plot %s' % ','.join(plot_c))
    for item in all_res:
        i = 1
        for res in item[1]:
            if res[0] in ('sat', 'unsat'):
                print('{}, {:2f} # {}'.format(i, res[1], res[2]))  # res[2] is filename
                i += 1
        print('e')
