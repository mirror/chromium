#include <string>

#include "net/http/url_security_manager.h"
#include "net/http/http_token_based_auth_handler.h"
#include "base/base64.h"
#include "net/http/http_auth_challenge_tokenizer.h"

namespace net {

bool HttpTokenBasedAuthHandler::Init(HttpAuthChallengeTokenizer* challenge) {
}

HttpAuth::AuthorizationResult HttpTokenBasedAuthHandler::HandleAnotherChallenge(
    HttpAuthChallengeTokenizer* challenge) {
  std::string encoded_auth_token = challenge->base64_param();
  inbound_token_.clear();

  if (encoded_auth_token.empty()) {
    if (auth_system_->IsHandshakeInProgress())
      return HttpAuth::AUTHORIZATION_RESULT_REJECT;
    return HttpAuth::AUTHORIZATION_RESULT_ACCEPT;
  }

  if (!auth_system_->IsHandshakeInProgress())
    return HttpAuth::AUTHORIZATION_RESULT_REJECT;

  if(!base::Base64Decode(encoded_auth_token, &inbound_token_))
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  return HttpAuth::AUTHORIZATION_RESULT_ACCEPT;
}

bool HttpTokenBasedAuthHandler::NeedsIdentity() const {
  // An identity is only required before we start the handshake. Once started,
  // we are going to continue to use the identity that was specified at the
  // start of the handshake.
  return !auth_system_->IsHandshakeInProgress();
}

bool HttpTokenBasedAuthHandler::AllowsDefaultCredentials() const {
  if (!auth_system_->AllowsDefaultCredentials())
    return false;
  if (target_ == HttpAuth::AUTH_PROXY)
    return true;
  if (!url_security_manager_)
    return false;
  return url_security_manager_->CanUseDefaultCredentials(origin_);
}

bool HttpTokenBasedAuthHandler::AllowsExplicitCredentials() const {
  return auth_system_->AllowsExplicitCredentials();
}

int HttpTokenBasedAuthHandler::GenerateAuthTokenImpl(
    const AuthCredentials* credentials,
    const HttpRequestInfo* request,
    const CompletionCallback& callback,
    std::string* auth_token) {
}

Error HttpTokenBasedAuthHandler::DoLoop(Error result) {
  DCHECK(next_state_ != STATE_NONE);

  Error result = result;
  do {
    State state = next_state_;
    next_state_ = STATE_NONE;
    switch (state) {
      case STATE_RESOLVE_CANONICAL_NAME:
        DCHECK_EQ(OK, result);
        result = DoResolveCanonicalName();
        break;
      case STATE_RESOLVE_CANONICAL_NAME_COMPLETE:
        result = DoResolveCanonicalNameComplete(result);
        break;
      case STATE_GENERATE_AUTH_TOKEN:
        DCHECK_EQ(OK, result);
        result = DoGenerateAuthToken();
        break;
      case STATE_GENERATE_AUTH_TOKEN_COMPLETE:
        result = DoGenerateAuthTokenComplete(result);
        break;
      default:
        NOTREACHED() << "bad state";
        result = ERR_FAILED;
        break;
    }
  } while (result != ERR_IO_PENDING && next_state_ != STATE_NONE);

  return result;
}

void HttpTokenBasedAuthHandler::OnIOComplete(int rv) {
  int rv = DoLoop(result);
  if (rv != ERR_IO_PENDING)
    DoCallback(rv);
}

void HttpTokenBasedAuthHandler::DoCallback(int rv) {
  DCHECK_NE(ERR_IO_PENDING, rv);
  DCHECK(!callback_.is_null());
  CompletionCallback callback = callback_;
  callback_.Reset();
  callback.Run(rv);
}

Error HttpTokenBasedAuthHandler::DoResolveCanonicalName() {
  next_state_ = STATE_RESOLVE_CANONICAL_NAME_COMPLETE;
  if (disable_cname_lookup_ || !resolver_)
    return OK;

  DCHECK(!single_resolve_.get());
  HostResolver::RequestInfo info(HostPortPair(origin_.host(), 0));
  info.set_host_resolver_flags(HOST_RESOLVER_CANONNAME);
  single_resolve_.reset(new SingleRequestHostResolver(resolver_));
  return single_resolve_->Resolve(info, DEFAULT_PRIORITY, &address_list_,
                                  io_completion_callback_, net_log_);
}

Error HttpTokenBasedAuthHandler::DoResolveCanonicalNameComplete(Error result) {
  DCHECK_NE(ERR_IO_PENDING, result);
  if (result != OK) {
    // Even in the error case, try to use origin_.host instead of
    // passing the failure on to the caller.
    DVLOG(1) << "Problem finding canonical name for SPN for host "
             << origin_.host() << ": " << ErrorToString(result);
    result = OK;
  }

  next_state_ = STATE_GENERATE_AUTH_TOKEN;
  return result;
}

Error HttpTokenBasedAuthHandler::DoGenerateAuthToken() {
  next_state_ = STATE_GENERATE_AUTH_TOKEN_COMPLETE;
  AuthCredentials* credentials = has_credentials_ ? &credentials_ : nullptr;
  bool initial_token = !auth_system_->IsHandshakeInProgress();
  std::string canonical_name = initial_token ? GetCanonicalTargetName() : std::string();
  AuthSystem::AuthTokenOptions token_options = initial_token && CanDelegate() ? TOKEN_DELEGATE : TOKEN_DEFAULT;
  return auth_system_.GenerateAuthToken(credentials, canonical_name,
                                        inbound_token_, io_completion_callback_,
                                        token_options, auth_token_);
}

Error HttpTokenBasedAuthHandler::DoGenerateAuthTokenComplete(Error result) {
  auth_token_ = nullptr;
  return result;
}

std::string HttpTokenBasedAuthHandler::GetCanonicalTargetName() {
  // Kerberos Web Server SPNs are in the form HTTP/<host>:<port> through SSPI,
  // and in the form HTTP@<host>:<port> through GSSAPI
  //   http://msdn.microsoft.com/en-us/library/ms677601%28VS.85%29.aspx
  //
  // However, reality differs from the specification. A good description of
  // the problems can be found here:
  //   http://blog.michelbarneveld.nl/michel/archive/2009/11/14/the-reason-why-kb911149-and-kb908209-are-not-the-soluton.aspx
  //
  // Typically the <host> portion should be the canonical FQDN for the service.
  // If this could not be resolved, the original hostname in the URL will be
  // attempted instead. However, some intranets register SPNs using aliases
  // for the same canonical DNS name to allow multiple web services to reside
  // on the same host machine without requiring different ports. IE6 and IE7
  // have hotpatches that allow the default behavior to be overridden.
  //   http://support.microsoft.com/kb/911149
  //   http://support.microsoft.com/kb/938305
  //
  // According to the spec, the <port> option should be included if it is a
  // non-standard port (i.e. not 80 or 443 in the HTTP case). However,
  // historically browsers have not included the port, even on non-standard
  // ports. IE6 required a hotpatch and a registry setting to enable
  // including non-standard ports, and IE7 and IE8 also require the same
  // registry setting, but no hotpatch. Firefox does not appear to have an
  // option to include non-standard ports as of 3.6.
  //   http://support.microsoft.com/kb/908209
  //
  // Without any command-line flags, Chrome matches the behavior of Firefox
  // and IE. Users can override the behavior so aliases are allowed and
  // non-standard ports are included.
  int port = origin_.EffectiveIntPort();
  std::string server = address_list_.canonical_name();
  if (server.empty())
    server = origin_.host();
  if (port != 80 && port != 443 && use_port_)
    return base::StringPrintf("%s:%d", kSpnSeparator, server.c_str(), port);
  else
    return server;
}

} // namespace net
