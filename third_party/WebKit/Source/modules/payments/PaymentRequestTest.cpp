// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/payments/PaymentRequest.h"

#include "bindings/core/v8/JSONValuesForV8.h"
#include "bindings/core/v8/ScriptFunction.h"
#include "bindings/core/v8/V8BindingForTesting.h"
#include "bindings/modules/v8/V8PaymentResponse.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "modules/payments/PaymentAddress.h"
#include "modules/payments/PaymentResponse.h"
#include "modules/payments/PaymentTestHelper.h"
#include "platform/heap/HeapAllocator.h"
#include "testing/gtest/include/gtest/gtest.h"
#include <utility>

namespace blink {
namespace {

TEST(PaymentRequestTest, SecureContextRequired)
{
    V8TestingScope scope;
    scope.document().setSecurityOrigin(SecurityOrigin::create(KURL(KURL(), "http://www.example.com/")));

    PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());

    EXPECT_TRUE(scope.getExceptionState().hadException());
    EXPECT_EQ(SecurityError, scope.getExceptionState().code());
}

TEST(PaymentRequestTest, NoExceptionWithValidData)
{
    V8TestingScope scope;
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());

    EXPECT_FALSE(scope.getExceptionState().hadException());
}


TEST(PaymentRequestTest, SupportedMethodListRequired)
{
    V8TestingScope scope;
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest::create(scope.getScriptState(), HeapVector<PaymentMethodData>(), buildPaymentDetailsForTest(), scope.getExceptionState());

    EXPECT_TRUE(scope.getExceptionState().hadException());
    EXPECT_EQ(V8TypeError, scope.getExceptionState().code());
}

TEST(PaymentRequestTest, TotalRequired)
{
    V8TestingScope scope;
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), PaymentDetails(), scope.getExceptionState());

    EXPECT_TRUE(scope.getExceptionState().hadException());
    EXPECT_EQ(V8TypeError, scope.getExceptionState().code());
}

TEST(PaymentRequestTest, NullShippingOptionWhenNoOptionsAvailable)
{
    V8TestingScope scope;
    makePaymentRequestOriginSecure(scope.document());
    PaymentDetails details;
    details.setTotal(buildPaymentItemForTest());
    PaymentOptions options;
    options.setRequestShipping(true);

    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), details, options, scope.getExceptionState());

    EXPECT_TRUE(request->shippingOption().isNull());
}

TEST(PaymentRequestTest, NullShippingOptionWhenMultipleOptionsAvailable)
{
    V8TestingScope scope;
    makePaymentRequestOriginSecure(scope.document());
    PaymentDetails details;
    details.setTotal(buildPaymentItemForTest());
    details.setShippingOptions(HeapVector<ShippingOption>(2, buildShippingOptionForTest()));
    PaymentOptions options;
    options.setRequestShipping(true);

    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), details, options, scope.getExceptionState());

    EXPECT_TRUE(request->shippingOption().isNull());
}

TEST(PaymentRequestTest, DontSelectSingleAvailableShippingOptionByDefault)
{
    V8TestingScope scope;
    makePaymentRequestOriginSecure(scope.document());
    PaymentDetails details;
    details.setTotal(buildPaymentItemForTest());
    details.setShippingOptions(HeapVector<ShippingOption>(1, buildShippingOptionForTest(PaymentTestDataId, PaymentTestOverwriteValue, "standard")));

    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), details, scope.getExceptionState());

    EXPECT_TRUE(request->shippingOption().isNull());
}

TEST(PaymentRequestTest, DontSelectSingleAvailableShippingOptionWhenShippingNotRequested)
{
    V8TestingScope scope;
    makePaymentRequestOriginSecure(scope.document());
    PaymentDetails details;
    details.setTotal(buildPaymentItemForTest());
    details.setShippingOptions(HeapVector<ShippingOption>(1, buildShippingOptionForTest()));
    PaymentOptions options;
    options.setRequestShipping(false);

    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), details, options, scope.getExceptionState());

    EXPECT_TRUE(request->shippingOption().isNull());
}

TEST(PaymentRequestTest, DontSelectSingleUnselectedShippingOptionWhenShippingRequested)
{
    V8TestingScope scope;
    makePaymentRequestOriginSecure(scope.document());
    PaymentDetails details;
    details.setTotal(buildPaymentItemForTest());
    details.setShippingOptions(HeapVector<ShippingOption>(1, buildShippingOptionForTest()));
    PaymentOptions options;
    options.setRequestShipping(true);

    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), details, options, scope.getExceptionState());

    EXPECT_TRUE(request->shippingOption().isNull());
}

