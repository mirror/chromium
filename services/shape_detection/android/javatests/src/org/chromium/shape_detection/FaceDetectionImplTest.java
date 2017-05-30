// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.shape_detection;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.support.test.filters.SmallTest;
import android.test.InstrumentationTestCase;

import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.mojo.HandleMock;
import org.chromium.mojo.system.SharedBufferHandle;
import org.chromium.shape_detection.mojom.FaceDetection;
import org.chromium.shape_detection.mojom.FaceDetectionResult;
import org.chromium.shape_detection.mojom.FaceDetectorOptions;

import java.nio.ByteBuffer;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.TimeUnit;

/**
 * Test suite for FaceDetectionImpl.
 */
public class FaceDetectionImplTest extends InstrumentationTestCase {
    public final Bitmap MONA_LISA_BITMAP = bitmapFromFile("mona_lisa.png");

    public FaceDetectionImplTest() {}

    private static Bitmap bitmapFromFile(String relPath) {
        String path = UrlUtils.getIsolatedTestFilePath("services/test/data/" + relPath);
        // NOTE: On O, we can just use BimapFactory.Options.outConfig = BitmapConfig.ARGB_8888.
        Bitmap pngBitmap = BitmapFactory.decodeFile(path);
        int width = pngBitmap.getWidth();
        int height = pngBitmap.getHeight();
        int[] pixels = new int[width * height];
        pngBitmap.getPixels(pixels, 0, width, 0, 0, width, height);
        return Bitmap.createBitmap(pixels, width, height, Bitmap.Config.ARGB_8888);
    }

    private static SharedBufferHandle toMockSharedBufferHandle(final Bitmap bitmap) {
        return new HandleMock() {
            @Override
            public ByteBuffer map(long offset, long length, MapFlags flags) {
                ByteBuffer buffer = ByteBuffer.allocate(bitmap.getByteCount());
                bitmap.copyPixelsToBuffer(buffer);
                buffer.rewind();
                return buffer;
            }
        };
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
        FaceDetectionResult[] toReturn = queue.poll(5L, TimeUnit.SECONDS);
        assertNotNull(toReturn);
        frameData.close();
        return toReturn;
    }

    private static FaceDetectionResult[] detect(Bitmap bitmap) throws InterruptedException {
        SharedBufferHandle frameData = toMockSharedBufferHandle(bitmap);
        return detect(frameData, bitmap.getWidth(), bitmap.getHeight());
    }

    @SmallTest
    @Feature({"ShapeDetection"})
    public void testDetectFailsWithBadWidth() throws Exception {
        SharedBufferHandle frameData = toMockSharedBufferHandle(MONA_LISA_BITMAP);
        FaceDetectionResult[] results = detect(frameData, MONA_LISA_BITMAP.getWidth(), 0);
        assertEquals(0, results.length);
    }

    @SmallTest
    @Feature({"ShapeDetection"})
    public void testDetectFailsWithBadHeight() throws Exception {
        SharedBufferHandle frameData = toMockSharedBufferHandle(MONA_LISA_BITMAP);
        FaceDetectionResult[] results = detect(frameData, 0, MONA_LISA_BITMAP.getHeight());
        assertEquals(0, results.length);
    }

    @SmallTest
    @Feature({"ShapeDetection"})
    public void testDetectFailsWithInsufficientData() throws Exception {
        SharedBufferHandle frameData = toMockSharedBufferHandle(MONA_LISA_BITMAP);
        FaceDetectionResult[] results =
                detect(frameData, MONA_LISA_BITMAP.getWidth(), MONA_LISA_BITMAP.getHeight() + 1);
        assertEquals(0, results.length);
    }

    @SmallTest
    @Feature({"ShapeDetection"})
    public void testDetectSucceedsOnValidImage() throws Exception {
        FaceDetectionResult[] results = detect(MONA_LISA_BITMAP);
        assertEquals(1, results.length);
        assertEquals(40.0, results[0].boundingBox.width, 0.1);
        assertEquals(40.0, results[0].boundingBox.height, 0.1);
        assertEquals(24.0, results[0].boundingBox.x, 0.1);
        assertEquals(20.0, results[0].boundingBox.y, 0.1);
    }
}
