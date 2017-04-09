#!/usr/bin/python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This is a test for extract_list_of_idl_methods.py from the same directory.
# Usage:
# $ find third_party/WebKit/Source -name *.idl -type f | \
#       grep -v core/inspector/InspectorInstrumentation.idl | \
#       xargs -n 1 third_party/WebKit/Source/bindings/scripts/extract_list_of_idl_methods.py \
#           >~/idl_blacklist.txt
# $ cat ~/idl_blacklist.txt | \
#       third_party/WebKit/Source/bindings/scripts/test_extract_list_of_idl_methods.py

import sys


def main():
    actual_blacklist = sys.stdin.read()
    expected_entries = [
        # ImplementedAs:
        "Document:::url:::0",                     # |contentURI| attribute: getter.
        "LocalDOMWindow:::defaultStatus:::0",     # |defaultstatus| attribute: getter.
        "LocalDOMWindow:::setDefaultStatus:::1",  # |defaultstatus| attribute: setter.
        "LocalDOMWindow:::screenX:::0",           # Interface (Window -> DOMWindow).

        # RaisesException:
        "Document:::cookie:::1",     # Attribute getter and setter.
        "Document:::setCookie:::2",  # Attribute getter and setter.
        "Document:::domain:::0",     # Attribute setter only.
        "Document:::setDomain:::2",  # Attribute setter only.
        "Document:::close:::1",      # Operation.

        # CallWith:
        "LocalDOMWindow:::print:::1",  # Operation. CallWith=ScriptState.
        "Location:::reload:::1",       # Operation. CallWith=CurrentWindow.
        "Location:::assign:::4",       # Operation. CallWith=(CurrentWindow,EnteredWindow).
        "IDBRequest:::source:::1",     # Attribute. CallWith=ScriptState.

        # Parameter count tests:
        "Element:::scrollIntoView:::1",      # Optional args.
        "Document:::writeln:::3",            # Variadic args.
        "DOMWindowTimers:::setTimeout:::5",  # Variadic args + optional args.
        "OriginTrialsTestPartial:::staticAttributePartial:::0",

        # CachedAttribute:
        "NavigatorLanguage:::languages:::0",
        "NavigatorLanguage:::hasLanguagesChanged:::0",

        # Capitalization of acronyms via |capitalize|:
        "CSSRule:::cssText:::0",
        "CSSRule:::setCSSText:::1",

        # Partial interface:
        "DOMWindowPerformance:::performance:::1",             # Partial interface.
        "DataTransferItemFileSystem:::webkitGetAsEntry:::2",  # w/o ImplementedAs.
        "PropertyRegistration:::registerProperty:::3",        # Static method.
        "DOMWindowTimers:::clearTimeout:::2",                 # LegacyTreatAsPartialInterface.

        # Uncapitalize:
        "BiquadFilterNode:::q:::0",

        # Interesting return types.
        "AnimationEffectTimingReadOnly:::duration:::1",        # |double or DOMString|.
        "CSSCalcLength:::ch:::1",                              # |double?|.
        "Animation:::timeline:::0",                            # |AnimationTimeline?|.
        "AnimationEffectReadOnly:::getComputedTiming:::1",     # Dictionary.
        "DictionaryTest:::get:::1",                            # Dictionary.
        "WebGL2RenderingContextBase:::getUniformIndices:::3",  # |sequence<GLuint>?|.

        # Custom:
        "CustomEvent:::initCustomEvent:::4",  # [Custom=CallEpilogue].
        "Document:::createTouch:::11",        # [Custom=CallPrologue].
        "SQLStatementErrorCallback:::handleEvent:::2",  # [Custom].

        # Optional parameters:
        "Blob:::slice:::1",
        "Blob:::slice:::2",
        "Blob:::slice:::3",
        "Blob:::slice:::4",

        # SetWrapperReferenceFrom:
        "DOMImplementation:::document:::0",

        # PostMessage extended attribute:
        "DedicatedWorkerGlobalScope:::postMessage:::3",
        "DedicatedWorkerGlobalScope:::postMessage:::4",

        # Event handlers:
        "AudioScheduledSourceNode:::onended:::0",
        "AudioScheduledSourceNode:::setOnended:::1",
    ]
    unexpected_entries = [
        # PutForwards:
        "Document:::setLocation",

        # Read-only attributes:
        "Document:::setOrigin",

        # Reflect:
        "HTMLInputElement:::formTarget",

        # Constructor:
        "XMLHttpRequest::Constructor",

        # Number of arguments:
        "Blob:::slice:::0",
        "Blob:::slice:::5",
        "DedicatedWorkerGlobalScope:::postMessage:::2",
        "DedicatedWorkerGlobalScope:::postMessage:::5",
    ]
    all_tests_passed = True
    for expected_entry in expected_entries:
        # Expectations contain *full* lines.
        expected_entry_with_delims = "\n" + expected_entry + "\n"
        if expected_entry_with_delims not in actual_blacklist:
            print "FAIL: Missing entry: " + expected_entry
            all_tests_passed = False
    for unexpected_entry in unexpected_entries:
        # Expectations start at the beginning of a line
        # (but don't necessarily span the whole actual line).
        unexpected_entry_with_delims = "\n" + unexpected_entry
        if unexpected_entry_with_delims in actual_blacklist:
            print "FAIL: Unexpected entry: " + unexpected_entry
            all_tests_passed = False
    if all_tests_passed:
        print "All tests passed."


if __name__ == '__main__':
    sys.exit(main())
