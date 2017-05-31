// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.shape_detection;

import static org.junit.Assert.assertNull;

import android.graphics.Bitmap;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.util.Feature;
import org.chromium.mojo.system.SharedBufferHandle;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.shape_detection.mojom.FaceDetection;
import org.chromium.shape_detection.mojom.FaceDetectionResult;
import org.chromium.shape_detection.mojom.FaceDetectorOptions;
import org.chromium.testing.local.LocalRobolectricTestRunner;

import java.nio.ByteBuffer;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.TimeUnit;

/**
 * Test suite for conversion-to-Frame utils.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class FaceDetectionImplTest {
    static final Bitmap IMAGE = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);

    public FaceDetectionImplTest() {}

    @Before
    public void setUp() {}

    private static SharedBufferHandle toSharedBufferHandle(Bitmap bitmap) {
        ByteBuffer buffer = ByteBuffer.allocate(bitmap.getByteCount());
        bitmap.copyPixelsToBuffer(buffer);
        SharedBufferHandle handle =
                CoreImpl.getInstance().createSharedBuffer(null, bitmap.getByteCount());
        handle.unmap(buffer);
        return handle;
    }

    private static FaceDetectionResult[] detect(SharedBufferHandle frameData, final int width,
            final int height) throws InterruptedException {
        FaceDetectorOptions options = new FaceDetectorOptions();
        options.fastMode = false;
        options.maxDetectedFaces = 32;
        FaceDetection detector = new FaceDetectionImpl(options);

        final ArrayBlockingQueue<FaceDetectionResult[]> queue = new ArrayBlockingQueue<>(1);
        detector.detect(frameData, width, height, new FaceDetection.DetectResponse() {
            public void call(FaceDetectionResult[] results) {
                queue.add(results);
            }
        });
        FaceDetectionResult[] toReturn = queue.poll(5, TimeUnit.SECONDS);
        frameData.close();
        return toReturn;
    }

    @Test
    @Feature({"ShapeDetection"})
    public void detectFailsWithBadHeight() throws Exception {
        SharedBufferHandle frameData = toSharedBufferHandle(IMAGE);
        FaceDetectionResult[] results = detect(frameData, -1, 100);
        assertNull(results);
    }
}
