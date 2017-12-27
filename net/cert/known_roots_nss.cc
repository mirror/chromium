// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/known_roots_nss.h"

#include <cert.h>
#include <dlfcn.h>
#include <pk11pub.h>
#include <secmod.h>

#include <memory>

#include "base/memory/protected_memory.h"
#include "base/memory/protected_memory_cfi.h"
#include "crypto/nss_util_internal.h"
#include "net/base/hash_value.h"
#include "net/cert/x509_util_nss.h"

namespace net {

namespace {

#if !defined(CKA_NSS_MOZILLA_CA_POLICY)
#define CKA_NSS_MOZILLA_CA_POLICY (CKA_NSS + 34)
#endif

using PK11HasAttributeSetFunction = CK_BBOOL (*)(PK11SlotInfo* slot,
                                                 CK_OBJECT_HANDLE id,
                                                 CK_ATTRIBUTE_TYPE type,
                                                 PRBool haslock);
static PROTECTED_MEMORY_SECTION
    base::ProtectedMemory<PK11HasAttributeSetFunction>
        g_pk11_has_attribute_set;

const base::ProtectedMemory<PK11HasAttributeSetFunction>&
ResolvePK11HasAttributeSet() {
  static base::ProtectedMemory<PK11HasAttributeSetFunction>::Initializer init(
      &g_pk11_has_attribute_set,
      reinterpret_cast<PK11HasAttributeSetFunction>(
          dlsym(RTLD_DEFAULT, "PK11_HasAttributeSet")));
  return g_pk11_has_attribute_set;
}

}  // namespace

// IsKnownRoot returns true if the given certificate is one that we believe
// is a standard (as opposed to user-installed) root.
bool IsKnownRoot(CERTCertificate* root) {
  if (!root || !root->slot)
    return false;

  bool known_root = false;
  if (*ResolvePK11HasAttributeSet() != nullptr) {
    // Historically, the set of root certs was determined based on whether or
    // not it was part of nssckbi.[so,dll], the read-only PKCS#11 module that
    // exported the certs with trust settings. However, some distributions,
    // notably those in the Red Hat family, replace nssckbi with a redirect to
    // their own store, such as from p11kit, which can support more robust
    // trust settings, like per-system trust, admin-defined, and user-defined
    // trust.
    //
    // As a given certificate may exist in multiple modules and slots, scan
    // through all of the available modules, all of the (connected) slots on
    // those modules, and check to see if it has the CKA_NSS_MOZILLA_CA_POLICY
    // attribute set. This attribute indicates its from the upstream Mozilla
    // trust store, and these distributions preserve the attribute as a flag.
    crypto::AutoSECMODListReadLock lock_id;
    for (const SECMODModuleList* item = SECMOD_GetDefaultModuleList();
         !known_root && item != nullptr; item = item->next) {
      for (int i = 0; !known_root && i < item->module->slotCount; ++i) {
        PK11SlotInfo* slot = item->module->slots[i];
        if (PK11_IsPresent(slot) && PK11_HasRootCerts(slot)) {
          CK_OBJECT_HANDLE handle = PK11_FindCertInSlot(slot, root, nullptr);
          known_root = handle != CK_INVALID_HANDLE &&
                       UnsanitizedCfiCall(ResolvePK11HasAttributeSet())(
                           root->slot, handle, CKA_NSS_MOZILLA_CA_POLICY,
                           PR_FALSE) == CK_TRUE;
        }
      }
    }
  }

  // This magic name is taken from
  // http://bonsai.mozilla.org/cvsblame.cgi?file=mozilla/security/nss/lib/ckfw/builtins/constants.c&rev=1.13&mark=86,89#79
  return known_root ||
         0 == strcmp(PK11_GetSlotName(root->slot), "NSS Builtin Objects");
}

}  // namespace net
