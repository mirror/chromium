// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/payments/PaymentTestHelper.h"

#include "bindings/core/v8/ScriptState.h"
#include "core/dom/Document.h"
#include "modules/payments/CurrencyAmount.h"
#include "modules/payments/PaymentMethodData.h"
#include "platform/heap/HeapAllocator.h"

namespace blink {
namespace {

// PaymentItem and ShippingOption have identical structure
// except for the "id" field, which is present only in ShippingOption.
template <typename PaymentItemOrShippingOption>
void setValues(PaymentItemOrShippingOption& original, PaymentTestDataToChange data, PaymentTestModificationType modificationType, const String& valueToUse)
{
    CurrencyAmount itemAmount;
    if (data == PaymentTestDataCurrencyCode) {
        if (modificationType == PaymentTestOverwriteValue)
            itemAmount.setCurrency(valueToUse);
    } else {
        itemAmount.setCurrency("USD");
    }
    if (data == PaymentTestDataValue) {
        if (modificationType == PaymentTestOverwriteValue)
            itemAmount.setValue(valueToUse);
    } else {
        itemAmount.setValue("9.99");
    }

    if (data != PaymentTestDataAmount || modificationType != PaymentTestRemoveKey)
        original.setAmount(itemAmount);

    if (data == PaymentTestDataLabel) {
        if (modificationType == PaymentTestOverwriteValue)
            original.setLabel(valueToUse);
    } else {
        original.setLabel("Label");
    }
}

} // namespace

PaymentItem buildPaymentItemForTest(PaymentTestDataToChange data, PaymentTestModificationType modificationType, const String& valueToUse)
{
    DCHECK_NE(data, PaymentTestDataId);
    PaymentItem item;
    setValues(item, data, modificationType, valueToUse);
    return item;
}

ShippingOption buildShippingOptionForTest(PaymentTestDataToChange data, PaymentTestModificationType modificationType, const String& valueToUse)
{
    ShippingOption shippingOption;
    if (data == PaymentTestDataId) {
        if (modificationType == PaymentTestOverwriteValue)
            shippingOption.setId(valueToUse);
    } else {
        shippingOption.setId("id");
    }
    setValues(shippingOption, data, modificationType, valueToUse);
    return shippingOption;
}

PaymentDetails buildPaymentDetailsForTest(PaymentTestDetailToChange detail, PaymentTestDataToChange data, PaymentTestModificationType modificationType, const String& valueToUse)
{
    PaymentItem total;
    if (detail == PaymentTestDetailTotal)
        total = buildPaymentItemForTest(data, modificationType, valueToUse);
    else
        total = buildPaymentItemForTest();

    PaymentItem item;
    if (detail == PaymentTestDetailItem)
        item = buildPaymentItemForTest(data, modificationType, valueToUse);
    else
        item = buildPaymentItemForTest();

    ShippingOption shippingOption;
    if (detail == PaymentTestDetailShippingOption)
        shippingOption = buildShippingOptionForTest(data, modificationType, valueToUse);
    else
        shippingOption = buildShippingOptionForTest();

    PaymentDetails result;
    result.setTotal(total);
    result.setDisplayItems(HeapVector<PaymentItem>(1, item));
    result.setShippingOptions(HeapVector<ShippingOption>(2, shippingOption));

    return result;
}

HeapVector<PaymentMethodData> buildPaymentMethodDataForTest()
{
    HeapVector<PaymentMethodData> methodData(1, PaymentMethodData());
    methodData[0].setSupportedMethods(Vector<String>(1, "foo"));
    return methodData;
}

mojom::blink::PaymentResponsePtr buildPaymentResponseForTest()
{
    mojom::blink::PaymentResponsePtr result = mojom::blink::PaymentResponse::New();
    result->total_amount = mojom::blink::CurrencyAmount::New();
    return result;
}

void makePaymentRequestOriginSecure(Document& document)
{
    document.setSecurityOrigin(SecurityOrigin::create(KURL(KURL(), "https://www.example.com/")));
}

PaymentRequestMockFunctionScope::PaymentRequestMockFunctionScope(ScriptState* scriptState)
    : m_scriptState(scriptState)
{
}

PaymentRequestMockFunctionScope::~PaymentRequestMockFunctionScope()
{
    v8::MicrotasksScope::PerformCheckpoint(m_scriptState->isolate());
    for (MockFunction* mockFunction : m_mockFunctions) {
        testing::Mock::VerifyAndClearExpectations(mockFunction);
    }
}

v8::Local<v8::Function> PaymentRequestMockFunctionScope::expectCall()
{
    m_mockFunctions.append(new MockFunction(m_scriptState));
    EXPECT_CALL(*m_mockFunctions.last(), call(testing::_));
    return m_mockFunctions.last()->bind();
}

v8::Local<v8::Function> PaymentRequestMockFunctionScope::expectNoCall()
{
    m_mockFunctions.append(new MockFunction(m_scriptState));
    EXPECT_CALL(*m_mockFunctions.last(), call(testing::_)).Times(0);
    return m_mockFunctions.last()->bind();
}

PaymentRequestMockFunctionScope::MockFunction::MockFunction(ScriptState* scriptState)
    : ScriptFunction(scriptState)
{
    ON_CALL(*this, call(testing::_)).WillByDefault(testing::ReturnArg<0>());
}

v8::Local<v8::Function> PaymentRequestMockFunctionScope::MockFunction::bind()
{
    return bindToV8Function();
}

} // namespace blink
