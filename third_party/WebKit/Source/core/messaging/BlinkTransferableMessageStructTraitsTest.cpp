// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/V8ArrayBuffer.h"
#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "core/messaging/BlinkTransferableMessage.h"
#include "core/messaging/BlinkTransferableMessageStructTraits.h"
#include "core/typed_arrays/DOMArrayBuffer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/Source/bindings/core/v8/V8BindingForTesting.h"
#include "third_party/WebKit/common/message_port/message_port.mojom-blink.h"
#include "third_party/WebKit/common/message_port/message_port_channel.h"

#include "bindings/core/v8/V8ImageBitmap.h"
#include "bindings/core/v8/serialization/V8ScriptValueDeserializer.h"
#include "core/messaging/MessagePort.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace blink {

namespace {

class BlinkTransferableMessageStructTraitsTest : public ::testing::Test {
 protected:
  BlinkTransferableMessageStructTraitsTest() {}
  ~BlinkTransferableMessageStructTraitsTest() override {}

  DISALLOW_COPY_AND_ASSIGN(BlinkTransferableMessageStructTraitsTest);
};

}  // namespace

TEST_F(BlinkTransferableMessageStructTraitsTest,
       ArrayBufferTransferOutOfScopeSucceeds) {
  // Out of scope
  mojo::Message mojo_message = []() -> mojo::Message {
    V8TestingScope scope;
    v8::Isolate* isolate = scope.GetIsolate();
    size_t num_elements = 8;
    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(isolate, num_elements);
    uint8_t* contents = static_cast<uint8_t*>(ab->GetContents().Data());
    for (size_t i = 0; i < num_elements; i++)
      contents[i] = static_cast<uint8_t>(i);

    DOMArrayBuffer* array_buffer =
        V8ArrayBuffer::ToImpl(v8::Local<v8::Object>::Cast(ab));
    Transferables transferables;
    transferables.array_buffers.push_back(array_buffer);
    SerializedScriptValue::SerializeOptions options;
    options.transferables = &transferables;
    ExceptionState exceptionState(isolate, ExceptionState::kExecutionContext,
                                  "MessageChannel", "postMessage");
    scoped_refptr<SerializedScriptValue> ssv =
        SerializedScriptValue::Serialize(isolate, ab, options, exceptionState);
    BlinkTransferableMessage msg;
    msg.message = ssv;
    mojo::Message mm =
        mojom::blink::TransferableMessage::SerializeAsMessage(&msg);
    return mm;
  }();

  BlinkTransferableMessage out;
  bool deserialized = mojom::blink::TransferableMessage::DeserializeFromMessage(
      std::move(mojo_message), &out);
  ASSERT_TRUE(deserialized);
  ASSERT_EQ(out.message->GetArrayBufferContentsArray().size(), 1U);
  WTF::ArrayBufferContents& abc = out.message->GetArrayBufferContentsArray()[0];
  std::vector<uint8_t> contents(static_cast<uint8_t*>(abc.Data()),
                                static_cast<uint8_t*>(abc.Data()) + 8);

  for (uint8_t i = 0; i < 8; i++) {
    ASSERT_TRUE(contents[i] == i);
  }
  auto& abcs = out.message->GetArrayBufferContentsArray();
  abcs.clear();
}

TEST_F(BlinkTransferableMessageStructTraitsTest,
       ArrayBufferContentsLazySerializationSucceeds) {
  // Out of scope
  V8TestingScope scope;
  v8::Isolate* isolate = scope.GetIsolate();
  size_t num_elements = 8;
  v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(isolate, num_elements);
  void* originalContentsData = ab->GetContents().Data();
  uint8_t* contents = static_cast<uint8_t*>(originalContentsData);
  for (size_t i = 0; i < num_elements; i++)
    contents[i] = static_cast<uint8_t>(i);

  DOMArrayBuffer* original_array_buffer =
      V8ArrayBuffer::ToImpl(v8::Local<v8::Object>::Cast(ab));
  Transferables transferables;
  transferables.array_buffers.push_back(original_array_buffer);
  SerializedScriptValue::SerializeOptions options;
  options.transferables = &transferables;
  ExceptionState exceptionState(isolate, ExceptionState::kExecutionContext,
                                "MessageChannel", "postMessage");
  scoped_refptr<SerializedScriptValue> ssv =
      SerializedScriptValue::Serialize(isolate, ab, options, exceptionState);
  BlinkTransferableMessage msg;
  msg.message = ssv;
  mojo::Message mojo_message =
      mojom::blink::TransferableMessage::WrapAsMessage(std::move(msg));

  BlinkTransferableMessage out;
  ASSERT_TRUE(mojom::blink::TransferableMessage::DeserializeFromMessage(
      std::move(mojo_message), &out));
  ASSERT_EQ(out.message->GetArrayBufferContentsArray().size(), 1U);

  // When using WrapAsMessage, the deserialized ArrayBufferContents should own
  // the original ArrayBufferContents' data (as opposed to a copy of the data).
  WTF::ArrayBufferContents& deserialized_contents =
      out.message->GetArrayBufferContentsArray()[0];
  ASSERT_EQ(originalContentsData, deserialized_contents.Data());

  // The original ArrayBufferContents should be neutered.
  ASSERT_EQ(NULL, ab->GetContents().Data());
  ASSERT_TRUE(original_array_buffer->IsNeutered());
}

