// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.shape_detection;

import static org.junit.Assert.assertEquals;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLog;

import org.chromium.base.test.util.Feature;
import org.chromium.mojo.system.SharedBufferHandle;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.shape_detection.mojom.FaceDetection;
import org.chromium.shape_detection.mojom.FaceDetectionResult;
import org.chromium.shape_detection.mojom.FaceDetectorOptions;
import org.chromium.testing.local.LocalRobolectricTestRunner;

import java.nio.ByteBuffer;
import java.util.Base64;

/**
 * Test suite for conversion-to-Frame utils.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class FaceDetectionImplTest {
    static final byte[] MONA_LISA_PNG_BYTES = Base64.getDecoder().decode(
            "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");
    public final Bitmap MONA_LISA_BITMAP;
    public final int WIDTH;
    public final int HEIGHT;
    public final int NUM_BYTES;

    public FaceDetectionImplTest() {
        // NOTE: In O, we can use BitmapFactory.Options.outConfig.
        Bitmap pngBitmap =
                BitmapFactory.decodeByteArray(MONA_LISA_PNG_BYTES, 0, MONA_LISA_PNG_BYTES.length);
        WIDTH = pngBitmap.getWidth();
        HEIGHT = pngBitmap.getHeight();
        NUM_BYTES = WIDTH * HEIGHT * 4;
        int[] pixels = new int[WIDTH * HEIGHT];
        pngBitmap.getPixels(pixels, 0, WIDTH, 0, 0, WIDTH, HEIGHT);
        MONA_LISA_BITMAP = Bitmap.createBitmap(pixels, WIDTH, HEIGHT, Bitmap.Config.ARGB_8888);
    }

    @Before
    public void setUp() {
        ShadowLog.stream = System.out;
        MockitoAnnotations.initMocks(this);
    }

    private ByteBuffer monaLisaBuffer() {
        ByteBuffer buffer = ByteBuffer.allocate(NUM_BYTES);
        MONA_LISA_BITMAP.copyPixelsToBuffer(buffer);
        return buffer;
    }

    private SharedBufferHandle toSharedBufferHandle(Bitmap bitmap) {
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
        FaceDetection.DetectResponse callback = Mockito.mock(FaceDetection.DetectResponse.class);
        ArgumentCaptor<FaceDetectionResult[]> argument =
                ArgumentCaptor.forClass(FaceDetectionResult[].class);
        detector.detect(frameData, width, height, callback);
        Mockito.verify(callback, Mockito.timeout(1000 /* milliseconds */).times(1))
                .call(argument.capture());
        return argument.getValue();
    }

    @Test
    @Feature({"ShapeDetection"})
    public void detectFailsWithBadHeight() throws Exception {
        // SharedBufferHandle frameData = toSharedBufferHandle(IMAGE);
        SharedBufferHandle frameData = Mockito.mock(SharedBufferHandle.class);
        Mockito.when(frameData.isValid()).thenReturn(true);
        FaceDetectionResult[] results = detect(frameData, -1, HEIGHT);
        assertEquals(0, results.length);
    }

    @Test
    @Feature({"ShapeDetection"})
    public void detectSucceedsOnValidImage() throws Exception {
        // SharedBufferHandle frameData = Mockito.mock(SharedBufferHandle.class);
        SharedBufferHandle frameData =
                Mockito.mock(SharedBufferHandle.class, Mockito.withSettings().verboseLogging());
        Mockito.when(frameData.isValid()).thenReturn(true);
        ByteBuffer buff = monaLisaBuffer();
        System.out.println("bytes: " + NUM_BYTES);
        System.out.println("capacity: " + buff.capacity());
        Mockito.when(frameData.map(
                Mockito.eq(0L),
                Mockito.eq(NUM_BYTES),
                Mockito.any(SharedBufferHandle.MapFlags.class)))
                        .thenReturn(buff);
        System.out.println(": " + buff);
        FaceDetectionResult[] results = detect(frameData, WIDTH, HEIGHT);
        assertEquals(1, results.length);
    }

    //@Test
    //@Feature({"ShapeDetection"
    // public void detectHandlesOddWidths() throws Exception {
    //    SharedBufferHandle frameData = Mockito.mock(SharedBufferHandle.class);
    //    Mockito.when(frameData.isValid()).thenReturn(true);
    //    FaceDetectionResult[] results = detect(frameData, -1, 100);
    //    assertEquals(0, results.length);
    //}
}
