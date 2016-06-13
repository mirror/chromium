// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "web/CompositorProxyClientImpl.h"

#include "core/dom/CompositorProxy.h"
#include "modules/compositorworker/CompositorWorkerGlobalScope.h"
#include "platform/TraceEvent.h"
#include "web/CompositorMutatorImpl.h"
#include "wtf/CurrentTime.h"

namespace blink {

CompositorProxyClientImpl::CompositorProxyClientImpl(CompositorMutatorImpl* mutator)
    : m_mutator(mutator)
    , m_globalScope(nullptr)
{
}

DEFINE_TRACE(CompositorProxyClientImpl)
{
    CompositorProxyClient::trace(visitor);
    visitor->trace(m_mutator);
    visitor->trace(m_globalScope);
    visitor->trace(m_proxies);
}

void CompositorProxyClientImpl::setGlobalScope(WorkerGlobalScope* scope)
{
    TRACE_EVENT0("compositor-worker", "CompositorProxyClientImpl::setGlobalScope");
    DCHECK(!m_globalScope);
    DCHECK(scope);
    m_globalScope = static_cast<CompositorWorkerGlobalScope*>(scope);
    m_mutator->registerProxyClient(this);
}

void CompositorProxyClientImpl::requestAnimationFrame()
{
    TRACE_EVENT0("compositor-worker", "CompositorProxyClientImpl::requestAnimationFrame");
    m_requestedAnimationFrameCallbacks = true;
    m_mutator->setNeedsMutate();
}

bool CompositorProxyClientImpl::mutate(double monotonicTimeNow)
{
    if (!m_globalScope)
        return false;

    TRACE_EVENT0("compositor-worker", "CompositorProxyClientImpl::mutate");
    if (!m_requestedAnimationFrameCallbacks)
        return false;

    m_requestedAnimationFrameCallbacks = executeAnimationFrameCallbacks(monotonicTimeNow);

    return m_requestedAnimationFrameCallbacks;
}

bool CompositorProxyClientImpl::executeAnimationFrameCallbacks(double monotonicTimeNow)
{
    TRACE_EVENT0("compositor-worker", "CompositorProxyClientImpl::executeAnimationFrameCallbacks");
    // Convert to zero based document time in milliseconds consistent with requestAnimationFrame.
    double highResTimeMs = 1000.0 * (monotonicTimeNow - m_globalScope->timeOrigin());
    const bool shouldReinvoke = m_globalScope->executeAnimationFrameCallbacks(highResTimeMs);
    return shouldReinvoke;
}

void CompositorProxyClientImpl::registerCompositorProxy(CompositorProxy* proxy)
{
    m_proxies.add(proxy);
}

void CompositorProxyClientImpl::unregisterCompositorProxy(CompositorProxy* proxy)
{
    m_proxies.remove(proxy);
}

} // namespace blink
