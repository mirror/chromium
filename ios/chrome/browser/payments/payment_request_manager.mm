// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/payments/payment_request_manager.h"

#include "base/ios/ios_util.h"
#import "base/ios/weak_nsobject.h"
#import "base/mac/bind_objc_block.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#import "base/values.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "ios/chrome/browser/autofill/personal_data_manager_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/payments/js_payment_request_manager.h"
#import "ios/chrome/browser/payments/payment_request_coordinator.h"
#import "ios/chrome/browser/payments/payment_request_web_state_observer.h"
#include "ios/web/public/favicon_status.h"
#include "ios/web/public/navigation_item.h"
#include "ios/web/public/navigation_manager.h"
#include "ios/web/public/payments/payment_request.h"
#include "ios/web/public/ssl_status.h"
#import "ios/web/public/url_scheme_util.h"
#import "ios/web/public/web_state/crw_web_view_proxy.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#include "ios/web/public/web_state/url_verification_constants.h"
#include "ios/web/public/web_state/web_state.h"

namespace {
// Command prefix for injected JavaScript.
const std::string kCommandPrefix = "paymentRequest";
}  // namespace

@interface PaymentRequestManager ()<PaymentRequestCoordinatorDelegate,
                                    PaymentRequestWebStateDelegate> {
  // View controller used to present the PaymentRequest view controller.
  base::WeakNSObject<UIViewController> _baseViewController;

  // PersonalDataManager used to manage user credit cards and addresses.
  autofill::PersonalDataManager* _personalDataManager;

  // WebState for the tab this object is attached to.
  web::WebState* _webState;

  // Observer for |_webState|.
  std::unique_ptr<PaymentRequestWebStateObserver> _webStateObserver;

  // Object that manages JavaScript injection into the web view.
  base::WeakNSObject<JSPaymentRequestManager> _paymentRequestJsManager;

  // Boolean to track if the current WebState is enabled (JS callback is set
  // up).
  BOOL _webStateEnabled;

  // Boolean to track if the script has been injected in the current page. This
  // is a faster check than asking the JS controller.
  BOOL _isScriptInjected;

  // True when close has been called and the PaymentRequest coordinator has
  // been destroyed.
  BOOL _closed;

  // Coordinator used to create and present the PaymentRequest view controller.
  base::scoped_nsobject<PaymentRequestCoordinator> _paymentRequestCoordinator;
}

// Synchronous method executed by -asynchronouslyEnablePaymentRequest:
- (void)doEnablePaymentRequest:(BOOL)enabled;

// Initialize the PaymentRequest JavaScript.
- (void)initializeWebViewForPaymentRequest;

// Handler for injected JavaScript callbacks.
- (BOOL)handleScriptCommand:(const base::DictionaryValue&)JSONCommand;

// Handles invocations of PaymentRequest.show(). The value of the JavaScript
// PaymentRequest object should be provided in |message|. Returns YES if the
// invocation was successful.
- (BOOL)handleRequestShow:(const base::DictionaryValue&)message;

// Handles invocations of PaymentResponse.complete(). Returns YES if the
// invocation was successful.
- (BOOL)handleResponseComplete;

@end

@implementation PaymentRequestManager

@synthesize enabled = _enabled;
@synthesize webState = _webState;

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                              browserState:
                                  (ios::ChromeBrowserState*)browserState {
  if ((self = [super init])) {
    _baseViewController.reset(viewController);

    _personalDataManager =
        autofill::PersonalDataManagerFactory::GetForBrowserState(
            browserState->GetOriginalChromeBrowserState());

    // Set up the web state observer. This lasts as long as this object does,
    // but it will observe and un-observe the web tabs as it changes over time.
    _webStateObserver.reset(new PaymentRequestWebStateObserver(self));
  }
  return self;
}

- (instancetype)init {
  NOTREACHED();
  return nil;
}

- (void)setWebState:(web::WebState*)webState {
  [self disconnectWebState];
  if (webState) {
    _paymentRequestJsManager.reset(
        base::mac::ObjCCastStrict<JSPaymentRequestManager>(
            [webState->GetJSInjectionReceiver()
                instanceOfClass:[JSPaymentRequestManager class]]));
    _webState = webState;
    _webStateObserver->ObserveWebState(webState);
    [self enableCurrentWebState];
  } else {
    _webState = nullptr;
  }
}

- (void)enablePaymentRequest:(BOOL)enabled {
  // Asynchronously enables PaymentRequest, so that some preferences
  // (UIAccessibilityIsVoiceOverRunning(), for example) have time to synchronize
  // with their own notifications.
  base::WeakNSObject<PaymentRequestManager> weakSelf(self);
  dispatch_async(dispatch_get_main_queue(), ^{
    [weakSelf doEnablePaymentRequest:enabled];
  });
}

- (void)doEnablePaymentRequest:(BOOL)enabled {
  BOOL changing = _enabled != enabled;
  if (changing) {
    if (!enabled) {
      [self dismissUI];
    }
    _enabled = enabled;
    [self enableCurrentWebState];
  }
}

- (void)cancelRequest {
  [self dismissUI];
  [_paymentRequestJsManager rejectRequestPromise:@"Request cancelled by user."
                               completionHandler:nil];
}

- (void)close {
  if (_closed)
    return;

  _closed = YES;
  [self disableCurrentWebState];
  [self setWebState:nil];
  [self dismissUI];
}

