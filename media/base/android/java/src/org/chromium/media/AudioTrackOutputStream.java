// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

import android.annotation.SuppressLint;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Build;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.nio.ByteBuffer;

@JNINamespace("media")
class AudioTrackOutputStream {
    static class AudioBufferInfo {
        private final int mFrames;
        private final int mSize;

        public AudioBufferInfo(int frames, int size) {
            mFrames = frames;
            mSize = size;
        }

        public int getFrames() {
            return mFrames;
        }

        public int getSize() {
            return mSize;
        }
    }

    // Provide dependency injection points for unit tests.
    interface Callback {
        int getMinBufferSize(int sampleRateInHz, int channelConfig, int audioFormat);
        AudioTrack createAudioTrack(int streamType, int sampleRateInHz, int channelConfig,
                int audioFormat, int bufferSizeInBytes, int mode);
        AudioBufferInfo onMoreData(ByteBuffer audioData, long delayInFrames);
        long getAddress(ByteBuffer byteBuffer);
    }

    private static final String TAG = "AudioTrackOutput";
    // Must be the same as AudioBus::kChannelAlignment.
    private static final int CHANNEL_ALIGNMENT = 16;

    private long mNativeAudioTrackOutputStream;
    private Callback mCallback;
    private AudioTrack mAudioTrack;
    private int mBufferSizeInBytes;
    private WorkerThread mWorkerThread;

    private int mLastPlaybackHeadPosition;
    private long mTotalPlayedFrames;
    private long mTotalReadFrames;

    private ByteBuffer mReadBuffer;
    private ByteBuffer mWriteBuffer;
    private byte[] mWriteByteArray;
    private int mWrittenSize;
    private int mLeftSize;

    class WorkerThread extends Thread {
        private volatile boolean mDone = false;

        public void finish() {
            mDone = true;
        }

        public void run() {
            // This should not be a busy loop, since the thread would be blocked in either
            // AudioSyncReader::WaitUntilDataIsReady() or AudioTrack.write().
            while (!mDone) {
                int left = writeData();
                // AudioTrack.write() failed, exit the run loop.
                if (left < 0) break;
                // Only partial data is written, retry again.
                if (left > 0) continue;

                readMoreData();
            }
        }
    }

    @CalledByNative
    private static AudioTrackOutputStream create() {
        return new AudioTrackOutputStream(null);
    }

    @VisibleForTesting
    static AudioTrackOutputStream create(Callback callback) {
        return new AudioTrackOutputStream(callback);
    }

    private AudioTrackOutputStream(Callback callback) {
        mCallback = callback;
        if (mCallback != null) return;

        mCallback = new Callback() {
            @Override
            public int getMinBufferSize(int sampleRateInHz, int channelConfig, int audioFormat) {
                return AudioTrack.getMinBufferSize(sampleRateInHz, channelConfig, audioFormat);
            }

            @Override
            public AudioTrack createAudioTrack(int streamType, int sampleRateInHz,
                    int channelConfig, int audioFormat, int bufferSizeInBytes, int mode) {
                return new AudioTrack(streamType, sampleRateInHz, channelConfig, audioFormat,
                        bufferSizeInBytes, mode);
            }

            @Override
            public AudioBufferInfo onMoreData(ByteBuffer audioData, long delayInFrames) {
                return nativeOnMoreData(mNativeAudioTrackOutputStream, audioData, delayInFrames);
            }

            @Override
            public long getAddress(ByteBuffer byteBuffer) {
                return nativeGetAddress(mNativeAudioTrackOutputStream, byteBuffer);
            }
        };
    }

