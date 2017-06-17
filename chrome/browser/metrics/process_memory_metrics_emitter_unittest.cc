// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/process_memory_metrics_emitter.h"

#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "components/ukm/test_ukm_recorder.h"
#include "testing/gtest/include/gtest/gtest.h"

using GlobalMemoryDumpPtr = memory_instrumentation::mojom::GlobalMemoryDumpPtr;
using ProcessMemoryDumpPtr =
    memory_instrumentation::mojom::ProcessMemoryDumpPtr;
using ProcessType = memory_instrumentation::mojom::ProcessType;

namespace {

class ProcessMemoryMetricsEmitterFake : public ProcessMemoryMetricsEmitter {
 public:
  explicit ProcessMemoryMetricsEmitterFake() {}

  void ReceivedMemoryDump(
      uint64_t dump_guid,
      bool success,
      memory_instrumentation::mojom::GlobalMemoryDumpPtr ptr) override {
    ProcessMemoryMetricsEmitter::ReceivedMemoryDump(dump_guid, success,
                                                    std::move(ptr));
  }

 private:
  ~ProcessMemoryMetricsEmitterFake() override {}

  DISALLOW_COPY_AND_ASSIGN(ProcessMemoryMetricsEmitterFake);
};

void PopulateBrowserMetrics(GlobalMemoryDumpPtr& global_dump,
                            base::flat_map<const char*, int64_t>& metrics_mb) {
  ProcessMemoryDumpPtr pmd(
      memory_instrumentation::mojom::ProcessMemoryDump::New());
  pmd->process_type = ProcessType::BROWSER;
  pmd->os_dump.resident_set_kb = metrics_mb["Resident"] * 1024;
  pmd->chrome_dump.malloc_total_kb = metrics_mb["Malloc"] * 1024;
  pmd->private_footprint = metrics_mb["PrivateMemoryFootprint"] * 1024;
  global_dump->process_dumps.push_back(std::move(pmd));
}

void PopulateRendererMetrics(GlobalMemoryDumpPtr& global_dump,
                             base::flat_map<const char*, int64_t>& metrics_mb) {
  ProcessMemoryDumpPtr pmd(
      memory_instrumentation::mojom::ProcessMemoryDump::New());
  pmd->process_type = ProcessType::RENDERER;
  pmd->os_dump.resident_set_kb = metrics_mb["Resident"] * 1024;
  pmd->chrome_dump.malloc_total_kb = metrics_mb["Malloc"] * 1024;
  pmd->private_footprint = metrics_mb["PrivateMemoryFootprint"] * 1024;
  pmd->chrome_dump.partition_alloc_total_kb =
      metrics_mb["PartitionAlloc"] * 1024;
  pmd->chrome_dump.blink_gc_total_kb = metrics_mb["BlinkGC"] * 1024;
  pmd->chrome_dump.v8_total_kb = metrics_mb["V8"] * 1024;
  global_dump->process_dumps.push_back(std::move(pmd));
}

void PopulateGpuMetrics(GlobalMemoryDumpPtr& global_dump,
                        base::flat_map<const char*, int64_t>& metrics_mb) {
  ProcessMemoryDumpPtr pmd(
      memory_instrumentation::mojom::ProcessMemoryDump::New());
  pmd->process_type = ProcessType::GPU;
  pmd->os_dump.resident_set_kb = metrics_mb["Resident"] * 1024;
  pmd->chrome_dump.malloc_total_kb = metrics_mb["Malloc"] * 1024;
  pmd->private_footprint = metrics_mb["PrivateMemoryFootprint"] * 1024;
  global_dump->process_dumps.push_back(std::move(pmd));
}

void PopulateMetrics(GlobalMemoryDumpPtr& global_dump,
                     ProcessType ptype,
                     base::flat_map<const char*, int64_t>& metrics_mb) {
  switch (ptype) {
    case ProcessType::BROWSER:
      PopulateBrowserMetrics(global_dump, metrics_mb);
      break;
    case ProcessType::RENDERER:
      PopulateRendererMetrics(global_dump, metrics_mb);
      break;
    case ProcessType::GPU:
      PopulateGpuMetrics(global_dump, metrics_mb);
      break;
    default:
      FAIL() << "Unknown process type case " << ptype << ".";
  }
}

}  // namespace

