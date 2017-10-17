// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

import android.media.MediaCodec;
import android.os.Build;
import android.util.SparseArray;

import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.media.MediaCodecUtil.BitrateAdjustmentTypes;
import org.chromium.media.MediaCodecUtil.MimeTypes;

import java.nio.ByteBuffer;

/**
 * This class extends MediaCodecBridge for encoding processing.
 */
@JNINamespace("media")
class MediaCodecEncoder extends MediaCodecBridge {
    private static final String TAG = "cr_MediaCodecEncoder";

    private SparseArray<ByteBuffer> mOutputBuffers;
    // SPS and PPS NALs (Config frame) for H.264
    private ByteBuffer mConfigData = null;

    protected MediaCodecEncoder(MediaCodec mediaCodec, String mime,
            boolean adaptivePlaybackSupported, BitrateAdjustmentTypes bitrateAdjustmentType) {
        super(mediaCodec, mime, adaptivePlaybackSupported, bitrateAdjustmentType);

        mOutputBuffers = new SparseArray<>();
    }

    @Override
    protected ByteBuffer getOutputBuffer(int index) {
        return mOutputBuffers.get(index);
    }

    @Override
    protected void releaseOutputBuffer(int index, boolean render) {
        try {
            mMediaCodec.releaseOutputBuffer(index, render);
            mOutputBuffers.remove(index);
        } catch (IllegalStateException e) {
            Log.e(TAG, "Failed to release output buffer", e);
        }
    }

    @SuppressWarnings("deprecation")
    @Override
    protected int dequeueOutputBufferInternal(MediaCodec.BufferInfo info, long timeoutUs) {
        int indexOrStatus = -1;

        try {
            indexOrStatus = mMediaCodec.dequeueOutputBuffer(info, timeoutUs);

            ByteBuffer codecOutputBuffer = null;
            if (indexOrStatus >= 0) {
                boolean isConfigFrame = (info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0;
                if (isConfigFrame) {
                    Log.d(TAG, "Config frame generated. Offset: %d, size: %d", info.offset,
                            info.size);
                    codecOutputBuffer = getMediaCodecOutputBuffer(indexOrStatus);
                    codecOutputBuffer.position(info.offset);
                    codecOutputBuffer.limit(info.offset + info.size);

                    mConfigData = ByteBuffer.allocateDirect(info.size);
                    mConfigData.put(codecOutputBuffer);
                    // Log few SPS header bytes to check profile and level.
                    StringBuilder spsData = new StringBuilder();
                    for (int i = 0; i < (info.size < 8 ? info.size : 8); i++) {
                        spsData.append(Integer.toHexString(mConfigData.get(i) & 0xff)).append(" ");
                    }
                    Log.i(TAG, "spsData: %s", spsData.toString());
                    mConfigData.rewind();
                    // Release buffer back.
                    mMediaCodec.releaseOutputBuffer(indexOrStatus, false);
                    // Query next output.
                    indexOrStatus = mMediaCodec.dequeueOutputBuffer(info, timeoutUs);
                }
            }

            if (indexOrStatus >= 0) {
                codecOutputBuffer = getMediaCodecOutputBuffer(indexOrStatus);
                codecOutputBuffer.position(info.offset);
                codecOutputBuffer.limit(info.offset + info.size);

                // Check key frame flag.
                boolean isKeyFrame = (info.flags & MediaCodec.BUFFER_FLAG_SYNC_FRAME) != 0;
                if (isKeyFrame) {
                    Log.d(TAG, "Key frame generated");
                }
                final ByteBuffer frameBuffer;
                if (isKeyFrame && mMime.equals(MimeTypes.VIDEO_H264)) {
                    Log.d(TAG, "Appending config frame of size %d to output buffer with size %d",
                            mConfigData.capacity(), info.size);
                    // For H.264 encoded key frame append SPS and PPS NALs at the start.
                    frameBuffer = ByteBuffer.allocateDirect(mConfigData.capacity() + info.size);
                    frameBuffer.put(mConfigData);
                    info.offset = 0;
                    info.size += mConfigData.capacity();
                } else {
                    frameBuffer = ByteBuffer.allocateDirect(info.offset + info.size);
                    frameBuffer.position(info.offset);
                }
                frameBuffer.put(codecOutputBuffer);
                frameBuffer.rewind();
                mOutputBuffers.put(indexOrStatus, frameBuffer.duplicate());
            }
        } catch (IllegalStateException e) {
            Log.e(TAG, "Failed to dequeue output buffer", e);
        }

        return indexOrStatus;
    }

    // Call this function with catching IllegalStateException.
    @SuppressWarnings("deprecation")
    private ByteBuffer getMediaCodecOutputBuffer(int index) {
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT) {
            return mMediaCodec.getOutputBuffers()[index];
        }

        ByteBuffer outputBuffer = mMediaCodec.getOutputBuffer(index);
        if (outputBuffer == null) {
            throw new IllegalStateException("Got null output buffer");
        }
        return outputBuffer;
    }
}
