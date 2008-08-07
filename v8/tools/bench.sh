#!/bin/sh

if [ $# -lt 1 ]; then
  echo "Usage: `basename $0` [RUNTIME]..."
  echo "Benchmark RUNTIME(s); show results on standard output."
  exit 1
fi

DIRNAME=`dirname $0`
DIRPATH=`cd $DIRNAME; pwd`

BCHPATH=$DIRPATH/../samples

if [ $# -eq 1 ]; then
  for rev in `dir -1 /auto/JavaScriptV8/Archive | sort -rn`; do
    if [ -f "/auto/JavaScriptV8/Archive/$rev/bin/v8" ]; then
      echo "Comparing $* against V8 revision $rev"
      ARGS="$* $rev"
      break
    fi
  done
else
  ARGS=$*
fi

COMPARERT=""
RUNTIMES=""
LINKS=""
for r in $ARGS; do
    REV="/auto/JavaScriptV8/Archive/$r/bin/v8"
    if [ -f "`dirname $r`/$r" ]; then
	RT="`dirname $r`/$r"
    elif [ -f "`which $r`" ]; then
	RT="`which $r`"
    elif [ -f $REV ]; then
        ln -fs $REV v8-$r
        RT="`dirname v8-$r`/v8-$r"
        LINKS="$LINKS $RT"
    else
	echo "Error: Cannot find runtime '$r'."
	exit 1
    fi
    if [ "x$COMPARERT" = "x" ]; then
      COMPARERT=`basename $RT`
    fi
    RUNTIMES="$RUNTIMES --runtime $RT"
done

BCHS=
BCHS="$BCHS benchpress.js" 
BCHS="$BCHS richards.js"
BCHS="$BCHS delta-blue.js" 
BCHS="$BCHS cryptobench.js"
BCHS="$BCHS ../benchmarks/sunspider.js"

BCHSARG=
for BCH in $BCHS; do
    BCHSARG="$BCHSARG --benchmark $BCHPATH/$BCH"
done

java -jar $DIRPATH/benchmark.jar \
  $RUNTIMES \
  --compare $COMPARERT \
  $BCHSARG

# Remove temporary symbolic links.
for link in $LINKS; do
  rm -f $link
done