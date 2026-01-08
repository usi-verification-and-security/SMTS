#!/bin/bash

ROOT_DIR=$(realpath $(dirname "$0"))

DATA_DIR="$ROOT_DIR/data"
PLOTS_DIR="$ROOT_DIR/plots"

SMTS_ROOT_DIR="$ROOT_DIR/SMTS"
SMTS_EXPERIMENT_DIR="$SMTS_ROOT_DIR/experiments"
SMTS_SCRIPTS_DIR="$SMTS_EXPERIMENT_DIR/scripts"
SMTS_TACAS_SCRIPTS_DIR="$SMTS_SCRIPTS_DIR/tacas26"

OPENSMT_ROOT_DIR="$ROOT_DIR/opensmt"

FINAL_DATA_DIR="$SMTS_EXPERIMENT_DIR/data/tacas26"
FINAL_OUTPUT_DIR="$FINAL_DATA_DIR/outputs"
FINAL_PLOT_DIR="$FINAL_DATA_DIR/plots"

ACTIONS=(run make-plots)
MODES=(quick short full)
LOGICS=(
    QF_LRA
    QF_LIA
)

function usage {
    printf "USAGE: %s <action> <mode> [<logic>]\n" $(basename "$0")
    printf "ACTIONS: %s\n" "${ACTIONS[*]}"
    printf "MODES: %s\n" "${MODES[*]}"
    printf "LOGICS: %s\n" "${LOGICS[*]}"

    [[ -n $1 ]] && exit $1
}

CMDLINE_ARGS="$*"

set -e

[[ -z $1 ]] && usage 0
ACTION="$1"
shift

[[ -z $1 ]] && usage 0
MODE="$1"
shift

[[ -n $1 ]] && LOGIC="$1"

found=0
for a in ${ACTIONS[@]}; do
    [[ $a == $ACTION ]] && found=1
done
(( $found )) || {
    printf "Unrecognized action: %s\n" "$ACTION" >&2
    usage 1 >&2
}

found=0
for a in ${MODES[@]}; do
    [[ $a == $MODE ]] && found=1
done
(( $found )) || {
    printf "Unrecognized mode: %s\n" "$MODE" >&2
    usage 1 >&2
}

[[ -n $LOGIC ]] && {
    found=0
    for a in ${LOGICS[@]}; do
        [[ $a == $LOGIC ]] && found=1
    done
    (( $found )) || {
        printf "Unrecognized logic: %s\n" "$LOGIC" >&2
        usage 1 >&2
    }
}

VERSION=final-alg
export nts=''
os='1 2 4 8'


FILES=()
for l in ${LOGICS[@]}; do
    [[ -n $LOGIC && $l != $LOGIC ]] && continue
    FILES+=("$DATA_DIR/${l}_files")
done

unset FILES_SUFFIX
case $MODE in
    quick)
        BOUNDS=(15 40)
        TIMEOUT=300

        FILES_SUFFIX=_uniq
        ;;
    short)
        BOUNDS=(100 250)
        TIMEOUT=600

        FILES_SUFFIX=_gt1s
        ;;
    full)
        TIMEOUT=1200
        ;;
esac

export TIMEOUT

for i in ${!FILES[@]}; do
    FILES[$i]+=$FILES_SUFFIX
done

