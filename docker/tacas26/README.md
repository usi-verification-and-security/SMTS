# #1788 Parallel SMT Solving via Iterative Tree Partitioning - Artifact

This Docker-based artifact image reproduces data
presented in the paper #1788
``Parallel SMT Solving via Iterative Tree Partitioning''
accepted at TACAS 2026.

The software runs a parallel SMT solver SMTS and a sequential SMT solver OpenSMT
on SMT-LIB benchmarks from QF_LRA and QF_LIA logics.
The resulting data are scatter and cactus plots.

## Updates

The previous version is `v1` with DOI 10.5281/zenodo.18197587

This is version `v2` with DOI 10.5281/zenodo.18330747

The updates are:

* Fixed file paths in the list of benchmark files
* Added `smoke` mode that allows to run within 30 minutes
* Minor fixes in plotting scripts
* Extended the instructions on retrieving the plots to the host machine

## Contents

* `README.md`: this read-me file
* `LICENSE`: the MIT license
* `SMTS-tacas26.tar`: the Docker image

The Docker image contains:
* `runme.sh`: the run script (you should not need to use anything else)
* `plots`: the resulting plots will be placed there and should reproduce the plots presented in the paper
* `data`: contains the benchmarks and the solvers will produce their outputs there (it is not important for the reviewer)
* `SMTS`: repository of the parallel SMT solver SMTS
* `opensmt`: repository of the sequential SMT solver OpenSMT

## Instructions

### Docker image

In order to run the Docker image, take the following steps:

* Load the image with: `docker load -i smts-tacas26.tar`
* Run the image with: `docker run -it smts-tacas26:latest`
* Follow the instructions below (this `README.md` file is also present in the image)  
  Each step uses the main script `runme.sh` and should print a message about a successful run at the end.

### Run experiments

To reproduce the experiments, run `./runme.sh` and follow the help message.
First, use the `run` action with a selected mode (described below).
You may select a concrete logic (`QF_LRA` of `QF_LIA`), otherwise both are processed.

Complete experiments are extensive and contain all SMT-LIB benchmarks from QF_LIA (13224) and QF_LRA logics (1753).
The experiments require (if running both logics):
* `smoke`: ~ 24 minutes of wall time and 32 GB of memory,
* `quick`: ~ 7 hours of wall time and 64 GB of memory,
* `short`: ~ 48 hours of wall time and 96 GB of memory,
* `full`: 96 days of wall time and 128 GB of memory.

To adequately run all benchmarks, one would need to use an HPC cluster, which is not automated by this artifact (justified below).
Hence, we suggest to run the `short` mode.

The `smoke` mode uses 2-min timeout and randomly selects 3 benchmarks from QF_LRA and 3 from QF_LIA from a subset of benchmarks where one candidate per a group with similar runtimes was selected (`QF_L?A_files_uniq`: 576 and 144 benchmarks, resp.), and it only runs the experiments necessary for Figure 1.
The `quick` mode uses 5-min timeout and randomly selects 15 benchmarks from QF_LRA and 40 benchmarks from QF_LIA from the same subset of benchmarks as described for the `smoke` mode.
For a longer but still incomplete run, use the `short` mode that uses 10-min timeout and randomly selects 100 benchmarks from QF_LRA and 250 benchmarks from QF_LIA where benchmarks that were solved in less than a second were discarded (`QF_L?A_files_gt1s`: 4830 and 642 benchmarks, resp.).
All experiments, that use 20-min timeout, are reproduced with the `full` mode (`QF_L?A_files`).
Note that the tool is focused on more difficult benchmarks rather than on easy ones, so the results that used only 5-min timeout may be misleading.

If needed, exporting and setting variable `TIMEOUT` will override the above settings of the time limit.
For example, `export TIMEOUT=120` will force the variable to 2 minutes.

The script runs OpenSMT on the background and one instance of SMTS at a time, using 1, 2, 4, and 8 solvers in parallel. Running more instances of the parallel solver SMTS on a single machine is unfortunately not possible.

Note that the runs of the parallel solver are in principle not exactly reproducible,
because of time-dependent phenomena and OS scheduling of particular solvers that usually execute in different orders.

We used computational nodes equipped with two sockets of
Intel Xeon E5-2650 v3 (10 cores, 25M Cache, 2.3 GHz) and with 64GB memory,
running CentOS 8.2.2004.x86 64.

### Getting the data

After running the experiments finishes, run again `./runme.sh` script
with `make-plots` command for the respective mode (and possibly logic).
This will produce scatter and cactus plots into the `plots` directory.

You need to move the plots to the host machine to display the plots using your graphical environment. Assuming that this docker container is the last one you run, you can get the container ID with
```
CONTAINER_ID=$(docker container ls -lq)
```
To copy a file from the container, use `docker cp <id>:<path> <dst>`.
For example:
```
docker cp $CONTAINER_ID:/tacas26/plots/fig_1a_QF_LRA.pdf .
```
To copy all plots:
```
docker cp $CONTAINER_ID:/tacas26/plots .
```

The plots have transparent background.
For example, using `magick`, display plot `plots/fig_1a_QF_LRA.pdf` by
```
display -alpha off plots/fig_1a_QF_LRA.pdf
```

## Claimed badges

### Available

The package is published at Zenodo.

### Functional

* The package is well documented and automated and is easy to use.
* The experimental data presented in the plots are replicated if run on all benchmarks.
* Automating independent parallelization of all runs for the reviewers would require an anonymized external access to an HPC cluster.
  Our institution, unfortunately, forbids to access our HPC resources to external users.
