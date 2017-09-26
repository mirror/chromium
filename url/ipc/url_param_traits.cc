// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "url/ipc/url_param_traits.h"

#include "url/gurl.h"

namespace IPC {

void ParamTraits<GURL>::Write(base::Pickle* m, const GURL& p) {
  if (p.possibly_invalid_spec().length() > url::kMaxURLChars || !p.is_valid()) {
    m->WriteBool(false);
    return;
  }
  m->WriteBool(true);
  m->WriteString(p.possibly_invalid_spec());
  // TODO(brettw) bug 684583: Add encoding for query params.
}

bool ParamTraits<GURL>::Read(const base::Pickle* m,
                             base::PickleIterator* iter,
                             GURL* p) {
  bool valid = false;
  if (!iter->ReadBool(&valid))
    return false;
  if (!valid) {
    *p = GURL();
    return true;
  }
  std::string s;
  if (!iter->ReadString(&s) || s.length() > url::kMaxURLChars)
    return false;
  *p = GURL(s);
  if (!p->is_valid())
    return false;
  return true;
}

void ParamTraits<GURL>::Log(const GURL& p, std::string* l) {
  l->append(p.spec());
}

}  // namespace IPC
