// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.update_client;

import org.chromium.base.annotations.CalledByNativeUnchecked;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.third_party.android.util.apk.ApkSignatureSchemeV2Verifier;
import org.chromium.third_party.android.util.apk.ApkSignatureSchemeV2Verifier.SignatureNotFoundException;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.security.GeneralSecurityException;
import java.security.InvalidKeyException;
import java.security.PublicKey;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;

/**
 * Utility methods for verifying APK packages.
 */
@JNINamespace("update_client")
public class ApkUtils {
    /**
     * Verifies the given APK is signed with a certificate that has the provided public key.
     *
     * @param publicKey The public key associated with a certificate that was used to sign the APK.
     */
    @CalledByNativeUnchecked
    public static void verify(RandomAccessFile apkFile, PublicKey publicKey)
            throws IOException, SignatureNotFoundException, GeneralSecurityException {
        X509Certificate[][] certChains = ApkSignatureSchemeV2Verifier.verify(apkFile);
        assert certChains.length > 0;
        // If there is more than one certificate, and none match the given public key, we'll throw
        // the InvalidKeyException for the last certificate.
        InvalidKeyException lastException = null;
        for (X509Certificate[] chain : certChains) {
            assert chain.length > 0;
            for (X509Certificate cert : chain) {
                try {
                    cert.verify(publicKey);
                    return;
                } catch (InvalidKeyException e) {
                    lastException = e;
                }
            }
        }
        // lastException should never be null. It could only be null if certChains contained no
        // certificates, which ApkSignatureSchemeV2Verifier.verify won't allow.
        assert lastException != null;
        throw lastException;
    }

    /**
     * Returns the public key contained in the given X.509 certificate.
     *
     * @param certFile The path to the certificate file from which to load the PublicKey.
     */
    @CalledByNativeUnchecked
    public static PublicKey loadPublicKey(String certFile)
            throws IOException, CertificateException {
        CertificateFactory certFactory = CertificateFactory.getInstance("X.509");
        try (FileInputStream inputStream = new FileInputStream(certFile)) {
            X509Certificate cert = (X509Certificate) certFactory.generateCertificate(inputStream);
            return cert.getPublicKey();
        }
    }
}