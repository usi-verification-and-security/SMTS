#!/usr/bin/env python3

import csv
import sys
import argparse
import json
import os
import yaml

# Default timeout
def_to = 1200

def die(s):
    print("{}: {}".format(sys.argv[0], s), file=sys.stderr)
    sys.exit(1)

usage = """
Create cactus gnuplot scripts using the png driver.
The results files should consist of lines of the form

<name> <result> <time>

where <name> identifies uniquely a formula, <result> is indet, sat,
unsat, or unsound and <time> is a floating point number.  If no input
files are given, the script reads the results file names instead from
stdin, one per line.
"""

# def getParallelSolvers(f):

#     COL_SOLVER_NAME = "Solver Name"
#     COL_TRACK_CLOUD = "Cloud Track"
#     COL_TRACK_PARALLEL = "Parallel Track"

#     res = set()
#     with open(f) as csvfile:
#         reader = csv.DictReader(csvfile)
#         for row in reader:
#             if row[COL_TRACK_CLOUD] != "" or row[COL_TRACK_PARALLEL] != "":
#                 res.add(row[COL_SOLVER_NAME])
#     return res

# def getUnsoundSolvers(unsound_csv_name, divisions, track):
#     COL_LOGIC = "logic"
#     COL_SOLVER_NAME = "solver name"

#     track_map = \
#         json.loads(open(divisions).read())["track_{}".format(track)]

#     logicToDivision = dict()

#     for d in track_map.keys():
#         for l in track_map[d]:
#             logicToDivision[l] = d

#     res = dict()
#     with open(unsound_csv_name) as unsound_csv:
#         reader = csv.DictReader(unsound_csv)
#         for row in reader:
#             division = logicToDivision[row[COL_LOGIC]]
#             if division not in res:
#                 res[division] = set()
#             res[division].add(row[COL_SOLVER_NAME])
#     return res

def parseArgs():
    parser = argparse.ArgumentParser(description=usage)
    parser.add_argument("-o", "--output", required=True, \
            help="The output file, where gnuplot will generate the"\
                "figure")
    parser.add_argument("-t", "--timeout", required=False, type=float, help="timeout")
    # parser.add_argument("-c", "--csv", required=False, \
    #         help="Csv indicating whether the solver is a cloud /" \
    #             "parallel solver")
    # parser.add_argument("-u", "--unsound", required=False, \
    #         help="Csv indicating which solvers are known to be unsound"
    #             "in which logic")
    # parser.add_argument("-d", "--division", required=True, \
    #         help="The division to process.")
    # parser.add_argument("-D", "--divisions", required=True, \
    #         help="The divisions map file.")
    # parser.add_argument("-t", "--track", required=True, \
    #         help="The track to process.")
    parser.add_argument("lists", metavar="N", nargs='*', \
            help="A result list")
    parser.add_argument("-y", "--year", type=int, help="year: used for placing plot files in the directory structure")

    return parser.parse_args()

if __name__ == '__main__':

    args = parseArgs()

    if args.timeout:
        to = args.timeout
    else:
        to = def_to

    output = args.output

    description_name = os.path.splitext(output)[0]

    description = dict()
    description['layout'] = "plot"
    description['name'] = os.path.basename(description_name)
    # description['track'] = args.track
    description['type'] = "cactus"
    # description['division'] = args.division
    description['year'] = args.year
    description['plot'] = os.path.basename(output)

    input_files = args.lists if len(args.lists) > 0 else \
            list(map(lambda x: x.strip(), sys.stdin.readlines()))

    parallelSolvers = set()
    # parallelSolvers = set() if args.csv == None \
    #         else getParallelSolvers(args.csv)

    unsoundSolvers = dict()
    # unsoundSolvers = dict() if args.unsound == None \
    #         else getUnsoundSolvers(args.unsound, \
    #                 args.divisions, args.track)

    # if args.division in unsoundSolvers.keys():
    #     description['unsound'] = []
    #     for solver in unsoundSolvers[args.division]:
    #         description['unsound'].append({'name': solver})


    results = dict()
    max_runtime = list()
    min_runtime = list()

    for input_file in input_files:
        result = open(input_file).readlines()

        solverName = input_file.split("/")[-1].removesuffix(".list")

        use_log = True

        def getRes(lst):
            names = dict()
            resList = list()
            for el in lst:
                rec = el.split()
                name = rec[0]
                res = rec[1]
                time = -1
                if len(rec) > 2:
                    time = rec[2]
                if name in names:
                    print("Duplicate result: %s" % name)
                    sys.exit(1)
                if (res == 'indet') and float(time) < to:
                    time = -2 # mem out
                elif (res == 'indet'):
                    time = -1 # time out
                elif (res == 'unsound'):
                    time = -3 # unsound
                names[name] = True
                resList.append([res, float(time)])
            return resList

        try:
            results[solverName] = getRes(result)
        except UnboundLocalError as e:
            print(e)
            print("Problem: %s" % (input_file))
            sys.exit(1)
        try:
            max_runtime.append(max(map(lambda x: x[1], results[solverName])))
            min_runtime.append(min(filter(lambda x: x >= 0, \
                    map(lambda x: x[1], results[solverName]))))
        except ValueError as e:
            pass
#            print("No results for {}".format(input_file))
#            sys.exit(1)

    if len(max_runtime) == 0:
        assert len(min_runtime) == 0
        # die("No result for any input file on track {}, "\
        #     "division {}".format(args.track, args.division))
        die("No result for any input file")
        sys.exit(1)

    max_all = max(max_runtime)
    low = min(min_runtime)

    if (use_log):
        bnd = 1.5*to
    else:
        bnd = 1.01*to

    def postProc(resList):
        for r in resList:
            if r[1] < 0:
                r[1] = bnd

    for k in results.keys():
        postProc(results[k])
        results[k].sort(key=lambda x: x[1])
        # add the index
        for i in range(0, len(results[k])):
            if results[k][i][1] == bnd:
                results[k][i].append(i)
                break
            else:
                results[k][i].append(i+1)

    print('#!/usr/bin/env gnuplot')
    print('set term svg dynamic fname "Arvo"')

    print('set output "%s"' % output)
    print('set xlabel "timeout"')
    print('set ylabel "instances"')
    if (use_log):
        print('set logscale x')
    print('set key left top')
    print('set xrange [%f:%f]' % (low, bnd))
    # print('set pointsize 1')

    print('plot %s' % (\
        # ", ".join(['"-" with linespoints title "%s" %s' % \
        ", ".join(['"-" with lines title "%s" %s' % \
                    # (x if args.division not in unsoundSolvers.keys() or \
                    #       x not in unsoundSolvers[args.division] \
                    #     else "{}*".format(x), \
                    (x if True \
                        else "{}*".format(x), \
                    "" if x not in parallelSolvers else "lw 3") \
                    for x in results.keys() ])))

    for name in results.keys():
        result = results[name]
        print("\n".join(map(lambda x: " ".join(map(str, x[1:3])), result)))
        print("e")

    # descr_file = open("{}.md".format(description_name), 'w')
    # descr_file.write("---\n{}\n---".format(yaml.dump(description)))
