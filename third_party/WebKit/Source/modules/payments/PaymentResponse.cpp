// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/payments/PaymentResponse.h"

#include "bindings/core/v8/JSONValuesForV8.h"
#include "modules/payments/PaymentAddress.h"
#include "modules/payments/PaymentCompleter.h"
#include "wtf/Assertions.h"

namespace mojo {

template <>
struct TypeConverter<blink::CurrencyAmount, blink::mojom::blink::CurrencyAmount> {
    static blink::CurrencyAmount Convert(const blink::mojom::blink::CurrencyAmount& input)
    {
        blink::CurrencyAmount output;
        output.setCurrency(input.currency);
        output.setValue(input.value);
        return output;
    }
};

} // namespace mojo

namespace blink {

PaymentResponse::PaymentResponse(mojom::blink::PaymentResponsePtr response, PaymentCompleter* paymentCompleter)
    : m_methodName(response->method_name)
    , m_totalAmount(response->total_amount->To<CurrencyAmount>())
    , m_stringifiedDetails(response->stringified_details)
    , m_shippingAddress(response->shipping_address ? new PaymentAddress(std::move(response->shipping_address)) : nullptr)
    , m_payerEmail(response->payer_email)
    , m_payerPhone(response->payer_phone)
    , m_paymentCompleter(paymentCompleter)
{
    DCHECK(m_paymentCompleter);
}

PaymentResponse::~PaymentResponse()
{
}

ScriptValue PaymentResponse::details(ScriptState* scriptState, ExceptionState& exceptionState) const
{
    return ScriptValue(scriptState, fromJSONString(scriptState, m_stringifiedDetails, exceptionState));
}

ScriptPromise PaymentResponse::complete(ScriptState* scriptState, bool success)
{
    return m_paymentCompleter->complete(scriptState, success);
}

DEFINE_TRACE(PaymentResponse)
{
    visitor->trace(m_shippingAddress);
    visitor->trace(m_paymentCompleter);
}

} // namespace blink
