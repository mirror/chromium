Chromium Codesearch Plugin for Vim
==================================

Installation
------------

Add the `codesearch` directory to your runtime path.

```viml
set rtp += path-to-chromium/src/tools/vim/codesearch
```

Documentation
-------------

Once installed, you should be able to run `:help crcs.txt` to view the help
file. If that doesn't work, try running the following:

```viml
helptags path-to-chromium/src/tools/vim/codesearch/doc
```
... or just ...

```viml
helptags ALL
```

TODO/Wishlist
-------------

1. Exception handling. Since the plugin is very much in alpha, exceptions are
   exposed to vim. This makes it handy for debugging, but is a pain for
   end-users, if there are any.

1. Implement jump-to logic via an internal mapping instead of using lots of
   markup that messes up layout.

1. Show the commit that was last indexed by codesearch and warn if a jump target
   differs from the version of the file that was indexed.

1. Add a mode and hooks for code exploration and code auditing.


Bugs
----

Contact one of the `tools/vim/OWNERS`.
