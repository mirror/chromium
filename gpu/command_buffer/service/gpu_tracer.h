// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains the GPUTrace class.
#ifndef GPU_COMMAND_BUFFER_SERVICE_GPU_TRACER_H_
#define GPU_COMMAND_BUFFER_SERVICE_GPU_TRACER_H_

#include <string>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/gpu_export.h"
#include "ui/gl/gl_bindings.h"

namespace gpu {
namespace gles2 {

class Outputter;
class GPUTrace;

// Id used to keep trace namespaces separate
enum GpuTracerSource {
  kTraceGroupInvalid = -1,

  kTraceGroupMarker = 0,
  kTraceCHROMIUM = 1,
  kTraceDecoder = 2,

  NUM_TRACER_SOURCES
};

enum GpuTracerType {
  kTracerTypeInvalid = -1,

  kTracerTypeARBTimer,
  kTracerTypeDisjointTimer
};

// Central accesser to CPU Time
class GPU_EXPORT CPUTime
    : public base::RefCounted<CPUTime> {
 public:
  CPUTime();

  virtual int64 GetCurrentTime();

 protected:
  virtual ~CPUTime();
  friend class base::RefCounted<CPUTime>;

  DISALLOW_COPY_AND_ASSIGN(CPUTime);
};

// Marker structure for a Trace.
struct TraceMarker {
  TraceMarker(const std::string& category, const std::string& name);
  ~TraceMarker();

  std::string category_;
  std::string name_;
  scoped_refptr<GPUTrace> trace_;
};

// Traces GPU Commands.
class GPU_EXPORT GPUTracer
    : public base::SupportsWeakPtr<GPUTracer> {
 public:
  explicit GPUTracer(gles2::GLES2Decoder* decoder);
  virtual ~GPUTracer();

  // Scheduled processing in decoder begins.
  bool BeginDecoding();

  // Scheduled processing in decoder ends.
  bool EndDecoding();

  // Begin a trace marker.
  bool Begin(const std::string& category, const std::string& name,
             GpuTracerSource source);

  // End the last started trace marker.
  bool End(GpuTracerSource source);

  virtual bool IsTracing();

  // Retrieve the name of the current open trace.
  // Returns empty string if no current open trace.
  const std::string& CurrentCategory(GpuTracerSource source) const;
  const std::string& CurrentName(GpuTracerSource source) const;

 protected:
  // Trace Processing.
  scoped_refptr<GPUTrace> CreateTrace(const std::string& category,
                                      const std::string& name);
  virtual scoped_refptr<Outputter> CreateOutputter(const std::string& name);
  virtual scoped_refptr<CPUTime> CreateCPUTime();
  virtual GpuTracerType DetermineTracerType();
  virtual void PostTask();

  void Process();
  void ProcessTraces();

  void CalculateTimerOffset();
  void IssueProcessTask();

  scoped_refptr<Outputter> outputter_;
  scoped_refptr<CPUTime> cpu_time_;
  std::vector<TraceMarker> markers_[NUM_TRACER_SOURCES];
  std::deque<scoped_refptr<GPUTrace> > traces_;

  const unsigned char* gpu_trace_srv_category;
  const unsigned char* gpu_trace_dev_category;
  gles2::GLES2Decoder* decoder_;

  int64 timer_offset_;

  GpuTracerType tracer_type_;
  bool gpu_timing_synced_;
  bool gpu_executing_;
  bool process_posted_;

  DISALLOW_COPY_AND_ASSIGN(GPUTracer);
};

class Outputter : public base::RefCounted<Outputter> {
 public:
  virtual void TraceDevice(const std::string& category,
                           const std::string& name,
                           int64 start_time,
                           int64 end_time) = 0;

  virtual void TraceServiceBegin(const std::string& category,
                                 const std::string& name,
                                 void* id) = 0;

  virtual void TraceServiceEnd(const std::string& category,
                               const std::string& name,
                               void* id) = 0;

 protected:
  virtual ~Outputter() {}
  friend class base::RefCounted<Outputter>;
};

class TraceOutputter : public Outputter {
 public:
  static scoped_refptr<TraceOutputter> Create(const std::string& name);
  void TraceDevice(const std::string& category,
                   const std::string& name,
                   int64 start_time,
                   int64 end_time) override;

  void TraceServiceBegin(const std::string& category,
                         const std::string& name,
                         void* id) override;

  void TraceServiceEnd(const std::string& category,
                       const std::string& name,
                       void* id) override;

 protected:
  friend class base::RefCounted<Outputter>;
  explicit TraceOutputter(const std::string& name);
  ~TraceOutputter() override;

  base::Thread named_thread_;
  uint64 local_trace_id_;

  DISALLOW_COPY_AND_ASSIGN(TraceOutputter);
};

class GPU_EXPORT GPUTrace
    : public base::RefCounted<GPUTrace> {
 public:
  GPUTrace(scoped_refptr<Outputter> outputter,
           scoped_refptr<CPUTime> cpu_time,
           const std::string& category,
           const std::string& name,
           int64 offset,
           GpuTracerType tracer_type);

  bool IsEnabled() { return tracer_type_ != kTracerTypeInvalid; }

  void Start(bool trace_service);
  void End(bool tracing_service);
  bool IsAvailable();
  void Process();

 private:
  ~GPUTrace();

  void Output();

  friend class base::RefCounted<GPUTrace>;

  std::string category_;
  std::string name_;
  scoped_refptr<Outputter> outputter_;
  scoped_refptr<CPUTime> cpu_time_;

  int64 offset_;
  int64 start_time_;
  int64 end_time_;
  GpuTracerType tracer_type_;
  bool end_requested_;

  GLuint queries_[2];

  DISALLOW_COPY_AND_ASSIGN(GPUTrace);
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_GPU_TRACER_H_
