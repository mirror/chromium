// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

import android.media.MediaCodec;
import android.os.Build;
import android.util.SparseArray;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.MainDex;
import org.chromium.media.MediaCodecUtil.BitrateAdjustmentTypes;
import org.chromium.media.MediaCodecUtil.MimeTypes;

import java.nio.ByteBuffer;

/**
 * A MediaCodec video encoder wrapper for adapting the API and catching exceptions.
 */
// TODO(braveyao): Move all the encoder implementations into this file and make it
// an independent class.
@JNINamespace("media")
class MediaCodecEncoder extends MediaCodecBridge {
    private static final String TAG = "cr_MediaCodecEncoder";

    private MediaCodec mMediaCodec;
    private boolean mFlushed;
    private long mLastPresentationTimeUs;
    private String mMime;
    private boolean mAdaptivePlaybackSupported;

    private BitrateAdjustmentTypes mBitrateAdjustmentType = BitrateAdjustmentTypes.NO_ADJUSTMENT;

    private SparseArray<ByteBuffer> mOutputFrameBuffers;
    // SPS and PPS NALs (Config frame) for H.264
    private ByteBuffer mConfigData = null;

    @MainDex
    private static class EncoderDequeueOutputResult extends DequeueOutputResult {
        private final int mStatus;
        private final int mIndex;
        private final int mFlags;
        private final int mOffset;
        private final long mPresentationTimeMicroseconds;
        private final int mNumBytes;

        private EncoderDequeueOutputResult(int status, int index, int flags, int offset,
                long presentationTimeMicroseconds, int numBytes) {
            super(status, index, flags, offset, presentationTimeMicroseconds, numBytes);
            mStatus = status;
            mIndex = index;
            mFlags = flags;
            mOffset = offset;
            mPresentationTimeMicroseconds = presentationTimeMicroseconds;
            mNumBytes = numBytes;
        }

        @CalledByNative("DequeueOutputResult")
        private int status() {
            return mStatus;
        }

        @CalledByNative("DequeueOutputResult")
        private int index() {
            return mIndex;
        }

        @CalledByNative("DequeueOutputResult")
        private int flags() {
            return mFlags;
        }

        @CalledByNative("DequeueOutputResult")
        private int offset() {
            return mOffset;
        }

        @CalledByNative("DequeueOutputResult")
        private long presentationTimeMicroseconds() {
            return mPresentationTimeMicroseconds;
        }

        @CalledByNative("DequeueOutputResult")
        private int numBytes() {
            return mNumBytes;
        }
    }

    private MediaCodecEncoder(MediaCodec mediaCodec, String mime,
            boolean adaptivePlaybackSupported, BitrateAdjustmentTypes bitrateAdjustmentType) {
        super(mediaCodec, mime, adaptivePlaybackSupported, bitrateAdjustmentType);
        assert mediaCodec != null;
        mMediaCodec = mediaCodec;
        mMime = mime;
        mLastPresentationTimeUs = 0;
        mFlushed = true;
        mAdaptivePlaybackSupported = adaptivePlaybackSupported;
        mBitrateAdjustmentType = bitrateAdjustmentType;

        mOutputFrameBuffers = new SparseArray<>();
    }

    @CalledByNative
    public static MediaCodecEncoder create(String mime) {
        MediaCodecUtil.CodecCreationInfo info = new MediaCodecUtil.CodecCreationInfo();
        try {
            info = MediaCodecUtil.createEncoder(mime);
        } catch (Exception e) {
            Log.e(TAG, "Failed to create MediaCodec: %s", mime);
        }

        if (info.mediaCodec == null) return null;

        return new MediaCodecEncoder(
                info.mediaCodec, mime, info.supportsAdaptivePlayback, info.bitrateAdjustmentType);
    }

    @CalledByNative
    public ByteBuffer getOutputBuffer(int index) {
        return mOutputFrameBuffers.get(index);
    }

