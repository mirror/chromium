// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/enterprise/chrome_keymaster.h"

#include <cert.h>
#include <dlfcn.h>
#include <keyhi.h>
#include <pk11pub.h>
#include <pkcs11.h>

#include "base/logging.h"
#include "crypto/scoped_nss_types.h"
#include "net/cert/x509_util_nss.h"

namespace arc {

namespace {

const uint32_t MAX_SIZE = 8000;

static char const* PKCS11_SO_NAME = "libchaps.so";
static void* pkcs11_so = nullptr;

// list of all pkcs#11 functions
static CK_FUNCTION_LIST* pkcs11 = nullptr;

CK_RV pkcs11_initialize() {
  CK_RV rv = CKR_OK;
  CK_RV (*C_GetFunctionList)(CK_FUNCTION_LIST_PTR_PTR) = nullptr;

  pkcs11_so = dlopen(PKCS11_SO_NAME, RTLD_NOW);
  if (!pkcs11_so) {
    LOG(ERROR) << "Error loading pkcs#11 so: " << dlerror();
    return CKR_GENERAL_ERROR;
  }

  if ((C_GetFunctionList = (CK_RV(*)(CK_FUNCTION_LIST_PTR_PTR))dlsym(
           pkcs11_so, "C_GetFunctionList")) == nullptr) {
    dlclose(pkcs11_so);
    LOG(ERROR) << "dlsym C_GetFunctionList faukedL : " << dlerror();
    return CKR_FUNCTION_FAILED;
  }

  if ((rv = C_GetFunctionList(&pkcs11)) != CKR_OK) {
    LOG(ERROR) << "C_GetFunctionList call failed: " << rv;
    dlclose(pkcs11_so);
    return rv;
  }

  if ((rv = pkcs11->C_Initialize(nullptr)) != CKR_OK) {
    LOG(ERROR) << "C_initialize failed: " << rv;
    dlclose(pkcs11_so);
    return rv;
  }

  return CKR_OK;
}

CK_RV pkcs11_finalize(const std::set<int>& slot_ids) {
  CK_RV rv = CKR_OK;

  if (!pkcs11 || !pkcs11_so) {
    return CKR_OK;
  }

  for (CK_SLOT_ID slot_id : slot_ids) {
    if ((rv = pkcs11->C_CloseAllSessions(slot_id)) != CKR_OK) {
      LOG(ERROR) << "C_CloseAllSessions(" << slot_id << ") failed: " << rv;
    }
  }

  if ((rv = pkcs11->C_Finalize(nullptr)) != CKR_OK) {
    LOG(ERROR) << "C_Finalize failed: " << rv;
  }

  pkcs11 = nullptr;
  dlclose(pkcs11_so);
  pkcs11_so = nullptr;

  return rv;
}

bool IsSupportedDigestOperation(CK_MECHANISM_TYPE mechanism) {
  switch (mechanism) {
    case CKM_SHA_1:
    case CKM_SHA256:
    case CKM_SHA384:
    case CKM_SHA512:
      return true;
    default:
      return false;
  }
}

bool GetMechanism(const std::vector<mojom::KeyParamPtr>& params,
                  CK_MECHANISM* mechanism) {
  mojom::Algorithm algorithm;
  mojom::Padding padding;
  mojom::Digest digest;
  for (const auto& param : params) {
    if (!param)
      continue;
    if (param->is_algorithm())
      algorithm = param->get_algorithm();
    if (param->is_digest())
      digest = param->get_digest();
    if (param->is_padding())
      padding = param->get_padding();
  }

  switch (algorithm) {
    case mojom::Algorithm::ALGORITHM_RSA:
      switch (digest) {
        case mojom::Digest::DIGEST_NONE:
          switch (padding) {
            case mojom::Padding::PAD_NONE:
              mechanism->mechanism = 0;
              return true;
            case mojom::Padding::PAD_RSA_PKCS1_1_5_SIGN:
              mechanism->mechanism = CKM_RSA_PKCS;
              return true;
            default:
              LOG(ERROR) << "Unsupported padding " << padding << " for RSA.";
              return false;
          }
        case mojom::Digest::DIGEST_SHA1:
          switch (padding) {
            case mojom::Padding::PAD_NONE:
              mechanism->mechanism = CKM_SHA_1;
              return true;
            case mojom::Padding::PAD_RSA_PKCS1_1_5_SIGN:
              mechanism->mechanism = CKM_SHA1_RSA_PKCS;
              return true;
            default:
              LOG(ERROR) << "Unsupported padding " << padding << " for RSA.";
              return false;
          }
        case mojom::Digest::DIGEST_SHA_2_256:
          switch (padding) {
            case mojom::Padding::PAD_NONE:
              mechanism->mechanism = CKM_SHA256;
              return true;
            case mojom::Padding::PAD_RSA_PKCS1_1_5_SIGN:
              mechanism->mechanism = CKM_SHA256_RSA_PKCS;
              return true;
            default:
              LOG(ERROR) << "Unsupported padding " << padding << " for RSA.";
              return false;
          }
        case mojom::Digest::DIGEST_SHA_2_384:
          switch (padding) {
            case mojom::Padding::PAD_NONE:
              mechanism->mechanism = CKM_SHA384;
              return true;
            case mojom::Padding::PAD_RSA_PKCS1_1_5_SIGN:
              mechanism->mechanism = CKM_SHA384_RSA_PKCS;
              return true;
            default:
              LOG(ERROR) << "Unsupported padding " << padding << " for RSA.";
              return false;
          }
        case mojom::Digest::DIGEST_SHA_2_512:
          switch (padding) {
            case mojom::Padding::PAD_NONE:
              mechanism->mechanism = CKM_SHA512;
              return true;
            case mojom::Padding::PAD_RSA_PKCS1_1_5_SIGN:
              mechanism->mechanism = CKM_SHA512_RSA_PKCS;
              return true;
            default:
              LOG(ERROR) << "Unsupported padding " << padding << " for RSA.";
              return false;
          }
        default:
          LOG(ERROR) << "Usupported digest " << digest << " for RSA";
          return false;
      }
    default:
      LOG(ERROR) << "Unsupported algorithm: " << algorithm;
      return false;
  }
}

SECKEYPrivateKey* GetPrivateKey(
    const scoped_refptr<net::X509Certificate>& cert) {
  net::ScopedCERTCertificate certificate(
      net::x509_util::CreateCERTCertificateFromX509Certificate(cert.get()));
  if (!certificate)
    return nullptr;

  return PK11_FindKeyByAnyCert(certificate.get(), nullptr);
}

}  // namespace

struct OperationInfo {
  OperationInfo() {}
  explicit OperationInfo(scoped_refptr<net::X509Certificate> cert)
      : certificate(std::move(cert)) {}
  scoped_refptr<net::X509Certificate> certificate;
  std::vector<uint8_t> data;
  bool is_digest;
};

ChromeKeymaster::ChromeKeymaster() {
  pkcs11_initialize();
}

ChromeKeymaster::~ChromeKeymaster() {
  pkcs11_finalize(slot_ids_);
}

bool ChromeKeymaster::Begin(scoped_refptr<net::X509Certificate> cert,
                            const std::vector<mojom::KeyParamPtr>& params,
                            uint64_t* operation_handle) {
  CK_MECHANISM mechanism;
  if (!GetMechanism(params, &mechanism)) {
    LOG(ERROR) << "Required mechanism is not supported.";
    return false;
  }

  *operation_handle = 0;
  SECKEYPrivateKey* priv_key = GetPrivateKey(cert);
  if (!priv_key)
    return false;

  CK_SLOT_ID slot_id = PK11_GetSlotID(priv_key->pkcs11Slot);
  CK_RV rv = CKR_OK;
  CK_SESSION_HANDLE session_id;
  if ((rv = pkcs11->C_OpenSession(slot_id, CKF_SERIAL_SESSION | CKF_RW_SESSION,
                                  nullptr, nullptr, &session_id)) != CKR_OK) {
    LOG(ERROR) << "C_OpenSession failed: " << rv;
    return false;
  }
  slot_ids_.insert(slot_id);
  *operation_handle = (uint64_t)session_id;

  OperationInfo info(std::move(cert));

  if (mechanism.mechanism == 0) {
    operation_table_[*operation_handle] = info;
    return true;
  }

  if (IsSupportedDigestOperation(mechanism.mechanism)) {
    if (pkcs11->C_DigestInit(session_id, &mechanism) != CKR_OK) {
      return false;
    }
    info.is_digest = true;
  } else {
    if ((rv = pkcs11->C_SignInit(session_id, &mechanism, priv_key->pkcs11ID)) !=
        CKR_OK) {
      return false;
    }
  }
  operation_table_[*operation_handle] = info;
  return true;
}

bool ChromeKeymaster::Update(uint64_t operation_handle,
                             const std::vector<uint8_t>& data,
                             int32_t* input_consumed) {
  *input_consumed = 0;
  if (operation_table_.count(operation_handle)) {
    if (operation_table_[operation_handle].is_digest) {
      if (pkcs11->C_DigestUpdate((CK_SESSION_HANDLE)operation_handle,
                                 (CK_BYTE_PTR)data.data(),
                                 data.size()) != CKR_OK) {
        return false;
      }
      *input_consumed = data.size();
      return true;
    } else {
      if (operation_table_[operation_handle].data.size() + data.size() >=
          MAX_SIZE) {
        return false;
      } else {
        operation_table_[operation_handle].data.insert(
            operation_table_[operation_handle].data.end(), data.begin(),
            data.end());
        *input_consumed = data.size();
        return true;
      }
    }
  }

  if (pkcs11->C_SignUpdate((CK_SESSION_HANDLE)operation_handle,
                           (CK_BYTE_PTR)data.data(), data.size()) != CKR_OK) {
    return false;
  }
  *input_consumed = data.size();
  return true;
}

bool ChromeKeymaster::Abort(uint64_t operation_handle) {
  CK_SESSION_HANDLE session_id = (CK_SESSION_HANDLE)operation_handle;

  if (operation_table_.count(operation_handle)) {
    if (operation_table_[operation_handle].is_digest) {
      pkcs11->C_DigestFinal(session_id, 0, 0);
    }
    operation_table_.erase(operation_handle);
  } else {
    pkcs11->C_SignFinal(session_id, 0, 0);
  }

  if (pkcs11->C_CloseSession(session_id) != CKR_OK) {
    return false;
  }
  return true;
}

bool ChromeKeymaster::Finish(uint64_t operation_handle,
                             std::vector<uint8_t>* signature) {
  CK_SESSION_HANDLE session_id = (CK_SESSION_HANDLE)operation_handle;

  if (operation_table_.count(operation_handle)) {
    if (operation_table_[operation_handle].is_digest) {
      CK_ULONG max_size = MAX_SIZE;
      operation_table_[operation_handle].data.resize(max_size);
      if (pkcs11->C_DigestFinal(session_id,
                                operation_table_[operation_handle].data.data(),
                                &max_size) != CKR_OK) {
        return false;
      }
      operation_table_[operation_handle].data.resize(max_size);
    }

    SECKEYPrivateKey* key =
        GetPrivateKey(operation_table_[operation_handle].certificate);
    if (!key)
      return false;

    int len = PK11_SignatureLen(key);
    if (len <= 0) {
      LOG(ERROR) << "PK11_SignatureLen failed.";
      return false;
    }
    signature->resize(len);

    SECItem hash;
    hash.data = operation_table_[operation_handle].data.data();
    hash.len = operation_table_[operation_handle].data.size();

    SECItem sig;
    sig.data = signature->data();
    sig.len = signature->size();

    PK11_Sign(key, &sig, &hash);
    operation_table_.erase(operation_handle);
  } else {
    CK_ULONG max_size = MAX_SIZE;
    signature->resize(max_size);
    if (pkcs11->C_SignFinal(session_id, signature->data(), &max_size) !=
        CKR_OK) {
      return false;
    }
    signature->resize(max_size);
  }
  if (pkcs11->C_CloseSession(session_id) != CKR_OK) {
    return false;
  }
  return true;
}

std::vector<mojom::KeyParamPtr> ChromeKeymaster::GetKeyCharacteristics(
    const scoped_refptr<net::X509Certificate>& cert) {
  SECKEYPrivateKey* priv_key = GetPrivateKey(cert);
  if (!priv_key)
    return std::vector<mojom::KeyParamPtr>();

  std::vector<mojom::KeyParamPtr> params;
  switch (SECKEY_GetPrivateKeyType(priv_key)) {
    case rsaKey: {
      auto value = mojom::KeyParam::New();
      value->set_algorithm(mojom::Algorithm::ALGORITHM_RSA);
      params.push_back(std::move(value));
    }

      {
        auto value = mojom::KeyParam::New();
        value->set_digest(mojom::Digest::DIGEST_NONE);
        params.push_back(std::move(value));
      }

      {
        auto value = mojom::KeyParam::New();
        value->set_digest(mojom::Digest::DIGEST_SHA1);
        params.push_back(std::move(value));
      }

      {
        auto value = mojom::KeyParam::New();
        value->set_digest(mojom::Digest::DIGEST_SHA_2_256);
        params.push_back(std::move(value));
      }

      {
        auto value = mojom::KeyParam::New();
        value->set_digest(mojom::Digest::DIGEST_SHA_2_384);
        params.push_back(std::move(value));
      }

      {
        auto value = mojom::KeyParam::New();
        value->set_digest(mojom::Digest::DIGEST_SHA_2_512);
        params.push_back(std::move(value));
      }

      {
        auto value = mojom::KeyParam::New();
        value->set_padding(mojom::Padding::PAD_NONE);
        params.push_back(std::move(value));
      }

      {
        auto value = mojom::KeyParam::New();
        value->set_padding(mojom::Padding::PAD_RSA_PKCS1_1_5_SIGN);
        params.push_back(std::move(value));
      }
    default:
      LOG(ERROR) << "Unsupported corporate key type: "
                 << SECKEY_GetPrivateKeyType(priv_key);
      return std::vector<mojom::KeyParamPtr>();
  }
  return params;
}

}  // namespace arc
