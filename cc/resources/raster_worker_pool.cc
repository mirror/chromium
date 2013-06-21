// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/raster_worker_pool.h"

#include "base/json/json_writer.h"
#include "base/metrics/histogram.h"
#include "base/values.h"
#include "cc/debug/devtools_instrumentation.h"
#include "cc/debug/traced_value.h"
#include "cc/resources/picture_pile_impl.h"
#include "skia/ext/lazy_pixel_ref.h"
#include "skia/ext/paint_simplifier.h"

namespace cc {

namespace {

class DisableLCDTextFilter : public SkDrawFilter {
 public:
  // SkDrawFilter interface.
  virtual bool filter(SkPaint* paint, SkDrawFilter::Type type) OVERRIDE {
    if (type != SkDrawFilter::kText_Type)
      return true;

    paint->setLCDRenderText(false);
    return true;
  }
};

class RasterWorkerPoolTaskImpl : public internal::RasterWorkerPoolTask {
 public:
  RasterWorkerPoolTaskImpl(const Resource* resource,
                           PicturePileImpl* picture_pile,
                           gfx::Rect content_rect,
                           float contents_scale,
                           RasterMode raster_mode,
                           bool use_color_estimator,
                           const RasterTaskMetadata& metadata,
                           RenderingStatsInstrumentation* rendering_stats,
                           const RasterWorkerPool::RasterTask::Reply& reply,
                           TaskVector* dependencies)
      : internal::RasterWorkerPoolTask(resource, dependencies),
        picture_pile_(picture_pile),
        content_rect_(content_rect),
        contents_scale_(contents_scale),
        raster_mode_(raster_mode),
        use_color_estimator_(use_color_estimator),
        metadata_(metadata),
        rendering_stats_(rendering_stats),
        reply_(reply) {}

  void RunAnalysisOnThread(unsigned thread_index) {
    TRACE_EVENT1("cc",
                 "RasterWorkerPoolTaskImpl::RunAnalysisOnThread",
                 "metadata",
                 TracedValue::FromValue(metadata_.AsValue().release()));

    DCHECK(picture_pile_.get());
    DCHECK(rendering_stats_);

    PicturePileImpl* picture_clone =
        picture_pile_->GetCloneForDrawingOnThread(thread_index);

    DCHECK(picture_clone);

    base::TimeTicks start_time = rendering_stats_->StartRecording();
    picture_clone->AnalyzeInRect(content_rect_, contents_scale_, &analysis_);
    base::TimeDelta duration = rendering_stats_->EndRecording(start_time);

    // Record the solid color prediction.
    UMA_HISTOGRAM_BOOLEAN("Renderer4.SolidColorTilesAnalyzed",
                          analysis_.is_solid_color);
    rendering_stats_->AddTileAnalysisResult(duration,
                                            analysis_.is_solid_color);

    // Clear the flag if we're not using the estimator.
    analysis_.is_solid_color &= use_color_estimator_;
  }

  bool RunRasterOnThread(SkDevice* device, unsigned thread_index) {
    TRACE_EVENT1(
        "cc", "RasterWorkerPoolTaskImpl::RunRasterOnThread",
        "metadata", TracedValue::FromValue(metadata_.AsValue().release()));
    devtools_instrumentation::ScopedLayerTask raster_task(
        devtools_instrumentation::kRasterTask, metadata_.layer_id);

    DCHECK(picture_pile_.get());
    DCHECK(device);

    if (analysis_.is_solid_color)
      return false;

    PicturePileImpl* picture_clone =
        picture_pile_->GetCloneForDrawingOnThread(thread_index);

    SkCanvas canvas(device);

    skia::RefPtr<SkDrawFilter> draw_filter;
    switch (raster_mode_) {
      case LOW_QUALITY_RASTER_MODE:
        draw_filter = skia::AdoptRef(new skia::PaintSimplifier);
        break;
      case HIGH_QUALITY_NO_LCD_RASTER_MODE:
        draw_filter = skia::AdoptRef(new DisableLCDTextFilter);
        break;
      case HIGH_QUALITY_RASTER_MODE:
        break;
      case NUM_RASTER_MODES:
      default:
        NOTREACHED();
    }

    canvas.setDrawFilter(draw_filter.get());

    if (rendering_stats_->record_rendering_stats()) {
      PicturePileImpl::RasterStats raster_stats;
      picture_clone->RasterToBitmap(
          &canvas, content_rect_, contents_scale_, &raster_stats);
      rendering_stats_->AddRaster(
          raster_stats.total_rasterize_time,
          raster_stats.best_rasterize_time,
          raster_stats.total_pixels_rasterized,
          metadata_.is_tile_in_pending_tree_now_bin);

      HISTOGRAM_CUSTOM_COUNTS(
          "Renderer4.PictureRasterTimeUS",
          raster_stats.total_rasterize_time.InMicroseconds(),
          0,
          100000,
          100);
    } else {
      picture_clone->RasterToBitmap(
          &canvas, content_rect_, contents_scale_, NULL);
    }
    return true;
  }

