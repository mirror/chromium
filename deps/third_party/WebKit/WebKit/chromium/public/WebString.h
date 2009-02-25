/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebString_h
#define WebString_h

#include "WebCommon.h"

#if defined(WEBKIT_IMPLEMENTATION)
namespace WebCore { class String; }
#else
#include <base/string16.h>
#endif

namespace WebKit {

    class WebStringPrivate;

    // A UTF-16 string container
    class WebString {
    public:
        ~WebString() { reset(); }

        WebString() : m_private(0) { }
        WebString(const WebString& s) : m_private(0) { assign(s); }

        WebString& operator=(const WebString& s)
        {
            assign(s);
            return *this;
        }

        WEBKIT_API void reset();
        WEBKIT_API void assign(const WebString&);
        WEBKIT_API void assign(const WebUChar* characters, size_t len);

        WEBKIT_API size_t length() const;
        WEBKIT_API const WebUChar* characters() const;

        bool isEmpty() const { return length() == 0; }
        bool isNull() const { return m_private == 0; }

#if defined(WEBKIT_IMPLEMENTATION)
        WebString(const WebCore::String&);
        WebString& operator=(const WebCore::String&);
        operator WebCore::String() const;
#else
        WebString(const string16& s) : m_private(0)
        {
            assign(s.data(), s.length());
        }

        WebString& operator=(const string16& s)
        {
            assign(s.data(), s.length());
            return *this;
        }

        operator string16() const
        {
            size_t len = length();
            return len ? string16(characters(), len) : string16();
        }
#endif

    private:
        void assign(WebStringPrivate*);
        WebStringPrivate* m_private;
    };

} // namespace WebKit

#endif
