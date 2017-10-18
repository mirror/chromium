// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.payments;

import java.util.Arrays;

/** Java equivalent of components/payments/content/web_app_manifest_section.h */
public final class WebAppManifestSection {
    public WebAppManifestSection(String id, long minVersion, int numberOfFingerprints) {
        this.id = id;
        this.minVersion = minVersion;
        fingerprints = new byte[numberOfFingerprints][];
    }

    public WebAppManifestSection(String id, long minVersion, byte[][] fingerprints) {
        this.id = id;
        this.minVersion = minVersion;
        // Copy the array so FindBugs does not complain of exposing private data.
        this.fingerprints = Arrays.copyOf(fingerprints, fingerprints.length);
    }

    // The package name of the app.
    public final String id;

    // Minimum version number of the app.
    public final long minVersion;

    // The result of SHA256(signing certificate bytes) for each certificate in the
    // app.
    public final byte[][] fingerprints;
}
