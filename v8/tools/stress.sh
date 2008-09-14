#!/bin/sh

if [ $# -lt 1 ]; then
  echo "Usage: `basename $0` [RUNTIME] [OPTIONS]*"
  echo "Stress tests RUNTIME; show results on standard output."
  exit 1
fi

RUNTIME="$* --enable-slow-asserts --debug-code --verify-heap"
DIRNAME=`dirname $0`
DIRPATH="`cd $DIRNAME; pwd`/../samples/"

while [ $? -eq 0 ]; do
  SEED=$RANDOM
  SECONDS=300
  # Generate a random number for the GC interval setting,
  # but let it be slightly biased towards low values.
  let GC="$RANDOM % ((100 ** ($RANDOM % 4)) * 50)"
  echo "Running for $SECONDS seconds with seed $SEED and GCs every $GC allocations"
  $RUNTIME \
    --gc-interval=$GC \
    -e "const SECONDS=$SECONDS,SEED=$SEED,PREFIX='$DIRPATH'" \
    ${DIRPATH}stress.js >/dev/null
done
