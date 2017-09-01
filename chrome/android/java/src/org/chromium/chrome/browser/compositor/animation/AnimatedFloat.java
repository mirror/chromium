// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.animation;

import android.util.Property;

/**
 * This is a float that is designed to be used with an {@link android.animation.Animator}. This
 * class simply wraps a primitive float variable and extends the {@link Property} class to be
 * optimized for use with a {@link CompositorAnimator}.
 */
public class AnimatedFloat extends Property<AnimatedFloat, Float> {
    /** A name for the parent class. */
    private static final String sName = "ANIMATED_FLOAT";

    /** The value that this object is maintaining. */
    private float mValue;

    public AnimatedFloat() {
        super(Float.class, sName);
    }

    /**
     * Set the value maintained by this object.
     * @param value The new value.
     */
    public void set(float value) {
        mValue = value;
    }

    @Override
    public void set(AnimatedFloat f, Float value) {
        f.mValue = value;
    }

    /**
     * Set the value maintained by this object.
     * @return The value held by this object.
     */
    public float get() {
        return mValue;
    }

    @Override
    public Float get(AnimatedFloat f) {
        return f.mValue;
    }
}
