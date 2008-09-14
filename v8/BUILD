# -*- mode: python; -*-
#
# Copyright 2007-2008 Google Inc.
# All Rights Reserved.
#
# Description:
#   The Google3 version of v8.

# Allmost all of V8 is authored by Google, but there are a few 3rd-party files
# and the JSCRE regular expression library. The files have their own BUILD rules
# (named *_3rd_party) with the necessary licenses clause. The JSCRE library also
# has its own licences clause.
licenses(["unencumbered"])

# ------------------------------------------------------------------------------
#
# Requires gconfig --cpu=piii (only builds in 32bit mode!)
#
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
#
#   W A R N I N G :   T H I S   I S   W O R K   I N   P R O G R E S S !
#
#   Do not use this package w/o talking first to the V8 team.
#   Contacts: bak@google.com (Lars Bak) - TL
#             gri@google.com (Robert Griesemer)
#
# ------------------------------------------------------------------------------


# ------------------------------------------------------------------------------
# Binaries

# The V8 stand-alone application implementing standard JS.
cc_binary(name = "v8",
          srcs = [ "app_cc_files" ],
          deps = [ "core_lib" ],
          copts = [ "-DGOOGLE3" ] )


# ARM build disabled for now: Some of the platform-independent
# files generate different code depending on the configuration
# flags. Since they are compiled only once, we get link errors
# for the ARM build (or ia32, depending on build order).
# TODO: find a solution for this problem or give up on the
# ARM build (which is really only here to verify that it can
# be built).
#
# The V8 stand-alone build with ARM code generation.
# This is a build only, and won't run.
#cc_binary(name = "v8a",
#          srcs = [ "app_cc_files" ],
#          deps = [ "core_lib_arm" ],
#          copts = [ "-D__arm__",
#                   "-DGOOGLE3",
#                   "-DARM_IA32_BUILD_ONLY" ] )


# The V8 stand-alone application implementing standard JS
# with debugging capabilities.
cc_binary(name = "v8d",
          srcs = [ "app_cc_files" ],
          deps = [ "core_lib_d" ],
          copts = [ "-DGOOGLE3" ] )


# The v8x build is disabled for now due to lack of engineering support
# and use.
#
# The V8 stand-alone application implementing standard JS
# with debugging capabilities and Google3-specific extensions.
#cc_binary(name = "v8x",
#          srcs = [ "app_cc_files" ],
#          deps = [ "core_lib_d",
#                   "ext_lib" ],
#          copts = [ "-DGOOGLE3" ] )


# ------------------------------------------------------------------------------
# Libraries
#
# NOTE: The public interface for these libraries is third_party/v8/public/v8.h.
#       No other v8 header files must be included, they are for internal use
#       only.
#
# TODO: Complete componentization (proper separate BUILD file in public
#       directory, explicit marking of external interfaces).

# The v8 core library. Link this library for standard JS.
cc_library(name = "core_lib",
           srcs = [ "core_cc_files",
                    "core_cc_files_3rd_party",
                    "core_cc_files_ia32",
                    "core_cc_files_ia32_3rd_party",
                    "core_natives_file" ],
           deps = [ "core_natives_file",
                    "//third_party/v8/src/pcre/jscre:jscre_lib" ],
           copts = [ "-DENABLE_DISASSEMBLER",
                     "-DGOOGLE3" ],
           alwayslink = 1 )


# The v8 core build with ARM code generation.
# This is a build only, and won't run.
#cc_library(name = "core_lib_arm",
#           srcs = [ "core_cc_files",
#                    "core_cc_files_3rd_party",
#                    "core_cc_files_arm",
#                    "core_natives_file" ],
#           deps = [ "core_natives_file",
#                    "//third_party/v8/src/pcre/jscre:jscre_lib" ],
#           copts = [ "-D__arm__",
#                     "-DGOOGLE3",
#                     "-DARM_IA32_BUILD_ONLY" ],
#           alwayslink = 1 )