  // Overridden from internal::RasterWorkerPoolTask:
  virtual bool RunOnThread(SkDevice* device, unsigned thread_index) OVERRIDE {
    RunAnalysisOnThread(thread_index);
    return RunRasterOnThread(device, thread_index);
  }
  virtual void DispatchCompletionCallback() OVERRIDE {
    reply_.Run(analysis_, !HasFinishedRunning() || WasCanceled());
  }

 protected:
  virtual ~RasterWorkerPoolTaskImpl() {}

 private:
  PicturePileImpl::Analysis analysis_;
  scoped_refptr<PicturePileImpl> picture_pile_;
  gfx::Rect content_rect_;
  float contents_scale_;
  RasterMode raster_mode_;
  bool use_color_estimator_;
  RasterTaskMetadata metadata_;
  RenderingStatsInstrumentation* rendering_stats_;
  const RasterWorkerPool::RasterTask::Reply reply_;

  DISALLOW_COPY_AND_ASSIGN(RasterWorkerPoolTaskImpl);
};

class ImageDecodeWorkerPoolTaskImpl : public internal::WorkerPoolTask {
 public:
  ImageDecodeWorkerPoolTaskImpl(skia::LazyPixelRef* pixel_ref,
                                int layer_id,
                                RenderingStatsInstrumentation* rendering_stats,
                                const RasterWorkerPool::Task::Reply& reply)
      : pixel_ref_(pixel_ref),
        layer_id_(layer_id),
        rendering_stats_(rendering_stats),
        reply_(reply) {}

  // Overridden from internal::WorkerPoolTask:
  virtual void RunOnThread(unsigned thread_index) OVERRIDE {
    TRACE_EVENT0("cc", "ImageDecodeWorkerPoolTaskImpl::RunOnThread");
    devtools_instrumentation::ScopedLayerTask image_decode_task(
        devtools_instrumentation::kImageDecodeTask, layer_id_);
    base::TimeTicks start_time = rendering_stats_->StartRecording();
    pixel_ref_->Decode();
    base::TimeDelta duration = rendering_stats_->EndRecording(start_time);
    rendering_stats_->AddDeferredImageDecode(duration);
  }
  virtual void DispatchCompletionCallback() OVERRIDE {
    reply_.Run(!HasFinishedRunning());
  }

 protected:
  virtual ~ImageDecodeWorkerPoolTaskImpl() {}

 private:
  skia::LazyPixelRef* pixel_ref_;
  int layer_id_;
  RenderingStatsInstrumentation* rendering_stats_;
  const RasterWorkerPool::Task::Reply reply_;

  DISALLOW_COPY_AND_ASSIGN(ImageDecodeWorkerPoolTaskImpl);
};

class RasterFinishedWorkerPoolTaskImpl : public internal::WorkerPoolTask {
 public:
  RasterFinishedWorkerPoolTaskImpl(
      base::MessageLoopProxy* origin_loop,
      const base::Closure& on_raster_finished_callback)
      : origin_loop_(origin_loop),
        on_raster_finished_callback_(on_raster_finished_callback) {
  }

  // Overridden from internal::WorkerPoolTask:
  virtual void RunOnThread(unsigned thread_index) OVERRIDE {
    origin_loop_->PostTask(FROM_HERE, on_raster_finished_callback_);
  }
  virtual void DispatchCompletionCallback() OVERRIDE {}

 private:
  virtual ~RasterFinishedWorkerPoolTaskImpl() {}

  scoped_refptr<base::MessageLoopProxy> origin_loop_;
  const base::Closure on_raster_finished_callback_;

