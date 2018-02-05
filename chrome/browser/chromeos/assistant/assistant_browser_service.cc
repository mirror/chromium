#include "chrome/browser/chromeos/assistant/assistant_browser_service.h"

#include "ash/content/card_view.h"
#include "base/bind.h"
#include "base/logging.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace chromeos {
namespace assistant {

namespace {
const char kScopeAuthGcm[] = "https://www.googleapis.com/auth/gcm";
const char kScopeAssistant[] =
    "https://www.googleapis.com/auth/assistant-sdk-prototype";
}  // namespace

AssistantBrowserService::AssistantBrowserService()
    : binding_(this), weak_factory_(this) {
  registry_.AddInterface<ash::mojom::AssistantConnector>(
      base::Bind(&AssistantBrowserService::BindConnectorRequest,
                 weak_factory_.GetWeakPtr()));
  registrar_.Add(this, chrome::NOTIFICATION_LOGIN_USER_PROFILE_PREPARED,
                 content::NotificationService::AllSources());
}

AssistantBrowserService::~AssistantBrowserService() = default;

void AssistantBrowserService::BindConnectorRequest(
    ash::mojom::AssistantConnectorRequest request) {
  LOG(ERROR) << "BindConnectorRequest";
  binding_.Bind(std::move(request));
}

void AssistantBrowserService::OnStart() {
  LOG(ERROR) << "Assistant browser service started";
}

void AssistantBrowserService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  LOG(ERROR) << "BindInterface";
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void AssistantBrowserService::ShowAssistantCard() {
  LOG(ERROR) << "Show card!";
  ash::CardView::ShowCard(profile_, GURL("data: text/html, <h1>hello</h1>"),
                          gfx::Rect(0, 0, 100, 100));
}

void AssistantBrowserService::SendTextQuery(const std::string& query) {
  LOG(ERROR) << "Sending query " << query;
  assistant_->SendTextQuery(query);
}

void AssistantBrowserService::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  LOG(ERROR) << "User profile prepared!";
  profile_ = content::Details<Profile>(details).ptr();
  auto* signin_manager = SigninManagerFactory::GetForProfile(profile_);
  account_id_ = signin_manager->GetAuthenticatedAccountId();
  RefreshToken();
}

void AssistantBrowserService::RefreshToken() {
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert(kScopeAssistant);
  scopes.insert(kScopeAuthGcm);
  GetIdentityManager()->GetAccessToken(
      account_id_, scopes, "cros_opa",
      base::Bind(&AssistantBrowserService::GetAccessTokenCallback,
                 weak_factory_.GetWeakPtr()));
}

identity::mojom::IdentityManager*
AssistantBrowserService::GetIdentityManager() {
  if (!identity_manager_.is_bound()) {
    content::BrowserContext::GetConnectorFor(profile_)->BindInterface(
        "identity", mojo::MakeRequest(&identity_manager_));
  }
  return identity_manager_.get();
}

void AssistantBrowserService::GetAccessTokenCallback(
    const base::Optional<std::string>& token,
    base::Time time,
    const GoogleServiceAuthError& error) {
  if (token.has_value()) {
    LOG(ERROR) << "Got access token " << token.value();
    if (!assistant_) {
      assistant_ =
          std::make_unique<::assistant::Assistant>(token.value(), this);
    }
  } else
    LOG(ERROR) << "Not getting token";
}

void AssistantBrowserService::OnHtmlCardReceived(const std::string& html) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(ash::CardView::ShowCard, profile_,
                 GURL("data: text/html," + html), gfx::Rect(0, 360, 400, 320)));
}

}  // namespace assistant
}  // namespace chromeos
