// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_string.h"
#include "base/logging.h"
#include "chrome/browser/certificate_viewer.h"
#include "chrome/grit/generated_resources.h"
#include "jni/CertificateViewer_jni.h"
#include "net/cert/x509_certificate.h"
#include "ui/base/l10n/l10n_util.h"

using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

void ShowCertificateViewer(content::WebContents* web_contents,
                           gfx::NativeWindow parent,
                           net::X509Certificate* cert) {
  // For Android, showing the certificate is always handled in Java.
  NOTREACHED();
}

static ScopedJavaLocalRef<jstring> CertificateViewer__GetCertIssuedToText(
    JNIEnv* env,
    const JavaParamRef<jclass>&) {
  return ConvertUTF8ToJavaString(
      env, l10n_util::GetStringUTF8(IDS_CERT_INFO_SUBJECT_GROUP));
}

static ScopedJavaLocalRef<jstring> CertificateViewer__GetCertInfoCommonNameText(
    JNIEnv* env,
    const JavaParamRef<jclass>&) {
  return ConvertUTF8ToJavaString(
      env, l10n_util::GetStringUTF8(IDS_CERT_INFO_COMMON_NAME_LABEL));
}

static ScopedJavaLocalRef<jstring> CertificateViewer__GetCertInfoOrganizationText(
    JNIEnv* env,
    const JavaParamRef<jclass>&) {
  return ConvertUTF8ToJavaString(
      env, l10n_util::GetStringUTF8(IDS_CERT_INFO_ORGANIZATION_LABEL));
}

static ScopedJavaLocalRef<jstring> CertificateViewer__GetCertInfoSerialNumberText(
    JNIEnv* env,
    const JavaParamRef<jclass>&) {
  return ConvertUTF8ToJavaString(
      env, l10n_util::GetStringUTF8(IDS_CERT_INFO_SERIAL_NUMBER_LABEL));
}

static ScopedJavaLocalRef<jstring> CertificateViewer__GetCertInfoOrganizationUnitText(
    JNIEnv* env,
    const JavaParamRef<jclass>&) {
  return ConvertUTF8ToJavaString(
      env, l10n_util::GetStringUTF8(IDS_CERT_INFO_ORGANIZATIONAL_UNIT_LABEL));
}

static ScopedJavaLocalRef<jstring> CertificateViewer__GetCertIssuedByText(
    JNIEnv* env,
    const JavaParamRef<jclass>&) {
  return ConvertUTF8ToJavaString(
      env, l10n_util::GetStringUTF8(IDS_CERT_INFO_ISSUER_GROUP));
}

static ScopedJavaLocalRef<jstring> CertificateViewer__GetCertValidityText(
    JNIEnv* env,
    const JavaParamRef<jclass>&) {
  return ConvertUTF8ToJavaString(
      env, l10n_util::GetStringUTF8(IDS_CERT_INFO_VALIDITY_GROUP));
}

static ScopedJavaLocalRef<jstring> CertificateViewer__GetCertIssuedOnText(
    JNIEnv* env,
    const JavaParamRef<jclass>&) {
  return ConvertUTF8ToJavaString(
      env, l10n_util::GetStringUTF8(IDS_CERT_INFO_ISSUED_ON_LABEL));
}

static ScopedJavaLocalRef<jstring> CertificateViewer__GetCertExpiresOnText(
    JNIEnv* env,
    const JavaParamRef<jclass>&) {
  return ConvertUTF8ToJavaString(
      env, l10n_util::GetStringUTF8(IDS_CERT_INFO_EXPIRES_ON_LABEL));
}

static ScopedJavaLocalRef<jstring> CertificateViewer__GetCertFingerprintsText(
    JNIEnv* env,
    const JavaParamRef<jclass>&) {
  return ConvertUTF8ToJavaString(
      env, l10n_util::GetStringUTF8(IDS_CERT_INFO_FINGERPRINTS_GROUP));
}

static ScopedJavaLocalRef<jstring> CertificateViewer__GetCertSHA256FingerprintText(
    JNIEnv* env,
    const JavaParamRef<jclass>&) {
  return ConvertUTF8ToJavaString(
      env, l10n_util::GetStringUTF8(IDS_CERT_INFO_SHA256_FINGERPRINT_LABEL));
}

static ScopedJavaLocalRef<jstring> CertificateViewer__GetCertSHA1FingerprintText(
    JNIEnv* env,
    const JavaParamRef<jclass>&) {
  return ConvertUTF8ToJavaString(
      env, l10n_util::GetStringUTF8(IDS_CERT_INFO_SHA1_FINGERPRINT_LABEL));
}
