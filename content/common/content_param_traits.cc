// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/content_param_traits.h"

#include <stddef.h>

#include "base/strings/string_number_conversions.h"
#include "content/common/frame_message_structs.h"
#include "ipc/ipc_message_attachment.h"
#include "ipc/ipc_mojo_message_helper.h"
#include "ipc/ipc_mojo_param_traits.h"
#include "net/base/ip_endpoint.h"
#include "third_party/WebKit/common/message_port/message_port.mojom.h"
#include "third_party/WebKit/common/message_port/message_port_channel.h"
#include "third_party/WebKit/common/message_port/transferable_message.h"
#include "third_party/WebKit/common/message_port/transferable_message_struct_traits.h"
#include "ui/accessibility/ax_modes.h"
#include "ui/base/ui_base_switches_util.h"
#include "ui/events/blink/web_input_event_traits.h"

namespace IPC {

void ParamTraits<WebInputEventPointer>::Write(base::Pickle* m,
                                              const param_type& p) {
  m->WriteData(reinterpret_cast<const char*>(p), p->size());
}

bool ParamTraits<WebInputEventPointer>::Read(const base::Pickle* m,
                                             base::PickleIterator* iter,
                                             param_type* r) {
  const char* data;
  int data_length;
  if (!iter->ReadData(&data, &data_length)) {
    NOTREACHED();
    return false;
  }
  if (data_length < static_cast<int>(sizeof(blink::WebInputEvent))) {
    NOTREACHED();
    return false;
  }
  param_type event = reinterpret_cast<param_type>(data);
  // Check that the data size matches that of the event.
  if (data_length != static_cast<int>(event->size())) {
    NOTREACHED();
    return false;
  }
  const size_t expected_size_for_type =
      ui::WebInputEventTraits::GetSize(event->GetType());
  if (data_length != static_cast<int>(expected_size_for_type)) {
    NOTREACHED();
    return false;
  }
  *r = event;
  return true;
}

void ParamTraits<WebInputEventPointer>::Log(const param_type& p,
                                            std::string* l) {
  l->append("(");
  LogParam(p->size(), l);
  l->append(", ");
  LogParam(p->GetType(), l);
  l->append(", ");
  LogParam(p->TimeStampSeconds(), l);
  l->append(")");
}

void ParamTraits<blink::MessagePortChannel>::Write(base::Pickle* m,
                                                   const param_type& p) {
  ParamTraits<mojo::MessagePipeHandle>::Write(m, p.ReleaseHandle().release());
}

bool ParamTraits<blink::MessagePortChannel>::Read(const base::Pickle* m,
                                                  base::PickleIterator* iter,
                                                  param_type* r) {
  mojo::MessagePipeHandle handle;
  if (!ParamTraits<mojo::MessagePipeHandle>::Read(m, iter, &handle))
    return false;
  *r = blink::MessagePortChannel(mojo::ScopedMessagePipeHandle(handle));
  return true;
}

void ParamTraits<blink::MessagePortChannel>::Log(const param_type& p,
                                                 std::string* l) {}

void ParamTraits<ui::AXMode>::Write(base::Pickle* m, const param_type& p) {
  IPC::WriteParam(m, p.mode());
}

bool ParamTraits<ui::AXMode>::Read(const base::Pickle* m,
                                   base::PickleIterator* iter,
                                   param_type* r) {
  uint32_t value;
  if (!IPC::ReadParam(m, iter, &value))
    return false;
  *r = ui::AXMode(value);
  return true;
}

void ParamTraits<ui::AXMode>::Log(const param_type& p, std::string* l) {}

void ParamTraits<scoped_refptr<storage::BlobHandle>>::Write(
    base::Pickle* m,
    const param_type& p) {
  WriteParam(m, p != nullptr);
  if (p) {
    auto info = p->Clone().PassInterface();
    m->WriteUInt32(info.version());
    MojoMessageHelper::WriteMessagePipeTo(m, info.PassHandle());
  }
}

bool ParamTraits<scoped_refptr<storage::BlobHandle>>::Read(
    const base::Pickle* m,
    base::PickleIterator* iter,
    param_type* r) {
  bool is_not_null;
  if (!ReadParam(m, iter, &is_not_null))
    return false;
  if (!is_not_null)
    return true;

  uint32_t version;
  if (!ReadParam(m, iter, &version))
    return false;
  mojo::ScopedMessagePipeHandle handle;
  if (!MojoMessageHelper::ReadMessagePipeFrom(m, iter, &handle))
    return false;
  DCHECK(handle.is_valid());
  blink::mojom::BlobPtr blob;
  blob.Bind(blink::mojom::BlobPtrInfo(std::move(handle), version));
  *r = base::MakeRefCounted<storage::BlobHandle>(std::move(blob));
  return true;
}

void ParamTraits<scoped_refptr<storage::BlobHandle>>::Log(const param_type& p,
                                                          std::string* l) {
  l->append("<storage::BlobHandle>");
}

// static
void ParamTraits<content::FrameMsg_ViewChanged_Params>::Write(
    base::Pickle* m,
    const param_type& p) {
  DCHECK(switches::IsMusHostingViz() ||
         (p.frame_sink_id.has_value() && p.frame_sink_id->is_valid()));
  WriteParam(m, p.frame_sink_id);
}

bool ParamTraits<content::FrameMsg_ViewChanged_Params>::Read(
    const base::Pickle* m,
    base::PickleIterator* iter,
    param_type* r) {
  if (!ReadParam(m, iter, &(r->frame_sink_id)))
    return false;
  if (!switches::IsMusHostingViz() &&
      (!r->frame_sink_id || !r->frame_sink_id->is_valid())) {
    NOTREACHED();
    return false;
  }
  return true;
}

// static
void ParamTraits<content::FrameMsg_ViewChanged_Params>::Log(const param_type& p,
                                                            std::string* l) {
  l->append("(");
  LogParam(p.frame_sink_id, l);
  l->append(")");
}

void ParamTraits<scoped_refptr<base::RefCountedData<
    blink::TransferableMessage>>>::Write(base::Pickle* m, const param_type& p) {
  mojo::Message mojo_message =
      blink::mojom::TransferableMessage::SerializeAsMessage(&p->data);
  mojo_message = mojo::Message(mojo_message.TakeMojoMessage());
  m->WriteData(reinterpret_cast<const char*>(mojo_message.payload()),
               mojo_message.payload_num_bytes());
  WriteParam(m, base::checked_cast<int>(mojo_message.handles()->size()));
  for (mojo::ScopedHandle& handle : *mojo_message.mutable_handles()) {
    m->WriteAttachment(MessageAttachment::CreateFromMojoHandle(
        std::move(handle), MessageAttachment::Type::MOJO_HANDLE));
  }
}

bool ParamTraits<
    scoped_refptr<base::RefCountedData<blink::TransferableMessage>>>::
    Read(const base::Pickle* m, base::PickleIterator* iter, param_type* r) {
  const char* data;
  int length;
  if (!iter->ReadData(&data, &length))
    return false;
  int handle_count;
  if (!iter->ReadLength(&handle_count))
    return false;
  std::vector<mojo::ScopedHandle> handles(handle_count);
  for (auto& handle : handles) {
    scoped_refptr<base::Pickle::Attachment> attachment;
    if (!m->ReadAttachment(iter, &attachment))
      return false;

    if (static_cast<MessageAttachment*>(attachment.get())->GetType() !=
        MessageAttachment::Type::MOJO_HANDLE) {
      return false;
    }
    handle =
        static_cast<MessageAttachment*>(attachment.get())->TakeMojoHandle();
  }

  mojo::Message mojo_message(0, 0, 0, 0, nullptr);
  *mojo_message.mutable_handles() = std::move(handles);
  mojo_message.payload_buffer()->Allocate(length);
  std::memcpy(mojo_message.mutable_payload(), data, length);
  *r = new base::RefCountedData<blink::TransferableMessage>();
  if (!blink::mojom::TransferableMessage::DeserializeFromMessage(
          std::move(mojo_message), &(*r)->data))
    return false;
  return true;
}

void ParamTraits<scoped_refptr<
    base::RefCountedData<blink::TransferableMessage>>>::Log(const param_type& p,
                                                            std::string* l) {
  l->append("<blink::TransferableMessage>");
}

}  // namespace IPC

// Generate param traits write methods.
#include "ipc/param_traits_write_macros.h"
namespace IPC {
#undef CONTENT_COMMON_CONTENT_PARAM_TRAITS_MACROS_H_
#include "content/common/content_param_traits_macros.h"
}  // namespace IPC

// Generate param traits read methods.
#include "ipc/param_traits_read_macros.h"
namespace IPC {
#undef CONTENT_COMMON_CONTENT_PARAM_TRAITS_MACROS_H_
#include "content/common/content_param_traits_macros.h"
}  // namespace IPC

// Generate param traits log methods.
#include "ipc/param_traits_log_macros.h"
namespace IPC {
#undef CONTENT_COMMON_CONTENT_PARAM_TRAITS_MACROS_H_
#include "content/common/content_param_traits_macros.h"
}  // namespace IPC
