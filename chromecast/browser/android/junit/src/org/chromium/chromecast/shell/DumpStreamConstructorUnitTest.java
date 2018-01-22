// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import static org.hamcrest.Matchers.containsString;
import static org.hamcrest.Matchers.not;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;
import android.support.test.InstrumentationRegistry;
import org.chromium.base.ContextUtils;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

// TODO(sandv): Add test cases as need arises.
/**
 * Tests for DumpStreamConstructor.
 *
 */
@RunWith(BlockJUnit4ClassRunner.class)
public class DumpStreamConstructorUnitTest {
    @Test
    public void testTextHeader() {
        assertEquals(DumpStreamConstructor.generateTextFieldMineHeader("testName"),
                "Content-Disposition: form-data; name=\"testName\"\nContent-Type: text/plain\n\n");
    }
    @Test
    public void testFileHeader() {
        assertEquals(DumpStreamConstructor.generateFileMimeHeader("testName", "testFileName"),
                "Content-Disposition: form-data; name=\"testName\"; filename=\"testFileName\"\nContent-Type: text/plain\n\n");
    }

    private static final String CRASH_DUMP_DIR = "Crash Reports";

    private File mCrashDir;
    private File mCacheDir;
    private static final String BOUNDARY = "TESTBOUNDARY";

    public static void setUpMinidumpFile(File file, String boundary) throws IOException {
        PrintWriter minidumpWriter = null;
        try {
            minidumpWriter = new PrintWriter(new FileWriter(file));
            minidumpWriter.println("--" + boundary);
            minidumpWriter.println("Content-Disposition: form-data; name=\"prod\"");
            minidumpWriter.println();
            minidumpWriter.println("Chrome_Android");
            minidumpWriter.println("--" + boundary);
            minidumpWriter.println("Content-Disposition: form-data; name=\"ver\"");
            minidumpWriter.println();
            minidumpWriter.println("1");
            minidumpWriter.println(boundary + "--");
            minidumpWriter.flush();
        } finally {
            if (minidumpWriter != null) {
                minidumpWriter.close();
            }
        }
    }
    private File createMinidumpFileInCrashDir(String name) throws IOException {
        File minidumpFile = new File(mCrashDir, name);
        setUpMinidumpFile(minidumpFile, BOUNDARY);
        return minidumpFile;
    }

    /**
     * Returns the cache directory where we should store minidumps.
     * Can be overriden by sub-classes to allow for use with different cache directories.
     */
    public File getExistingCacheDir() {
        return ContextUtils.getApplicationContext().getCacheDir();
    }

    private void setUp() throws Exception {
        ContextUtils.initApplicationContextForTests(
                InstrumentationRegistry.getTargetContext().getApplicationContext());

        if (mCacheDir == null) {
            mCacheDir = getExistingCacheDir();
        }
        if (mCrashDir == null) {
            mCrashDir = new File(mCacheDir, CRASH_DUMP_DIR);
        }
        if (!mCrashDir.isDirectory() && !mCrashDir.mkdir()) {
            throw new Exception("Unable to create directory: " + mCrashDir.getAbsolutePath());
        }
    }
}
