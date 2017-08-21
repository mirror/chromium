// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_DB_MESSAGE_FILTER_H_
#define CONTENT_CHILD_DB_MESSAGE_FILTER_H_

#include <stdint.h>

#include "base/strings/string16.h"
#include "content/common/database.mojom.h"

namespace url {
class Origin;
}  // namespace url

namespace content {

// Receives database messages from the browser process and processes them on the
// IO thread.
class DBMessageFilter : public content::mojom::Database {
 public:
  DBMessageFilter();
  ~DBMessageFilter() override;

 private:
  // content::mojom::Database:
  void UpdateSize(const url::Origin& origin,
                  const base::string16& name,
                  int64_t size) override;
  void UpdateSpaceAvailable(const url::Origin& origin,
                            int64_t space_available) override;
  void ResetSpaceAvailable(const url::Origin& origin) override;
  void CloseImmediately(const url::Origin& origin,
                        const base::string16& name) override;
};

}  // namespace content

#endif  // CONTENT_CHILD_DB_MESSAGE_FILTER_H_