TEST(PaymentRequestTest, SelectSingleSelectedShippingOptionWhenShippingRequested)
{
    V8TestingScope scope;
    makePaymentRequestOriginSecure(scope.document());
    PaymentDetails details;
    details.setTotal(buildPaymentItemForTest());
    HeapVector<ShippingOption> shippingOptions(1, buildShippingOptionForTest(PaymentTestDataId, PaymentTestOverwriteValue, "standard"));
    shippingOptions[0].setSelected(true);
    details.setShippingOptions(shippingOptions);
    PaymentOptions options;
    options.setRequestShipping(true);

    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), details, options, scope.getExceptionState());

    EXPECT_EQ("standard", request->shippingOption());
}

TEST(PaymentRequestTest, SelectOnlySelectedShippingOptionWhenShippingRequested)
{
    V8TestingScope scope;
    makePaymentRequestOriginSecure(scope.document());
    PaymentDetails details;
    details.setTotal(buildPaymentItemForTest());
    HeapVector<ShippingOption> shippingOptions(2);
    shippingOptions[0] = buildShippingOptionForTest(PaymentTestDataId, PaymentTestOverwriteValue, "standard");
    shippingOptions[0].setSelected(true);
    shippingOptions[1] = buildShippingOptionForTest(PaymentTestDataId, PaymentTestOverwriteValue, "express");
    details.setShippingOptions(shippingOptions);
    PaymentOptions options;
    options.setRequestShipping(true);

    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), details, options, scope.getExceptionState());

    EXPECT_EQ("standard", request->shippingOption());
}

TEST(PaymentRequestTest, SelectLastSelectedShippingOptionWhenShippingRequested)
{
    V8TestingScope scope;
    makePaymentRequestOriginSecure(scope.document());
    PaymentDetails details;
    details.setTotal(buildPaymentItemForTest());
    HeapVector<ShippingOption> shippingOptions(2);
    shippingOptions[0] = buildShippingOptionForTest(PaymentTestDataId, PaymentTestOverwriteValue, "standard");
    shippingOptions[0].setSelected(true);
    shippingOptions[1] = buildShippingOptionForTest(PaymentTestDataId, PaymentTestOverwriteValue, "express");
    shippingOptions[1].setSelected(true);
    details.setShippingOptions(shippingOptions);
    PaymentOptions options;
    options.setRequestShipping(true);

    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), details, options, scope.getExceptionState());

    EXPECT_EQ("express", request->shippingOption());
}

TEST(PaymentRequestTest, RejectShowPromiseOnInvalidShippingAddress)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnShippingAddressChange(mojom::blink::PaymentAddress::New());
}

TEST(PaymentRequestTest, RejectShowPromiseWithRequestShippingTrueAndEmptyShippingAddressInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestShipping(true);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = buildPaymentResponseForTest();

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
}

TEST(PaymentRequestTest, RejectShowPromiseWithRequestShippingTrueAndInvalidShippingAddressInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestShipping(true);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = buildPaymentResponseForTest();
    response->shipping_address = mojom::blink::PaymentAddress::New();

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
}

TEST(PaymentRequestTest, RejectShowPromiseWithRequestShippingFalseAndShippingAddressExistsInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestShipping(false);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = mojom::blink::PaymentResponse::New();
    response->shipping_address = mojom::blink::PaymentAddress::New();
    response->shipping_address->country = "US";
    response->shipping_address->language_code = "en";
    response->shipping_address->script_code = "Latn";

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
}

class PaymentResponseFunction : public ScriptFunction {
public:
    static v8::Local<v8::Function> create(ScriptState* scriptState, ScriptValue* outValue)
    {
        PaymentResponseFunction* self = new PaymentResponseFunction(scriptState, outValue);
        return self->bindToV8Function();
    }

    ScriptValue call(ScriptValue value) override
    {
        DCHECK(!value.isEmpty());
        *m_value = value;
        return value;
    }

private:
    PaymentResponseFunction(ScriptState* scriptState, ScriptValue* outValue)
        : ScriptFunction(scriptState)
        , m_value(outValue)
    {
        DCHECK(m_value);
    }

    ScriptValue* m_value;
};

