// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.animation;

import android.animation.Animator;
import android.animation.TimeInterpolator;
import android.content.Context;

import org.chromium.base.VisibleForTesting;

import java.util.ArrayList;

/**
 * An animator that can be used for animations in the Browser Compositor.
 */
public class CompositorAnimator extends Animator {
    /** The context associated with the instance of Chrome running the animator. */
    private final Context mContext;

    /** The list of listeners for events through the life of an animation. */
    private final ArrayList<AnimatorListener> mListeners = new ArrayList<>();

    /** The list of frame update listeners for this animation. */
    private final ArrayList<AnimatorUpdateListener> mAnimatorUpdateListeners = new ArrayList<>();

    /** The time interpolator for the animator. */
    private TimeInterpolator mTimeInterpolator;

    /**
     * The amount of time in ms that has passed since the animation has started. This includes any
     * delay that was set.
     */
    private long mTimeSinceStartMs;

    /**
     * The fraction that the animation is complete. This number is in the range [0, 1] and accounts
     * for the set time interpolator.
     */
    private float mAnimatedFraction;

    /** The duration of the animation in ms. */
    private long mDurationMs;

    /**
     * The animator's start delay in ms. Once {@link #start()} is called, updates are not sent until
     * this time has passed.
     */
    private long mStartDelayMs;

    /** Whether or not {@link #start()} has been called on this animator and it has not ended. */
    private boolean mIsStarted;

    /**
     * Whether or not {@link #cancel()} has been called on this animator and it has not been
     * restarted.
     */
    private boolean mIsCanceled;

    /** Whether or not the start delay has passed and the animation is processing updates. */
    private boolean mIsRunning;

    /**
     * Whether or not the animation has played to completion. This is reset when the animator
     * restarts.
     */
    private boolean mHasEnded;

    /** An interface for listening for frames of an animation. */
    public interface AnimatorUpdateListener {
        /**
         * A notification of the occurrence of another frame of the animation.
         * @param animator The animator that was updated.
         */
        void onAnimationUpdate(CompositorAnimator animator);
    }

    /**
     * Create a new animator for the current context.
     * @param context The Android {@link Context} to create the animator for. This is tied to a
     *                particular instance of Chrome.
     */
    public CompositorAnimator(Context context) {
        mContext = context;
    }

    /**
     * Push an update to the animation. This should be called while the start delay
     * @param deltaTimeMs The time since the previous frame. The first time this is called after an
     *                    animation starts, the value will be ignored and 0 will be passed instead.
     */
    public void doAnimationFrame(long deltaTimeMs) {
        mTimeSinceStartMs += deltaTimeMs;

        if (mTimeSinceStartMs > mStartDelayMs) {
            mIsRunning = true;
        } else {
            // If the animation delay has not passed, do not process the update.
            return;
        }

        // Clamp the time to be between 0 and the duration, taking into account the start delay.
        long finalTimeMs = Math.min(mTimeSinceStartMs - mStartDelayMs, mDurationMs);

        mAnimatedFraction = finalTimeMs / mDurationMs;
        if (mTimeInterpolator != null) {
            mAnimatedFraction = mTimeInterpolator.getInterpolation(mAnimatedFraction);
        }

        // Push update to listeners.
        ArrayList<AnimatorUpdateListener> clonedList =
                (ArrayList<AnimatorUpdateListener>) mAnimatorUpdateListeners.clone();
        for (AnimatorUpdateListener listener : clonedList) listener.onAnimationUpdate(this);

        if (finalTimeMs == mDurationMs) end();
    }

    /**
     * @return The animated fraction after being passed through the time interpolator, if set.
     */
    @VisibleForTesting
    public float getAnimatedFraction() {
        return mAnimatedFraction;
    }

    /**
     * Add a listener for frame occurrences.
     * @param listener The listener to add.
     */
    public void addUpdateListener(AnimatorUpdateListener listener) {
        mAnimatorUpdateListeners.add(listener);
    }

    /**
     * @param listener The listener to remove.
     */
    public void removeUpdateListener(AnimatorListener listener) {
        mAnimatorUpdateListeners.remove(listener);
    }

    /**
     * @return Whether or not the animation has ended after being started. If the animation is
     *         started after ending, this value will be reset to true.
     */
    public boolean hasEnded() {
        return mHasEnded;
    }

    @Override
    public void addListener(AnimatorListener listener) {
        mListeners.add(listener);
    }

    @Override
    public void removeListener(AnimatorListener listener) {
        mListeners.remove(listener);
    }

    @Override
    public void removeAllListeners() {
        mListeners.clear();
        mAnimatorUpdateListeners.clear();
    }

    @Override
    public void start() {
        if (mIsStarted) return;

        super.start();
        mIsStarted = true;
        mIsCanceled = false;
        mHasEnded = false;
        CompositorAnimationHandler.getInstance(mContext).startAnimator(this);
        mTimeSinceStartMs = 0;

        ArrayList<AnimatorListener> clonedList = (ArrayList<AnimatorListener>) mListeners.clone();
        for (AnimatorListener listener : clonedList) listener.onAnimationStart(this);
    }

    @Override
    public void cancel() {
        if (!mIsStarted) return;

        super.cancel();
        mIsStarted = false;
        mIsCanceled = true;

        ArrayList<AnimatorListener> clonedList = (ArrayList<AnimatorListener>) mListeners.clone();
        for (AnimatorListener listener : clonedList) listener.onAnimationCancel(this);

        end();
    }

    @Override
    public void end() {
        if (!mIsStarted && !mIsCanceled && mIsRunning) return;

        super.end();
        mIsStarted = false;
        mIsRunning = false;
        mHasEnded = true;

        ArrayList<AnimatorListener> clonedList = (ArrayList<AnimatorListener>) mListeners.clone();
        for (AnimatorListener listener : clonedList) listener.onAnimationEnd(this);
    }

    @Override
    public long getStartDelay() {
        return mStartDelayMs;
    }

    @Override
    public void setStartDelay(long startDelayMs) {
        if (startDelayMs < 0) startDelayMs = 0;
        mStartDelayMs = startDelayMs;
    }

    @Override
    public CompositorAnimator setDuration(long durationMs) {
        if (durationMs < 0) durationMs = 0;
        mDurationMs = durationMs;
        return this;
    }

    @Override
    public long getDuration() {
        return mDurationMs;
    }

    @Override
    public void setInterpolator(TimeInterpolator timeInterpolator) {
        mTimeInterpolator = timeInterpolator;
    }

    @Override
    public boolean isRunning() {
        return mIsRunning;
    }
}
