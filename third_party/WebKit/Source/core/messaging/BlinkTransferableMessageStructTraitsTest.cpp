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

scoped_refptr<SerializedScriptValue> BuildSerializedScriptValue(
    v8::Isolate* isolate,
    v8::Local<v8::Value> value,
    Transferables& transferables) {
  SerializedScriptValue::SerializeOptions options;
  options.transferables = &transferables;
  ExceptionState exceptionState(isolate, ExceptionState::kExecutionContext,
                                "MessageChannel", "postMessage");

  return SerializedScriptValue::Serialize(isolate, value, options,
                                          exceptionState);
}

TEST_F(BlinkTransferableMessageStructTraitsTest,
       ArrayBufferTransferOutOfScopeSucceeds) {
  // Build the original ArrayBuffer in a closure's scope to simulate situations
  // where a buffer may be freed twice.
  mojo::Message mojo_message = []() -> mojo::Message {
    V8TestingScope scope;
    v8::Isolate* isolate = scope.GetIsolate();
    size_t num_elements = 8;
    v8::Local<v8::ArrayBuffer> v8Buffer =
        v8::ArrayBuffer::New(isolate, num_elements);
    uint8_t* contents = static_cast<uint8_t*>(v8Buffer->GetContents().Data());
    for (size_t i = 0; i < num_elements; i++)
      contents[i] = static_cast<uint8_t>(i);

    DOMArrayBuffer* array_buffer =
        V8ArrayBuffer::ToImpl(v8::Local<v8::Object>::Cast(v8Buffer));
    Transferables transferables;
    transferables.array_buffers.push_back(array_buffer);
    BlinkTransferableMessage msg;
    msg.message =
        BuildSerializedScriptValue(scope.GetIsolate(), v8Buffer, transferables);
    return mojom::blink::TransferableMessage::SerializeAsMessage(&msg);
  }();

  BlinkTransferableMessage out;
  bool deserialized = mojom::blink::TransferableMessage::DeserializeFromMessage(
      std::move(mojo_message), &out);
  ASSERT_TRUE(deserialized);
  ASSERT_EQ(out.message->GetArrayBufferContentsArray().size(), 1U);
  WTF::ArrayBufferContents& abc = out.message->GetArrayBufferContentsArray()[0];
  std::vector<uint8_t> deserializedContents(
      static_cast<uint8_t*>(abc.Data()), static_cast<uint8_t*>(abc.Data()) + 8);

  for (uint8_t i = 0; i < 8; i++) {
    ASSERT_TRUE(deserializedContents[i] == i);
  }
}

TEST_F(BlinkTransferableMessageStructTraitsTest,
       ArrayBufferContentsLazySerializationSucceeds) {
  V8TestingScope scope;
  v8::Isolate* isolate = scope.GetIsolate();
  size_t num_elements = 8;
  v8::Local<v8::ArrayBuffer> v8Buffer =
      v8::ArrayBuffer::New(isolate, num_elements);
  void* originalContentsData = v8Buffer->GetContents().Data();
  uint8_t* contents = static_cast<uint8_t*>(originalContentsData);
  for (size_t i = 0; i < num_elements; i++)
    contents[i] = static_cast<uint8_t>(i);

  DOMArrayBuffer* original_array_buffer =
      V8ArrayBuffer::ToImpl(v8::Local<v8::Object>::Cast(v8Buffer));
  Transferables transferables;
  transferables.array_buffers.push_back(original_array_buffer);
  BlinkTransferableMessage msg;
  msg.message =
      BuildSerializedScriptValue(scope.GetIsolate(), v8Buffer, transferables);
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
  ASSERT_EQ(NULL, v8Buffer->GetContents().Data());
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
  // Build the original ImageBitmap in a closure's scope to simulate situations
  // where a buffer may be freed twice.
  mojo::Message mojo_message = []() -> mojo::Message {
    V8TestingScope scope;
    ImageBitmap* image_bitmap = CreateBitmap();
    v8::Local<v8::Value> wrapper = ToV8(image_bitmap, scope.GetScriptState());
    Transferables transferables;
    transferables.image_bitmaps.push_back(image_bitmap);
    BlinkTransferableMessage msg;
    msg.message =
        BuildSerializedScriptValue(scope.GetIsolate(), wrapper, transferables);
    return mojom::blink::TransferableMessage::SerializeAsMessage(&msg);
  }();

  BlinkTransferableMessage out;
  ASSERT_TRUE(mojom::blink::TransferableMessage::DeserializeFromMessage(
      std::move(mojo_message), &out));
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
  // Create the mojo message.
  V8TestingScope scope;
  ImageBitmap* image_bitmap = CreateBitmap();
  scoped_refptr<blink::SharedBuffer> original_bitmap_data =
      image_bitmap->BitmapImage()->Data();
  v8::Local<v8::Value> wrapper = ToV8(image_bitmap, scope.GetScriptState());
  Transferables transferables;
  transferables.image_bitmaps.push_back(std::move(image_bitmap));
  BlinkTransferableMessage msg;
  msg.message =
      BuildSerializedScriptValue(scope.GetIsolate(), wrapper, transferables);
  mojo::Message mojo_message =
      mojom::blink::TransferableMessage::WrapAsMessage(std::move(msg));

  // Deserialize the mojo message.
  BlinkTransferableMessage out;
  ASSERT_TRUE(mojom::blink::TransferableMessage::DeserializeFromMessage(
      std::move(mojo_message), &out));
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
