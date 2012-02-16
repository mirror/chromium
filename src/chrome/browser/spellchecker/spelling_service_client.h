// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SPELLCHECKER_SPELLING_SERVICE_CLIENT_H_
#define CHROME_BROWSER_SPELLCHECKER_SPELLING_SERVICE_CLIENT_H_
#pragma once

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/string16.h"
#include "content/public/common/url_fetcher_delegate.h"

class Profile;
class TextCheckClientDelegate;

namespace WebKit {
struct WebTextCheckingResult;
}

// A class that encapsulates a JSON-RPC call to the Spelling service to check
// text there. This class creates a JSON-RPC request, sends the request to the
// service with URLFetcher, parses a response from the service, and calls a
// provided callback method. When a user deletes this object before it finishes
// a JSON-RPC call, this class cancels the JSON-RPC call without calling the
// callback method. A simple usage is creating a SpellingServiceClient and
// calling its RequestTextCheck method as listed in the following snippet.
//
//   class MyClient {
//    public:
//     MyClient();
//     virtual ~MyClient();
//
//     void OnTextCheckComplete(
//         int tag,
//         const std::vector<WebKit::WebTextCheckingResult>& results) {
//       ...
//     }
//
//     void MyTextCheck(Profile* profile, const string16& text) {
//        client_.reset(new SpellingServiceClient);
//        client_->RequestTextCheck(profile, 0, text,
//            base::Bind(&MyClient::OnTextCheckComplete,
//                       base::Unretained(this));
//     }
//    private:
//     scoped_ptr<SpellingServiceClient> client_;
//   };
//
class SpellingServiceClient : public content::URLFetcherDelegate {
 public:
  typedef base::Callback<void(
      int /* tag */,
      const std::vector<WebKit::WebTextCheckingResult>& /* results */)>
          TextCheckCompleteCallback;

  SpellingServiceClient();
  virtual ~SpellingServiceClient();

  // content::URLFetcherDelegate implementation.
  virtual void OnURLFetchComplete(const content::URLFetcher* source) OVERRIDE;

  // Sends a text-check request to the Spelling service. When we send a request
  // to the Spelling service successfully, this function returns true. (This
  // does not mean the service finishes checking text successfully.) We will
  // call |callback| when we receive a text-check response from the service.
  bool RequestTextCheck(Profile* profile,
                        int tag,
                        const string16& text,
                        const TextCheckCompleteCallback& callback);

 private:
  // Parses a JSON-RPC response from the Spelling service.
  bool ParseResponse(const std::string& data,
                     std::vector<WebKit::WebTextCheckingResult>* results);

  // The URLFetcher object used for sending a JSON-RPC request.
  scoped_ptr<content::URLFetcher> fetcher_;

  // The callback function to be called when we receive a response from the
  // Spelling service and parse it.
  TextCheckCompleteCallback callback_;

  // The identifier provided by users so they can identify a text-check request.
  // When a JSON-RPC call finishes successfully, this value is used as the
  // first parameter to |callback_|.
  int tag_;
};

#endif  // CHROME_BROWSER_SPELLCHECKER_SPELLING_SERVICE_CLIENT_H_
