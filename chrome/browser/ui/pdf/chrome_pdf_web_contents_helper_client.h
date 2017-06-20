// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PDF_CHROME_PDF_WEB_CONTENTS_HELPER_CLIENT_H_
#define CHROME_BROWSER_UI_PDF_CHROME_PDF_WEB_CONTENTS_HELPER_CLIENT_H_

#include "base/macros.h"
#include "components/pdf/browser/pdf_web_contents_helper_client.h"
#include "ui/touch_selection/selection_event_type.h"
#include "ui/touch_selection/touch_selection_controller.h"
#include "ui/touch_selection/touch_selection_controller_client_manager.h"

namespace ui {
class TouchHandleDrawable;
}  // namespace ui

class ChromePDFWebContentsHelperClient
    : public pdf::PDFWebContentsHelperClient,
      public ui::TouchSelectionControllerClient,
      public ui::TouchSelectionControllerClientManager::Observer {
 public:
  ChromePDFWebContentsHelperClient();
  ~ChromePDFWebContentsHelperClient() override;


 private:
  // pdf::PDFWebContentsHelperClient:
  void UpdateContentRestrictions(content::WebContents* contents,
                                 int content_restrictions) override;

  void OnPDFHasUnsupportedFeature(content::WebContents* contents) override;

  void OnSaveURL(content::WebContents* contents) override;

  void OnSelectionChanged(content::WebContents* contents,
                          const gfx::Point& left,
                          int32_t left_height,
                          const gfx::Point& right,
                          int32_t right_height) override;

  void InitTouchSelectionClientManager(content::WebContents* contents) override;

  // ui::TouchSelectionControllerClient:
  bool SupportsAnimation() const override;
  void SetNeedsAnimate() override {}
  void MoveCaret(const gfx::PointF& position) override;
  void MoveRangeSelectionExtent(const gfx::PointF& extent) override;
  void SelectBetweenCoordinates(const gfx::PointF& base,
                                const gfx::PointF& extent) override;
  void OnSelectionEvent(ui::SelectionEventType event) override;
  std::unique_ptr<ui::TouchHandleDrawable> CreateDrawable() override;

  // ui::TouchSelectionControllerClientManager::Observer:
  void OnManagerWillDestroy(
      ui::TouchSelectionControllerClientManager* manager) override;

  ui::TouchSelectionControllerClientManager* touch_selection_client_manager_;

  DISALLOW_COPY_AND_ASSIGN(ChromePDFWebContentsHelperClient);
};

#endif  // CHROME_BROWSER_UI_PDF_CHROME_PDF_WEB_CONTENTS_HELPER_CLIENT_H_
