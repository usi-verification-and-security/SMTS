shopt -s extglob

files_file="$1"
shift

files_base=$(basename "$files_file")

conf_base=_nt-32_o-

fname_base=$(ls *_files* | head -n 1)
fname_base=${fname_base/_files*/_files}

[[ -n $CP ]] && fcmd=cp
[[ -n $MV ]] && fcmd=mv

for conf in {-p,}{-l,}${conf_base}8 -p-l${conf_base}{2,4} -p${conf_base}1; do
    ofile=${fname_base}_cat$conf
    cat $(ls -v *files+(_+([0-9])-+([0-9]))$conf) |  awk '{ if (data[$1]) { } else { data[$1]=1 ; print $0 ; } }' >$ofile
    [[ -n $files_file ]] && {
        outs=($(comm -123 --total <(sort "$files_file") <(awk '{ print $1 }' <$ofile | sort)))
        (( ${outs[1]} > 0 )) && {
            printf '%s is not subset of %s\n' $ofile "$files_file"
            exit 1
        }
        [[ -z $PARTIAL_CHECK ]] && (( ${outs[0]} > 0 )) && {
            printf '%s is not equal to %s\n' $ofile "$files_file"
            exit 1
        }
    }
    [[ -n $fcmd ]] && $fcmd ${fname_base}_cat$conf ../${fname_base}$conf
done