class ProcessMemoryMetricsEmitterTest
    : public testing::TestWithParam<ProcessType> {
 public:
  ProcessMemoryMetricsEmitterTest()
      : test_ukm_recorder_(new ukm::TestUkmRecorder()) {}
  ~ProcessMemoryMetricsEmitterTest() override {}

  void CheckMemoryUkmEntryMetrics(
      const ukm::TestUkmRecorder* ukm_recorder,
      size_t entry_num,
      base::flat_map<const char*, int64_t> expected_metrics) {
    const ukm::mojom::UkmEntry* entry = ukm_recorder->GetEntry(entry_num);
    for (auto it = expected_metrics.begin(); it != expected_metrics.end();
         ++it) {
      const ukm::mojom::UkmMetric* actual_metric =
          ukm_recorder->FindMetric(entry, it->first);
      EXPECT_EQ(it->second, actual_metric->value);
    }
  }

 protected:
  base::flat_map<const char*, int64_t> GetExpectedProcessMetrics(
      ProcessType ptype) {
    switch (ptype) {
      case ProcessType::BROWSER:
        return GetExpectedBrowserMetrics();
      case ProcessType::RENDERER:
        return GetExpectedRendererMetrics();
      case ProcessType::GPU:
        return GetExpectedGpuMetrics();
      default:
        return base::flat_map<const char*, int64_t>();
    }
  }

  std::unique_ptr<ukm::TestUkmRecorder> test_ukm_recorder_;

 private:
  base::flat_map<const char*, int64_t> GetExpectedBrowserMetrics() {
    return base::flat_map<const char*, int64_t>(
        {
            {"ProcessType", static_cast<int64_t>(ProcessType::BROWSER)},
            {"Resident", 10},
            {"Malloc", 20},
            {"PrivateMemoryFootprint", 30},
        },
        base::KEEP_FIRST_OF_DUPES);
  }

  base::flat_map<const char*, int64_t> GetExpectedRendererMetrics() {
    return base::flat_map<const char*, int64_t>(
        {
            {"ProcessType", static_cast<int64_t>(ProcessType::RENDERER)},
            {"Resident", 110},
            {"Malloc", 120},
            {"PrivateMemoryFootprint", 130},
            {"PartitionAlloc", 140},
            {"BlinkGC", 150},
            {"V8", 160},
        },
        base::KEEP_FIRST_OF_DUPES);
  }

  base::flat_map<const char*, int64_t> GetExpectedGpuMetrics() {
    return base::flat_map<const char*, int64_t>(
        {
            {"ProcessType", static_cast<int64_t>(ProcessType::GPU)},
            {"Resident", 210},
            {"Malloc", 220},
            {"PrivateMemoryFootprint", 230},
        },
        base::KEEP_FIRST_OF_DUPES);
  }

  DISALLOW_COPY_AND_ASSIGN(ProcessMemoryMetricsEmitterTest);
};

TEST_P(ProcessMemoryMetricsEmitterTest, CollectsSingleProcessUKMs) {
  base::flat_map<const char*, int64_t> expected_metrics =
      GetExpectedProcessMetrics(GetParam());

  GlobalMemoryDumpPtr global_dump(
      memory_instrumentation::mojom::GlobalMemoryDump::New());
  PopulateMetrics(global_dump, GetParam(), expected_metrics);

  scoped_refptr<ProcessMemoryMetricsEmitterFake> emitter(
      new ProcessMemoryMetricsEmitterFake());
  emitter->ReceivedMemoryDump(0, true, std::move(global_dump));

  CheckMemoryUkmEntryMetrics(test_ukm_recorder_.get(), 0, expected_metrics);
}

INSTANTIATE_TEST_CASE_P(SinglePtype,
                        ProcessMemoryMetricsEmitterTest,
                        testing::Values(ProcessType::BROWSER,
                                        ProcessType::RENDERER,
                                        ProcessType::GPU));

