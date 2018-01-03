// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_INVALIDATOR_H_
#define MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_INVALIDATOR_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "mojo/public/cpp/bindings/bindings_export.h"

namespace mojo {

// Notifies weak interface bindings to be invalidated when this object is
// destroyed.
class MOJO_CPP_BINDINGS_EXPORT InterfaceInvalidator {
 public:
  InterfaceInvalidator();
  ~InterfaceInvalidator();

  class Observer {
   public:
    virtual void OnInvalidate() = 0;
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(const Observer* observer);

  base::WeakPtr<InterfaceInvalidator> GetWeakPtr();

 private:
  void NotifyInvalidate();

  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<InterfaceInvalidator> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(InterfaceInvalidator);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_INVALIDATOR_H_
