import random
import logging

port = 3000                     # Default listening port
db_path = None                  # sqlite3 event db path absolute or relative to the config file
table_prefix = ''               # db table prefix
##+ portfolio_max not implemented
portfolio_max = 0               # 0 if no limit, >= portfolio_min : max. no. solvers per node
portfolio_min = 1               # >= 1 : min. no. solvers per node
partitioning = False            # Enable partitioning
node_timeout = 24               # timeout per each solver
dynamic_node_timeout = True     # increase the timeout over time
dynamic_node_timeout_factor = 2 # factor by which the dynamic node timeout is increased
dynamic_node_timeout_threshold_factor = 4 # dynamic node timeout is increased when the overall time exceeds this factor threshold
partition_policy = [1, 2]       # [number of solvers, number of partition per solver]
max_tree_size_factor = 2        # >= 1 : max. relative size of the tree compared to solvers (excluding solved nodes)
partitioning_boost_factor = 0   # in [-1,1] : increases/decreases the probability of partitioning (up to 0 and 1, resp.)
balance_partitioning = True     # balances the order of selecting nodes for partitioning within the same depth
shuffle_partitioning = False    # randomizes the order of selecting nodes for partitioning within the same depth
balance_morpeach = False        # balances the order of selecting nodes for placement within the same depth
shuffle_morpeach = False        # randomizes the order of selecting nodes for placement within the same depth
move_to_new_partition_timeout_factor = 0.1 # solver with new partitions is instantly moved there only if its runtime is lower than factor of timeout
redundant_only_if_tree_changed = True  # trigger re-placement of redundant solvers only when solved nodes or received new nodes; or after any event
nonredundant_max = 2            # 0 if no limit, portfolio_min <= x <= portfolio_max
solving_timeout = 2000          # None for no timeout
max_memory = 10000               # max memory for each solver process in MB
build_path = "../../build"      # build path absolute or relative to the config file
lemma_sharing = False           # enabling lemma sharing
lemma_amount = 1000             # None for auto
lemma_db_path = None            # sqlite3 lemmas db path absolute or relative to the config file
lemma_resend = False            # send same lemmas multiple times to solver
log_level = logging.INFO        # logging.DEBUG
incremental = 2                 # 0: always restart. 1: only push. 2: always incremental
fixedpoint_partition = False    # automatic partition for fixedpoint instances
files_path = []                 # list of files path absolute or relative to the config file, to be loaded at server startup
gui = False                     # enable GUI
opensmt = 0                     # number of opensmt2 processes
z3spacer = 0                    # number of z3spacer processes
sally = 0                       # number of sally processes
idle_quit = True                # quit smts after solving the last instance
printFilename = False           # print filename in front of the result
printRuntime = False            # print elapsed runtime
enableLog = False               # enable logging system-widely
visualize_tree = False          # draw a partition tree when timout reached based on incoming event from solvers
debug = False
lemmaPush_timeoutMin = 15000    # a timespan to distribute solvers "push operation" to the lemma server
lemmaPush_timeoutMax = 30000
lemmaPull_timeoutMin = 20000    # a timespan to distribute solvers "pull operation" to the lemma server
lemmaPull_timeoutMax = 40000
clientLogColorMode = False      # to enable color at logging
##! should not be in config, here just to make it "global"
partition_count = 1             # total number of remaining partitions
n_timeouts_to_count_partition = 4  # no. timeouts after which the node is not anymore counted into the size of the tree
spit_preference = False

# parameters is a dictionary solver_name.solver_parameter -> value:(int, str, callable)  where:
# solver_parameter is a valid parameter for the solver solver_name and
# value is either the parameter value or
# a callable where value() -> (int, str) is the parameter value
# the callable is called every time the solver is asked to solve something
#
# a single key can be overridden without copying the entire object
parameters = {
    "OpenSMT2.seed": lambda: random.randint(0, 0xFFFFFF),
    "OpenSMT2.split-type": ":scatter-split",
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
