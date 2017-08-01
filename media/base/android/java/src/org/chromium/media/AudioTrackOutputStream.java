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
    // Provide dependency injection points for unit tests.
    interface Callback {
        int getMinBufferSize(int sampleRateInHz, int channelConfig, int audioFormat);
        AudioTrack createAudioTrack(int streamType, int sampleRateInHz, int channelConfig,
                int audioFormat, int bufferSizeInBytes, int mode);
        int onMoreData(ByteBuffer audioData, long totalPlayedFrames);
    }

    private static final String TAG = "AudioTrackOutput";
    private Callback mCallback;
    private AudioTrack mAudioTrack;
    private long mNativeAudioTrackOutputStream;
    private int mBufferSizeInBytes;

    private int mLastPlaybackHeadPosition;
    private long mTotalPlayedFrames;

    private ByteBuffer mAudioBuffer;
    private byte[] mAudioByteArray;
    private WorkerThread mWorkerThread;

    class WorkerThread extends Thread {
        private volatile boolean mDone = false;

        public void finish() {
            mDone = true;
        }

        public void run() {
            while (!mDone) {
                if (!readMoreData()) {
                    msleep(10);
                }
            }
        }

        private void msleep(int msec) {
            try {
                Thread.sleep(msec);
            } catch (InterruptedException e) {
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
        if (mCallback == null) {
            mCallback = new Callback() {
                @Override
                public int getMinBufferSize(
                        int sampleRateInHz, int channelConfig, int audioFormat) {
                    return AudioTrack.getMinBufferSize(sampleRateInHz, channelConfig, audioFormat);
                }

                @Override
                public AudioTrack createAudioTrack(int streamType, int sampleRateInHz,
                        int channelConfig, int audioFormat, int bufferSizeInBytes, int mode) {
                    return new AudioTrack(streamType, sampleRateInHz, channelConfig, audioFormat,
                            bufferSizeInBytes, mode);
                }

                @Override
                public int onMoreData(ByteBuffer audioData, long totalPlayedFrames) {
                    return nativeOnMoreData(
                            mNativeAudioTrackOutputStream, audioData, totalPlayedFrames);
                }
            };
        }
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
        int channelConfig = getChannelConfig(channelCount);
        mBufferSizeInBytes =
                4 * mCallback.getMinBufferSize(sampleRate, channelConfig, sampleFormat);

        assert mAudioTrack == null;

        try {
            Log.d(TAG, "Crate AudioTrack with sample rate:%d, channel:%d, format:%d ", sampleRate,
                    channelConfig, sampleFormat);

            mAudioTrack = mCallback.createAudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
                    channelConfig, sampleFormat, mBufferSizeInBytes, AudioTrack.MODE_STREAM);
            assert mAudioTrack != null;
            mLastPlaybackHeadPosition = 0;
            mTotalPlayedFrames = 0;
        } catch (IllegalArgumentException ile) {
            Log.e(TAG, "Exception creating AudioTrack for playback: ", ile);
            return false;
        }

        if (mAudioTrack.getState() == AudioTrack.STATE_UNINITIALIZED) {
            Log.e(TAG, "Cannot create AudioTrack");
            mAudioTrack = null;
            return false;
        }

        return true;
    }

    @CalledByNative
    void start(long nativeAudioTrackOutputStream) {
        Log.d(TAG, "AudioTrackOutputStream.start()");
        if (mWorkerThread != null) return;

        mNativeAudioTrackOutputStream = nativeAudioTrackOutputStream;

        mAudioBuffer = ByteBuffer.allocateDirect(mBufferSizeInBytes);
        if (!mAudioBuffer.hasArray()) {
            mAudioByteArray = new byte[mBufferSizeInBytes];
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

    private boolean readMoreData() {
        if (mNativeAudioTrackOutputStream == 0) return false;

        int position = mAudioTrack.getPlaybackHeadPosition();
        mTotalPlayedFrames += position - mLastPlaybackHeadPosition;
        mLastPlaybackHeadPosition = position;

        int size = mCallback.onMoreData(mAudioBuffer, mTotalPlayedFrames);
        if (size <= 0) {
            return false;
        }

        ByteBuffer readOnlyBuffer = mAudioBuffer.asReadOnlyBuffer();
        int result = writeAudio(readOnlyBuffer, size);

        if (result < 0) {
            Log.e(TAG, "AudioTrack.write() failed. Error:" + result);
            return false;
        } else if (result != size) {
            Log.e(TAG, "AudioTrack.write() incomplete. Data size: %d, written size: %d", size,
                    result);
            return false;
        }

        return true;
    }

    @SuppressLint("NewApi")
    private int writeAudio(ByteBuffer buffer, int size) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            return mAudioTrack.write(buffer, size, AudioTrack.WRITE_BLOCKING);
        } else {
            if (buffer.hasArray()) {
                return mAudioTrack.write(buffer.array(), buffer.arrayOffset(), size);
            } else {
                buffer.get(mAudioByteArray);
                return mAudioTrack.write(mAudioByteArray, 0, size);
            }
        }
    }

    private native int nativeOnMoreData(
            long nativeAudioTrackOutputStream, ByteBuffer audioData, long totalPlayedFrames);
    private native void nativeOnError(long nativeAudioTrackOutputStream);
}