TEST(PaymentRequestTest, ResolveShowPromiseWithRequestShippingTrueAndValidShippingAddressInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestShipping(true);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = buildPaymentResponseForTest();
    response->shipping_address = mojom::blink::PaymentAddress::New();
    response->shipping_address->country = "US";
    response->shipping_address->language_code = "en";
    response->shipping_address->script_code = "Latn";

    ScriptValue outValue;
    request->show(scope.getScriptState()).then(PaymentResponseFunction::create(scope.getScriptState(), &outValue), funcs.expectNoCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
    v8::MicrotasksScope::PerformCheckpoint(scope.isolate());
    PaymentResponse* pr = V8PaymentResponse::toImplWithTypeCheck(scope.isolate(), outValue.v8Value());

    EXPECT_EQ("US", pr->shippingAddress()->country());
    EXPECT_EQ("en-Latn", pr->shippingAddress()->languageCode());
}

TEST(PaymentRequestTest, ResolveShowPromiseWithRequestShippingFalseAndEmptyShippingAddressInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestShipping(false);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());

    ScriptValue outValue;
    request->show(scope.getScriptState()).then(PaymentResponseFunction::create(scope.getScriptState(), &outValue), funcs.expectNoCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(buildPaymentResponseForTest());
    v8::MicrotasksScope::PerformCheckpoint(scope.isolate());
    PaymentResponse* pr = V8PaymentResponse::toImplWithTypeCheck(scope.isolate(), outValue.v8Value());

    EXPECT_EQ(nullptr, pr->shippingAddress());
}

TEST(PaymentRequestTest, OnShippingOptionChange)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectNoCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnShippingOptionChange("standardShipping");
}

TEST(PaymentRequestTest, CannotCallShowTwice)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    request->show(scope.getScriptState());

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());
}

TEST(PaymentRequestTest, CannotCallCompleteTwice)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    request->show(scope.getScriptState());
    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(buildPaymentResponseForTest());
    request->complete(scope.getScriptState(), false);

    request->complete(scope.getScriptState(), true).then(funcs.expectNoCall(), funcs.expectCall());
}

TEST(PaymentRequestTest, RejectShowPromiseOnError)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnError();
}

TEST(PaymentRequestTest, RejectCompletePromiseOnError)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    request->show(scope.getScriptState());
    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(buildPaymentResponseForTest());

    request->complete(scope.getScriptState(), true).then(funcs.expectNoCall(), funcs.expectCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnError();
}

TEST(PaymentRequestTest, ResolvePromiseOnComplete)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    request->show(scope.getScriptState());
    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(buildPaymentResponseForTest());

    request->complete(scope.getScriptState(), true).then(funcs.expectCall(), funcs.expectNoCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnComplete();
}

TEST(PaymentRequestTest, RejectShowPromiseOnUpdateDetailsFailure)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    request->onUpdatePaymentDetailsFailure(ScriptValue::from(scope.getScriptState(), "oops"));
}

TEST(PaymentRequestTest, RejectCompletePromiseOnUpdateDetailsFailure)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    request->show(scope.getScriptState()).then(funcs.expectCall(), funcs.expectNoCall());
    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(buildPaymentResponseForTest());

    request->complete(scope.getScriptState(), true).then(funcs.expectNoCall(), funcs.expectCall());

    request->onUpdatePaymentDetailsFailure(ScriptValue::from(scope.getScriptState(), "oops"));
}

TEST(PaymentRequestTest, IgnoreUpdatePaymentDetailsAfterShowPromiseResolved)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    request->show(scope.getScriptState()).then(funcs.expectCall(), funcs.expectNoCall());
    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(buildPaymentResponseForTest());

    request->onUpdatePaymentDetails(ScriptValue::from(scope.getScriptState(), "foo"));
}

TEST(PaymentRequestTest, RejectShowPromiseOnNonPaymentDetailsUpdate)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    request->onUpdatePaymentDetails(ScriptValue::from(scope.getScriptState(), "NotPaymentDetails"));
}

TEST(PaymentRequestTest, RejectShowPromiseOnInvalidPaymentDetailsUpdate)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    request->onUpdatePaymentDetails(ScriptValue::from(scope.getScriptState(), fromJSONString(scope.getScriptState(), "{}", scope.getExceptionState())));
    EXPECT_FALSE(scope.getExceptionState().hadException());
}

