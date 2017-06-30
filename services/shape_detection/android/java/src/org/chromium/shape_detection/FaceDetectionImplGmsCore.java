// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.shape_detection;

import android.graphics.PointF;
import android.util.SparseArray;

import com.google.android.gms.vision.Frame;
import com.google.android.gms.vision.face.Face;
import com.google.android.gms.vision.face.FaceDetector;
import com.google.android.gms.vision.face.Landmark;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.gfx.mojom.RectF;
import org.chromium.mojo.system.MojoException;
import org.chromium.shape_detection.mojom.FaceDetection;
import org.chromium.shape_detection.mojom.FaceDetectionResult;
import org.chromium.shape_detection.mojom.FaceDetectorOptions;
import org.chromium.shape_detection.mojom.LandmarkType;

import java.util.ArrayList;
import java.util.List;

/**
 * Google Play services implementation of the FaceDetection service defined in
 * services/shape_detection/public/interfaces/facedetection.mojom
 */
public class FaceDetectionImplGmsCore implements FaceDetection {
    private static final String TAG = "FaceDetectionImpl";
    private static final int MAX_FACES = 32;
    private final int mMaxFaces;
    private final boolean mFastMode;
    private final FaceDetector mFaceDetector;

    FaceDetectionImplGmsCore(FaceDetectorOptions options) {
        FaceDetector.Builder builder =
                new FaceDetector.Builder(ContextUtils.getApplicationContext());
        mMaxFaces = Math.min(options.maxDetectedFaces, MAX_FACES);
        mFastMode = options.fastMode;

        try {
            builder.setMode(mFastMode ? FaceDetector.FAST_MODE : FaceDetector.ACCURATE_MODE);
            builder.setLandmarkType(FaceDetector.ALL_LANDMARKS);
            if (mMaxFaces == 1) {
                builder.setProminentFaceOnly(true);
            }
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Unexpected exception " + e);
            assert false;
        }

        mFaceDetector = builder.build();
    }

    @Override
    public void detect(org.chromium.skia.mojom.Bitmap bitmapData, DetectResponse callback) {
        // The vision library will be downloaded the first time the API is used
        // on the device; this happens "fast", but it might have not completed,
        // bail in this case.
        if (!mFaceDetector.isOperational()) {
            Log.e(TAG, "FaceDetector is not operational");

            // Fallback to Android's FaceDetectionImpl.
            FaceDetectorOptions options = new FaceDetectorOptions();
            options.fastMode = mFastMode;
            options.maxDetectedFaces = mMaxFaces;
            FaceDetectionImpl detector = new FaceDetectionImpl(options);
            detector.detect(bitmapData, callback);
            return;
        }

        Frame frame = BitmapUtils.convertToFrame(bitmapData);
        if (frame == null) {
            Log.e(TAG, "Error converting SharedMemory to Frame");
            callback.call(new FaceDetectionResult[0]);
            return;
        }

        final SparseArray<Face> faces = mFaceDetector.detect(frame);

        FaceDetectionResult[] faceArray = new FaceDetectionResult[faces.size()];
        for (int i = 0; i < faces.size(); i++) {
            faceArray[i] = new FaceDetectionResult();
            final Face face = faces.valueAt(i);

            final List<Landmark> landmarks = face.getLandmarks();
            ArrayList<org.chromium.shape_detection.mojom.Landmark> mojoLandmarks =
                    new ArrayList<org.chromium.shape_detection.mojom.Landmark>(landmarks.size());

            int leftEyeIndex = 0, rightEyeIndex = 0, bottomMouthIndex = 0;
            for (int j = 0; j < landmarks.size(); j++) {
                final Landmark landmark = landmarks.get(j);
                final int landmarkType = landmark.getType();
                final boolean flag = true;
                switch (landmarkType) {
                    case Landmark.LEFT_EYE:
                        leftEyeIndex = j;
                        break;
                    case Landmark.RIGHT_EYE:
                        rightEyeIndex = j;
                        break;
                    case Landmark.BOTTOM_MOUTH:
                        bottomMouthIndex = j;
                        break;
                    default:
                        flag = false;
                }
                if (flag) {
                    org.chromium.shape_detection.mojom.Landmark mojoLandmark =
                            new org.chromium.shape_detection.mojom.Landmark();
                    mojoLandmark.location = new org.chromium.gfx.mojom.PointF();
                    mojoLandmark.location.x = landmark.getPosition().x;
                    mojoLandmark.location.y = landmark.getPosition().y;
                    mojoLandmark.type = landmarkType == Landmark.BOTTOM_MOUTH ? LandmarkType.MOUTH
                                                                              : LandmarkType.EYE;
                    mojoLandmarks.add(mojoLandmark);
                }
            }
            faceArray[i].landmarks = mojoLandmarks.toArray(
                    new org.chromium.shape_detection.mojom.Landmark[mojoLandmarks.size()]);

            final PointF corner = face.getPosition();
            final PointF leftEyePoint = landmarks.get(leftEyeIndex).getPosition();
            final float eyesDistance =
                    leftEyePoint.x - landmarks.get(rightEyeIndex).getPosition().x;
            final PointF midPoint = new PointF(corner.x + face.getWidth() / 2, leftEyePoint.y);
            faceArray[i].boundingBox = new RectF();
            faceArray[i].boundingBox.x = midPoint.x - eyesDistance;
            faceArray[i].boundingBox.y = midPoint.y - eyesDistance;
            faceArray[i].boundingBox.width = 2 * eyesDistance;
            faceArray[i].boundingBox.height = 2 * eyesDistance;
        }
        callback.call(faceArray);
    }

    @Override
    public void close() {
        mFaceDetector.release();
    }

    @Override
    public void onConnectionError(MojoException e) {
        close();
    }
}
