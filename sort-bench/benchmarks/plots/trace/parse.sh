#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"

for f in *.out
do
  ## get filename without extension
  inFile="$(basename "$f" | sed 's/\.[a-z0-9]*$/\.in/')"
  ## grep only traces and pipe it to another file
  grep '^-- \[TRACE' "$f" | sed 's/^\(--\s\[TRACE\]\s*\)\(.*\)/\2/' > "$inFile"

  # determine if therer is a header or not
  hasHeader="n"
  if [[ -n "$(grep '^.*context,\s*unit.*state$' $inFile)" ]]
  then
    hasHeader="y"
  fi
  ## generate a csv suitable for generating flame graphs
  plotCsv="$(Rscript "$SCRIPT_DIR/polish.r" "$inFile" "$hasHeader" 2>/dev/null)"
  ## Use awk to trim leading and trailing whitespace
  plotCsv="$(echo "${plotCsv}" | awk '{gsub(/^ +| +$/,"")} {print $0}')"
  # replace .csv with .svg
  plotSvg="$(echo "$plotCsv" | sed 's/\.[a-z0-9]*$/\.svg/')"
  ## generate the flame graph
  perl "$SCRIPT_DIR/trace.pl" "$plotCsv" > "${plotSvg}"
  ## output all to html
  finalHtml="$(basename "$f" | sed 's/\.[a-z0-9]*$/\.html/')"
  cat "$plotSvg" > "$finalHtml"
  summaryHtml="$(echo "$plotSvg" | sed 's/-plot\.svg$/-summary\.html/')"
  cat "$summaryHtml" >> "$finalHtml"
  ## success message
  [[ "$?" == "0" ]] && echo "plot generated for: $f"
done;

rm -f *.in
rm -f *-plot.csv
rm -f *-summary.html

