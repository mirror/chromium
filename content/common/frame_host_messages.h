// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "build/build_config.h"
#include "content/common/frame_message_enums.h"
#include "content/common/navigation_gesture.h"
#include "content/public/common/frame_navigate_params.h"
#include "content/public/common/page_state.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/WebKit/public/platform/WebInsecureRequestPolicy.h"

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT

IPC_STRUCT_TRAITS_BEGIN(content::FrameNavigateParams)
  IPC_STRUCT_TRAITS_MEMBER(nav_entry_id)
  IPC_STRUCT_TRAITS_MEMBER(frame_unique_name)
  IPC_STRUCT_TRAITS_MEMBER(item_sequence_number)
  IPC_STRUCT_TRAITS_MEMBER(document_sequence_number)
  IPC_STRUCT_TRAITS_MEMBER(url)
  IPC_STRUCT_TRAITS_MEMBER(base_url)
  IPC_STRUCT_TRAITS_MEMBER(referrer)
  IPC_STRUCT_TRAITS_MEMBER(transition)
  IPC_STRUCT_TRAITS_MEMBER(redirects)
  IPC_STRUCT_TRAITS_MEMBER(should_update_history)
  IPC_STRUCT_TRAITS_MEMBER(contents_mime_type)
  IPC_STRUCT_TRAITS_MEMBER(socket_address)
IPC_STRUCT_TRAITS_END()

// Parameters structure for FrameHostMsg_DidCommitProvisionalLoad, which has
// too many data parameters to be reasonably put in a predefined IPC message.
IPC_STRUCT_BEGIN_WITH_PARENT(FrameHostMsg_DidCommitProvisionalLoad_Params,
                             content::FrameNavigateParams)
  IPC_STRUCT_TRAITS_PARENT(content::FrameNavigateParams)

  // This is the value from the browser (copied from the navigation request)
  // indicating whether it intended to make a new entry. TODO(avi): Remove this
  // when the pending entry situation is made sane and the browser keeps them
  // around long enough to match them via nav_entry_id.
  IPC_STRUCT_MEMBER(bool, intended_as_new_entry)

  // Whether this commit created a new entry.
  IPC_STRUCT_MEMBER(bool, did_create_new_entry)

  // Whether this commit should replace the current entry.
  IPC_STRUCT_MEMBER(bool, should_replace_current_entry)

  // The gesture that initiated this navigation.
  IPC_STRUCT_MEMBER(content::NavigationGesture, gesture)

  // The HTTP method used by the navigation.content/public/common/page_state.h
  IPC_STRUCT_MEMBER(std::string, method)

  // The POST body identifier. -1 if it doesn't exist.
  IPC_STRUCT_MEMBER(int64_t, post_id)

  // Whether the frame navigation resulted in no change of the document within
  // the frame. For example, the navigation may have just resulted in
  // scrolling to a named anchor.
  IPC_STRUCT_MEMBER(bool, was_within_same_document)

  // The status code of the HTTP request.
  IPC_STRUCT_MEMBER(int, http_status_code)

  // This flag is used to warn if the renderer is displaying an error page,
  // so that we can set the appropriate page type.
  IPC_STRUCT_MEMBER(bool, url_is_unreachable)

  // Serialized history item state to store in the navigation entry.
  IPC_STRUCT_MEMBER(content::PageState, page_state)

  // Original request's URL.
  IPC_STRUCT_MEMBER(GURL, original_request_url)

  // User agent override used to navigate.
  IPC_STRUCT_MEMBER(bool, is_overriding_user_agent)

  // Notifies the browser that for this navigation, the session history was
  // successfully cleared.
  IPC_STRUCT_MEMBER(bool, history_list_was_cleared)

  // The routing_id of the render view associated with the navigation.
  // We need to track the RenderViewHost routing_id because of downstream
  // dependencies (crbug.com/392171 DownloadRequestHandle, SaveFileManager,
  // ResourceDispatcherHostImpl, MediaStreamUIProxy,
  // SpeechRecognitionDispatcherHost and possibly others). They look up the view
  // based on the ID stored in the resource requests. Once those dependencies
  // are unwound or moved to RenderFrameHost (crbug.com/304341) we can move the
  // client to be based on the routing_id of the RenderFrameHost.
  IPC_STRUCT_MEMBER(int, render_view_routing_id)

  // Origin of the frame.  This will be replicated to any associated
  // RenderFrameProxies.
  IPC_STRUCT_MEMBER(url::Origin, origin)

  // How navigation metrics starting on UI action for this load should be
  // reported.
  IPC_STRUCT_MEMBER(FrameMsg_UILoadMetricsReportType::Value, report_type)

  // Timestamp at which the UI action that triggered the navigation originated.
  IPC_STRUCT_MEMBER(base::TimeTicks, ui_timestamp)

  // The insecure request policy the document for the load is enforcing.
  IPC_STRUCT_MEMBER(blink::WebInsecureRequestPolicy, insecure_request_policy)

  // True if the document for the load is a unique origin that should be
  // considered potentially trustworthy.
  IPC_STRUCT_MEMBER(bool, has_potentially_trustworthy_unique_origin)

  // See WebSearchableFormData for a description of these.
  // Not used by PlzNavigate: in that case these fields are sent to the browser
  // in BeginNavigationParams.
  IPC_STRUCT_MEMBER(GURL, searchable_form_url)
  IPC_STRUCT_MEMBER(std::string, searchable_form_encoding)

  // This is a non-decreasing value that the browser process can use to
  // identify and discard compositor frames that correspond to now-unloaded
  // web content.
  IPC_STRUCT_MEMBER(uint32_t, content_source_id)
IPC_STRUCT_END()