TEST(PaymentRequestTest, ClearShippingOptionOnPaymentDetailsUpdateWithoutShippingOptions)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentDetails details;
    details.setTotal(buildPaymentItemForTest());
    PaymentOptions options;
    options.setRequestShipping(true);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), details, options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    EXPECT_TRUE(request->shippingOption().isNull());
    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectNoCall());
    String detailWithShippingOptions = "{\"total\": {\"label\": \"Total\", \"amount\": {\"currency\": \"USD\", \"value\": \"5.00\"}},"
        "\"shippingOptions\": [{\"id\": \"standardShippingOption\", \"label\": \"Standard shipping\", \"amount\": {\"currency\": \"USD\", \"value\": \"5.00\"}, \"selected\": true}]}";
    request->onUpdatePaymentDetails(ScriptValue::from(scope.getScriptState(), fromJSONString(scope.getScriptState(), detailWithShippingOptions, scope.getExceptionState())));
    EXPECT_FALSE(scope.getExceptionState().hadException());
    EXPECT_EQ("standardShippingOption", request->shippingOption());
    String detailWithoutShippingOptions = "{\"total\": {\"label\": \"Total\", \"amount\": {\"currency\": \"USD\", \"value\": \"5.00\"}}}";

    request->onUpdatePaymentDetails(ScriptValue::from(scope.getScriptState(), fromJSONString(scope.getScriptState(), detailWithoutShippingOptions, scope.getExceptionState())));

    EXPECT_FALSE(scope.getExceptionState().hadException());
    EXPECT_TRUE(request->shippingOption().isNull());
}

TEST(PaymentRequestTest, ClearShippingOptionOnPaymentDetailsUpdateWithMultipleUnselectedShippingOptions)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestShipping(true);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectNoCall());
    String detail = "{\"total\": {\"label\": \"Total\", \"amount\": {\"currency\": \"USD\", \"value\": \"5.00\"}},"
        "\"shippingOptions\": [{\"id\": \"slow\", \"label\": \"Slow\", \"amount\": {\"currency\": \"USD\", \"value\": \"5.00\"}},"
        "{\"id\": \"fast\", \"label\": \"Fast\", \"amount\": {\"currency\": \"USD\", \"value\": \"50.00\"}}]}";

    request->onUpdatePaymentDetails(ScriptValue::from(scope.getScriptState(), fromJSONString(scope.getScriptState(), detail, scope.getExceptionState())));
    EXPECT_FALSE(scope.getExceptionState().hadException());

    EXPECT_TRUE(request->shippingOption().isNull());
}

TEST(PaymentRequestTest, UseTheSelectedShippingOptionFromPaymentDetailsUpdate)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestShipping(true);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectNoCall());
    String detail = "{\"total\": {\"label\": \"Total\", \"amount\": {\"currency\": \"USD\", \"value\": \"5.00\"}},"
        "\"shippingOptions\": [{\"id\": \"slow\", \"label\": \"Slow\", \"amount\": {\"currency\": \"USD\", \"value\": \"5.00\"}},"
        "{\"id\": \"fast\", \"label\": \"Fast\", \"amount\": {\"currency\": \"USD\", \"value\": \"50.00\"}, \"selected\": true}]}";

    request->onUpdatePaymentDetails(ScriptValue::from(scope.getScriptState(), fromJSONString(scope.getScriptState(), detail, scope.getExceptionState())));
    EXPECT_FALSE(scope.getExceptionState().hadException());

    EXPECT_EQ("fast", request->shippingOption());
}

