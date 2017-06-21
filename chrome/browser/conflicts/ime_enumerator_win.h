// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_IME_ENUMERATOR_WIN_H_
#define CHROME_BROWSER_CONFLICTS_IME_ENUMERATOR_WIN_H_

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"

namespace base {
class SequencedTaskRunner;
}

// Finds third-party IMEs (Input Method Editor) installed on the computer by
// enumerating the registry.
class ImeEnumerator {
 public:
  // The path to the registry key where IMEs are registered.
  static const wchar_t kImeRegistryKey[];
  static const wchar_t kClassIdRegistryKeyFormat[];

  using OnImeEnumeratedCallback =
      base::Callback<void(const base::FilePath&, uint32_t, uint32_t)>;

  explicit ImeEnumerator(
      const OnImeEnumeratedCallback& on_ime_enumerated_callback);
  ~ImeEnumerator();

 private:
  // Enumerates registered third-party IMEs, and invokes |callback| via the
  // |task_runner| once per IME found. Must be called on a blocking sequence.
  static void EnumerateImesOnBlockingSequence(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      const OnImeEnumeratedCallback& callback);

  // Invokes |on_ime_enumerated_callback_|. This function is bound using a
  // weak_ptr to provide thread-safety.
  void OnImeEnumerated(const base::FilePath& path,
                       uint32_t size_of_image,
                       uint32_t time_date_stamp);

  // Retrieves the inproc server dll associated to |guid|.
  static bool GetInprocServerDll(const wchar_t* guid, base::FilePath* dll);

  OnImeEnumeratedCallback on_ime_enumerated_callback_;

  base::WeakPtrFactory<ImeEnumerator> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ImeEnumerator);
};

#endif  // CHROME_BROWSER_CONFLICTS_IME_ENUMERATOR_WIN_H_
