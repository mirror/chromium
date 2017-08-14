// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_QR_SCANNER_REQUIREMENTS_QR_SCANNER_PRESENTING_H_
#define IOS_CHROME_BROWSER_UI_QR_SCANNER_REQUIREMENTS_QR_SCANNER_PRESENTING_H_

@protocol QRScannerPresenting

- (void)presentQRScannerViewController:(UIViewController*)controller;

- (void)dismissQRScannerViewController:(UIViewController*)controller
                            completion:(void (^)(void))completion;

@end

#endif  // IOS_CHROME_BROWSER_UI_QR_SCANNER_REQUIREMENTS_QR_SCANNER_PRESENTING_H_