TEST(PaymentRequestTest, ResolveShowPromiseWithRequestPayerEmailTrueAndValidPayerEmailInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestPayerEmail(true);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = mojom::blink::PaymentResponse::New();
    response->total_amount = mojom::blink::CurrencyAmount::New();
    response->payer_email = "abc@gmail.com";

    ScriptValue outValue;
    request->show(scope.getScriptState()).then(PaymentResponseFunction::create(scope.getScriptState(), &outValue), funcs.expectNoCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
    v8::MicrotasksScope::PerformCheckpoint(scope.isolate());
    PaymentResponse* pr = V8PaymentResponse::toImplWithTypeCheck(scope.isolate(), outValue.v8Value());

    EXPECT_EQ("abc@gmail.com", pr->payerEmail());
}

TEST(PaymentRequestTest, RejectShowPromiseWithRequestPayerEmailTrueAndEmptyPayerEmailInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestPayerEmail(true);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = mojom::blink::PaymentResponse::New();
    response->total_amount = mojom::blink::CurrencyAmount::New();
    response->payer_email = "";

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
}

TEST(PaymentRequestTest, RejectShowPromiseWithRequestPayerEmailTrueAndNullPayerEmailInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestPayerEmail(true);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = mojom::blink::PaymentResponse::New();
    response->total_amount = mojom::blink::CurrencyAmount::New();
    response->payer_email = String();

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
}

TEST(PaymentRequestTest, RejectShowPromiseWithRequestPayerEmailFalseAndNonNullPayerEmailInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestPayerEmail(false);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = mojom::blink::PaymentResponse::New();
    response->total_amount = mojom::blink::CurrencyAmount::New();
    response->payer_email = "";

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
}

TEST(PaymentRequestTest, ResolveShowPromiseWithRequestPayerEmailFalseAndNullPayerEmailInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestPayerEmail(false);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = mojom::blink::PaymentResponse::New();
    response->total_amount = mojom::blink::CurrencyAmount::New();
    response->payer_email = String();

    ScriptValue outValue;
    request->show(scope.getScriptState()).then(PaymentResponseFunction::create(scope.getScriptState(), &outValue), funcs.expectNoCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
    v8::MicrotasksScope::PerformCheckpoint(scope.isolate());
    PaymentResponse* pr = V8PaymentResponse::toImplWithTypeCheck(scope.isolate(), outValue.v8Value());

    EXPECT_TRUE(pr->payerEmail().isNull());
}

TEST(PaymentRequestTest, ResolveShowPromiseWithRequestPayerPhoneTrueAndValidPayerPhoneInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestPayerPhone(true);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = mojom::blink::PaymentResponse::New();
    response->total_amount = mojom::blink::CurrencyAmount::New();
    response->payer_phone = "0123";

    ScriptValue outValue;
    request->show(scope.getScriptState()).then(PaymentResponseFunction::create(scope.getScriptState(), &outValue), funcs.expectNoCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
    v8::MicrotasksScope::PerformCheckpoint(scope.isolate());
    PaymentResponse* pr = V8PaymentResponse::toImplWithTypeCheck(scope.isolate(), outValue.v8Value());

    EXPECT_EQ("0123", pr->payerPhone());
}

TEST(PaymentRequestTest, RejectShowPromiseWithRequestPayerPhoneTrueAndEmptyPayerPhoneInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestPayerPhone(true);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = mojom::blink::PaymentResponse::New();
    response->total_amount = mojom::blink::CurrencyAmount::New();
    response->payer_phone = "";

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
}

TEST(PaymentRequestTest, RejectShowPromiseWithRequestPayerPhoneTrueAndNullPayerPhoneInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestPayerPhone(true);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = mojom::blink::PaymentResponse::New();
    response->total_amount = mojom::blink::CurrencyAmount::New();
    response->payer_phone = String();

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
}

TEST(PaymentRequestTest, RejectShowPromiseWithRequestPayerPhoneFalseAndNonNulPayerPhoneInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestPayerPhone(false);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = mojom::blink::PaymentResponse::New();
    response->total_amount = mojom::blink::CurrencyAmount::New();
    response->payer_phone = "";

    request->show(scope.getScriptState()).then(funcs.expectNoCall(), funcs.expectCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
}

TEST(PaymentRequestTest, ResolveShowPromiseWithRequestPayerPhoneFalseAndNullPayerPhoneInResponse)
{
    V8TestingScope scope;
    PaymentRequestMockFunctionScope funcs(scope.getScriptState());
    makePaymentRequestOriginSecure(scope.document());
    PaymentOptions options;
    options.setRequestPayerPhone(false);
    PaymentRequest* request = PaymentRequest::create(scope.getScriptState(), buildPaymentMethodDataForTest(), buildPaymentDetailsForTest(), options, scope.getExceptionState());
    EXPECT_FALSE(scope.getExceptionState().hadException());
    mojom::blink::PaymentResponsePtr response = mojom::blink::PaymentResponse::New();
    response->total_amount = mojom::blink::CurrencyAmount::New();
    response->payer_phone = String();

    ScriptValue outValue;
    request->show(scope.getScriptState()).then(PaymentResponseFunction::create(scope.getScriptState(), &outValue), funcs.expectNoCall());

    static_cast<mojom::blink::PaymentRequestClient*>(request)->OnPaymentResponse(std::move(response));
    v8::MicrotasksScope::PerformCheckpoint(scope.isolate());
    PaymentResponse* pr = V8PaymentResponse::toImplWithTypeCheck(scope.isolate(), outValue.v8Value());

    EXPECT_TRUE(pr->payerPhone().isNull());
}

} // namespace
} // namespace blink
