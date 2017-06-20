// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/pdf/chrome_pdf_web_contents_helper_client.h"

#include "chrome/browser/download/download_stats.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tab_contents/core_tab_helper.h"
#include "content/public/browser/render_widget_host_view.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
#include "ui/gfx/geometry/point_f.h"

namespace {

content::WebContents* GetWebContentsToUse(
    content::WebContents* web_contents) {
  // If we're viewing the PDF in a MimeHandlerViewGuest, use its embedder
  // WebContents.
  auto* guest_view =
      extensions::MimeHandlerViewGuest::FromWebContents(web_contents);
  if (guest_view)
    return guest_view->embedder_web_contents();
  return web_contents;
}

}  // namespace

ChromePDFWebContentsHelperClient::ChromePDFWebContentsHelperClient() :
touch_selection_client_manager_(nullptr) {
}

void ChromePDFWebContentsHelperClient::InitTouchSelectionClientManager(
    content::WebContents* contents) {
  DCHECK(contents);
  auto* view = contents->GetRenderWidgetHostView();
  if (!view)
    return;

  touch_selection_client_manager_ =
      view->touch_selection_controller_client_manager();
  if (!touch_selection_client_manager_)
    return;

  touch_selection_client_manager_->AddObserver(this);
}

ChromePDFWebContentsHelperClient::~ChromePDFWebContentsHelperClient() {
  if (touch_selection_client_manager_) {
    touch_selection_client_manager_->InvalidateClient(this);
    touch_selection_client_manager_->RemoveObserver(this);
  }
}

void ChromePDFWebContentsHelperClient::UpdateContentRestrictions(
    content::WebContents* contents,
    int content_restrictions) {
  CoreTabHelper* core_tab_helper =
      CoreTabHelper::FromWebContents(GetWebContentsToUse(contents));
  // |core_tab_helper| is NULL for WebViewGuest.
  if (core_tab_helper)
    core_tab_helper->UpdateContentRestrictions(content_restrictions);
}

void ChromePDFWebContentsHelperClient::OnPDFHasUnsupportedFeature(
    content::WebContents* contents) {
  // There is no more Adobe pluging for PDF so there is not much we can do in
  // this case. Maybe simply download the file.
}

void ChromePDFWebContentsHelperClient::OnSaveURL(
    content::WebContents* contents) {
  RecordDownloadSource(DOWNLOAD_INITIATED_BY_PDF_SAVE);
}

void ChromePDFWebContentsHelperClient::OnSelectionChanged(
    content::WebContents* contents,
    const gfx::Point& left,
    int32_t left_height,
    const gfx::Point& right,
    int32_t right_height) {
  if (!touch_selection_client_manager_)
    InitTouchSelectionClientManager(contents);

  fprintf(stderr, "Selection %d,%d x %d -> %d,%d x %d\n", left.x(), left.y(),
          left_height, right.x(), right.y(), right_height);
  if (touch_selection_client_manager_) {
    gfx::SelectionBound start;
    gfx::SelectionBound end;
    start.SetEdgeTop(gfx::PointF(left.x(), left.y()));
    start.SetEdgeTop(gfx::PointF(left.x(), left.y() + left_height));
    end.SetEdgeTop(gfx::PointF(right.x(), right.y()));
    end.SetEdgeTop(gfx::PointF(right.x(), right.y() + right_height));
    touch_selection_client_manager_->UpdateClientSelectionBounds(
        start, end, this, nullptr);
  }
}

bool ChromePDFWebContentsHelperClient::SupportsAnimation() const {
  return false;
}

void ChromePDFWebContentsHelperClient::MoveCaret(const gfx::PointF& position) {}

void ChromePDFWebContentsHelperClient::MoveRangeSelectionExtent(
    const gfx::PointF& extent) {}

void ChromePDFWebContentsHelperClient::SelectBetweenCoordinates(
    const gfx::PointF& base,
    const gfx::PointF& extent) {}

void ChromePDFWebContentsHelperClient::OnSelectionEvent(
    ui::SelectionEventType event) {}

std::unique_ptr<ui::TouchHandleDrawable>
ChromePDFWebContentsHelperClient::CreateDrawable() {
  // We can return null here, as the manager will look after this.
  return std::unique_ptr<ui::TouchHandleDrawable>();
}

void ChromePDFWebContentsHelperClient::OnManagerWillDestroy(
      ui::TouchSelectionControllerClientManager* manager) {
  DCHECK(manager == touch_selection_client_manager_);
  manager->RemoveObserver(this);
  touch_selection_client_manager_ = nullptr;
}
