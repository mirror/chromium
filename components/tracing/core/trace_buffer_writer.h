// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRACING_CORE_TRACE_BUFFER_WRITER_H_
#define COMPONENTS_TRACING_CORE_TRACE_BUFFER_WRITER_H_

#include "base/macros.h"
#include "components/tracing/tracing_export.h"
#include "components/tracing/core/scattered_stream_writer.h"
#include "components/tracing/core/trace_event.h"

namespace tracing {
namespace v2 {

class ProtoZeroMessage;
class TraceRingBuffer;

// This class is the entry-point to add events to the TraceRingBuffer.
// It acts as a glue layer between the protobuf classes (ProtoZeroMessage) and
// the chunked trace ring buffer (TraceRingBuffer). This class is deliberately
// NOT thread safe. The expected design is that each thread owns an instance of
// TraceBufferWriter and that trace events produced on one thread are not passed
// to other threads.
class TRACING_EXPORT TraceBufferWriter : public ScatteredStreamWriter::Delegate {
 public:
  // |trace_buffer| is the underlying ring buffer for taking and returning
  // chunks. |writer_id| is an identifier, unique for each writer, which is
  // appended to the header of each chunk. Its purpose is to allow the importer
  // to reconstruct the logical sequence of chunks for a given writer. Think to
  // this as a thread-id (but in rare cases, some threads can have more than
  // one writer, hence why this cannot be just a thread_id).
  TraceBufferWriter(TraceRingBuffer* trace_buffer, uint32_t writer_id);
  ~TraceBufferWriter();

  // TODO(primiano): this will soon return a TraceEventHandle, which is a safer
  // wrapper around TraceEvent that prevents use-after-recycling in DHECK mode.
  TraceEvent* AddEvent();

  // ScatteredStreamWriter::Delegate implementation.

  // Called by the ProtoZeroMessage's ScatteredStreamWriter when the caller
  // tries to append a new field to the proto which overflows the current chunk.
  ContiguousMemoryRange GetNewBuffer() override;

 private:
  ContiguousMemoryRange AcquireNewChunk(bool event_continues_from_prev_chunk);
  ContiguousMemoryRange WriteEventPreamble();

  TraceRingBuffer* trace_buffer_;
  const uint32_t writer_id_;

  // Monotonic counter (within the scope of writer_id_) of chunks produced by
  // each instance.
  uint32_t chunk_seq_id_;

  // The last chunk acquired. nullptr before the first call to AddEvent().
  TraceRingBuffer::Chunk* chunk_;

  // TODO comment before sending CL out.
  uint8_t* continue_on_next_chunk_ptr_;
  uint8_t* event_start_addr_;

  ScatteredStreamWriter stream_writer_;

  TraceEvent event_;

  DISALLOW_COPY_AND_ASSIGN(TraceBufferWriter);
};

}  // namespace v2
}  // namespace tracing

#endif  // COMPONENTS_TRACING_CORE_TRACE_BUFFER_WRITER_H_