ImageBitmap* CreateBitmap() {
  sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(8, 4);
  surface->getCanvas()->clear(SK_ColorRED);
  return ImageBitmap::Create(
      StaticBitmapImage::Create(surface->makeImageSnapshot()));
}

void AssertBitmapsEqual(ImageBitmap* b1, ImageBitmap* b2) {
  EXPECT_EQ(b1->width(), b2->width());
  EXPECT_EQ(b1->height(), b2->height());
}

TEST_F(BlinkTransferableMessageStructTraitsTest,
       BitmapTransferOutOfScopeSucceeds) {
  // Out of scope
  mojo::Message mojo_message = []() -> mojo::Message {
    V8TestingScope scope;
    v8::Isolate* isolate = scope.GetIsolate();
    ImageBitmap* image_bitmap = CreateBitmap();
    // Serialize and deserialize it.
    v8::Local<v8::Value> wrapper = ToV8(image_bitmap, scope.GetScriptState());

    Transferables transferables;
    transferables.image_bitmaps.push_back(std::move(image_bitmap));
    SerializedScriptValue::SerializeOptions options;
    options.transferables = &transferables;
    ExceptionState exceptionState(isolate, ExceptionState::kExecutionContext,
                                  "MessageChannel", "postMessage");

    scoped_refptr<SerializedScriptValue> ssv = SerializedScriptValue::Serialize(
        isolate, wrapper, options, exceptionState);
    BlinkTransferableMessage msg;
    msg.message = ssv;
    mojo::Message mm =
        mojom::blink::TransferableMessage::SerializeAsMessage(&msg);
    return mm;
  }();

  BlinkTransferableMessage out;
  bool deserialized = mojom::blink::TransferableMessage::DeserializeFromMessage(
      std::move(mojo_message), &out);
  ASSERT_TRUE(deserialized);
  ASSERT_EQ(out.message->GetImageBitmapContentsArray().size(), 1U);
  scoped_refptr<blink::StaticBitmapImage> deserialized_bitmap_contents =
      out.message->GetImageBitmapContentsArray()[0];
  ImageBitmap* deserialized_bitmap =
      ImageBitmap::Create(std::move(deserialized_bitmap_contents));
  ImageBitmap* original_bitmap = CreateBitmap();
  AssertBitmapsEqual(original_bitmap, deserialized_bitmap);
  auto& abcs = out.message->GetImageBitmapContentsArray();
  abcs.clear();
}

TEST_F(BlinkTransferableMessageStructTraitsTest,
       BitmapLazySerializationSucceeds) {
  // Out of scope
  V8TestingScope scope;
  v8::Isolate* isolate = scope.GetIsolate();
  ImageBitmap* image_bitmap = CreateBitmap();
  scoped_refptr<blink::SharedBuffer> original_bitmap_data =
      image_bitmap->BitmapImage()->Data();
  // Serialize and deserialize it.
  v8::Local<v8::Value> wrapper = ToV8(image_bitmap, scope.GetScriptState());

  Transferables transferables;
  transferables.image_bitmaps.push_back(std::move(image_bitmap));
  SerializedScriptValue::SerializeOptions options;
  options.transferables = &transferables;
  ExceptionState exceptionState(isolate, ExceptionState::kExecutionContext,
                                "MessageChannel", "postMessage");

  scoped_refptr<SerializedScriptValue> ssv = SerializedScriptValue::Serialize(
      isolate, wrapper, options, exceptionState);
  BlinkTransferableMessage msg;
  msg.message = ssv;
  mojo::Message mojo_message =
      mojom::blink::TransferableMessage::WrapAsMessage(std::move(msg));

  BlinkTransferableMessage out;
  bool deserialized = mojom::blink::TransferableMessage::DeserializeFromMessage(
      std::move(mojo_message), &out);
  ASSERT_TRUE(deserialized);
  ASSERT_EQ(out.message->GetImageBitmapContentsArray().size(), 1U);
  scoped_refptr<blink::StaticBitmapImage> deserialized_bitmap_contents =
      out.message->GetImageBitmapContentsArray()[0];
  ImageBitmap* deserialized_bitmap =
      ImageBitmap::Create(std::move(deserialized_bitmap_contents));
  ImageBitmap* original_bitmap = CreateBitmap();
  AssertBitmapsEqual(original_bitmap, deserialized_bitmap);

  // When using WrapAsMessage, the deserialized bitmap should own
  // the original bitmap' data (as opposed to a copy of the data).
  ASSERT_EQ(original_bitmap_data, deserialized_bitmap->BitmapImage()->Data());
}

}  // namespace blink
