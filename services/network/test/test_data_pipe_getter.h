// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_TEST_TEST_DATA_PIPE_GETTER_H_
#define SERVICES_NETWORK_TEST_TEST_DATA_PIPE_GETTER_H_

#include <memory>
#include <string>

#include "mojo/public/c/system/data_pipe.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/network/public/interfaces/data_pipe_getter.mojom.h"

namespace network {

class TestDataPipeGetter : public mojom::DataPipeGetter {
 public:
  TestDataPipeGetter(const std::string& upload_string,
                     mojom::DataPipeGetterRequest request);
  ~TestDataPipeGetter() override;

  // If set to anything other than net::OK, won't bother to upload the body.
  void set_start_error(int32_t start_error);

  // If set to true, will advertise the body size as being 1 byte larger than
  // upload string, so the data pipe will be closed when the URLLoader is still
  // expecting more data.
  void set_pipe_closed_early(bool pipe_closed_early);

  // mojom::DataPipeGetter implementation:
  void Read(mojo::ScopedDataPipeProducerHandle pipe,
            ReadCallback callback) override;
  void Clone(mojom::DataPipeGetterRequest request) override;

 private:
  void MojoReadyCallback(MojoResult result,
                         const mojo::HandleSignalsState& state);
  void WriteData();

  const std::string upload_string_;
  int32_t start_error_ = 0;  // net::OK
  bool pipe_closed_early_ = false;

  mojo::BindingSet<mojom::DataPipeGetter> bindings_;

  mojo::ScopedDataPipeProducerHandle upload_body_pipe_;
  // Must be below |upload_body_pipe_|, so it's deleted first.
  std::unique_ptr<mojo::SimpleWatcher> handle_watcher_;
  size_t write_position_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestDataPipeGetter);
};

}  // namespace network

#endif  // SERVICES_NETWORK_TEST_TEST_DATA_PIPE_GETTER_H_
