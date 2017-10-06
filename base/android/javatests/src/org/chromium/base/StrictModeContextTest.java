// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.os.StrictMode;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Tests for the StrictModeContext class.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class StrictModeContextTest {
    private StrictMode.ThreadPolicy mOldThreadPolicy;
    private StrictMode.VmPolicy mOldVmPolicy;
    private File mFileForWriting;
    private File mFileForReading;

    @Before
    public void setUp() throws Exception {
        mFileForWriting = File.createTempFile("foo", "bar");
        mFileForReading = File.createTempFile("foo", "baz");
        enableStrictMode();
    }

    @After
    public void tearDown() {
        disableStrictMode();
        mFileForReading.delete();
        mFileForWriting.delete();
    }

    private void enableStrictMode() {
        mOldThreadPolicy = StrictMode.getThreadPolicy();
        mOldVmPolicy = StrictMode.getVmPolicy();
        StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
                                           .detectAll()
                                           .penaltyLog()
                                           .penaltyDeath()
                                           .build());
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectAll().penaltyLog().penaltyDeath().build());
    }

    private void disableStrictMode() {
        StrictMode.setThreadPolicy(mOldThreadPolicy);
        StrictMode.setVmPolicy(mOldVmPolicy);
    }

    private void writeToDisk() {
        try {
            new FileOutputStream(mFileForWriting).write("Foo".getBytes());
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private void assertWriteToDiskThrows() {
        boolean didThrow = false;
        try {
            writeToDisk();
        } catch (Exception e) {
            didThrow = true;
        }
        Assert.assertTrue("Expected disk write to  throw.", didThrow);
    }

    private void readFromDisk() {
        try {
            new FileInputStream(mFileForReading).read();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private void assertReadFromDiskThrows() {
        boolean didThrow = false;
        try {
            readFromDisk();
        } catch (Exception e) {
            didThrow = true;
        }
        Assert.assertTrue("Expected disk read to  throw.", didThrow);
    }

    @Test
    @SmallTest
    public void testAllowDiskWrites() {
        try (StrictModeContext unused = StrictModeContext.allowDiskWrites()) {
            writeToDisk();
        }
        assertWriteToDiskThrows();
    }

    @Test
    @SmallTest
    public void testAllowDiskReads() {
        try (StrictModeContext unused = StrictModeContext.allowDiskReads()) {
            readFromDisk();
            assertWriteToDiskThrows();
        }
        assertReadFromDiskThrows();
    }
}
