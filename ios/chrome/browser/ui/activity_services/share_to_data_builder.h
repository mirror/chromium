// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_SHARE_TO_DATA_BUILDER_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_SHARE_TO_DATA_BUILDER_H_

@class ShareToData;
@class Tab;

namespace web {
class WebState;
}  // namespace web

namespace activity_services {

// Returns a ShareToData object using data from |tab| and |web_state|. It is an
// error to pass a different WebState than the one for the given Tab.  |tab|
// must not be nil, but |web_state| can be.  This function may return nil.
//
// TODO(crbug.com/681867): This function takes both a WebState and a Tab because
// some functionality (thumbnail generation and printing) have not been moved
// out of Tab yet.  Once that functionality is moved to tab helpers, remove the
// Tab argument from this function.
ShareToData* ShareToDataForTab(Tab* tab, const web::WebState* web_state);

}  // namespace activity_services

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_SHARE_TO_DATA_BUILDER_H_
