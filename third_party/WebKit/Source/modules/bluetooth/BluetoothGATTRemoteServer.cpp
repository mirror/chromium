// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/bluetooth/BluetoothGATTRemoteServer.h"

#include "bindings/core/v8/CallbackPromiseAdapter.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/page/Page.h"
#include "modules/bluetooth/BluetoothError.h"
#include "modules/bluetooth/BluetoothGATTService.h"
#include "modules/bluetooth/BluetoothSupplement.h"
#include "modules/bluetooth/BluetoothUUID.h"
#include "public/platform/modules/bluetooth/WebBluetooth.h"
#include "wtf/OwnPtr.h"

namespace blink {

BluetoothGATTRemoteServer::BluetoothGATTRemoteServer(const String& deviceId)
    : m_deviceId(deviceId)
    , m_connected(false)
{
}

BluetoothGATTRemoteServer* BluetoothGATTRemoteServer::create(const String& deviceId)
{
    return new BluetoothGATTRemoteServer(deviceId);
}

void BluetoothGATTRemoteServer::disconnectIfConnected(ExecutionContext* context)
{
    if (m_connected) {
        m_connected = false;
        WebBluetooth* webbluetooth = BluetoothSupplement::fromExecutionContext(context);
        webbluetooth->disconnect(m_deviceId);
    }
}

class ConnectCallback : public WebBluetoothGATTServerConnectCallbacks {
public:
    ConnectCallback(BluetoothGATTRemoteServer* gatt, ScriptPromiseResolver* resolver)
        : m_gatt(gatt)
        , m_resolver(resolver) {}

    void onSuccess() override
    {
        if (!m_resolver->executionContext() || m_resolver->executionContext()->activeDOMObjectsAreStopped())
            return;
        m_gatt->setConnected(true);
        if (!toDocument(m_resolver->executionContext())->page()->isPageVisible()) {
            m_gatt->disconnectIfConnected(m_resolver->executionContext());
        }
        m_resolver->resolve(m_gatt);
    }

    void onError(const WebBluetoothError& e) override
    {
        if (!m_resolver->executionContext() || m_resolver->executionContext()->activeDOMObjectsAreStopped())
            return;
        m_resolver->reject(BluetoothError::take(m_resolver, e));
    }
private:
    Persistent<BluetoothGATTRemoteServer> m_gatt;
    Persistent<ScriptPromiseResolver> m_resolver;
};

ScriptPromise BluetoothGATTRemoteServer::connect(ScriptState* scriptState)
{
    // TODO(ortuno): Allow connections when the tab is in the background.
    // This is a short term solution instead of implementing a tab indicator
    // for bluetooth connections.
    // https://crbug.com/579746
    if (!toDocument(scriptState->executionContext())->page()->isPageVisible()) {
        return ScriptPromise::rejectWithDOMException(scriptState, DOMException::create(SecurityError, "Connection is only allowed while the page is visible. This is a temporary measure until we are able to effectively communicate to the user that a page is connected to a device."));
    }

    WebBluetooth* webbluetooth = BluetoothSupplement::fromScriptState(scriptState);
    if (!webbluetooth)
        return ScriptPromise::rejectWithDOMException(scriptState, DOMException::create(NotSupportedError));

    ScriptPromiseResolver* resolver = ScriptPromiseResolver::create(scriptState);
    ScriptPromise promise = resolver->promise();
    webbluetooth->connect(m_deviceId, new ConnectCallback(this, resolver));
    return promise;
}

void BluetoothGATTRemoteServer::disconnect(ScriptState* scriptState)
{
    m_connected = false;
    WebBluetooth* webbluetooth = BluetoothSupplement::fromScriptState(scriptState);
    webbluetooth->disconnect(m_deviceId);
}

ScriptPromise BluetoothGATTRemoteServer::getPrimaryService(ScriptState* scriptState, const StringOrUnsignedLong& service, ExceptionState& exceptionState)
{
    WebBluetooth* webbluetooth = BluetoothSupplement::fromScriptState(scriptState);

    String serviceUUID = BluetoothUUID::getService(service, exceptionState);
    if (exceptionState.hadException())
        return exceptionState.reject(scriptState);

    ScriptPromiseResolver* resolver = ScriptPromiseResolver::create(scriptState);
    ScriptPromise promise = resolver->promise();
    webbluetooth->getPrimaryService(m_deviceId, serviceUUID, new CallbackPromiseAdapter<BluetoothGATTService, BluetoothError>(resolver));

    return promise;
}

} // namespace blink
