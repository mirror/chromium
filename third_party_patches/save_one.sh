#!/bin/bash

if test "$GCLIENT_SCM" != git ; then
  exit 0
fi

if echo "$GCLIENT_URL" | grep -vq @ ; then
  exit 0
fi

rev=`echo "$GCLIENT_URL" | cut -d@ -f2`
file="$loc/$(echo "$GCLIENT_DEP_PATH" | sed -e 's,/,.,g')".diff

git diff $rev > $file
if test \! -s $file; then
  rm $file
fi
