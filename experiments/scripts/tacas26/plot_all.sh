#!/bin/bash

TACAS_SCRIPTS_DIR=$(dirname $(realpath "$0"))
SCRIPTS_DIR=$(realpath "$TACAS_SCRIPTS_DIR/../")
EXPERIMENT_DIR=$(realpath "$SCRIPTS_DIR/../")
ROOT_DIR=$(realpath "$EXPERIMENT_DIR/../")

TACAS=$(basename "$TACAS_SCRIPTS_DIR")

DATA_DIR=$(realpath "$EXPERIMENT_DIR/data/")
TACAS_DATA_DIR=$(realpath "$DATA_DIR/$TACAS/")
TACAS_OUTPUT_DIR=$(realpath "$TACAS_DATA_DIR/outputs/")
TACAS_PLOT_DIR=$(realpath "$TACAS_DATA_DIR/plots/")

shopt -s extglob

function plot {
    local type=$1
    shift

    local files=("$@")
    local file1="${files[0]}"

    local logic=$(basename $(dirname "$file1"))

    local versions=()
    local params=()
    for f in "${files[@]}"; do
        local version=$(basename $(dirname $(dirname "$f")))
        versions+=("$version")

        local param=$(basename "$f")
        param="${param#${version}_${logic}*_files}"
        param="${param#*_selected}"
        param="${param##+(_+([0-9]))}"
        param="${param##+(-+([0-9]))}"
        param="${param/_nt-*_/_}"
        params+=("$param")
    done
    local version1="${versions[0]}"

    cd "$SCRIPTS_DIR"
    ./${type}_plot.sh "${files[@]}" &>o
    local ret=$?
    local out=$(<o)
    rm o
    (( $ret )) && {
        printf 'ERROR:\n%s\n' "$out" >&2
        exit $ret
    }

    local out_file="$TACAS_PLOT_DIR/${type}_${logic}_${version1}${params[0]}"
    if [[ $type == scatter ]]; then
        out_file+="__"
        if [[ $version1 == ${versions[1]} ]]; then
            out_file+="${params[1]}"
        else
            out_file+="${versions[1]}${params[1]}"
        fi
    elif [[ $type == cactus ]]; then
        local k=1
        [[ $version1 != ${versions[1]} ]] && {
            out_file+="__${versions[1]}${params[1]}"
            (( ++k ))
        }
        for p in "${params[@]:$k}"; do
            out_file+="__$p"
        done
    fi
    out_file+=.pdf

    mv plot.pdf "$out_file"
}

plot scatter "$TACAS_OUTPUT_DIR/opensmt/QF_LRA/opensmt_QF_LRA_files" "$TACAS_OUTPUT_DIR/final-alg/QF_LRA/final-alg_QF_LRA_files-p-l_nt-32_o-8"
plot scatter "$TACAS_OUTPUT_DIR/opensmt/QF_LRA/opensmt_QF_LRA_files" "$TACAS_OUTPUT_DIR/final-alg/QF_LRA/final-alg_QF_LRA_files-p_nt-32_o-1"
plot scatter "$TACAS_OUTPUT_DIR/final-alg/QF_LRA/final-alg_QF_LRA_files-l_nt-32_o-8" "$TACAS_OUTPUT_DIR/final-alg/QF_LRA/final-alg_QF_LRA_files-p-l_nt-32_o-8"
plot scatter "$TACAS_OUTPUT_DIR/final-alg/QF_LRA/final-alg_QF_LRA_files-p_nt-32_o-8" "$TACAS_OUTPUT_DIR/final-alg/QF_LRA/final-alg_QF_LRA_files-p-l_nt-32_o-8"
plot scatter "$TACAS_OUTPUT_DIR/final-alg/QF_LRA/final-alg_QF_LRA_files_nt-32_o-8" "$TACAS_OUTPUT_DIR/final-alg/QF_LRA/final-alg_QF_LRA_files-l_nt-32_o-8"

# plot cactus "$TACAS_OUTPUT_DIR/final-alg/QF_LRA/final-alg_QF_LRA_files-p_nt-32_o-1" "$TACAS_OUTPUT_DIR/final-alg/QF_LRA/final-alg_QF_LRA_files-p-l_nt-32_o-"*
plot cactus "$TACAS_OUTPUT_DIR/opensmt/QF_LRA/opensmt_QF_LRA_files" "$TACAS_OUTPUT_DIR/final-alg/QF_LRA/final-alg_QF_LRA_files-p_nt-32_o-1" "$TACAS_OUTPUT_DIR/final-alg/QF_LRA/final-alg_QF_LRA_files-p-l_nt-32_o-"{2,4,8}
