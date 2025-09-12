[[ -z $1 ]] && {
  echo 'Specify datafile' >&2
  exit 1
}

sed -i '/^;error/d' $1
