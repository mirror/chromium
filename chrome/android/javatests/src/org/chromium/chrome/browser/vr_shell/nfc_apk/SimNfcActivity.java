// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.nfc_apk;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.os.Bundle;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Activity to simulate an NFC scan of a Daydream View VR headset. This is so
 * that tests cannot interact directly with Java, e.g. Telemetry tests, can
 * still force Chrome to enter VR. Essentially a copy-paste of VrUtils.java's
 * NFC emulation, but as minimal as possible.
 */
public class SimNfcActivity extends Activity {
    private static final String APPLICATION_RECORD_STRING = "com.google.vr.vrcore";
    private static final String RESERVED_KEY = "google.vr/rsvd";
    private static final String VERSION_KEY = "google.vr/ver";
    private static final String DATA_KEY = "google.vr/data";
    private static final ByteOrder BYTE_ORDER = ByteOrder.BIG_ENDIAN;
    // Hard coded viewer ID (0x03) instead of using NfcParams and converting
    // to a byte array because NfcParams were removed from the GVR SDK
    private static final byte[] PROTO_BYTES = new byte[] {(byte) 0x08, (byte) 0x03};
    private static final int VERSION = 123;
    private static final int RESERVED = 456;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Make the NFC intent
        Intent intent = new Intent(NfcAdapter.ACTION_NDEF_DISCOVERED);
        NdefMessage[] messages = new NdefMessage[] {new NdefMessage(
                new NdefRecord[] {NdefRecord.createMime(RESERVED_KEY, intToByteArray(RESERVED)),
                        NdefRecord.createMime(VERSION_KEY, intToByteArray(VERSION)),
                        NdefRecord.createMime(DATA_KEY, PROTO_BYTES),
                        NdefRecord.createApplicationRecord(APPLICATION_RECORD_STRING)})};
        intent.putExtra(NfcAdapter.EXTRA_NDEF_MESSAGES, messages);
        intent.setComponent(new ComponentName(APPLICATION_RECORD_STRING,
                APPLICATION_RECORD_STRING + ".nfc.ViewerDetectionActivity"));

        // Send off intent and close this application
        startActivity(intent);
        finish();
    }

    private byte[] intToByteArray(int i) {
        final ByteBuffer bb = ByteBuffer.allocate(Integer.SIZE / Byte.SIZE);
        bb.order(BYTE_ORDER);
        bb.putInt(i);
        return bb.array();
    }
}
