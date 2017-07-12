// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.init;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * Tests for {@link ChainedTasks}.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class ChainedTasksTest {
    private static final class TestRunnable implements Runnable {
        private final List<String> mLogMessages;
        private final String mMessage;

        public TestRunnable(List<String> logMessages, String message) {
            mLogMessages = logMessages;
            mMessage = message;
        }

        @Override
        public void run() {
            mLogMessages.add(mMessage);
        }
    }

    @Test
    @SmallTest
    public void testSyncTasks() {
        final List<String> expectedMessages =
                Arrays.asList(new String[] {"First", "Second", "Third"});
        final List<String> messages = new ArrayList<>();
        final ChainedTasks tasks = new ChainedTasks();
        for (String message : expectedMessages) tasks.add(new TestRunnable(messages, message));

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                tasks.start(false);
                Assert.assertEquals(expectedMessages, messages);
            }
        });
    }

    @Test
    @SmallTest
    public void testAsyncTasks() throws Exception {
        List<String> expectedMessages = Arrays.asList(new String[] {"First", "Second", "Third"});
        final List<String> messages = new ArrayList<>();
        final ChainedTasks tasks = new ChainedTasks();
        final Semaphore finished = new Semaphore(0);

        for (String message : expectedMessages) tasks.add(new TestRunnable(messages, message));
        tasks.add(new Runnable() {
            @Override
            public void run() {
                finished.release();
            }
        });

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                tasks.start(true);
                Assert.assertTrue("No task should run synchronously", messages.isEmpty());
            }
        });
        Assert.assertTrue(finished.tryAcquire(1000, TimeUnit.MILLISECONDS));
        Assert.assertEquals(expectedMessages, messages);
    }

    @Test
    @SmallTest
    public void testAsyncTasksAreChained() throws Exception {
        List<String> expectedMessages =
                Arrays.asList(new String[] {"First", "Second", "High Priority", "Third"});
        final List<String> messages = new ArrayList<>();
        final ChainedTasks tasks = new ChainedTasks();
        final Semaphore secondTaskFinished = new Semaphore(0);
        final Semaphore waitForHighPriorityTask = new Semaphore(0);
        final Semaphore finished = new Semaphore(0);

        // Posts 2 tasks, waits for a high priority task to be posted from another thread, and
        // carries on.
        tasks.add(new TestRunnable(messages, "First"));
        tasks.add(new TestRunnable(messages, "Second"));
        tasks.add(new Runnable() {
            @Override
            public void run() {
                try {
                    secondTaskFinished.release();
                    waitForHighPriorityTask.acquire();
                } catch (InterruptedException e) {
                    Assert.fail();
                }
            }
        });
        tasks.add(new TestRunnable(messages, "Third"));
        tasks.add(new Runnable() {
            @Override
            public void run() {
                finished.release();
            }
        });

        tasks.start(true);
        Assert.assertTrue(secondTaskFinished.tryAcquire(1000, TimeUnit.MILLISECONDS));
        ThreadUtils.postOnUiThread(new TestRunnable(messages, "High Priority"));
        waitForHighPriorityTask.release();

        Assert.assertTrue(finished.tryAcquire(1000, TimeUnit.MILLISECONDS));
        Assert.assertEquals(expectedMessages, messages);
    }

    @Test
    @SmallTest
    public void testCanCancel() throws Exception {
        List<String> expectedMessages = Arrays.asList(new String[] {"First", "Second"});
        final List<String> messages = new ArrayList<>();
        final ChainedTasks tasks = new ChainedTasks();
        final Semaphore finished = new Semaphore(0);

        tasks.add(new TestRunnable(messages, "First"));
        tasks.add(new TestRunnable(messages, "Second"));
        tasks.add(new Runnable() {
            @Override
            public void run() {
                tasks.cancel();
            }
        });
        tasks.add(new TestRunnable(messages, "Third"));
        tasks.add(new Runnable() {
            @Override
            public void run() {
                finished.release();
            }
        });
        tasks.start(true);
        Assert.assertFalse(finished.tryAcquire(1000, TimeUnit.MILLISECONDS));
        Assert.assertEquals(expectedMessages, messages);
    }

    @Test
    @SmallTest
    public void testThreadRestrictions() throws Exception {
        ChainedTasks tasks = new ChainedTasks();
        List<String> messages = new ArrayList<>();
        final Semaphore cancelCalled = new Semaphore(0);

        for (String message : new String[] {"First", "Second"}) {
            tasks.add(new TestRunnable(messages, message));
        }
        // Wait for cancel to be called on the other thread.
        tasks.add(new Runnable() {
            @Override
            public void run() {
                try {
                    cancelCalled.acquire();
                } catch (InterruptedException e) {
                    Assert.fail();
                }
            }
        });

        tasks.start(true);
        try {
            tasks.cancel();
            Assert.fail("Cancel should not be callable from a non-UI thread");
        } catch (AssertionError e) {
            // Expected.
        } finally {
            cancelCalled.release();
        }
    }
}
