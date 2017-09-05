// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.update_client;

import org.chromium.base.annotations.CalledByNativeUnchecked;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.third_party.android.util.apk.ApkSignatureSchemeV2Verifier;
import org.chromium.third_party.android.util.apk.ApkSignatureSchemeV2Verifier.SignatureNotFoundException;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.PublicKey;
import java.security.SignatureException;
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
     * @param apkFile The path to the APK to verify.
     * @param publicKey The public key associated with a certificate that was used to sign the APK.
     */
    @CalledByNativeUnchecked
    public static void verify(String apkFile, PublicKey publicKey)
            throws IOException, SignatureNotFoundException, SecurityException,
                   NoSuchAlgorithmException, InvalidKeyException, NoSuchProviderException,
                   SignatureException, CertificateException {
        X509Certificate[][] certChains = ApkSignatureSchemeV2Verifier.verify(apkFile);
        // If there is more than one certificate, and none match the given public key, we'll throw
        // the InvalidKeyException for the last certificate.
        InvalidKeyException lastException = null;
        for (X509Certificate[] chain : certChains) {
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
        throw lastException;
    }

    /**
     * Returns the public key contained in the given X.509 certificate.
     *
     * @param certFile The path to the certificate file from which to load the PublicKey.
     * @return The certificate's public key.
     */
    @CalledByNativeUnchecked
    public static PublicKey loadPublicKey(String certFile)
            throws FileNotFoundException, IOException, CertificateException {
        CertificateFactory certFactory = CertificateFactory.getInstance("X.509");
        FileInputStream inputStream = new FileInputStream(certFile);
        try {
            X509Certificate cert = (X509Certificate) certFactory.generateCertificate(inputStream);
            return cert.getPublicKey();
        } finally {
            inputStream.close();
        }
    }
}