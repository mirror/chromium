// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"

#include "base/strings/stringprintf.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/stream_handle.h"
#include "content/public/browser/stream_info.h"
#include "content/public/common/url_constants.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_stream_manager.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_constants.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest_delegate.h"
#include "extensions/browser/process_manager.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/guest_view/guest_view_constants.h"
#include "extensions/strings/grit/extensions_strings.h"
#include "ipc/ipc_message_macros.h"
#include "net/base/url_util.h"

using content::WebContents;

namespace extensions {

StreamContainer::StreamContainer(scoped_ptr<content::StreamInfo> stream,
                                 const GURL& handler_url,
                                 const std::string& extension_id)
    : stream_(stream.Pass()),
      handler_url_(handler_url),
      extension_id_(extension_id),
      weak_factory_(this) {
}

StreamContainer::~StreamContainer() {
}

void StreamContainer::Abort() {
  stream_->handle.reset();
}

base::WeakPtr<StreamContainer> StreamContainer::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

// static
const char MimeHandlerViewGuest::Type[] = "mimehandler";

// static
GuestViewBase* MimeHandlerViewGuest::Create(
    content::BrowserContext* browser_context,
    content::WebContents* owner_web_contents,
    int guest_instance_id) {
  return new MimeHandlerViewGuest(browser_context,
                                  owner_web_contents,
                                  guest_instance_id);
}

MimeHandlerViewGuest::MimeHandlerViewGuest(
    content::BrowserContext* browser_context,
    content::WebContents* owner_web_contents,
    int guest_instance_id)
    : GuestView<MimeHandlerViewGuest>(browser_context,
                                      owner_web_contents,
                                      guest_instance_id),
      delegate_(ExtensionsAPIClient::Get()->CreateMimeHandlerViewGuestDelegate(
          this)) {
}

MimeHandlerViewGuest::~MimeHandlerViewGuest() {
}

WindowController* MimeHandlerViewGuest::GetExtensionWindowController() const {
  return NULL;
}

WebContents* MimeHandlerViewGuest::GetAssociatedWebContents() const {
  return web_contents();
}

const char* MimeHandlerViewGuest::GetAPINamespace() const {
  return "mimeHandlerViewGuestInternal";
}

int MimeHandlerViewGuest::GetTaskPrefix() const {
  return IDS_EXTENSION_TASK_MANAGER_MIMEHANDLERVIEW_TAG_PREFIX;
}

void MimeHandlerViewGuest::CreateWebContents(
    const base::DictionaryValue& create_params,
    const WebContentsCreatedCallback& callback) {
  create_params.GetString(mime_handler_view::kViewId, &view_id_);
  if (view_id_.empty()) {
    callback.Run(nullptr);
    return;
  }
  stream_ =
      MimeHandlerStreamManager::Get(browser_context())->ReleaseStream(view_id_);
  if (!stream_) {
    callback.Run(nullptr);
    return;
  }
  const Extension* mime_handler_extension =
      // TODO(lazyboy): Do we need handle the case where the extension is
      // terminated (ExtensionRegistry::TERMINATED)?
      ExtensionRegistry::Get(browser_context())
          ->enabled_extensions()
          .GetByID(stream_->extension_id());
  if (!mime_handler_extension) {
    LOG(ERROR) << "Extension for mime_type not found, mime_type = "
               << stream_->stream_info()->mime_type;
    callback.Run(NULL);
    return;
  }

  // Use the mime handler extension's SiteInstance to create the guest so it
  // goes under the same process as the extension.
  ProcessManager* process_manager = ProcessManager::Get(browser_context());
  content::SiteInstance* guest_site_instance =
      process_manager->GetSiteInstanceForURL(
          Extension::GetBaseURLFromExtensionId(GetOwnerSiteURL().host()));

  WebContents::CreateParams params(browser_context(), guest_site_instance);
  params.guest_delegate = this;
  callback.Run(WebContents::Create(params));
}

void MimeHandlerViewGuest::DidAttachToEmbedder() {
  web_contents()->GetController().LoadURL(
      stream_->handler_url(), content::Referrer(),
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, std::string());
}

void MimeHandlerViewGuest::DidInitialize(
    const base::DictionaryValue& create_params) {
  extension_function_dispatcher_.reset(
      new ExtensionFunctionDispatcher(browser_context(), this));
  if (delegate_)
    delegate_->AttachHelpers();
}

bool MimeHandlerViewGuest::ZoomPropagatesFromEmbedderToGuest() const {
  return false;
}

bool MimeHandlerViewGuest::Find(int request_id,
                                const base::string16& search_text,
                                const blink::WebFindOptions& options) {
  if (is_full_page_plugin()) {
    web_contents()->Find(request_id, search_text, options);
    return true;
  }
  return false;
}

content::WebContents* MimeHandlerViewGuest::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  return embedder_web_contents()->GetDelegate()->OpenURLFromTab(
      embedder_web_contents(), params);
}

bool MimeHandlerViewGuest::HandleContextMenu(
    const content::ContextMenuParams& params) {
  if (delegate_)
    return delegate_->HandleContextMenu(web_contents(), params);

  return false;
}

void MimeHandlerViewGuest::FindReply(content::WebContents* web_contents,
                                     int request_id,
                                     int number_of_matches,
                                     const gfx::Rect& selection_rect,
                                     int active_match_ordinal,
                                     bool final_update) {
  if (!attached() || !embedder_web_contents()->GetDelegate())
    return;

  embedder_web_contents()->GetDelegate()->FindReply(embedder_web_contents(),
                                                    request_id,
                                                    number_of_matches,
                                                    selection_rect,
                                                    active_match_ordinal,
                                                    final_update);
}

bool MimeHandlerViewGuest::SaveFrame(const GURL& url,
                                     const content::Referrer& referrer) {
  if (!attached())
    return false;

  embedder_web_contents()->SaveFrame(stream_->stream_info()->original_url,
                                     referrer);
  return true;
}

void MimeHandlerViewGuest::DocumentOnLoadCompletedInMainFrame() {
  embedder_web_contents()->Send(
      new ExtensionMsg_MimeHandlerViewGuestOnLoadCompleted(
          element_instance_id()));
}

bool MimeHandlerViewGuest::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MimeHandlerViewGuest, message)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_Request, OnRequest)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

base::WeakPtr<StreamContainer> MimeHandlerViewGuest::GetStream() const {
  if (!stream_)
    return base::WeakPtr<StreamContainer>();
  return stream_->GetWeakPtr();
}

void MimeHandlerViewGuest::OnRequest(
    const ExtensionHostMsg_Request_Params& params) {
  if (extension_function_dispatcher_) {
    extension_function_dispatcher_->Dispatch(
        params, web_contents()->GetRenderViewHost());
  }
}

}  // namespace extensions