  DISALLOW_COPY_AND_ASSIGN(RasterFinishedWorkerPoolTaskImpl);
};

void Noop() {}

const char* kWorkerThreadNamePrefix = "CompositorRaster";

}  // namespace

namespace internal {

RasterWorkerPoolTask::RasterWorkerPoolTask(
    const Resource* resource, TaskVector* dependencies)
    : did_run_(false),
      did_complete_(false),
      was_canceled_(false),
      resource_(resource) {
  dependencies_.swap(*dependencies);
}

RasterWorkerPoolTask::~RasterWorkerPoolTask() {
}

void RasterWorkerPoolTask::DidRun(bool was_canceled) {
  DCHECK(!did_run_);
  did_run_ = true;
  was_canceled_ = was_canceled;
}

bool RasterWorkerPoolTask::HasFinishedRunning() const {
  return did_run_;
}

bool RasterWorkerPoolTask::WasCanceled() const {
  return was_canceled_;
}

void RasterWorkerPoolTask::DidComplete() {
  DCHECK(!did_complete_);
  did_complete_ = true;
}

bool RasterWorkerPoolTask::HasCompleted() const {
  return did_complete_;
}

}  // namespace internal

scoped_ptr<base::Value> RasterTaskMetadata::AsValue() const {
  scoped_ptr<base::DictionaryValue> res(new base::DictionaryValue());
  res->Set("tile_id", TracedValue::CreateIDRef(tile_id).release());
  res->SetBoolean("is_tile_in_pending_tree_now_bin",
                  is_tile_in_pending_tree_now_bin);
  res->Set("resolution", TileResolutionAsValue(tile_resolution).release());
  res->SetInteger("source_frame_number", source_frame_number);
  return res.PassAs<base::Value>();
}

RasterWorkerPool::Task::Set::Set() {
}

RasterWorkerPool::Task::Set::~Set() {
}

void RasterWorkerPool::Task::Set::Insert(const Task& task) {
  DCHECK(!task.is_null());
  tasks_.push_back(task.internal_);
}

RasterWorkerPool::Task::Task() {
}

RasterWorkerPool::Task::Task(internal::WorkerPoolTask* internal)
    : internal_(internal) {
}

RasterWorkerPool::Task::~Task() {
}

void RasterWorkerPool::Task::Reset() {
  internal_ = NULL;
}

RasterWorkerPool::RasterTask::Queue::Queue() {
}

RasterWorkerPool::RasterTask::Queue::~Queue() {
}

void RasterWorkerPool::RasterTask::Queue::Append(
    const RasterTask& task, bool required_for_activation) {
  DCHECK(!task.is_null());
  tasks_.push_back(task.internal_);
  if (required_for_activation)
    tasks_required_for_activation_.insert(task.internal_.get());
}

RasterWorkerPool::RasterTask::RasterTask() {
}

RasterWorkerPool::RasterTask::RasterTask(
    internal::RasterWorkerPoolTask* internal)
    : internal_(internal) {
}

void RasterWorkerPool::RasterTask::Reset() {
  internal_ = NULL;
}

RasterWorkerPool::RasterTask::~RasterTask() {
}

RasterWorkerPool::RasterTaskGraph::RasterTaskGraph()
    : raster_finished_node_(new GraphNode),
      next_priority_(1u) {
}

RasterWorkerPool::RasterTaskGraph::~RasterTaskGraph() {
}

void RasterWorkerPool::RasterTaskGraph::InsertRasterTask(
    internal::WorkerPoolTask* raster_task,
    const TaskVector& decode_tasks) {
  DCHECK(!raster_task->HasCompleted());
  DCHECK(graph_.find(raster_task) == graph_.end());

  scoped_ptr<GraphNode> raster_node(new GraphNode);
  raster_node->set_task(raster_task);
  raster_node->set_priority(next_priority_++);

  // Insert image decode tasks.
  for (TaskVector::const_iterator it = decode_tasks.begin();
       it != decode_tasks.end(); ++it) {
    internal::WorkerPoolTask* decode_task = it->get();

    // Skip if already decoded.
    if (decode_task->HasCompleted())
      continue;

    raster_node->add_dependency();

    // Check if decode task already exists in graph.
    GraphNodeMap::iterator decode_it = graph_.find(decode_task);
    if (decode_it != graph_.end()) {
      GraphNode* decode_node = decode_it->second;
      decode_node->add_dependent(raster_node.get());
      continue;
    }

    scoped_ptr<GraphNode> decode_node(new GraphNode);
    decode_node->set_task(decode_task);
    decode_node->set_priority(next_priority_++);
    decode_node->add_dependent(raster_node.get());
    graph_.set(decode_task, decode_node.Pass());
  }

  raster_finished_node_->add_dependency();
  raster_node->add_dependent(raster_finished_node_.get());

  graph_.set(raster_task, raster_node.Pass());
}

// static
RasterWorkerPool::RasterTask RasterWorkerPool::CreateRasterTask(
    const Resource* resource,
    PicturePileImpl* picture_pile,
    gfx::Rect content_rect,
    float contents_scale,
    RasterMode raster_mode,
    bool use_color_estimator,
    const RasterTaskMetadata& metadata,
    RenderingStatsInstrumentation* rendering_stats,
    const RasterTask::Reply& reply,
    Task::Set* dependencies) {
  return RasterTask(new RasterWorkerPoolTaskImpl(resource,
                                                 picture_pile,
                                                 content_rect,
                                                 contents_scale,
                                                 raster_mode,
                                                 use_color_estimator,
                                                 metadata,
                                                 rendering_stats,
                                                 reply,
                                                 &dependencies->tasks_));
}

// static
RasterWorkerPool::Task RasterWorkerPool::CreateImageDecodeTask(
    skia::LazyPixelRef* pixel_ref,
    int layer_id,
    RenderingStatsInstrumentation* stats_instrumentation,
    const Task::Reply& reply) {
  return Task(new ImageDecodeWorkerPoolTaskImpl(pixel_ref,
                                                layer_id,
                                                stats_instrumentation,
                                                reply));
}

RasterWorkerPool::RasterWorkerPool(ResourceProvider* resource_provider,
                                   size_t num_threads)
    : WorkerPool(num_threads, kWorkerThreadNamePrefix),
      client_(NULL),
      resource_provider_(resource_provider),
      weak_ptr_factory_(this),
      schedule_raster_tasks_count_(0) {
}

RasterWorkerPool::~RasterWorkerPool() {
}

void RasterWorkerPool::SetClient(RasterWorkerPoolClient* client) {
  client_ = client;
}

void RasterWorkerPool::Shutdown() {
  TaskGraph empty;
  SetTaskGraph(&empty);
  WorkerPool::Shutdown();
  raster_tasks_.clear();
  // Cancel any pending OnRasterFinished callback.
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void RasterWorkerPool::SetRasterTasks(RasterTask::Queue* queue) {
  raster_tasks_.swap(queue->tasks_);
  raster_tasks_required_for_activation_.swap(
      queue->tasks_required_for_activation_);
}

void RasterWorkerPool::SetRasterTaskGraph(RasterTaskGraph* graph) {
  scoped_ptr<GraphNode> raster_finished_node(
      graph->raster_finished_node_.Pass());
  TaskGraph new_graph;
  new_graph.swap(graph->graph_);

  if (new_graph.empty()) {
    SetTaskGraph(&new_graph);
    raster_finished_task_ = NULL;
    return;
  }

  ++schedule_raster_tasks_count_;

  scoped_refptr<internal::WorkerPoolTask> new_raster_finished_task(
      new RasterFinishedWorkerPoolTaskImpl(
          base::MessageLoopProxy::current(),
          base::Bind(&RasterWorkerPool::OnRasterFinished,
                     weak_ptr_factory_.GetWeakPtr(),
                     schedule_raster_tasks_count_)));
  raster_finished_node->set_task(new_raster_finished_task.get());
  // Insert "raster finished" task before switching to new graph.
  new_graph.set(new_raster_finished_task.get(), raster_finished_node.Pass());
  SetTaskGraph(&new_graph);
  raster_finished_task_.swap(new_raster_finished_task);
}

bool RasterWorkerPool::IsRasterTaskRequiredForActivation(
    internal::RasterWorkerPoolTask* task) const {
  return
      raster_tasks_required_for_activation_.find(task) !=
      raster_tasks_required_for_activation_.end();
}

void RasterWorkerPool::OnRasterFinished(int64 schedule_raster_tasks_count) {
  TRACE_EVENT1("cc", "RasterWorkerPool::OnRasterFinished",
               "schedule_raster_tasks_count", schedule_raster_tasks_count);
  DCHECK_GE(schedule_raster_tasks_count_, schedule_raster_tasks_count);
  // Call OnRasterTasksFinished() when we've finished running all raster
  // tasks needed since last time SetRasterTaskGraph() was called.
  if (schedule_raster_tasks_count_ == schedule_raster_tasks_count)
    OnRasterTasksFinished();
}

}  // namespace cc
