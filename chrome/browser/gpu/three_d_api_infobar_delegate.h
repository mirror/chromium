// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GPU_THREE_D_API_INFOBAR_DELEGATE_H_
#define CHROME_BROWSER_GPU_THREE_D_API_INFOBAR_DELEGATE_H_

#include "components/infobars/core/confirm_infobar_delegate.h"
#include "content/public/common/three_d_api_types.h"
#include "url/gurl.h"

class InfoBarService;

class ThreeDAPIInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  // Creates a 3D API infobar and delegate and adds the infobar to
  // |infobar_service|.
  static void Create(InfoBarService* infobar_service,
                     const GURL& url,
                     content::ThreeDAPIType requester);

 private:
  enum DismissalHistogram {
    IGNORED,
    RELOADED,
    CLOSED_WITHOUT_ACTION,
    DISMISSAL_MAX
  };

  ThreeDAPIInfoBarDelegate(const GURL& url, content::ThreeDAPIType requester);
  ~ThreeDAPIInfoBarDelegate() override;

  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  int GetIconId() const override;
  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override;
  ThreeDAPIInfoBarDelegate* AsThreeDAPIInfoBarDelegate() override;
  base::string16 GetMessageText() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
  bool Cancel() override;

  GURL url_;
  content::ThreeDAPIType requester_;
  // Basically indicates whether the infobar was displayed at all, or
  // was a temporary instance thrown away by the InfobarService.
  mutable bool message_text_queried_;
  bool action_taken_;

  DISALLOW_COPY_AND_ASSIGN(ThreeDAPIInfoBarDelegate);
};

#endif  // CHROME_BROWSER_GPU_THREE_D_API_INFOBAR_DELEGATE_H_
