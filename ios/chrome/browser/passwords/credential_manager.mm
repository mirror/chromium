#import "ios/chrome/browser/passwords/credential_manager.h"

#import "base/mac/bind_objc_block.h"
#import "ios/web/public/origin_util.h"
#import "ios/web/public/web_state/web_state.h"
#include "url/origin.h"

namespace password_manager {

namespace {

// TODO(crbug.com/435047): Uncomment line below when registering a callback
// in constructor.
// const char kCommandPrefix[] = "credentials";

}  // namespace

CredentialManager::CredentialManager(PasswordManagerClient* client,
                                     web::WebState* webState)
    : CredentialManagerImpl(client) {
  // TODO(crbug.com/435047): Register a callback i.e. uncomment line below,
  // after checking context security is implemented.
  // webState->AddScriptCommandCallback(
  //     base::Bind(&CredentialManager::HandleScriptCommand,
  //     base::Unretained(this)), kCommandPrefix);
}

bool CredentialManager::HandleScriptCommand(const base::DictionaryValue& JSON,
                                            const GURL& originURL,
                                            bool userIsInteracting) {
  // TODO(crbug.com/435047): Check if context is secure.
  std::string command;
  if (!JSON.GetString("command", &command)) {
    DLOG(ERROR) << "RECEIVED BAD JSON - NO VALID 'command' FIELD";
    return false;
  }

  int requestId;
  if (!JSON.GetInteger("requestId", &requestId)) {
    DLOG(ERROR) << "RECEIVED BAD JSON - NO VALID 'requestId' FIELD";
    return false;
  }

  if (command == "credentials.get") {
    CredentialMediationRequirement mediation = ParseMediationRequirement(JSON);
    bool include_passwords = ParseIncludePasswords(JSON);
    std::vector<GURL> federations = ParseFederations(JSON);
    Get(mediation, include_passwords, federations,
        base::BindOnce(&CredentialManager::SendGetResponse,
                       base::Unretained(this), requestId));
    return true;
  }
  if (command == "credentials.store") {
    CredentialInfo credential = ParseCredentialDictionary(JSON);
    if (credential.type == CredentialType::CREDENTIAL_TYPE_EMPTY) {
      DLOG(ERROR) << "RECEIVED BAD JSON - NO VALID 'type' FIELD";
      return false;
    }
    Store(credential, base::BindOnce(&CredentialManager::SendStoreResponse,
                                     base::Unretained(this), requestId));
    return true;
  }
  if (command == "credentials.preventSilentAccess") {
    PreventSilentAccess(
        base::BindOnce(&CredentialManager::SendPreventSilentAccessResponse,
                       base::Unretained(this), requestId));
    return true;
  }
  return false;
}

CredentialMediationRequirement CredentialManager::ParseMediationRequirement(
    const base::DictionaryValue& JSON) {
  if (!JSON.HasKey("mediation")) {
    return CredentialMediationRequirement::kOptional;
  }
  std::string mediation;
  JSON.GetString("mediation", &mediation);
  if (mediation == "silent") {
    return CredentialMediationRequirement::kSilent;
  }
  if (mediation == "required") {
    return CredentialMediationRequirement::kRequired;
  }
  return CredentialMediationRequirement::kOptional;
}

bool CredentialManager::ParseIncludePasswords(
    const base::DictionaryValue& JSON) {
  if (!JSON.HasKey("password")) {
    return false;
  }
  bool include_passwords;
  JSON.GetBoolean("password", &include_passwords);
  return include_passwords;
}

std::vector<GURL> CredentialManager::ParseFederations(
    const base::DictionaryValue& JSON) {
  std::vector<GURL> federations;
  if (!JSON.HasKey("providers")) {
    return federations;
  }
  const base::ListValue* lst;
  JSON.GetList("providers", &lst);
  for (size_t i = 0; i < lst->GetSize(); i++) {
    std::string s;
    lst->GetString(i, &s);
    federations.push_back(GURL(s));
  }
  return federations;
}

CredentialType CredentialManager::ParseCredentialType(const std::string& str) {
  if (str == "PasswordCredential") {
    return CredentialType::CREDENTIAL_TYPE_PASSWORD;
  }
  if (str == "FederatedCredential") {
    return CredentialType::CREDENTIAL_TYPE_FEDERATED;
  }
  return CredentialType::CREDENTIAL_TYPE_EMPTY;
}

CredentialInfo CredentialManager::ParseCredentialDictionary(
    const base::DictionaryValue& JSON) {
  CredentialInfo credential;
  JSON.GetString("id", &credential.id);
  JSON.GetString("name", &credential.name);
  std::string iconURL;
  JSON.GetString("iconURL", &iconURL);
  credential.icon = GURL(iconURL);
  JSON.GetString("password", &credential.password);
  std::string federation;
  JSON.GetString("provider", &federation);
  credential.federation = url::Origin(GURL(federation));
  std::string type;
  JSON.GetString("type", &type);
  credential.type = ParseCredentialType(type);
  return credential;
}

void CredentialManager::SendGetResponse(int requestId,
                                        CredentialManagerError error,
                                        const base::Optional<CredentialInfo>&) {
}

void CredentialManager::SendPreventSilentAccessResponse(int requestId) {}

void CredentialManager::SendStoreResponse(int requestId) {}

}  // namespace password_manager
