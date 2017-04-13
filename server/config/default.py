import random
import logging

port = 3000  # listen port
db_path = None  # sqlite3 db path
table_prefix = ''  # db table prefix
portfolio_max = 0  # 0 if no limit
portfolio_min = 0  # 0 if no limit
partition_timeout = None  # None if no partitioning
partition_policy = [2, 2]  #
solving_timeout = None  # None for no timeout
lemma_amount = None  # None for auto
log_level = logging.INFO  # logging.DEBUG
incremental = 2  # 0: always restart. 1: only push. 2: always incremental
z3_path = None  # if None then z3.py assumed in python path
fixedpoint_partition = False  # automatic partition for fixedpoint instances
files = []  # list of files loaded at server startup

# parameters is a dictionary solver_name.solver_parameter -> value:(int, str, callable)  where:
# solver_parameter is a valid parameter for the solver solver_name and
# value is either the parameter value or
# a callable where value() -> (int, str) is the parameter value
# the callable is called every time the solver is asked to solve something
#
# a single key can be overridden without copying the entire object
parameters = {
    'OpenSMT2.seed': lambda: random.randint(0, 0xFFFFFF),
    'OpenSMT2.split': 'lookahead',

    'Spacer.fixedpoint.spacer.random_seed': lambda: random.randint(0, 0xFFFFFF),
    'Spacer.fixedpoint.spacer.restarts': 'false',
    'Spacer.fixedpoint.spacer.share_lemmas': 'true',
    'Spacer.fixedpoint.spacer.share_invariants': 'true',
    'Spacer.fixedpoint.spacer.share_lemmas.level': 0,
    'Spacer.fixedpoint.xform.slice': 'false',
    'Spacer.fixedpoint.xform.inline_linear': 'false',
    'Spacer.fixedpoint.xform.inline_eager': 'false',
    'Spacer.fixedpoint.xform.tail_simplifier_pve': 'false',
    'Spacer.fixedpoint.use_heavy_mev': 'true',
    'Spacer.fixedpoint.spacer.elim_aux': 'false',
    'Spacer.fixedpoint.spacer.reach_dnf': 'false'
}
