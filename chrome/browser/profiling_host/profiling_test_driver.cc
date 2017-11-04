// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiling_host/profiling_test_driver.h"

#include <string>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/process/process_handle.h"
#include "base/run_loop.h"
#include "base/task_scheduler/post_task.h"
#include "base/trace_event/trace_config_memory_test_util.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/tracing_controller.h"
#include "content/public/common/service_manager_connection.h"

namespace profiling {

namespace {

// Make some specific allocations in Browser to do a deeper test of the
// allocation tracking. On the renderer side, this is harder so all that's
// tested there is the existence of information.
constexpr int kBrowserAllocSize = 103 * 1024;
constexpr int kBrowserAllocCount = 2048;

// Test fixed-size partition alloc. The size must be aligned to system pointer
// size.
constexpr int kPartitionAllocSize = 8 * 23;
constexpr int kPartitionAllocCount = 107;
static const char* kPartitionAllocTypeName = "kPartitionAllocTypeName";

// On success, populates |pid|.
bool HasProcessWithName(base::Value* dump_json, std::string name, int* pid) {
  base::Value* events = dump_json->FindKey("traceEvents");
  for (base::Value& event : events->GetList()) {
    const base::Value* found_name =
        event.FindKeyOfType("name", base::Value::Type::STRING);
    if (!found_name)
      continue;
    if (found_name->GetString() != "process_name")
      continue;
    const base::Value* found_args =
        event.FindKeyOfType("args", base::Value::Type::DICTIONARY);
    if (!found_args)
      continue;
    const base::Value* found_process_name =
        found_args->FindKeyOfType("name", base::Value::Type::STRING);
    if (!found_process_name)
      continue;
    if (found_process_name->GetString() != name)
      continue;

    if (pid) {
      const base::Value* found_pid =
          event.FindKeyOfType("pid", base::Value::Type::INTEGER);
      if (!found_pid) {
        LOG(ERROR) << "Process missing pid.";
        return false;
      }
      *pid = found_pid->GetInt();
    }

    return true;
  }
  return false;
}

base::Value* FindHeapsV2(base::ProcessId pid, base::Value* dump_json) {
  base::Value* events = dump_json->FindKey("traceEvents");
  base::Value* dumps = nullptr;
  base::Value* heaps_v2 = nullptr;
  for (base::Value& event : events->GetList()) {
    const base::Value* found_name =
        event.FindKeyOfType("name", base::Value::Type::STRING);
    if (!found_name)
      continue;
    if (found_name->GetString() != "periodic_interval")
      continue;
    const base::Value* found_pid =
        event.FindKeyOfType("pid", base::Value::Type::INTEGER);
    if (!found_pid)
      continue;
    if (static_cast<base::ProcessId>(found_pid->GetInt()) != pid)
      continue;
    dumps = &event;
    heaps_v2 = dumps->FindPath({"args", "dumps", "heaps_v2"});
    if (heaps_v2)
      return heaps_v2;
  }
  return nullptr;
}

// Verify expectations are present in heap dump.
bool ValidateDump(base::Value* heaps_v2,
                  int expected_alloc_size,
                  int expected_alloc_count,
                  const char* allocator_name,
                  const char* type_name) {
  base::Value* sizes =
      heaps_v2->FindPath({"allocators", allocator_name, "sizes"});
  if (!sizes) {
    LOG(ERROR) << "Failed to find path: 'allocators." << allocator_name << ".sizes' in heaps v2";
    return false;
  }

  const base::Value::ListStorage& sizes_list = sizes->GetList();
  if (sizes_list.empty()) {
    LOG(ERROR) << "'allocators." << allocator_name << ".sizes' is an empty list";
    return false;
  }

  base::Value* counts =
      heaps_v2->FindPath({"allocators", allocator_name, "counts"});
  if (!counts) {
    LOG(ERROR) << "Failed to find path: 'allocators." << allocator_name << ".counts' in heaps v2";
    return false;
  }

  const base::Value::ListStorage& counts_list = counts->GetList();
  if (sizes_list.size() != counts_list.size()) {
    LOG(ERROR) << "'allocators." << allocator_name << ".sizes' does not have the same number of elements as *.counts";
    return false;
  }

  base::Value* types =
      heaps_v2->FindPath({"allocators", allocator_name, "types"});
  if (!types) {
    LOG(ERROR) << "Failed to find path: 'allocators." << allocator_name << ".types' in heaps v2";
    return false;
  }

  const base::Value::ListStorage& types_list = types->GetList();
  if (types_list.empty()) {
    LOG(ERROR) << "'allocators." << allocator_name << ".types' is an empty list";
    return false;
  }

  if (sizes_list.size() != types_list.size()) {
    LOG(ERROR) << "'allocators." << allocator_name << ".types' does not have the same number of elements as *.sizes";
    return false;
  }

  bool found_browser_alloc = false;
  size_t browser_alloc_index = 0;
  for (size_t i = 0; i < sizes_list.size(); i++) {
    if (counts_list[i].GetInt() == expected_alloc_count &&
        sizes_list[i].GetInt() != expected_alloc_size) {
      LOG(WARNING) << "Allocation candidate (size:" << sizes_list[i].GetInt()
                   << " count:" << counts_list[i].GetInt() << ")";
    }
    if (sizes_list[i].GetInt() == expected_alloc_size &&
        counts_list[i].GetInt() == expected_alloc_count) {
      browser_alloc_index = i;
      found_browser_alloc = true;
      break;
    }
  }

  if (!found_browser_alloc) {
    LOG(ERROR)
      << "Failed to find an allocation of the "
         "appropriate size. Did the send buffer "
         "not flush? (size: "
      << expected_alloc_size << " count:" << expected_alloc_count << ")";
    return false;
  }

  // Find the type, if an expectation was passed in.
  if (type_name) {
    bool found = false;
    int type = types_list[browser_alloc_index].GetInt();
    base::Value* strings = heaps_v2->FindPath({"maps", "strings"});
    for (base::Value& dict : strings->GetList()) {
      // Each dict has the format {"id":1,"string":"kPartitionAllocTypeName"}
      int id = dict.FindKey("id")->GetInt();
      if (id == type) {
        found = true;
        std::string name = dict.FindKey("string")->GetString();
        if (name != type_name) {
          LOG(ERROR) << "actual name: " << name << " expected name: " << type_name;
          return false;
        }
        break;
      }
    }
    if (!found) {
      LOG(ERROR) << "Failed to find type name string: " << type_name;
      return false;
    }
  }
  return true;
}

}  // namespace


ProfilingTestDriver::ProfilingTestDriver() {
    partition_allocator_.init();
}
ProfilingTestDriver::~ProfilingTestDriver() {}

bool ProfilingTestDriver::RunTest(const Options& options) {
  options_ = options;

  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    LOG(ERROR) << "The ProfilingTestDriver must be called on the UI thread.";
    return false;
  }

