import pathlib
import random
import server

port = 3000
portfolio_max = 0
portfolio_min = 0
partition_timeout = 10
partition_policy = [1, 8]
solving_timeout = 1000


def entrust(header: dict, solver: server.Solver, solvers: set):
    if solver.name == "Spacer":
        header["parameter.fixedpoint.xform.slice"] = "false"
        header["parameter.fixedpoint.xform.inline_linear"] = "false"
        header["parameter.fixedpoint.xform.inline_eager"] = "false"
        header["parameter.fixedpoint.xform.tail_simplifier_pve"] = "false"
        header["parameter.fixedpoint.use_heavy_mev"] = "true"
        header["parameter.fixedpoint.spacer.elim_aux"] = "false"
        header["parameter.fixedpoint.spacer.reach_dnf"] = "false"
        if len(solvers) % 3 == 0:
            # -p def
            header["-p"] = "def"
            header["parameter.fixedpoint.pdr.flexible_trace"] = "true"
            header["parameter.fixedpoint.reset_obligation_queue"] = "false"
        if len(solvers) % 3 == 1:
            # -p ic3
            header["-p"] = "ic3"
            header["parameter.fixedpoint.pdr.flexible_trace"] = "true"
        if len(solvers) % 3 == 2:
            # -p gpdr
            header["-p"] = "gpdr"


_benchmarks_path = pathlib.Path('/Users/matteo/dev/spacer/regressions/')
files = [str(i.resolve()) for i in _benchmarks_path.glob('*.smt2')]
files.remove('/Users/matteo/dev/spacer/regressions/false.smt2')

# files = ["/Users/matteo/dev/spacer/regressions/ex01-dmpl-inline-unsat.smt2"]

files = ["/Users/matteo/dev/opensmt2/test/std_benchmarks/NEQ_NEQ046_size6.smt2"]

# files=[str(pathlib.Path('../../opensmt2/test/std_benchmarks/NEQ_NEQ015_size6.smt2').resolve())]+\
#       [str(pathlib.Path('../../opensmt2/test/std_benchmarks/PEQ_PEQ013_size8.smt2').resolve())]


# files = ['prova']
# files=[str(pathlib.Path('../../opensmt2/test/std_benchmarks/PEQ_PEQ013_size8.smt2').resolve())]

# files = """/Users/matteo/dev/opensmt2/test/std_benchmarks/NEQ_NEQ015_size6.smt2
# /Users/matteo/dev/opensmt2/test/std_benchmarks/NEQ_NEQ032_size3.smt2
# /Users/matteo/dev/opensmt2/test/std_benchmarks/QG-classification_loops6_gensys_icl093.smt2
# /Users/matteo/dev/opensmt2/test/std_benchmarks/QG-classification_qg5_dead_dnd046.smt2
# /Users/matteo/dev/opensmt2/test/std_benchmarks/QG-classification_qg5_gensys_icl1069.smt2""".split('\n')
