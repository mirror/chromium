/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef PerformanceTiming_h
#define PerformanceTiming_h

#include "core/CoreExport.h"
#include "core/dom/ContextLifecycleObserver.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"

namespace blink {

class CSSTiming;
class DocumentLoadTiming;
class DocumentLoader;
class DocumentParserTiming;
class DocumentTiming;
class LocalFrame;
class PaintTiming;
class ResourceLoadTiming;
class ScriptState;
class ScriptValue;

// Legacy support for NT1(https://www.w3.org/TR/navigation-timing/).
class CORE_EXPORT PerformanceTiming final
    : public GarbageCollected<PerformanceTiming>,
      public ScriptWrappable,
      public DOMWindowClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(PerformanceTiming);

 public:
  static PerformanceTiming* Create(LocalFrame* frame) {
    return new PerformanceTiming(frame);
  }

#define PERFORMANCE_TIMING_SPEC_LIST(V) \
  V(navigationStart)                    \
  V(unloadEventStart)                   \
  V(unloadEventEnd)                     \
  V(redirectStart)                      \
  V(redirectEnd)                        \
  V(fetchStart)                         \
  V(domainLookupStart)                  \
  V(domainLookupEnd)                    \
  V(connectStart)                       \
  V(connectEnd)                         \
  V(secureConnectionStart)              \
  V(requestStart)                       \
  V(responseStart)                      \
  V(responseEnd)                        \
  V(domLoading)                         \
  V(domInteractive)                     \
  V(domContentLoadedEventStart)         \
  V(domContentLoadedEventEnd)           \
  V(domComplete)                        \
  V(loadEventStart)                     \
  V(loadEventEnd)

// The below are non-spec timings, for Page Load UMA metrics.
#define PERFORMANCE_TIMING_NON_SPEC_LIST(V)                                   \
  /* The time the first document layout is performed. */                      \
  V(FirstLayout)                                                              \
  /* The time the first paint operation was performed. */                     \
  V(FirstPaint)                                                               \
  /* The time the first paint operation for visible text was performed. */    \
  V(FirstTextPaint)                                                           \
  /* The time the first paint operation for image was performed. */           \
  V(FirstImagePaint)                                                          \
  /* The time of the first 'contentful' paint. A contentful paint is a paint  \
     that includes content of some kind (for example, text or image content). \
   */                                                                         \
  V(FirstContentfulPaint)                                                     \
  /* The time of the first 'meaningful' paint. A meaningful paint is a paint  \
     where the page's primary content is visible. */                          \
  V(FirstMeaningfulPaint)                                                     \
  V(ParseStart)                                                               \
  V(ParseStop)                                                                \
  V(ParseBlockedOnScriptLoadDuration)                                         \
  V(ParseBlockedOnScriptLoadFromDocumentWriteDuration)                        \
  V(ParseBlockedOnScriptExecutionDuration)                                    \
  V(ParseBlockedOnScriptExecutionFromDocumentWriteDuration)                   \
  V(AuthorStyleSheetParseDurationBeforeFCP)                                   \
  V(UpdateStyleDurationBeforeFCP)

#define PERFORMANCE_TIMING_LIST(V) \
  PERFORMANCE_TIMING_SPEC_LIST(V)  \
  PERFORMANCE_TIMING_NON_SPEC_LIST(V)

#define DECLARE_SPEC_TIMING_ACCESSOR(name) unsigned long long name() const;
  PERFORMANCE_TIMING_LIST(DECLARE_SPEC_TIMING_ACCESSOR)
#undef DECLARE_SPEC_TIMING_ACCESSOR

  ScriptValue toJSONForBinding(ScriptState*) const;

  DECLARE_VIRTUAL_TRACE();

  unsigned long long MonotonicTimeToIntegerMilliseconds(double) const;
  double IntegerMillisecondsToMonotonicTime(unsigned long long) const;

 private:
  explicit PerformanceTiming(LocalFrame*);

  const DocumentTiming* GetDocumentTiming() const;
  const CSSTiming* CssTiming() const;
  const DocumentParserTiming* GetDocumentParserTiming() const;
  const PaintTiming* GetPaintTiming() const;
  DocumentLoader* GetDocumentLoader() const;
  DocumentLoadTiming* GetDocumentLoadTiming() const;
  ResourceLoadTiming* GetResourceLoadTiming() const;
};

}  // namespace blink

#endif  // PerformanceTiming_h
