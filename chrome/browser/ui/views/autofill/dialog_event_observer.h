// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_DIALOG_EVENT_OBSERVER_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_DIALOG_EVENT_OBSERVER_H_

#include <list>

#include "base/run_loop.h"

namespace autofill {

// DialogEventObserver is used to wait on specific events that may have
// occured before the call to Wait(), or after, in which case a RunLoop is
// used.
//
// Usage:
// observer_ =
// base::MakeUnique<DialogEventObserver>(std:list<DialogEvent>(...));
//
// Do stuff, which (a)synchronously calls observer_->Observe(...).
//
// observer_->Wait();  <- Will either return right away if events were
//                     <- observed, or use a RunLoop's Run/Quit to wait.
template <typename DialogEvent>
class DialogEventObserver {
 public:
  explicit DialogEventObserver(std::list<DialogEvent> event_sequence);
  ~DialogEventObserver();

  // Either returns right away if all events were observed between this
  // object's construction and this call to Wait(), or use a RunLoop to wait
  // for them.
  void Wait();

  // Observes and event (quits the RunLoop if we are done waiting).
  void Observe(DialogEvent event);

 private:
  std::list<DialogEvent> events_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(DialogEventObserver);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_DIALOG_EVENT_OBSERVER_H_