    @SuppressWarnings("deprecation")
    private int getChannelConfig(int channelCount) {
        switch (channelCount) {
            case 1:
                return AudioFormat.CHANNEL_OUT_MONO;
            case 2:
                return AudioFormat.CHANNEL_OUT_STEREO;
            case 4:
                return AudioFormat.CHANNEL_OUT_QUAD;
            case 6:
                return AudioFormat.CHANNEL_OUT_5POINT1;
            case 8:
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    return AudioFormat.CHANNEL_OUT_7POINT1_SURROUND;
                } else {
                    return AudioFormat.CHANNEL_OUT_7POINT1;
                }
            default:
                return AudioFormat.CHANNEL_OUT_DEFAULT;
        }
    }

    @CalledByNative
    boolean open(int channelCount, int sampleRate, int sampleFormat) {
        assert mAudioTrack == null;

        int channelConfig = getChannelConfig(channelCount);
        mBufferSizeInBytes =
                4 * mCallback.getMinBufferSize(sampleRate, channelConfig, sampleFormat);

        try {
            Log.d(TAG, "Crate AudioTrack with sample rate:%d, channel:%d, format:%d ", sampleRate,
                    channelConfig, sampleFormat);

            mAudioTrack = mCallback.createAudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
                    channelConfig, sampleFormat, mBufferSizeInBytes, AudioTrack.MODE_STREAM);
            assert mAudioTrack != null;
        } catch (IllegalArgumentException ile) {
            Log.e(TAG, "Exception creating AudioTrack for playback: ", ile);
            return false;
        }

        if (mAudioTrack.getState() == AudioTrack.STATE_UNINITIALIZED) {
            Log.e(TAG, "Cannot create AudioTrack");
            mAudioTrack = null;
            return false;
        }

        mLastPlaybackHeadPosition = 0;
        mTotalPlayedFrames = 0;
        return true;
    }

    private ByteBuffer allocateAlignedByteBuffer(int capacity, int alignment) {
        int mask = alignment - 1;
        ByteBuffer buffer = ByteBuffer.allocateDirect(capacity + mask);
        long address = mCallback.getAddress(buffer);

        int offset = (alignment - (int) (address & mask)) & mask;
        buffer.position(offset);
        buffer.limit(offset + capacity);
        return buffer.slice();
    }

    @CalledByNative
    void start(long nativeAudioTrackOutputStream) {
        Log.d(TAG, "AudioTrackOutputStream.start()");
        if (mWorkerThread != null) return;

        mNativeAudioTrackOutputStream = nativeAudioTrackOutputStream;
        mTotalReadFrames = 0;

        mReadBuffer = allocateAlignedByteBuffer(mBufferSizeInBytes, CHANNEL_ALIGNMENT);
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP && !mReadBuffer.hasArray()) {
            mWriteByteArray = new byte[mBufferSizeInBytes];
        }

        mAudioTrack.play();

        mWorkerThread = new WorkerThread();
        mWorkerThread.start();
    }

    @CalledByNative
    void stop() {
        Log.d(TAG, "AudioTrackOutputStream.stop()");
        if (mWorkerThread != null) {
            mWorkerThread.finish();
            try {
                mWorkerThread.interrupt();
                mWorkerThread.join();
            } catch (SecurityException e) {
                Log.e(TAG, "Exception while waiting for AudioTrack worker thread finished: ", e);
            } catch (InterruptedException e) {
                Log.e(TAG, "Exception while waiting for AudioTrack worker thread finished: ", e);
            }
            mWorkerThread = null;
        }

        mAudioTrack.pause();
        mAudioTrack.flush();
        mLastPlaybackHeadPosition = 0;
        mTotalPlayedFrames = 0;
        mNativeAudioTrackOutputStream = 0;
    }

    @SuppressWarnings("deprecation")
    @CalledByNative
    void setVolume(double volume) {
        // Chrome sends the volume in the range [0, 1.0], whereas Android
        // expects the volume to be within [0, getMaxVolume()].
        float scaledVolume = (float) (volume * mAudioTrack.getMaxVolume());
        mAudioTrack.setStereoVolume(scaledVolume, scaledVolume);
    }

    @CalledByNative
    void close() {
        Log.d(TAG, "AudioTrackOutputStream.close()");
        if (mAudioTrack != null) {
            mAudioTrack.release();
            mAudioTrack = null;
        }
    }

    @CalledByNative
    AudioBufferInfo createAudioBufferInfo(int frames, int size) {
        return new AudioBufferInfo(frames, size);
    }

    private boolean readMoreData() {
        assert mNativeAudioTrackOutputStream != 0;

        int position = mAudioTrack.getPlaybackHeadPosition();
        mTotalPlayedFrames += position - mLastPlaybackHeadPosition;
        mLastPlaybackHeadPosition = position;

        long delayInFrames = mTotalReadFrames - mTotalPlayedFrames;
        if (delayInFrames < 0) delayInFrames = 0;

        AudioBufferInfo info = mCallback.onMoreData(mReadBuffer.duplicate(), delayInFrames);
        if (info == null || info.getSize() <= 0) return false;

        mTotalReadFrames += info.getFrames();

        mWriteBuffer = mReadBuffer.asReadOnlyBuffer();
        if (mWriteByteArray != null) {
            mWriteBuffer.get(mWriteByteArray, 0, info.getSize());
        }
        mWrittenSize = 0;
        mLeftSize = info.getSize();

        return true;
    }

    private int writeData() {
        if (mLeftSize == 0) return 0;

        int written = writeAudioTrack();
        if (written < 0) {
            Log.e(TAG, "AudioTrack.write() failed. Error:" + written);
            return written;
        }

        assert mLeftSize >= written;
        mLeftSize -= written;
        mWrittenSize += written;

        return mLeftSize;
    }

    @SuppressLint("NewApi")
    private int writeAudioTrack() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            return mAudioTrack.write(mWriteBuffer, mLeftSize, AudioTrack.WRITE_BLOCKING);
        }

        if (mWriteBuffer.hasArray()) {
            return mAudioTrack.write(
                    mWriteBuffer.array(), mWriteBuffer.arrayOffset() + mWrittenSize, mLeftSize);
        }

        assert mWriteByteArray != null;
        return mAudioTrack.write(mWriteByteArray, mWrittenSize, mLeftSize);
    }

    private native AudioBufferInfo nativeOnMoreData(
            long nativeAudioTrackOutputStream, ByteBuffer audioData, long delayInFrames);
    private native void nativeOnError(long nativeAudioTrackOutputStream);
    private native long nativeGetAddress(long nativeAudioTrackOutputStream, ByteBuffer byteBuffer);
}
