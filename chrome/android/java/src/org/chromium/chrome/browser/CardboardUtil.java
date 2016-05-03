// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

/**
 * Utility class for Cardboard activity.
 */
public class CardboardUtil {
    private static final int BYTES_PER_FLOAT = 4;
    private static final int BYTES_PER_SHORT = 2;

    /**
     * Create rectangular texture float buffer.
     */
    public static FloatBuffer makeRectangularTextureBuffer(
            float left, float right, float bottom, float top) {
        float[] position = new float[] {
                left, bottom, left, top, right, bottom, left, top, right, top, right, bottom};
        return makeFloatBuffer(position);
    }

    /**
     * Convert float array to a FloatBuffer for use in OpenGL calls.
     */
    public static FloatBuffer makeFloatBuffer(float[] data) {
        FloatBuffer result = ByteBuffer.allocateDirect(data.length * BYTES_PER_FLOAT)
                                     .order(ByteOrder.nativeOrder())
                                     .asFloatBuffer();
        result.put(data).position(0);
        return result;
    }
}