- (void)enableCurrentWebState {
  if (![self webState]) {
    return;
  }

  if (_enabled && [self webStateContentIsSecureHTML]) {
    if (!_webStateEnabled) {
      base::WeakNSObject<PaymentRequestManager> weakSelf(self);
      auto callback =
          base::BindBlock(^bool(const base::DictionaryValue& JSON,
                                const GURL& originURL, bool userIsInteracting) {
            base::scoped_nsobject<PaymentRequestManager> strongSelf(
                [weakSelf retain]);
            // |originURL| and |userIsInteracting| aren't used.
            return [strongSelf handleScriptCommand:JSON];
          });
      [self webState]->AddScriptCommandCallback(callback, kCommandPrefix);

      _webStateEnabled = YES;
    }

    [self initializeWebViewForPaymentRequest];
  } else {
    [self disableCurrentWebState];
  }
}

- (void)disableCurrentWebState {
  if (_webStateEnabled) {
    _webState->RemoveScriptCommandCallback(kCommandPrefix);
    _webStateEnabled = NO;
  }
}

- (void)disconnectWebState {
  if (_webState) {
    _paymentRequestJsManager.reset();
    _webStateObserver->ObserveWebState(nullptr);
    [self disableCurrentWebState];
  }
}

- (void)initializeWebViewForPaymentRequest {
  DCHECK(_webStateEnabled);

  if (![self webStateContentIsSecureHTML]) {
    return;
  }

  [_paymentRequestJsManager inject];
  _isScriptInjected = YES;
}

- (BOOL)handleScriptCommand:(const base::DictionaryValue&)JSONCommand {
  if (![self webStateContentIsSecureHTML]) {
    return NO;
  }

  std::string command;
  if (!JSONCommand.GetString("command", &command)) {
    DLOG(ERROR) << "RECEIVED BAD JSON - NO 'command' field";
    return NO;
  }

  if (command == "paymentRequest.requestShow") {
    return [self handleRequestShow:JSONCommand];
  }
  if (command == "paymentRequest.responseComplete") {
    return [self handleResponseComplete];
  }
  return NO;
}

- (BOOL)handleRequestShow:(const base::DictionaryValue&)message {
  // TODO(crbug.com/602666): check that there's not already a pending request.
  // TODO(crbug.com/602666): compare our supported payment types (i.e. autofill
  //   credit card types) against the merchant supported types and return NO
  //   if the intersection is empty.

  const base::DictionaryValue* paymentRequestData;
  web::PaymentRequest paymentRequest;
  if (!message.GetDictionary("payment_request", &paymentRequestData)) {
    DLOG(ERROR) << "JS message parameter 'payment_request' is missing";
    return NO;
  }
  if (!paymentRequest.FromDictionaryValue(*paymentRequestData)) {
    DLOG(ERROR) << "JS message parameter 'payment_request' is invalid";
    return NO;
  }

  UIImage* pageFavicon = nil;
  web::NavigationItem* navigationItem =
      [self webState]->GetNavigationManager()->GetVisibleItem();
  if (navigationItem && !navigationItem->GetFavicon().image.IsEmpty())
    pageFavicon = navigationItem->GetFavicon().image.ToUIImage();
  NSString* pageTitle = base::SysUTF16ToNSString([self webState]->GetTitle());
  NSString* pageHost =
      base::SysUTF8ToNSString([self webState]->GetLastCommittedURL().host());
  _paymentRequestCoordinator.reset([[PaymentRequestCoordinator alloc]
      initWithBaseViewController:_baseViewController
             personalDataManager:_personalDataManager]);
  [_paymentRequestCoordinator setPaymentRequest:paymentRequest];
  [_paymentRequestCoordinator setPageFavicon:pageFavicon];
  [_paymentRequestCoordinator setPageTitle:pageTitle];
  [_paymentRequestCoordinator setPageHost:pageHost];
  [_paymentRequestCoordinator setDelegate:self];

  [_paymentRequestCoordinator start];

  return YES;
}

- (BOOL)handleResponseComplete {
  // TODO(crbug.com/602666): Check that there *is* a pending response here.
  // TODO(crbug.com/602666): Indicate success or failure in the UI.

  [self dismissUI];

  // TODO(crbug.com/602666): Reject the promise on failure.
  [_paymentRequestJsManager resolveResponsePromise:nil];

  return YES;
}

- (void)dismissUI {
  [_paymentRequestCoordinator stop];
  _paymentRequestCoordinator.reset();
}

- (BOOL)webStateContentIsSecureHTML {
  if (![self webState]) {
    return NO;
  }

  if (!web::UrlHasWebScheme([self webState]->GetLastCommittedURL()) ||
      ![self webState]->ContentIsHTML()) {
    return NO;
  }

  const web::NavigationItem* navigationItem =
      [self webState]->GetNavigationManager()->GetLastCommittedItem();
  return navigationItem &&
         navigationItem->GetSSL().security_style ==
             web::SECURITY_STYLE_AUTHENTICATED;
}

#pragma mark - PaymentRequestCoordinatorDelegate methods

- (void)paymentRequestCoordinatorDidCancel {
  [self cancelRequest];
}

- (void)paymentRequestCoordinatorDidConfirm:
    (web::PaymentResponse)paymentResponse {
  [_paymentRequestJsManager resolveRequestPromise:paymentResponse
                                completionHandler:nil];
}

#pragma mark - PaymentRequestWebStateDelegate methods

- (void)pageLoadedWithStatus:(web::PageLoadCompletionStatus)loadStatus {
  if (loadStatus != web::PageLoadCompletionStatus::SUCCESS)
    return;

  [self dismissUI];
  _isScriptInjected = NO;
  [self enableCurrentWebState];
}

@end
