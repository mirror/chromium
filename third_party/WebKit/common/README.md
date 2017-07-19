# Blink 'common' directory

This directory contains the common Web Platform stuff that needs to be shared by renderer-side browser-side code.

Things that live in third_party/WebKit/Source and browser-side code (e.g. content/browser for the time being) can both depend on this directory, while anything in this directory should NOT depend on them.
