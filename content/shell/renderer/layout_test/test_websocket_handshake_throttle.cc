// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "test_websocket_handshake_throttle.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "url/gurl.h"

namespace {

using Callbacks = blink::WebCallbacks<void, const blink::WebString&>;

// Checks for a valid "throttle-delay-ms" parameter and returns it as a
// TimeDelta if it exists. Otherwise returns a zero TimeDelta.
base::TimeDelta ExtractDelayFromUrl(const GURL& url) {
  if (!url.has_query())
    return base::TimeDelta();

  base::StringPiece query = url.query_piece();
  url::Component component(0, static_cast<int>(query.length()));
  url::Component key;
  url::Component value;
  while (url::ExtractQueryKeyValue(query.data(), &component, &key, &value)) {
    base::StringPiece key_piece = query.substr(key.begin, key.len);
    if (key_piece != "throttle-delay-ms")
      continue;

    base::StringPiece value_piece = query.substr(value.begin, value.len);
    int value_int;
    if (!base::StringToInt(value_piece, &value_int) || value_int < 0)
      return base::TimeDelta();

    return base::TimeDelta::FromMilliseconds(value_int);
  }

  // Parameter was not found.
  return base::TimeDelta();
}

}  // namespace

namespace content {

void TestWebSocketHandshakeThrottle::ThrottleHandshake(const blink::WebURL& url,
                                                       blink::WebLocalFrame*,
                                                       Callbacks* callbacks) {
  DCHECK(callbacks);

  // This use of Unretained is safe because this object is destroyed before
  // |callbacks| is freed. Destroying this object prevents the timer from
  // firing.
  timer_.Start(
      FROM_HERE, ExtractDelayFromUrl(url),
      base::BindRepeating(&Callbacks::OnSuccess, base::Unretained(callbacks)));
}

}  // namespace content
