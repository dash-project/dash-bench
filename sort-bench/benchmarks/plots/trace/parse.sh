#!/bin/bash

DIR=$(dirname "$(readlink -f "$0")")

for f in "$DIR"/*.out
do
  echo "file: $f"
  ## get filename without extension
  inFile="$(basename "$f" | sed 's/\.[a-z0-9]*$/\.in/')"
  ## grep only traces and pipe it to another file
  grep '^-- \[TRACE' "$f" | sed 's/^\(--\s\[TRACE\]\s*\)\(.*\)/\2/' > "$DIR/$inFile"

  # determine if therer is a header or not
  hasHeader="n"
  if [[ -n "$(grep '^.*context,\s*unit.*state$' $DIR/$inFile)" ]]
  then
    hasHeader="y"
  fi
  ## generate a csv suitable for generating flame graphs
  plotCsv="$(Rscript polish.r "$inFile" "$hasHeader" 2>/dev/null)"
  ## Use awk to trim leading and trailing whitespace
  plotCsv="$(echo "${plotCsv}" | awk '{gsub(/^ +| +$/,"")} {print $0}')"
  ## generate the flame graph
  perl $DIR/trace.pl "$plotCsv" > "${plotCsv}.svg"
  ## success message
  [[ "$?" == "0" ]] && echo "plot generated for: $f"
done;

#rm -f *.in
#rm -f *-plot.csv