  // The only thing to test for Mode::kNone is that profiling hasn't started.
  if (options_.mode == ProfilingProcessHost::Mode::kNone) {
    if (ProfilingProcessHost::has_started()) {
      LOG(ERROR) << "Profiling should not have started";
      return false;
    }
    return true;
  }

  if (!CheckOrStartProfiling())
    return false;

  MakeTestAllocations();
  CollectResults();

  std::unique_ptr<base::Value> dump_json =
      base::JSONReader::Read(serialized_trace_->data());
  if (!dump_json) {
    LOG(ERROR) << "Failed to deserialize trace.";
    return false;
  }

  if (!ValidateBrowserAllocations(dump_json.get())) {
    LOG(ERROR) << "Failed to validate browser allocations";
    return false;
  }

  if (!ValidateRendererAllocations(dump_json.get())) {
    LOG(ERROR) << "Failed to validate renderer allocations";
    return false;
  }

  return true;
}


bool ProfilingTestDriver::CheckOrStartProfiling() {
  if (options_.profiling_already_started) {
    if (ProfilingProcessHost::has_started())
      return true;
    LOG(ERROR) << "Profiling should have been started, but wasn't";
    return false;
  }

  content::ServiceManagerConnection* connection = content::ServiceManagerConnection::GetForProcess();
  if (!connection) {
    LOG(ERROR) << "A ServiceManagerConnection was not available for the current process.";
    return false;
  }

  ProfilingProcessHost::Start(connection, options_.mode);
  return true;
}

void ProfilingTestDriver::MakeTestAllocations() {
    leaks_.reserve(2 * kBrowserAllocCount + kPartitionAllocSize);
    for (int i = 0; i < kBrowserAllocCount; ++i) {
      leaks_.push_back(new char[kBrowserAllocSize]);
    }

    for (int i = 0; i < kPartitionAllocCount; ++i) {
      leaks_.push_back(static_cast<char*>(
          PartitionAllocGeneric(partition_allocator_.root(),
                                kPartitionAllocSize, kPartitionAllocTypeName)));
    }

    for (int i = 0; i < kBrowserAllocCount; ++i) {
      leaks_.push_back(new char[i + 1]);  // Variadic allocation.
      total_variadic_allocations_ += i + 1;
    }

    // // Navigate around to force allocations in the renderer.
    // ASSERT_TRUE(embedded_test_server()->Start());
    // ui_test_utils::NavigateToURL(
    //     browser(), embedded_test_server()->GetURL("/english_page.html"));
    // // Vive la France!
    // ui_test_utils::NavigateToURL(
    //     browser(), embedded_test_server()->GetURL("/french_page.html"));
  }

