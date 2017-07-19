#include "core/frame/LocalFrameClient.h"

namespace blink {

LocalFrameClient::SupplementInstallCallback
    LocalFrameClient::supplement_install_callback_ = nullptr;

void LocalFrameClient::RegisterSupplementInstallCallback(
    SupplementInstallCallback callback) {
  supplement_install_callback_ = callback;
}

void LocalFrameClient::InstallSupplements(Document& document,
                                          Settings& settings) {
  if (supplement_install_callback_) {
    supplement_install_callback_(document, settings);
  }
}

}  // namespace blink
