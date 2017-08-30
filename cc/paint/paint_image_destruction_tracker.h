// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_IMAGE_DESTRUCTION_TRACKER_H_
#define CC_PAINT_PAINT_IMAGE_DESTRUCTION_TRACKER_H_

#include "base/memory/singleton.h"
#include "base/observer_list_threadsafe.h"
#include "cc/paint/paint_export.h"
#include "cc/paint/paint_image.h"

namespace cc {

class CC_PAINT_EXPORT PaintImageDestructionTracker {
 public:
  class CC_PAINT_EXPORT Observer {
   public:
    virtual ~Observer() {}

    virtual void DestroyCachedDecode(PaintImage::Id paint_image_id,
                                     PaintImage::ContentId content_id) = 0;
  };

  static PaintImageDestructionTracker* GetInstance();

  void AddObserver(Observer* obs);
  void RemoveObserver(Observer* obs);

  void NotifyImageDestroyed(PaintImage::Id paint_image_id,
                            PaintImage::ContentId content_id);

 private:
  friend struct base::DefaultSingletonTraits<PaintImageDestructionTracker>;

  PaintImageDestructionTracker();
  ~PaintImageDestructionTracker();

  scoped_refptr<base::ObserverListThreadSafe<Observer>> observers_;

  DISALLOW_COPY_AND_ASSIGN(PaintImageDestructionTracker);
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_IMAGE_DESTRUCTION_TRACKER_H_
