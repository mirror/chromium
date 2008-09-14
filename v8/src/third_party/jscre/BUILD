# Copyright 2008 Google Inc.  All rights reserved.
#
# Description:
#   JSCRE, a library for JavaScript-compatible regular expresions.

licenses(["notice"])  # BSD

exports_files(["LICENSE" ])

# JSCRE - JS-specific subset/version of PCRE.
cc_library(name = "jscre_lib",
           srcs = [ "pcre_compile.cpp",
                    "pcre_exec.cpp",
                    "pcre_tables.cpp",
                    "pcre_ucp_searchfuncs.cpp",
                    "pcre_xclass.cpp" ],
           includes = [ "..", "." ],
           nocopts = "-W.*" )
