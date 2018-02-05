#ifndef CHROME_BROWSER_CHROMEOS_ASSISTANT_ASSISTANT_BROWSER_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_ASSISTANT_ASSISTANT_BROWSER_SERVICE_H_

#include "ash/public/interfaces/assistant_connector.mojom.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "libassistant/contrib/internal/chromeos/assistant.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

class Profile;
class GoogleServiceAuthError;

namespace chromeos {
namespace assistant {
class AssistantBrowserService : public ash::mojom::AssistantConnector,
                                public service_manager::Service,
                                public content::NotificationObserver,
                                public ::assistant::AssistantObserver {
 public:
  AssistantBrowserService();
  ~AssistantBrowserService() override;

 private:
  // ash::mojom::AssistantConnector overrides:
  void ShowAssistantCard() override;
  void SendTextQuery(const std::string& query) override;

  // service_manager::Service overrides:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  // content::NotificationObserver overrides.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  void BindConnectorRequest(ash::mojom::AssistantConnectorRequest request);

  void GetAccessTokenCallback(const base::Optional<std::string>& token,
                              base::Time time,
                              const GoogleServiceAuthError& error);

  // assistant::Assistant overrides.
  void OnHtmlCardReceived(const std::string& html) override;

  void RefreshToken();

  identity::mojom::IdentityManager* GetIdentityManager();

  Profile* profile_;
  content::NotificationRegistrar registrar_;

  service_manager::BinderRegistry registry_;
  mojo::Binding<ash::mojom::AssistantConnector> binding_;

  identity::mojom::IdentityManagerPtr identity_manager_;

  std::string account_id_;
  std::unique_ptr<::assistant::Assistant> assistant_;

  base::WeakPtrFactory<AssistantBrowserService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AssistantBrowserService);
};

}  // namespace assistant
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_ASSISTANT_ASSISTANT_BROWSER_SERVICE_H_