# The v8 core library with debugging features enabled.
# Link this library for standard JS with debugging capability.
cc_library(name = "core_lib_d",
           srcs = [ "core_cc_files",
                    "core_cc_files_3rd_party",
                    "core_cc_files_ia32",
                    "core_cc_files_ia32_3rd_party",
                    "core_natives_file" ],
           deps = [ "core_natives_file",
                    "//third_party/v8/src/pcre/jscre:jscre_lib" ],
           copts = [ "-DENABLE_LOGGING_AND_PROFILING",
                     "-DENABLE_DISASSEMBLER",
                     "-DGOOGLE3" ],
           alwayslink = 1 )


# Google3-specific JS extensions. Link this library
# (in addition to core_lib) to enable the extensions.
#cc_library(name = "ext_lib",
#           srcs = [ "ext_cc_files",
#                    "ext_natives_file" ],
#           deps = [ "ext_natives_file",
#                    "//file/localfile",
#                    "//net/proto/globaldb",
#                    "//sstable",
#                    "//strings" ],
#           copts = [ "-DENABLE_DISASSEMBLER",
#                     "-DGOOGLE3" ],
#           alwayslink = 1 )


# ------------------------------------------------------------------------------
# Generated files

genrule(name = "core_natives_file",
        srcs = [ "core_js_files" ],
        outs = [ "natives.cc" ],
        cmd = "python g3tools/js2c.py Natives third_party/v8/src $@ $(SRCS)",
        deps = [ "g3tools/js2c.py" ] )

genrule(name = "ext_natives_file",
        srcs = [ "ext_js_files" ],
        outs = [ "ext_natives.cc" ],
        cmd = "python g3tools/js2c.py ExtNatives third_party/v8/src/extensions/extensions.h $@ $(SRCS)",
        deps = [ "g3tools/js2c.py" ] )


# ------------------------------------------------------------------------------
# Tests

# A smoke test for all v8 binaries.
sh_test(name = "smoketest",
        srcs = [ "g3tools/smoketest.sh" ],
        size = "small",
        data = [ "//third_party/v8/tests:.",
                 "//testing/shbase",
                 "g3tools/unittest.sh",
                 "v8",
                 # "v8a",  # not actually run, but we want it to be built anyway
                 "v8d",
                 # "v8x",  # disabled for now since it's not built anymore
               ] )


# Some special tests run with various binaries.
sh_test(name = "special_regtest",
        srcs = [ "g3tools/special_regtest.sh" ],
        size = "small",
        data = [ "//third_party/v8/tests:.",
                 "//testing/shbase",
                 "v8",
                 # "v8a",  # not actually run, but we want it to be built anyway
                 "v8d",
                 # "v8x",  # disabled for now since it's not built anymore
               ] )


# The following tests are regression tests: Each test consists of a set of .js
# files and corresponding "golden" .out files. The .js files are run and the
# stdout output is compared against the .out files.
#
# The files used for the regression test live in a subdirectory "tests"
# which is why we need the dependency on "third_party/v8/tests:.".

# Runs with v8.
sh_test(name = "core_regtest",
        srcs = [ "g3tools/core_regtest.sh" ],
        size = "large",
        data = [ "//third_party/v8/tests:.",
                 "//testing/shbase",
                 "g3tools/regress.sh",
                 "v8" ] )


# Runs with v8d.
sh_test(name = "mjsunit_regtest",
        srcs = [ "g3tools/mjsunit_regtest.sh" ],
        size = "large",
        data = [ "//third_party/v8/tests:.",
                 "//testing/shbase",
                 "g3tools/mjsunit_regress.sh",
                 "v8d" ] )


# Requires running v8x-sepcific extensions.
# Disabled for now. TODO(gri) FIX THIS.
#sh_test(name = "ext_regtest",
#                srcs = [ "g3tools/ext_regtest.sh" ],
#                size = "medium",
#                data = [ "//third_party/v8/tests:.",
#                         "//testing/shbase",
#                         "g3tools/regress.sh",
#                         "v8x" ] )


# ------------------------------------------------------------------------------
# Core file lists
# (These lists should remain in sync w/ the corresponding BUILD files in the
# Subversion repository of V8.)

filegroup(name = "app_cc_files",
          srcs = [ "src/main.cc",
                   "src/shell.cc" ] )