void ProfilingTestDriver::CollectResults() {
  base::RunLoop run_loop;

  // Once the ProfilingProcessHost has dumped to the trace, stop the trace and
  // collate the results into |result|, then quit the nested run loop.
  auto finish_sink_callback = base::Bind(
      [](scoped_refptr<base::RefCountedString>* result, base::Closure finished,
         std::unique_ptr<const base::DictionaryValue> metadata,
         base::RefCountedString* in) {
        *result = in;
        std::move(finished).Run();
      },
      &serialized_trace_, run_loop.QuitClosure());
  scoped_refptr<content::TracingController::TraceDataEndpoint> sink =
      content::TracingController::CreateStringEndpoint(
          std::move(finish_sink_callback));
  base::OnceClosure stop_tracing_closure = base::BindOnce(
      base::IgnoreResult<bool (content::TracingController::*)(  // NOLINT
          const scoped_refptr<content::TracingController::TraceDataEndpoint>&)>(
          &content::TracingController::StopTracing),
      base::Unretained(content::TracingController::GetInstance()), sink);
  base::OnceClosure stop_tracing_ui_thread_closure =
      base::BindOnce(base::IgnoreResult(&base::TaskRunner::PostTask),
                     base::ThreadTaskRunnerHandle::Get(), FROM_HERE,
                     std::move(stop_tracing_closure));
  profiling::ProfilingProcessHost::GetInstance()
      ->SetDumpProcessForTracingCallback(
          std::move(stop_tracing_ui_thread_closure));

  // Spin a nested RunLoop until the heap dump has been added to the trace.
  content::TracingController::GetInstance()->StartTracing(
      base::trace_event::TraceConfig(
          base::trace_event::TraceConfigMemoryTestUtil::
              GetTraceConfig_PeriodicTriggers(100000, 100000)),
      base::Closure());
  run_loop.Run();
}

bool ProfilingTestDriver::ValidateBrowserAllocations(base::Value* dump_json) {
    base::Value* heaps_v2 =
        FindHeapsV2(base::Process::Current().Pid(), dump_json);
    bool result =
        ValidateDump(heaps_v2, kBrowserAllocSize * kBrowserAllocCount,
                     kBrowserAllocCount, "malloc", nullptr);
    if (!result) {
      LOG(ERROR) << "Failed to validate malloc fixed allocations";
      return false;
    }

    result = ValidateDump(heaps_v2, total_variadic_allocations_,
                                         kBrowserAllocCount, "malloc",
                                         nullptr);
    if (!result) {
      LOG(ERROR) << "Failed to validate malloc variadic allocations";
      return false;
    }

    result = ValidateDump(
        heaps_v2, kPartitionAllocSize * kPartitionAllocCount,
        kPartitionAllocCount, "partition_alloc", kPartitionAllocTypeName);
    if (!result) {
      LOG(ERROR) << "Failed to validate PA allocations";
      return false;
    }

    result = HasProcessWithName(dump_json, "Browser", nullptr);
    if (!result) {
      LOG(ERROR) << "Failed to find process with name: Browser";
      return false;
    }

    return true;
}

bool ProfilingTestDriver::ValidateRendererAllocations(base::Value* dump_json) {
  int pid;
  bool result = HasProcessWithName(dump_json, "Renderer", &pid);
  if (!result) {
    LOG(ERROR) << "Failed to find process with name Renderer";
    return false;
  }

  base::ProcessId renderer_pid = static_cast<base::ProcessId>(pid);
  base::Value* heaps_v2 = FindHeapsV2(renderer_pid, dump_json);
  if (options_.mode == ProfilingProcessHost::Mode::kAll) {
    if (!heaps_v2) {
      LOG(ERROR) << "Failed to find heaps v2 for renderer";
      return false;
    }

    // ValidateDump doesn't always succeed for the renderer, since we don't do
    // anything to flush allocations, there are very few allocations recorded
    // by the heap profiler. When we do a heap dump, we prune small
    // allocations...and this can cause all allocations to be pruned.
    // ASSERT_NO_FATAL_FAILURE(ValidateDump(dump_json.get(), 0, 0));
  } else {
    if (heaps_v2) {
      LOG(ERROR) << "There should be no heap dump for the renderer.";
      return false;
    }
  }

  return true;
}

}  // namespace profiling
