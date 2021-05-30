import random
import logging

port = 3000  # listen port
db_path = None  # sqlite3 event db path absolute or relative to the config file
table_prefix = ''  # db table prefix
portfolio_max = 0  # 0 if no limit
portfolio_min = 0  # 0 if no limit
partition_timeout = 60  # None if no partitioning
partition_policy = [1, 2]  #
solving_timeout = 1200  # None for no timeout
max_memory = 0  # max memory for each solver process in MB
build_path = "../../build"  # build path absolute or relative to the config file
lemma_sharing = False  # enabling lemma sharing
lemma_amount = None  # None for auto
lemma_db_path = None  # sqlite3 lemmas db path absolute or relative to the config file
lemma_resend = False  # send same lemmas multiple times to solver
log_level = logging.INFO  # logging.DEBUG
incremental = 0  # 0: always restart. 1: only push. 2: always incremental
fixedpoint_partition = False  # automatic partition for fixedpoint instances
files_path = []  # list of files path absolute or relative to the config file, to be loaded at server startup
gui = False  # enable GUI
opensmt = 0  # number of opensmt2 processes
z3spacer = 0  # number of z3spacer processes
sally = 0  # number of sally processes
idle_quit = True  # quit smts after solving the last instance
enableLog = True
# parameters is a dictionary solver_name.solver_parameter -> value:(int, str, callable)  where:
# solver_parameter is a valid parameter for the solver solver_name and
# value is either the parameter value or
# a callable where value() -> (int, str) is the parameter value
# the callable is called every time the solver is asked to solve something
#
# a single key can be overridden without copying the entire object
parameters = {
    "OpenSMT2.seed": lambda: random.randint(0, 0xFFFFFF),
    "OpenSMT2.split-type": "scattering",
    "Spacer.fp.spacer.random_seed": lambda: random.randint(0, 0xFFFFFF),
    "Spacer.fp.spacer.restarts": "false",
    "Spacer.fp.spacer.p3.share_lemmas": "true",
    "Spacer.fp.spacer.p3.share_invariants": "true",
    "Spacer.fp.xform.slice": "false",
    "Spacer.fp.xform.inline_linear": "false",
    "Spacer.fp.xform.inline_eager": "false",
    "Spacer.fp.xform.tail_simplifier_pve": "false",
    "Spacer.fp.spacer.elim_aux": "false",
    "Spacer.fp.spacer.reach_dnf": "false",

    "SALLY.opensmt2-random_seed": lambda: random.randint(0, 0xFFFFFF),
    "SALLY.pdkind-minimize-frames": "true",
    "SALLY.pdkind-minimize-interpolants": "true",
    "SALLY.opensmt2-simplify_itp": "2",
    "SALLY.opensmt2-itp-lra": "0",
    "SALLY.opensmt2-itp-bool": "0",
    "SALLY.pdkind-induction-max": "0",
    "SALLY.share-reachability-lemmas" : "true",
    "SALLY.share-induction-lemmas": "true"
}
