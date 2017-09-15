#include "components/gcm_driver/gcm_receiver_impl.h"
#include "components/gcm_driver/gcm_app_handler.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/gcm_driver/gcm_receiver_typemap_traits.h"
#include "components/gcm_driver/gcm_service.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"

namespace gcm {

namespace {
class GCMAppHandlerWrapper : public GCMAppHandler {
 public:
  GCMAppHandlerWrapper(mojom::GCMHandlerPtr handler)
      : handler_(std::move(handler)) {}

  void OnStoreReset() override {}
  void OnMessage(const std::string& app_id,
                 const IncomingMessage& message) override {}
  void OnMessagesDeleted(const std::string& app_id) override {}
  void OnSendError(
      const std::string& app_id,
      const GCMClient::SendErrorDetails& send_error_details) override {}

  void OnSendAcknowledged(const std::string& app_id,
                          const std::string& message_id) override {}

 private:
  mojom::GCMHandlerPtr handler_;
};
}  // namespace

GCMReceiverImpl::GCMReceiverImpl(GCMService* service)
    : service_(service), binding_(this) {
  DCHECK(service_);
}

GCMReceiverImpl::~GCMReceiverImpl() {}

void GCMReceiverImpl::AddBinding(
    mojo::InterfaceRequest<mojom::GCMReceiver> request) {
  DCHECK(!binding_.is_bound());
  binding_.Bind(std::move(request));
}

void GCMReceiverImpl::AddGCMHandler(const std::string& app_id,
                                    mojom::GCMHandlerPtr handler) {
  app_handlers_->emplace(
      app_id, base::MakeUnique<GCMAppHandlerWrapper>(std::move(handler)));
  service_->gcm_driver()->AddAppHandler(app_id, handler);
}

void GCMReceiverImpl::RemoveGCMHandler(const std::string& app_id) {}

void GCMReceiverImpl::GetGCMHandler(const std::string& app_id,
                                    GetGCMHandlerCallback callback) {}

void GCMReceiverImpl::Register(const std::string& app_id,
                               const std::vector<std::string>& sender_ids,
                               RegisterCallback callback) {
  service_->gcm_driver()->Register(
      app_id, sender_ids,
      base::Bind(&GCMReceiverImpl::OnRegistrationCompletion,
                 base::Unretained(this), base::Passed(&callback)));
}

void GCMReceiverImpl::OnRegistrationCompletion(RegisterCallback callback,
                                               const std::string& iid_token,
                                               GCMClient::Result result) {
  std::move(callback).Run(iid_token, result);
}

}  // namespace gcm