    @CalledByNative
    public void releaseOutputBuffer(int index, boolean render) {
        try {
            mMediaCodec.releaseOutputBuffer(index, render);
            mOutputFrameBuffers.remove(index);
        } catch (IllegalStateException e) {
            Log.e(TAG, "Failed to release output buffer", e);
        }
    }

    @SuppressWarnings("deprecation")
    @CalledByNative
    public EncoderDequeueOutputResult dequeueOutputBuffer(long timeoutUs) {
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        int status = MediaCodecStatus.ERROR;
        int outputBufferSize = 0;
        int outputBufferOffset = 0;
        int index = -1;

        try {
            int indexOrStatus = mMediaCodec.dequeueOutputBuffer(info, timeoutUs);

            ByteBuffer codecOutputBuffer = null;
            if (indexOrStatus >= 0) {
                boolean isConfigFrame = (info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0;
                if (isConfigFrame) {
                    Log.i(TAG,
                            "Config frame generated. Offset: " + info.offset
                                    + ". Size: " + info.size);
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
                    Log.i(TAG, spsData.toString());
                    mConfigData.rewind();
                    // Release buffer back.
                    mMediaCodec.releaseOutputBuffer(indexOrStatus, false);
                    // Query next output.
                    indexOrStatus = mMediaCodec.dequeueOutputBuffer(info, timeoutUs);
                }
            }

            if (info.presentationTimeUs < mLastPresentationTimeUs) {
                info.presentationTimeUs = mLastPresentationTimeUs;
            }

            mLastPresentationTimeUs = info.presentationTimeUs;

            if (indexOrStatus >= 0) { // index!
                status = MediaCodecStatus.OK;
                index = indexOrStatus;

                codecOutputBuffer = getMediaCodecOutputBuffer(index);
                codecOutputBuffer.position(info.offset);
                codecOutputBuffer.limit(info.offset + info.size);
                outputBufferOffset = info.offset;
                outputBufferSize = info.size;

                // Check key frame flag.
                boolean isKeyFrame = (info.flags & MediaCodec.BUFFER_FLAG_SYNC_FRAME) != 0;
                if (isKeyFrame) {
                    Log.d(TAG, "Key frame generated");
                }
                final ByteBuffer frameBuffer;
                if (isKeyFrame && mMime.equals(MimeTypes.VIDEO_H264)) {
                    Log.d(TAG,
                            "Appending config frame of size " + mConfigData.capacity()
                                    + " to output buffer with offset " + info.offset + ", size "
                                    + info.size);
                    // For H.264 encoded key frame append SPS and PPS NALs at the start.
                    frameBuffer = ByteBuffer.allocateDirect(mConfigData.capacity() + info.size);
                    frameBuffer.put(mConfigData);
                    outputBufferOffset = 0;
                    outputBufferSize += mConfigData.capacity();
                } else {
                    frameBuffer = ByteBuffer.allocateDirect(outputBufferOffset + outputBufferSize);
                    frameBuffer.position(outputBufferOffset);
                }
                frameBuffer.put(codecOutputBuffer);
                frameBuffer.rewind();
                mOutputFrameBuffers.put(index, frameBuffer.duplicate());
            } else if (indexOrStatus == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                status = MediaCodecStatus.OUTPUT_BUFFERS_CHANGED;
            } else if (indexOrStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                status = MediaCodecStatus.OUTPUT_FORMAT_CHANGED;
            } else if (indexOrStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
                status = MediaCodecStatus.TRY_AGAIN_LATER;
            } else {
                Log.e(TAG, "Unexpected index_or_status: " + indexOrStatus);
                assert false;
            }
        } catch (IllegalStateException e) {
            status = MediaCodecStatus.ERROR;
            Log.e(TAG, "Failed to dequeue output buffer", e);
        }

        return new EncoderDequeueOutputResult(status, index, info.flags, outputBufferOffset,
                info.presentationTimeUs, outputBufferSize);
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
