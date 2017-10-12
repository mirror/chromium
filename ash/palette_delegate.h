// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PALETTE_DELEGATE_H_
#define ASH_PALETTE_DELEGATE_H_

#include "ash/ash_export.h"
#include "base/callback.h"
#include "base/macros.h"

namespace ash {

// This delegate allows the UI code in ash, e.g. |PaletteTray|, to perform
// Chrome-specific actions.
// TODO(jamescook): Move this to //ash/system/palette.
class PaletteDelegate {
 public:
  virtual ~PaletteDelegate() {}

  // Create a new note.
  virtual void CreateNote() = 0;

  // Returns true if there is a note-taking application available.
  virtual bool HasNoteApp() = 0;

  // Take a screenshot of the entire window.
  virtual void TakeScreenshot() = 0;

  // Take a screenshot of a user-selected region. |done| is called when the
  // partial screenshot session has finished; a screenshot may or may not have
  // been taken.
  virtual void TakePartialScreenshot(const base::Closure& done) = 0;

  // Cancels any active partial screenshot session.
  virtual void CancelPartialScreenshot() = 0;

 private:
  DISALLOW_ASSIGN(PaletteDelegate);
};

}  // namespace ash

#endif  // ASH_PALETTE_DELEGATE_H_
