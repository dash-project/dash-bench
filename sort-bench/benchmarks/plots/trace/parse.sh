#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"

TITLE="$1"

for f in *.out
do
  ## get filename without extension
  name="$(basename "$f" | sed 's/\.[a-z0-9]*$//')"
  inFile="${name}.in"
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
  nlines="$(wc -l $plotSvg | awk '{print $1}')"
  ## output all to html
  finalHtml="${name}.html"

  cat "$SCRIPT_DIR/skeleton.html" > "$finalHtml"

  if [[ -n "$TITLE" ]]
  then
    sed -i "s/TITLE_PLACEHOLDER/${TITLE}/g"
  fi

  sed -i -e "12r $plotSvg" "$finalHtml"

  summaryHtml="$(echo "$plotSvg" | sed 's/-plot\.svg$/-summary\.html/')"
  nl="$((14+$nlines+1))"
  sed -i -e "${nl}r $summaryHtml" "$finalHtml"
  ## success message
  [[ "$?" == "0" ]] && echo "plot generated for: $f"
done;

rm -f *.in
rm -f *-plot.csv
rm -f *-summary.html