unset MODE_SUFFIX
(( ${#BOUNDS[@]} )) && {
    MODE_SUFFIX="_${MODE}"
    for i in ${!FILES[@]}; do
        f="${FILES[$i]}"
        new_f="${f}${MODE_SUFFIX}"
        bnd=${BOUNDS[$i]}
        [[ -e $new_f ]] || {
            printf 'Generating a random subset of %d files from %s ...\n' $bnd "$f"
            shuf "$f" | head -n $bnd >"$new_f"
        }
        FILES[$i]="$new_f"
    done
}

case $ACTION in
    run)
        for f in "${FILES[@]}"; do
            [[ -r $f ]] || {
                printf 'File "%s" is not readable.\n' "$f" >&2
                exit 2
            }
            printf 'Running on %s benchmarks ...\n' $(basename "${f%_files*}")

            printf 'Running OpenSMT on the background ...\n'
            bash "$OPENSMT_ROOT_DIR/runme.sh" "$f" &

            for o in $os; do
                if (( $o < 8 )); then
                    ps=1
                    ls=1
                else
                    ps='1 0'
                    ls='1 0'
                fi
                for p in $ps; do
                    for l in $ls; do
                        [[ $p == 0 && $l == 0 ]] && continue
                        ## Unfortunately, cannot run multiple instances of SMTS on a single machine
                        printf 'Running SMTS with %d solvers, partitioning=%d, lemma sharing=%d ...\n' $o $p $l
                        PARTITIONING=$p LEMMA_SHARING=$l os=$o bash "$SMTS_TACAS_SCRIPTS_DIR/runme.sh" $VERSION "$f"
                    done
                done
            done

            wait
        done
        ;;

    make-plots)
        printf 'Preparing the data ...\n'
        rm -fr "$FINAL_DATA_DIR"
        mkdir -p "$FINAL_OUTPUT_DIR"
        mkdir -p "$FINAL_PLOT_DIR"
        cp -r "$DATA_DIR/$VERSION" "$FINAL_OUTPUT_DIR"
        cp -r "$DATA_DIR/opensmt" "$FINAL_OUTPUT_DIR"

        printf 'Generating plots ...\n'
        "$SMTS_TACAS_SCRIPTS_DIR/plot_all.sh" $LOGIC "${FILES_SUFFIX}${MODE_SUFFIX}"

        cp -v "$FINAL_PLOT_DIR/scatter_QF_LRA_opensmt__final-alg-p-l_o-8.pdf" "$PLOTS_DIR/fig_1a_QF_LRA.pdf"
        cp -v "$FINAL_PLOT_DIR/scatter_QF_LIA_opensmt__final-alg-p-l_o-8.pdf" "$PLOTS_DIR/fig_1b_QF_LIA.pdf"
        cp -v "$FINAL_PLOT_DIR/cactus_QF_LRA_opensmt__final-alg-p_o-1__-p-l_o-2__-p-l_o-4__-p-l_o-8.pdf" "$PLOTS_DIR/fig_2a_QF_LRA.pdf"
        cp -v "$FINAL_PLOT_DIR/cactus_QF_LIA_opensmt__final-alg-p_o-1__-p-l_o-2__-p-l_o-4__-p-l_o-8.pdf" "$PLOTS_DIR/fig_2b_QF_LIA.pdf"
        cp -v "$FINAL_PLOT_DIR/scatter_QF_LRA_opensmt__final-alg-p_o-1.pdf" "$PLOTS_DIR/fig_3a_QF_LRA.pdf"
        cp -v "$FINAL_PLOT_DIR/scatter_QF_LIA_opensmt__final-alg-p_o-1.pdf" "$PLOTS_DIR/fig_3b_QF_LIA.pdf"
        cp -v "$FINAL_PLOT_DIR/scatter_QF_LRA_final-alg-l_o-8__-p-l_o-8.pdf" "$PLOTS_DIR/fig_4a_QF_LRA.pdf"
        cp -v "$FINAL_PLOT_DIR/scatter_QF_LIA_final-alg-l_o-8__-p-l_o-8.pdf" "$PLOTS_DIR/fig_4b_QF_LIA.pdf"
        cp -v "$FINAL_PLOT_DIR/scatter_QF_LRA_final-alg-p_o-8__-p-l_o-8.pdf" "$PLOTS_DIR/fig_5a_QF_LRA.pdf"
        cp -v "$FINAL_PLOT_DIR/scatter_QF_LIA_final-alg-p_o-8__-p-l_o-8.pdf" "$PLOTS_DIR/fig_5b_QF_LIA.pdf"

        printf '\nThe resulting plots are placed at %s\n' "$PLOTS_DIR"
        ;;
esac

printf "\nArtifact script '%s' completed successfully.\n" "$0 $CMDLINE_ARGS"