filegroup(name = "core_cc_files",
          srcs = [ "src/codegen.cc",
                   "src/api.cc",
                   "src/unicode.cc",
                   "src/ast.cc",
                   "src/checks.cc",
                   "src/objects.cc",
                   "src/objects-debug.cc",
                   "src/messages.cc",
                   "src/spaces.cc",
                   "src/mark-compact.cc",
                   "src/heap.cc",
                   "src/flags.cc",
                   "src/code-stubs.cc",
                   "src/builtins.cc",
                   "src/token.cc",
                   "src/scanner.cc",
                   "src/parser.cc",
                   "src/rewriter.cc",
                   "src/usage-analyzer.cc",
                   "src/scopes.cc",
                   "src/scopeinfo.cc",
                   "src/prettyprinter.cc",
                   "src/utils.cc",
                   "src/conversions.cc",
                   "src/ic.cc",
                   "src/runtime.cc",
                   "src/compiler.cc",
                   "src/contexts.cc",
                   "src/bootstrapper.cc",
                   "src/v8.cc",
                   "src/frames.cc",
                   "src/log.cc",
                   "src/property.cc",
                   "src/stub-cache.cc",
                   "src/zone.cc",
                   "src/platform-linux.cc",
                   "src/dateparser.cc",
                   "src/handles.cc",
                   "src/top.cc",
                   "src/factory.cc",
                   "src/execution.cc",
                   "src/jsregexp.cc",
                   "src/debug.cc",
                   "src/string-stream.cc",
                   "src/global-handles.cc",
                   "src/counters.cc",
                   "src/v8-counters.cc",
                   "src/hashmap.cc",
                   "src/v8threads.cc",
                   "src/serialize.cc",
                   "src/accessors.cc",
                   "src/allocation.cc",
                   "src/snapshot-common.cc",
                   "src/snapshot-empty.cc",  # for now we don't use snapshots
                   "src/variables.cc",
                   "src/version.cc" ] )


filegroup(name = "core_cc_files_3rd_party",
          srcs = [ "src/dtoa-config.c",  # MIT license
                   "src/assembler.cc",  # BSD license w/ advertising clause (Sun, Google)
                 ],
          licenses = [ "notice" ] )


filegroup(name = "core_cc_files_ia32",
          srcs = [ "src/macro-assembler-ia32.cc",
                   "src/disassembler-ia32.cc",
                   "src/disasm-ia32.cc",
                   "src/builtins-ia32.cc",
                   "src/ic-ia32.cc",
                   "src/codegen-ia32.cc",
                   "src/frames-ia32.cc",
                   "src/stub-cache-ia32.cc",
                   "src/cpu-ia32.cc" ] )


filegroup(name = "core_cc_files_ia32_3rd_party",
          srcs = [ "src/assembler-ia32.cc" ],  # BSD license w/ advertising clause (Sun, Google)
          licenses = [ "notice" ] )


filegroup(name = "core_cc_files_arm",
          srcs = [ "src/assembler-arm.cc",
                   "src/macro-assembler-arm.cc",
                   "src/disassembler-arm.cc",
                   "src/builtins-arm.cc",
                   "src/ic-arm.cc",
                   "src/codegen-arm.cc",
                   "src/frames-arm.cc",
                   "src/stub-cache-arm.cc",
                   "src/cpu-arm.cc" ] )


filegroup(name = "core_js_files",
          srcs = [ "src/macros.py",
                   "src/runtime.js",
                   "src/v8natives.js",
                   "src/array.js",
                   "src/string.js",
                   "src/uri.js",
                   "src/math.js",
                   "src/messages.js",
                   "src/apinatives.js",
                   "src/debug-delay.js",
                   "src/mirror-delay.js",
                   "src/date-delay.js",
                   "src/regexp-delay.js" ] )


# ------------------------------------------------------------------------------
# Google3 extensions file lists

filegroup(name = "ext_cc_files",
          srcs = [ "src/extensions/extensions.cc",
                   "src/extensions/io_linux.cc",
                   "src/extensions/proto.cc",
                   "src/extensions/protoproxy.cc",
                   "src/extensions/runtime_ext.cc" ] )


# src/extensions/macros.py is the same as src/macros.py, but the build
# system doesn't allow a src file to be input for 2 targets, it seems.
# So we keep a copy around. Sigh...
filegroup(name = "ext_js_files",
          srcs = [ "src/extensions/macros.py",
                   "src/extensions/io.js",
                   "src/extensions/proto.js" ] )
