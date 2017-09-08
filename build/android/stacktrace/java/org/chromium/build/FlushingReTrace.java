// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.build;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.LineNumberReader;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;

import proguard.retrace.ReTrace;

/**
 * A wrapper around ReTrace that:
 *  1. Hardcodes a more useful line regular expression
 *  2. Disables output buffering
 */
public class FlushingReTrace {
    // Prefixed to LINE_PARSE_REGEX when --logcat is passed.
    private static final String LOGCAT_PREFIX = ".*?: ";

    // Note: Order of these sub-patterns defines their precedence.
    // Note: Deobfuscation of methods without the presense of line numbers basically never works.
    // There is a test for these pattern at //build/android/stacktrace/java_deobfuscate_test.py
    private static final String LINE_PARSE_REGEX = "(?:"
            // Based on default ReTrace regex, but with "at" changed to to allow :
            // E.g.: 06-22 13:58:02.895  4674  4674 E THREAD_STATE:     bLA.a(PG:173)
            // Normal stack trace lines look like:
            // \tat org.chromium.chrome.browser.tab.Tab.handleJavaCrash(Tab.java:682)
            + "(?:.*?(?::|\\bat)\\s+%c\\.%m\\s*\\(%s(?::%l)?\\))|"
            // E.g.: VFY: unable to resolve new-instance 3810 (LSome/Framework/Class;) in Lfoo/Bar;
            + "(?:.*L%C;.*)|"
            // E.g.: END SomeTestClass#someMethod
            + "(?:.*?%c#%m.*?)|"
            // E.g.: The member "Foo.bar"
            // E.g.: The class "Foobar"
            + "(?:.*?\"%c\\.%m\".*)|"
            + "(?:.*?\"%c\".*)|"
            // E.g.: java.lang.RuntimeException: Intentional Java Crash
            + "(?:%c:.*)|"
            // All lines that end with a class / class+method:
            // E.g.: The class: Foo
            // E.g.: INSTRUMENTATION_STATUS: class=Foo
            // E.g.: NoClassDefFoundError: SomeFrameworkClass in isTestClass for Foo
            // E.g.: Could not find class 'SomeFrameworkClass', referenced from method Foo.bar
            // E.g.: Could not find method SomeFrameworkMethod, referenced from method Foo.bar
            + "(?:.*(?:=|:\\s*|\\s+)%c\\.%m)|"
            + "(?:.*(?:=|:\\s*|\\s+)%c)"
            + ")";

    private static void usage() {
        System.err.println("Usage: echo $OBFUSCATED_CLASS | java_deobfuscate Foo.apk.mapping");
        System.err.println("Usage: java_deobfuscate --logcat Foo.apk.mapping < foo.log");
        System.err.println("Note: Deobfuscation of symbols outside the context of stack "
                + "traces will work only when lines match the regular expression defined "
                + "in FlushingReTrace.java.");
        System.err.println("Also: Deobfuscation of method names without associated line "
                + "numbers does not seem to work.");
        System.exit(1);
    }

    public static void main(String[] args) {
        if (args.length == 0 || args.length > 2) {
            usage();
        }
        String mappingPath = null;
        boolean logcat = false;
        for (String arg : args) {
            if ("--help".equals(arg)) {
                usage();
            } else if ("--logcat".equals(arg)) {
                logcat = true;
            } else if (arg.startsWith("-") || mappingPath != null) {
                usage();
            } else {
                mappingPath = arg;
            }
        }
        if (mappingPath == null) {
            usage();
        }

        File mappingFile = new File(args[0]);
        try {
            LineNumberReader reader = new LineNumberReader(
                    new BufferedReader(new InputStreamReader(System.in, "UTF-8")));

            // Enabling autoFlush is the main difference from ReTrace.main().
            boolean autoFlush = true;
            PrintWriter writer =
                    new PrintWriter(new OutputStreamWriter(System.out, "UTF-8"), autoFlush);

            boolean verbose = false;
            String lineParseRegex = LINE_PARSE_REGEX;
            if (logcat) {
                lineParseRegex = LOGCAT_PREFIX + LINE_PARSE_REGEX;
            }
            new ReTrace(lineParseRegex, verbose, mappingFile).retrace(reader, writer);
        } catch (IOException ex) {
            // Print a verbose stack trace.
            ex.printStackTrace();
            System.exit(1);
        }

        System.exit(0);
    }
}