/*TEST_F(ProcessMemoryMetricsEmitterTest, CollectBrowserUKMs) {
  base::flat_map<const char*, int64_t> expected_metrics =
      GetExpectedProcessMetrics(ProcessType::BROWSER);

  GlobalMemoryDumpPtr global_dump(
      memory_instrumentation::mojom::GlobalMemoryDump::New());
  PopulateMetrics(global_dump, ProcessType::BROWSER, expected_metrics);

  scoped_refptr<ProcessMemoryMetricsEmitterFake> emitter(
      new ProcessMemoryMetricsEmitterFake());
  emitter->ReceivedMemoryDump(0, true, std::move(global_dump));

  CheckMemoryUkmEntryMetrics(test_ukm_recorder_.get(), 0, expected_metrics);
}

TEST_F(ProcessMemoryMetricsEmitterTest, CollectRendererUKMs) {
  base::flat_map<const char*, int64_t> expected_metrics =
      GetExpectedProcessMetrics(ProcessType::RENDERER);

  GlobalMemoryDumpPtr global_dump(
      memory_instrumentation::mojom::GlobalMemoryDump::New());
  PopulateRendererMetrics(global_dump, expected_metrics);

  scoped_refptr<ProcessMemoryMetricsEmitterFake> emitter(
      new ProcessMemoryMetricsEmitterFake());
  emitter->ReceivedMemoryDump(0, true, std::move(global_dump));

  CheckMemoryUkmEntryMetrics(test_ukm_recorder_.get(), 0, expected_metrics);
}

TEST_F(ProcessMemoryMetricsEmitterTest, CollectGpuUKMs) {
  base::flat_map<const char*, int64_t> expected_metrics =
      GetExpectedProcessMetrics(ProcessType::GPU);

  GlobalMemoryDumpPtr global_dump(
      memory_instrumentation::mojom::GlobalMemoryDump::New());
  PopulateGpuMetrics(global_dump, expected_metrics);

  scoped_refptr<ProcessMemoryMetricsEmitterFake> emitter(
      new ProcessMemoryMetricsEmitterFake());
  emitter->ReceivedMemoryDump(0, true, std::move(global_dump));

  CheckMemoryUkmEntryMetrics(test_ukm_recorder_.get(), 0, expected_metrics);
}*/

TEST_F(ProcessMemoryMetricsEmitterTest, CollectsManyProcessUKMsSingleDump) {
  std::vector<ProcessType> entries_ptypes = {
      ProcessType::BROWSER, ProcessType::RENDERER, ProcessType::GPU,
      ProcessType::GPU,     ProcessType::RENDERER, ProcessType::BROWSER,
  };

  GlobalMemoryDumpPtr global_dump(
      memory_instrumentation::mojom::GlobalMemoryDump::New());
  std::vector<base::flat_map<const char*, int64_t>> entries_metrics;
  for (const auto& ptype : entries_ptypes) {
    auto expected_metrics = GetExpectedProcessMetrics(ptype);
    PopulateMetrics(global_dump, ptype, expected_metrics);
    entries_metrics.push_back(expected_metrics);
  }

  scoped_refptr<ProcessMemoryMetricsEmitterFake> emitter(
      new ProcessMemoryMetricsEmitterFake());
  emitter->ReceivedMemoryDump(0, true, std::move(global_dump));

  for (size_t i = 0; i < entries_ptypes.size(); ++i) {
    CheckMemoryUkmEntryMetrics(test_ukm_recorder_.get(), i, entries_metrics[i]);
  }
}

TEST_F(ProcessMemoryMetricsEmitterTest, CollectsManyProcessUKMsManyDumps) {
  std::vector<std::vector<ProcessType>> entries_ptypes = {
      {ProcessType::BROWSER, ProcessType::RENDERER, ProcessType::GPU},
      {ProcessType::GPU, ProcessType::RENDERER, ProcessType::BROWSER},
  };

  std::vector<base::flat_map<const char*, int64_t>> entries_metrics;
  scoped_refptr<ProcessMemoryMetricsEmitterFake> emitter(
      new ProcessMemoryMetricsEmitterFake());
  for (int i = 0; i < 2; ++i) {
    GlobalMemoryDumpPtr global_dump(
        memory_instrumentation::mojom::GlobalMemoryDump::New());
    for (const auto& ptype : entries_ptypes[i]) {
      auto expected_metrics = GetExpectedProcessMetrics(ptype);
      PopulateMetrics(global_dump, ptype, expected_metrics);
      entries_metrics.push_back(expected_metrics);
    }
    emitter->ReceivedMemoryDump(0, true, std::move(global_dump));
  }

  for (size_t i = 0; i < entries_ptypes.size(); ++i) {
    CheckMemoryUkmEntryMetrics(test_ukm_recorder_.get(), i, entries_metrics[i]);
  }
}
