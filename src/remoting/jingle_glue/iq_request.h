// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_JINGLE_GLUE_IQ_REQUEST_H_
#define REMOTING_JINGLE_GLUE_IQ_REQUEST_H_

#include <string>

#include "base/callback_old.h"
#include "base/gtest_prod_util.h"

namespace buzz {
class XmlElement;
}  // namespace buzz

namespace remoting {

// IqRequest class can be used to send an IQ stanza and then receive reply
// stanza for that request. It sends outgoing stanza when SendIq() is called,
// after that it forwards incoming reply stanza to the callback set with
// set_callback(). If each call to SendIq() will yield one invocation of the
// callback with the response.
class IqRequest {
 public:
  typedef Callback1<const buzz::XmlElement*>::Type ReplyCallback;

  IqRequest() {}
  virtual ~IqRequest() {}

  // Sends stanza of type |type| to |addressee|. |iq_body| contains body of
  // the stanza. Ownership of |iq_body| is transfered to IqRequest. Must
  // be called on the jingle thread.
  virtual void SendIq(const std::string& type, const std::string& addressee,
                      buzz::XmlElement* iq_body) = 0;

  // Sets callback that is called when reply stanza is received. Callback
  // is called on the jingle thread.
  virtual void set_callback(ReplyCallback* callback) = 0;

 protected:
  static buzz::XmlElement* MakeIqStanza(const std::string& type,
                                        const std::string& addressee,
                                        buzz::XmlElement* iq_body,
                                        const std::string& id);

 private:
  FRIEND_TEST_ALL_PREFIXES(IqRequestTest, MakeIqStanza);

  DISALLOW_COPY_AND_ASSIGN(IqRequest);
};

}  // namespace remoting

#endif  // REMOTING_JINGLE_GLUE_IQ_REQUEST_H_
