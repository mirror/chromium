// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.init;

import org.chromium.base.ThreadUtils;

import java.util.LinkedList;

/** Allows to chain multiple tasks on the UI thread, with the next task posted when one completes.
 */
public class ChainedTasks {
    LinkedList<Runnable> mTasks = new LinkedList<>();
    boolean mIsRunning = false;

    private final Runnable mRunAndPost = new Runnable() {
        @Override
        public void run() {
            if (mTasks.isEmpty()) {
                mIsRunning = false;
                return;
            }
            mTasks.pop().run();
            ThreadUtils.postOnUiThread(this);
        }
    };

    /**
     * Add a task to the list of tasks to run.
     *
     * Can be called from any thread, however should not be called from any other thread once
     * {@link run()} has been called.
     */
    public void add(Runnable task) {
        if (mIsRunning) ThreadUtils.assertOnUiThread();
        mTasks.add(task);
    }

    /**
     * Cancel the remaining tasks. Must be called from the UI thread.
     */
    public void cancel() {
        ThreadUtils.assertOnUiThread();
        mTasks.clear();
        mIsRunning = false;
    }

    /**
     * Posts or runs all the tasks, one by one.
     *
     * @param async if true, posts the tasks. Otherwise run them in a singl task. If called on the
     *              UI threads, run in the current task.
     */
    public void start(final boolean async) {
        mIsRunning = true;
        if (!async) {
            ThreadUtils.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    for (Runnable task : mTasks) task.run();
                    mTasks.clear();
                    mIsRunning = false;
                }
            });
        } else {
            ThreadUtils.postOnUiThread(mRunAndPost);
        }
    }
}
