// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_TRANSIENT_ELEMENT_H_
#define CHROME_BROWSER_VR_ELEMENTS_TRANSIENT_ELEMENT_H_

#include "base/callback.h"
#include "chrome/browser/vr/elements/textured_element.h"

namespace vr {

// The reason why a transient element hid itself.
enum class TransientElementHideReason : int {
  kTimeout,
  kSignal,  // Note that this is only used by ShowUntilSignalTransientElement.
};

// Base class for a transient element that automatically hides itself after some
// point in time. The exacly transience behavior depends on the subclass.
class TransientElement : public UiElement {
 public:
  typedef base::RepeatingCallback<void(TransientElementHideReason)> Callback;
  ~TransientElement() override;

  // Sets the elements visibility to the given value. If the visibility is
  // changing to true, it stays visible for the set timeout.
  void SetVisible(bool visible) override;
  void SetVisibleImmediately(bool visible) override;

  // Resets the time this element stays visible for if the element is currently
  // visible.
  void RefreshVisible();

  void set_callback(const Callback& callback) { callback_ = callback; }

 protected:
  explicit TransientElement(const base::TimeDelta& timeout);

  base::TimeDelta timeout_;
  base::TimeTicks set_visible_time_;
  Callback callback_;

 private:
  typedef UiElement super;

  DISALLOW_COPY_AND_ASSIGN(TransientElement);
};

// An element that hides itself after after a set timeout.
class SimpleTransientElement : public TransientElement {
 public:
  explicit SimpleTransientElement(const base::TimeDelta& timeout);
  ~SimpleTransientElement() override;

 private:
  bool OnBeginFrame(const base::TimeTicks& time,
                    const gfx::Vector3dF& head_direction) override;

  typedef TransientElement super;

  DISALLOW_COPY_AND_ASSIGN(SimpleTransientElement);
};

// An element that waits for a signal or timeout to hide itself once its been
// made visible. The element will stay visible for at least the set
// minimum duration regardless of when ::Signal is called. The set callback
// is triggered when the element hides itself.
class ShowUntilSignalTransientElement : public TransientElement {
 public:
  ShowUntilSignalTransientElement(const base::TimeDelta& min_duration,
                                  const base::TimeDelta& timeout,
                                  const Callback& callback);
  ~ShowUntilSignalTransientElement() override;

  // This must be called before the set timeout to hide the element.
  void Signal(bool value);

 private:
  bool OnBeginFrame(const base::TimeTicks& time,
                    const gfx::Vector3dF& head_direction) override;

  typedef TransientElement super;

  base::TimeDelta min_duration_;
  bool signaled_ = false;

  DISALLOW_COPY_AND_ASSIGN(ShowUntilSignalTransientElement);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_TRANSIENT_ELEMENT_H_
