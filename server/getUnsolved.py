#!/usr/bin/env python3

import argparse
import csv
# Set time limit for interesting benchmarks. The default here is 1 second.
TIME_LIMIT = 1
# Set the minimum for number of asserts in unsat core track
NUM_ASSERTS = 2

#==============================================================================

COL_BENCHMARK = 'benchmark'
COL_SOLVER = 'solver'
COL_CONFIG = 'configuration'
COL_CPU = 'cpu time'
COL_STATUS = 'status'
COL_RESULT = 'result'
COL_EXPECTED = 'expected'
COL_ASSERTS = 'number of asserts'

#==============================================================================
LOGICS = set([
  'ABV',
  'ABVFP',
  'ABVFPLRA',
  'ALIA',
  'ANIA',
  'AUFBV',
  'AUFBVDTLIA',
  'AUFBVDTNIA',
  'AUFBVFP',
  'AUFDTLIA',
  'AUFDTLIRA',
  'AUFDTNIRA',
  'AUFFPDTLIRA',
  'AUFFPDTNIRA',
  'AUFLIA',
  'AUFLIRA',
  'AUFNIA',
  'AUFNIRA',
  'BV',
  'BVFP',
  'BVFPLRA',
  'FP',
  'FPLRA',
  'LIA',
  'LRA',
  'NIA',
  'NRA',
  'QF_ABV',
  'QF_ABVFP',
  'QF_ABVFPLRA',
  'QF_ALIA',
  'QF_ANIA',
  'QF_AUFBV',
  'QF_AUFBVFP',
  'QF_AUFBVLIA',
  'QF_AUFBVNIA',
  'QF_AUFLIA',
  'QF_AUFNIA',
  'QF_AX',
  'QF_BV',
  'QF_BVFP',
  'QF_BVFPLRA',
  'QF_DT',
  'QF_FP',
  'QF_FPLRA',
  'QF_IDL',
  'QF_LIA',
  'QF_LIRA',
  'QF_LRA',
  'QF_NIA',
  'QF_NIRA',
  'QF_NRA',
  'QF_RDL',
  'QF_S',
  'QF_SLIA',
  'QF_SNIA',
  'QF_UF',
  'QF_UFBV',
  'QF_UFBVLIA',
  'QF_UFDT',
  'QF_UFDTLIRA',
  'QF_UFFP',
  'QF_UFFPDTLIRA',
  'QF_UFIDL',
  'QF_UFLIA',
  'QF_UFLRA',
  'QF_UFNIA',
  'QF_UFNRA',
  'UF',
  'UFBV',
  'UFBVFP',
  'UFBVLIA',
  'UFDT',
  'UFDTLIA',
  'UFDTLIRA',
  'UFDTNIA',
  'UFDTNIRA',
  'UFFPDTLIRA',
  'UFFPDTNIRA',
  'UFIDL',
  'UFLIA',
  'UFLRA',
  'UFNIA',
  'UFNRA'
])
#==============================================================================

def main():

    args = parse_args()

    # if (args.filter_csv and args.unsat) or \
    #       (args.filter_csv and args.sat) or \
    #       (args.unsat and args.sat):
    #     print("Provided filter_csvs (%s), unsat (%s), sat (%s). "\
    #             "Provide only one of these" % \
    #                     (", ".join(args.filter_csv),
    #                     args.unsat,
    #                     args.sat))
    #     sys.exit(1)

    if args.filter_csv is None:
        filter_csv = []
    else:
        filter_csv = args.filter_csv

    data_list = []
    for path in filter_csv:
        data_list.append(read_data_results(path)[args.filter_logic])


    # data_list = data_list[0][args.filter_logic]
    n_all_instances = 0
    n_custom_instances = 0
    for data in data_list:
        for benchmark_family in data:
            for benchmark in data[benchmark_family]:

                n_all_instances += 1
                counter = 0
                temp = []
                if args.solver is None:
                    for solver in data[benchmark_family][benchmark]:
                        # if solver[0] == args.solver:
                            # 'cubeandconquer_default' or solver[0]  == 'portfolio_default':
                        if solver[1] == args.result:

                            temp.append(solver)
                            counter += 1
                        else:
                            break
                    if len(data[benchmark_family][benchmark]) == counter:
                        n_custom_instances += 1
                        print(benchmark)
                else:
                    for solver in data[benchmark_family][benchmark]:
                        if solver[0] == args.solver and solver[1] == args.result:
                            n_custom_instances += 1
                            print(benchmark)
    print('Total number of instances: '+ str(n_all_instances))
    ptr = str(n_custom_instances)
    if args.solver is not None:
        ptr +=' - for solver '+ args.solver
    print('Total ' + args.result+' number of instances: '+ ptr)



    # # Print selected benchmarks
    # if args.out:
    #     with open(args.out, 'w') as outfile:
    #         benchmarks = ['{}{}'.format(args.prefix, b) \
    #                       for b in selected_benchmarks]
    #         outfile.write('\n'.join(benchmarks))

def parse_args():
    ap = argparse.ArgumentParser()
    ap.add_argument('-l', '--logics', dest='filter_logic',
                    help="Filter out benchmarks based on logic " \
                    "logics (semicolon separated) (default: all logics)")
    ap.add_argument('-f', '--filter', dest='filter_csv', action='append',
                    help="Filter out benchmarks based on csv")

    ap.add_argument('-r', '--result', dest='result', help="Filter out benchmarks based on result")
    ap.add_argument('-s', '--solver', dest='solver', help="Filter out benchmarks based on solver")
    return ap.parse_args()

def read_data_results(file_name):
    # Map logics to a dict mapping solvers to
    # {(benchmark, family):(status, expected_status, time)}
    data = {}

    with open(file_name, 'r') as file:
        reader = csv.reader(file)
        header = next(reader)

        for row in reader:
            drow = dict(zip(iter(header), iter(row)))

            # Read data
            benchmark = drow[COL_BENCHMARK].strip()
            benchmark = get_benchmark_name(benchmark)
            (logic, family, benchmark) = split_benchmark_to_logic_family(benchmark)

            # Results for each benchmark is stored as list of tuples.
            # data[logic][family][benchmark] = [...]
            # the list [...] contains the tuples for all solvers.
            if logic not in data:
                data[logic] = {}
            if family not in data[logic]:
                data[logic][family] = {}
            if benchmark not in data[logic][family]:
                data[logic][family][benchmark] = []

            results = data[logic][family][benchmark]

            solver = drow[COL_SOLVER].strip()
            config = drow[COL_CONFIG].strip()
            cpu_time = float(drow[COL_CPU].strip())
            status = drow[COL_RESULT].strip()
            expected = drow[COL_EXPECTED].strip() \
                if COL_EXPECTED in drow else 'starexec-unknown'
            solver_name = '{}_{}'.format(solver, config)
            results.append((solver_name, status, cpu_time, expected))

    return data
def split_benchmark_to_logic_family(benchmark):
    benchmark_split = benchmark.split("/")
    logic = benchmark_split[0]
    family = '/'.join(benchmark_split[:-1])
    benchmark = '/'.join(benchmark_split)
    return (logic, family, benchmark)

def get_benchmark_name(raw_str):
    raw_split = raw_str.strip().split('/')

    while (len(raw_split) > 0):
        if raw_split[0].upper() not in LOGICS:
            raw_split.pop(0)
        else:
            break
    if (len(raw_split) == 0):
        print("Logic not found: %s" % raw_str)
    raw_split[0] = raw_split[0].upper()
    return "/".join(raw_split)

if __name__ == '__main__':
    main()
