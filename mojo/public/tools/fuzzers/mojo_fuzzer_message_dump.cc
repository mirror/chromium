// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/threading/thread.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/tools/fuzzers/fuzz.mojom.h"

/* Dummy implementation of the FuzzInterface. */
class FuzzImpl : public fuzz::mojom::FuzzInterface {
 public:
  explicit FuzzImpl(fuzz::mojom::FuzzInterfaceRequest request)
      : binding_(this, std::move(request)) {}

  void FuzzBasic() override {}

  void FuzzBasicResp(FuzzBasicRespCallback callback) override {
    std::move(callback).Run();
  }

  void FuzzBasicSyncResp(FuzzBasicSyncRespCallback callback) override {
    std::move(callback).Run();
  }

  void FuzzArgs(fuzz::mojom::FuzzStructPtr a,
                fuzz::mojom::FuzzStructPtr b) override {}

  void FuzzArgsResp(fuzz::mojom::FuzzStructPtr a,
                    fuzz::mojom::FuzzStructPtr b,
                    FuzzArgsRespCallback callback) override {
    std::move(callback).Run();
  };

  void FuzzArgsSyncResp(fuzz::mojom::FuzzStructPtr a,
                        fuzz::mojom::FuzzStructPtr b,
                        FuzzArgsSyncRespCallback callback) override {
    std::move(callback).Run();
  };

  ~FuzzImpl() override {}

  /* Expose the binding to the fuzz harness. */
  mojo::Binding<FuzzInterface> binding_;
};

/* Environment for the fuzzer. Initializes the mojo EDK and sets up a
 * TaskScheduler, because Mojo messages must be sent and processed from
 * TaskRunners. */
struct Environment {
  Environment() : message_loop() {
    base::TaskScheduler::CreateAndStartWithDefaultParams(
        "MojoFuzzCorpusDumpProcess");
    mojo::edk::Init();
  }

  /* Message loop to send  messages on. */
  base::MessageLoop message_loop;

  /* Impl to be created. Stored in environment to keep it alive after
   * DumpMessages returns. */
  std::unique_ptr<FuzzImpl> impl;
};

Environment* env = new Environment();

class MessageDumper : public mojo::MessageReceiver {
 public:
  explicit MessageDumper(std::string directory)
      : directory_(directory), count_(0) {}

