// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.animation;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.content.Context;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.compositor.layouts.LayoutUpdateHost;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/**
 * The handler responsible for managing and pushing updates to all of the active
 * CompositorAnimators. This class contains a map of Context to CompositorAnimationHandler in order
 * to work correctly for multi-instance Chrome.
 */
public class CompositorAnimationHandler {
    /** A map of Android {@link Context} to animation handler. */
    private static Map<Context, CompositorAnimationHandler> sHandlerInstances = new HashMap<>();

    /** A list of all the handler's animators. */
    private final ArrayList<CompositorAnimator> mAnimators = new ArrayList<>();

    /** This handler's update host. */
    private LayoutUpdateHost mUpdateHost;

    /** Default constructor. */
    private CompositorAnimationHandler() {}

    /**
     * @param context An Android {@link Context} to get the handler for.
     * @return The static handler instance that is used by all compositor animators.
     */
    public static CompositorAnimationHandler getInstance(Context context) {
        if (sHandlerInstances.get(context) == null) {
            sHandlerInstances.put(context, new CompositorAnimationHandler());
        }
        return sHandlerInstances.get(context);
    }

    /**
     * @param host The host for requesting frame updates.
     */
    public final void setUpdateHost(LayoutUpdateHost host) {
        mUpdateHost = host;
    }

    /**
     * Add an animator to the list of known animators to start receiving updates.
     * @param animator The animator to start.
     */
    public final void startAnimator(final CompositorAnimator animator) {
        animator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator a) {
                mAnimators.remove(animator);
                animator.removeListener(this);
            }
        });
        mAnimators.add(animator);
        mUpdateHost.requestUpdate();
    }

    /**
     * Push an update to all the currently running animators.
     * @param deltaTimeMs The time since the previous update in ms.
     */
    public final void pushUpdate(long deltaTimeMs) {
        if (mAnimators.size() == 0) return;

        // Do updates to the animators. Use a cloned list so the original list can be modified in
        // the update loop.
        ArrayList<CompositorAnimator> clonedAnimatorList =
                (ArrayList<CompositorAnimator>) mAnimators.clone();
        for (CompositorAnimator animator : clonedAnimatorList) {
            animator.doAnimationFrame(deltaTimeMs);
            // Once the animation ends, it no longer needs to receive updates; remove it from the
            // handler's list of animations. Restarting the animation will re-add the animation to
            // this handler.
            if (animator.hasEnded()) mAnimators.remove(animator);
        }

        mUpdateHost.requestUpdate();
    }

    /**
     * Clean up this handler.
     */
    public final void destroy() {
        mUpdateHost = null;
        mAnimators.clear();
    }

    /**
     * @return The number of animations that are active inside this handler.
     */
    @VisibleForTesting
    public int getActiveAnimations() {
        return mAnimators.size();
    }
}
