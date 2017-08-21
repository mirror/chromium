// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.component_updater;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.components.component_updater.ApkSignatureSchemeV2Verifier.SignatureNotFoundException;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.security.NoSuchAlgorithmException;
import java.security.PublicKey;
import java.security.spec.InvalidKeySpecException;

/**
 *
 */
@JNINamespace("component_updater")
public class ComponentUpdater {
    /**
     * Verifies the given APK is signed with a certificate that has the provided public key.
     */
    @CalledByNative
    public static boolean verify(String apkFile, PublicKey pk) throws IOException {
        X509Certificate[][] certs;
        try {
            certs = ApkSignatureSchemeV2Verifier.verify(apkFile);
        } catch (SignatureNotFoundException | SecurityException e) {
            return false;
        }
        for (X509Certificate[] chain : certs) {
            for (X509Certificate cert : chain) {
                try {
                    cert.verify(pk);
                    return true;
                } catch (Exception e) {
                }
            }
        }
        return false;
    }

    /**
     * Returns the public key contained in the given X.509 certificate.
     */
    @CalledByNative
    public static PublicKey loadPublicKey(String certFile)
            throws FileNotFoundException, IOException, NoSuchAlgorithmException,
                   InvalidKeySpecException, CertificateException {
        CertificateFactory certFactory = CertificateFactory.getInstance("X.509");
        FileInputStream inputStream = new FileInputStream(certFile);
        X509Certificate cert = (X509Certificate) certFactory.generateCertificate(inputStream);
        return cert.getPublicKey();
    }
}