// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/cros/update_library.h"

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/observer_list.h"
#include "chrome/browser/chromeos/cros/cros_library.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace chromeos {

class UpdateLibraryImpl : public UpdateLibrary {
 public:
  UpdateLibraryImpl() : status_connection_(NULL) {}

  virtual ~UpdateLibraryImpl() {
    if (status_connection_) {
      chromeos::DisconnectUpdateProgress(status_connection_);
      status_connection_ = NULL;
    }
  }

  // Begin UpdateLibrary implementation.
  virtual void Init() OVERRIDE {
    DCHECK(CrosLibrary::Get()->libcros_loaded());
    CHECK(!status_connection_) << "Already initialized";
    status_connection_ =
        chromeos::MonitorUpdateStatus(&UpdateStatusHandler, this);
    // Asynchronously load the initial state.
    chromeos::RequestUpdateStatus(&UpdateStatusHandler, this);
  }

  virtual void AddObserver(Observer* observer) OVERRIDE {
    observers_.AddObserver(observer);
  }

  virtual void RemoveObserver(Observer* observer) OVERRIDE {
    observers_.RemoveObserver(observer);
  }

  virtual bool HasObserver(Observer* observer) OVERRIDE {
    return observers_.HasObserver(observer);
  }

  virtual void RequestUpdateCheck(chromeos::UpdateCallback callback,
                                  void* user_data) OVERRIDE {
    chromeos::RequestUpdateCheck(callback, user_data);
  }

  virtual void RebootAfterUpdate() OVERRIDE {
    chromeos::RebootIfUpdated();
  }

  virtual void SetReleaseTrack(const std::string& track) OVERRIDE {
    chromeos::SetUpdateTrack(track);
  }

  virtual void GetReleaseTrack(chromeos::UpdateTrackCallback callback,
                               void* user_data) OVERRIDE {
    chromeos::RequestUpdateTrack(callback, user_data);
  }
  // End UpdateLibrary implementation.

  virtual const UpdateLibrary::Status& status() const OVERRIDE{
    return status_;
  }

 private:
  static void UpdateStatusHandler(void* object, const UpdateProgress& status) {
    UpdateLibraryImpl* impl = static_cast<UpdateLibraryImpl*>(object);
    impl->UpdateStatus(Status(status));
  }

  void UpdateStatus(const Status& status) {
    // Called from UpdateStatusHandler, a libcros callback which should
    // always run on UI thread.
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

    status_ = status;
    FOR_EACH_OBSERVER(Observer, observers_, UpdateStatusChanged(status));
  }

  ObserverList<Observer> observers_;

  // A reference to the update api, to allow callbacks when the update
  // status changes.
  UpdateStatusConnection status_connection_;

  // The latest power status.
  Status status_;

  DISALLOW_COPY_AND_ASSIGN(UpdateLibraryImpl);
};

class UpdateLibraryStubImpl : public UpdateLibrary {
 public:
  UpdateLibraryStubImpl() {}
  virtual ~UpdateLibraryStubImpl() {}

  // Begin UpdateLibrary implementation.
  virtual void Init() OVERRIDE {}
  virtual void AddObserver(Observer* observer) OVERRIDE {}
  virtual void RemoveObserver(Observer* observer) OVERRIDE {}
  virtual bool HasObserver(Observer* observer) OVERRIDE { return false; }
  virtual void RequestUpdateCheck(chromeos::UpdateCallback callback,
                                  void* user_data) OVERRIDE {
    if (callback)
      callback(user_data, UPDATE_RESULT_FAILED, "stub update");
  }
  virtual void RebootAfterUpdate() OVERRIDE {}
  virtual void SetReleaseTrack(const std::string& track) OVERRIDE {}
  virtual void GetReleaseTrack(chromeos::UpdateTrackCallback callback,
                               void* user_data) OVERRIDE {
    if (callback)
      callback(user_data, "beta-channel");
  }
  // End UpdateLibrary implementation.

  virtual const UpdateLibrary::Status& status() const OVERRIDE {
    return status_;
  }

 private:
  Status status_;

  DISALLOW_COPY_AND_ASSIGN(UpdateLibraryStubImpl);
};

// static
UpdateLibrary* UpdateLibrary::GetImpl(bool stub) {
  UpdateLibrary* impl;
  if (stub)
    impl = new UpdateLibraryStubImpl();
  else
    impl = new UpdateLibraryImpl();
  impl->Init();
  return impl;
}

}  // namespace chromeos