  bool Accept(mojo::Message* message) override {
    base::FilePath path = directory_.Append(FILE_PATH_LITERAL("message_") +
                                            base::IntToString(count_++) +
                                            FILE_PATH_LITERAL(".mojomsg"));

    base::File file(path,
                    base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
    if (!file.IsValid()) {
      LOG(ERROR) << "Failed to create mojo message file: " << path.value();
      return false;
    }

    size_t size = message->data_num_bytes();
    const char* data = reinterpret_cast<const char*>(message->data());
    int ret = file.WriteAtCurrentPos(data, size);
    if (ret != static_cast<int>(size)) {
      LOG(ERROR) << "Failed to write " << size << " bytes.";
      return false;
    }
    return true;
  }

  base::FilePath directory_;
  int count_;
};

fuzz::mojom::FuzzStructPtr GetPopulatedFuzzStruct() {
  /* Make some Dummy structs. */
  fuzz::mojom::FuzzDummyStructPtr dummy1 = fuzz::mojom::FuzzDummyStruct::New();
  fuzz::mojom::FuzzDummyStructPtr dummy2 = fuzz::mojom::FuzzDummyStruct::New();

  /* Make some populated Unions. */
  fuzz::mojom::FuzzUnionPtr union_bool = fuzz::mojom::FuzzUnion::New();
  union_bool->set_fuzz_bool(true);

  fuzz::mojom::FuzzUnionPtr union_string = fuzz::mojom::FuzzUnion::New();
  union_string->set_fuzz_string(std::string("fuzz"));

  fuzz::mojom::FuzzUnionPtr union_struct_map = fuzz::mojom::FuzzUnion::New();
  std::unordered_map<std::string, fuzz::mojom::FuzzDummyStructPtr> struct_map;
  struct_map[std::string("fuzz")] = std::move(dummy1);
  union_struct_map->set_fuzz_struct_map(std::move(struct_map));

  std::vector<fuzz::mojom::FuzzUnionPtr> center;
  center.push_back(std::move(union_bool));

  std::unordered_map<int8_t,
                     base::Optional<std::vector<fuzz::mojom::FuzzUnionPtr>>>
      inner;
  inner['z'] = std::move(center);
  std::unordered_map<
      fuzz::mojom::FuzzEnum,
      std::unordered_map<
          int8_t, base::Optional<std::vector<fuzz::mojom::FuzzUnionPtr>>>>
      outer;
  outer[fuzz::mojom::FuzzEnum::FUZZ_VALUE0] = std::move(inner);
  fuzz::mojom::FuzzUnionPtr union_complex = fuzz::mojom::FuzzUnion::New();
  base::Optional<std::vector<std::unordered_map<
      fuzz::mojom::FuzzEnum,
      std::unordered_map<
          int8_t, base::Optional<std::vector<fuzz::mojom::FuzzUnionPtr>>>>>>
      complex_map;
  complex_map.emplace();
  complex_map.value().push_back(std::move(outer));

  union_complex->set_fuzz_complex(std::move(complex_map));

  /* Make a populated struct */
  std::vector<int8_t> fuzz_primitive_array = {'f', 'u', 'z', 'z'};
  std::unordered_map<std::string, int8_t> fuzz_primitive_map = {
      {std::string("fuzz"), 'z'}};
  std::unordered_map<std::string, std::vector<std::string>> fuzz_array_map = {
      {std::string("fuzz"), {std::string("fuzz1"), std::string("fuzz2")}}};
  std::unordered_map<fuzz::mojom::FuzzEnum, fuzz::mojom::FuzzUnionPtr>
      fuzz_union_map;
  fuzz_union_map[fuzz::mojom::FuzzEnum::FUZZ_VALUE1] =
      std::move(union_struct_map);
  std::vector<fuzz::mojom::FuzzUnionPtr> fuzz_union_array;
  fuzz_union_array.push_back(std::move(union_complex));
  std::vector<fuzz::mojom::FuzzStructPtr> fuzz_struct_array;
  base::Optional<std::vector<int8_t>> fuzz_nullable_array;

  std::vector<fuzz::mojom::FuzzStructPtr> struct_center;
  struct_center.push_back(fuzz::mojom::FuzzStruct::New());
  std::unordered_map<int8_t,
                     base::Optional<std::vector<fuzz::mojom::FuzzStructPtr>>>
      struct_inner;
  struct_inner['z'] = std::move(struct_center);
  std::unordered_map<
      fuzz::mojom::FuzzEnum,
      std::unordered_map<
          int8_t, base::Optional<std::vector<fuzz::mojom::FuzzStructPtr>>>>
      struct_outer;
  struct_outer[fuzz::mojom::FuzzEnum::FUZZ_VALUE0] = std::move(struct_inner);

  base::Optional<std::vector<std::unordered_map<
      fuzz::mojom::FuzzEnum,
      std::unordered_map<
          int8_t, base::Optional<std::vector<fuzz::mojom::FuzzStructPtr>>>>>>
      struct_complex_map;
  struct_complex_map.emplace();
  struct_complex_map.value().push_back(std::move(struct_outer));

  return fuzz::mojom::FuzzStruct::New(
      true,                            /* fuzz_bool */
      -1,                              /* fuzz_int8 */
      1,                               /* fuzz_uint8 */
      -(1 << 8),                       /* fuzz_int16 */
      1 << 8,                          /* fuzz_uint16 */
      -(1 << 16),                      /* fuzz_int32 */
      1 << 16,                         /* fuzz_uint32 */
      -((int64_t)1 << 32),             /* fuzz_int64 */
      (int64_t)1 << 32,                /* fuzz_uint64 */
      1.0,                             /* fuzz_float */
      1.0,                             /* fuzz_double */
      std::string("fuzz"),             /* fuzz_string */
      std::move(fuzz_primitive_array), /* fuzz_primitive_array */
      std::move(fuzz_primitive_map),   /* fuzz_primitive_map */
      std::move(fuzz_array_map),       /* fuzz_array_map */
      std::move(fuzz_union_map),       /* fuzz_union_map */
      std::move(fuzz_union_array),     /* fuzz_union_array */
      std::move(fuzz_struct_array),    /* fuzz_struct_array */
      std::move(fuzz_nullable_array),  /* fuzz_nullable_array */
      std::move(struct_complex_map));  /* fuzz_complex */
}

/* Callback used for messages with responses. Does nothing. */
void FuzzCallback() {}

/* Invokes each method in the FuzzInterface and dumps the messages to the
 * supplied directory. */
void DumpMessages(std::string output_directory) {
  fuzz::mojom::FuzzInterfacePtr fuzz;
  env->impl = base::MakeUnique<FuzzImpl>(MakeRequest(&fuzz));
  env->impl->binding_.AddFilter(
      base::MakeUnique<MessageDumper>(output_directory));

  /* Call methods in various ways to generate messages */
  fuzz->FuzzBasic();
  fuzz->FuzzBasicResp(base::Bind(FuzzCallback));
  fuzz->FuzzBasicSyncResp();
  fuzz->FuzzArgs(fuzz::mojom::FuzzStruct::New(),
                 fuzz::mojom::FuzzStructPtr(nullptr));
  fuzz->FuzzArgs(fuzz::mojom::FuzzStruct::New(), GetPopulatedFuzzStruct());
  fuzz->FuzzArgsResp(fuzz::mojom::FuzzStruct::New(), GetPopulatedFuzzStruct(),
                     base::Bind(FuzzCallback));
  fuzz->FuzzArgsResp(fuzz::mojom::FuzzStruct::New(), GetPopulatedFuzzStruct(),
                     base::Bind(FuzzCallback));
  fuzz->FuzzArgsSyncResp(fuzz::mojom::FuzzStruct::New(),
                         GetPopulatedFuzzStruct(), base::Bind(FuzzCallback));
  fuzz->FuzzArgsSyncResp(fuzz::mojom::FuzzStruct::New(),
                         GetPopulatedFuzzStruct(), base::Bind(FuzzCallback));
}

// Entry point for LibFuzzer.
int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: %s [output_directory]\n", argv[0]);
    exit(1);
  }
  std::string output_directory(argv[1]);

  /* Dump the messages from a MessageLoop, and wait for it to finish. */
  env->message_loop.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&DumpMessages, output_directory));
  base::RunLoop().RunUntilIdle();

  return 0;
}
