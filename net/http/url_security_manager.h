// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_URL_SECURITY_MANAGER_H_
#define NET_HTTP_URL_SECURITY_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "net/base/net_export.h"

class GURL;

namespace net {

class HttpAuthFilter;

// The URL security manager controls the policies (allow, deny, prompt user)
// regarding URL actions (e.g., sending the default credentials to a server).
class NET_EXPORT_PRIVATE URLSecurityManager {
 public:
  URLSecurityManager();
  virtual ~URLSecurityManager();

  enum NTLMCredentialsUsePolicy {
    DISALLOW_NTLM,  // NTLM shouldn't be used either with explicit or ambient
                    // credentials.
    ALLOW_EXPLICIT_CREDENTIALS_WITH_NTLM,  // Only use NTLM with explicit
                                           // credentials.
    ALLOW_AMBIENT_CREDENTIALS_WITH_NTLM  // Ambient and explicit credentials can
                                         // be used with NTLM.
  };

  // Creates a platform-dependent instance of URLSecurityManager.
  //
  // A security manager has two whitelists, a "default whitelist" that is a
  // whitelist of servers with which default credentials can be used, and a
  // "delegate whitelist" that is the whitelist of servers that are allowed to
  // have delegated Kerberos tickets.
  //
  // On creation both whitelists are NULL.
  //
  // If the default whitelist is NULL and the platform is Windows, it indicates
  // that security zone mapping should be used to determine whether default
  // credentials should be used. If the default whitelist is NULL and the
  // platform is non-Windows, it indicates that no servers should be
  // whitelisted.
  //
  // If the delegate whitelist is NULL no servers can have delegated Kerberos
  // tickets.
  //
  static std::unique_ptr<URLSecurityManager> Create();

  // Return true if NTLM is allowed to be used with explicit (user supplied)
  // credentials when authenticating with |auth_origin|.
  virtual bool CanUseExplicitCredentialsForNTLM(
      const GURL& auth_origin) const = 0;

  // Returns true if NTLM is allowed to be used with ambient credentials when
  // authenticating with |auth_origin|.
  virtual bool CanUseAmbientCredentialsForNTLM(
      const GURL& auth_origin) const = 0;

  // Returns true if we can send the default credentials to the server at
  // |auth_origin| for HTTP Negotiate authentication. Note that Negotiate may
  // involve NTLM authentication as well. The result of calling
  // CanUseAmbientCredentialsForNTLM() governs whether SPNEGO is allowed to
  // negotiate NTLM.
  virtual bool CanUseAmbientCredentialsForNegotiate(
      const GURL& auth_origin) const = 0;

  // Returns true if Kerberos delegation is allowed for the server at
  // |auth_origin| for HTTP Negotiate authentication.
  virtual bool CanDelegate(const GURL& auth_origin) const = 0;

  virtual void SetDefaultWhitelist(
      std::unique_ptr<HttpAuthFilter> whitelist_default) = 0;
  virtual void SetDelegateWhitelist(
      std::unique_ptr<HttpAuthFilter> whitelist_delegate) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(URLSecurityManager);
};

class URLSecurityManagerWhitelist : public URLSecurityManager {
 public:
  URLSecurityManagerWhitelist();
  ~URLSecurityManagerWhitelist() override;

  // URLSecurityManager
  bool CanUseExplicitCredentialsForNTLM(const GURL& auth_origin) const override;
  bool CanUseAmbientCredentialsForNTLM(const GURL& auth_origin) const override;
  bool CanUseAmbientCredentialsForNegotiate(
      const GURL& auth_origin) const override;
  bool CanDelegate(const GURL& auth_origin) const override;
  void SetDefaultWhitelist(
      std::unique_ptr<HttpAuthFilter> whitelist_default) override;
  void SetDelegateWhitelist(
      std::unique_ptr<HttpAuthFilter> whitelist_delegate) override;

 protected:
  bool HasDefaultWhitelist() const;

 private:
  std::unique_ptr<const HttpAuthFilter> whitelist_default_;
  std::unique_ptr<const HttpAuthFilter> whitelist_delegate_;
  NTLMCredentialsUsePolicy ntlm_credentials_policy_;

  DISALLOW_COPY_AND_ASSIGN(URLSecurityManagerWhitelist);
};

}  // namespace net

#endif  // NET_HTTP_URL_SECURITY_MANAGER_H_
