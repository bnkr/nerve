#!/bin/sh

if [ $# -eq 0 ]; then
  glob="pipeline/*.[ch]pp"
else
  glob="$*"
fi

for f in ${glob}; do
  g++ -o /dev/null -Wall -Wextra "$f" || exit $?
done
