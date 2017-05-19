// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Safe Observer/Observable implementation.
//
// When using ObserverListThreadSafe, we were running into issues since there
// was no synchronization between getting the existing value, and registering
// as an observer. See go/cast-platform-design-synchronized-value for more
// details.
//
// To fix this issue, and to make observing values safer and simpler in general,
// use the Observable/Observer pattern in this file. When you have a value that
// other code wants to observe (ie, get the value of and receive any changes),
// wrap that value in an Observable<T>. The value type T must be copyable and
// copy-assignable. The Observable must be constructed with the initial observed
// value, and the value may be updated at any time from any thread by calling
// SetValue(). You can also get the value using GetValue(); but note that this
// is not threadsafe (the value is returned without locking), so the caller must
// ensure safety by other means. Calling GetValueThreadSafe() is threadsafe but
// involves a mutex lock.
//
// Code that wants to observe the value calls Observe() on it at any point when
// the value is alive. Note that Observe() may be called safely from any thread.
// Observe() returns an Observer<T> instance, which MUST be used and destroyed
// only on the thread that called Observe(). The Observer initially contains the
// value that the Observable had when Observe() was called, and that value will
// be updated asynchronously whenever the Observable's SetValue() method is
// is called. The Observer's view of the observed value is returned by
// GetValue(); this is a low-cost call since there is no locking (the value is
// updated on the thread that constructed the Observer). Note that Observers are
// always updated asynchronously with PostTask(), even if they belong to the
// same thread that calls SetValue().
//
// Observers may be copied freely; the copy also observes the original
// Observable, and belongs to the thread that created the copy. Copying is safe
// even when the original Observable has been destroyed.
//
// Code may register a callback that is called whenever an Observer's value is
// updated, by calling SetOnUpdateCallback(). If you get an Observer by calling
// Observe() and then immediately call SetOnUpdateCallback() to register a
// a callback, you are guaranteed to get every value of the Observable starting
// from when you called Observe() - you get the initial value by calling
// GetValue() on the returned Observer, and any subsequent updates will trigger
// the callback so you can call GetValue() to get the new value. You will not
// receive callbacks for calls to SetValue() that occurred before you called
// Observe().
//
// Note that Observers are not default-constructible, since there is no way to
// construct it in a default state. In cases where you need to instantiate an
// Observer after your constructor, you can use a std::unique_ptr<Observer>
// instead, and initialize it when needed.
//
// Example usage:
//
// class MediaManager {
//  public:
//   MediaManager() : volume_(0.0f) {}
//
//   Observer<float> ObserveVolume() { return volume_.Observe(); }
//
//   // ... other methods ...
//
//  private:
//   // Assume this is called from some other internal code when the volume us
//   // updated.
//   void OnUpdateVolume(float new_volume) {
//     volume_.SetValue(new_volume);  // All observers will get the new value.
//   }
//
//   Observable<float> volume_;
// }
//
// class VolumeFeedbackManager {
//  public:
//   VolumeFeedbackManager(MediaManager* media_manager)
//       : volume_observer_(media_manager->ObserveVolume()) {
//     volume_observer_.SetOnUpdateCallback(
//         base::BindRepeating(&VolumeFeedbackManager::OnVolumeChanged,
//                             base::Unretained(this)));
//   }
//
//  private:
//   void OnVolumeChanged() {
//     ShowVolumeFeedback(volume_observer_.GetValue());
//   }
//
//   void ShowVolumeFeedback(float volume) {
//     // ... some implementation ...
//   }
// };
//

#ifndef CHROMECAST_BASE_OBSERVER_H_
#define CHROMECAST_BASE_OBSERVER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace chromecast {

template <typename T>
class ObservableInternals;

template <typename T>
class Observer {
 public:
  explicit Observer(scoped_refptr<ObservableInternals<T>> internals);
  Observer(const Observer& other);

  ~Observer();

  void SetOnUpdateCallback(base::RepeatingClosure callback) {
    on_update_callback_ = std::move(callback);
  }

  const T& GetValue() const {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return value_;
  }

 private:
  friend class ObservableInternals<T>;

  DISALLOW_ASSIGN(Observer);

  void SetValue(uint32_t update_index, const T& new_value);

  const scoped_refptr<ObservableInternals<T>> internals_;
  // Expected index of the next value update. Used to ignore updates from before
  // this Observer was created.
  uint32_t next_update_index_;
  T value_;
  base::RepeatingClosure on_update_callback_;
  SEQUENCE_CHECKER(sequence_checker_);
};

template <typename T>
class Observable {
  static_assert(std::is_copy_constructible<T>::value,
                "Observable values must be copyable");
  static_assert(std::is_copy_assignable<T>::value,
                "Observable values must be copy-assignable");

 public:
  explicit Observable(const T& initial_value);
  Observer<T> Observe();

  void SetValue(const T& new_value);
  T GetValue() const;

 private:
  // By using a refcounted object to store the value and observer list, we can
  // avoid tying the lifetime of Observable to its Observers or vice versa.
  const scoped_refptr<ObservableInternals<T>> internals_;

  DISALLOW_COPY_AND_ASSIGN(Observable);
};

template <typename T>
class ObservableInternals
    : public base::RefCountedThreadSafe<ObservableInternals<T>> {
 public:
  explicit ObservableInternals(const T& initial_value)
      : next_update_index_(0u), value_(initial_value) {}

  void SetValue(const T& new_value) {
    base::AutoLock lock(lock_);
    value_ = new_value;

    for (auto& item : per_sequence_) {
      item.task_runner->PostTask(
          FROM_HERE,
          base::BindOnce(&ObservableInternals::NotifyObservers,
                         item.observers.get(), next_update_index_, value_));
    }
    ++next_update_index_;
  }

  // NOT threadsafe!
  const T& GetValue() const { return value_; }

  T GetValueThreadSafe() const {
    base::AutoLock lock(lock_);
    return value_;
  }

  T AddObserver(Observer<T>* observer) {
    DCHECK(base::SequencedTaskRunnerHandle::IsSet());
    auto task_runner = base::SequencedTaskRunnerHandle::Get();

    base::AutoLock lock(lock_);
    observer->next_update_index_ = next_update_index_;
    for (size_t i = 0; i < per_sequence_.size(); ++i) {
      if (per_sequence_[i].task_runner == task_runner) {
        per_sequence_[i].AddObserver(observer);
        return value_;
      }
    }
    per_sequence_.emplace_back(std::move(task_runner), observer);
    return value_;
  }

  void RemoveObserver(Observer<T>* observer) {
    DCHECK(base::SequencedTaskRunnerHandle::IsSet());
    auto task_runner = base::SequencedTaskRunnerHandle::Get();

    base::AutoLock lock(lock_);
    for (size_t i = 0; i < per_sequence_.size(); ++i) {
      if (per_sequence_[i].task_runner == task_runner) {
        if (per_sequence_[i].RemoveObserver(observer)) {
          if (i != per_sequence_.size() - 1) {
            per_sequence_[i] = std::move(per_sequence_.back());
          }
          per_sequence_.pop_back();
        }
        return;
      }
    }
    NOTREACHED() << "Tried to remove observer from unknown task runner";
  }

 private:
  using ObserverList = std::vector<Observer<T>*>;

  struct PerSequenceInfo {
    PerSequenceInfo(scoped_refptr<base::SequencedTaskRunner> task_runner_in,
                    Observer<T>* observer)
        : task_runner(std::move(task_runner_in)),
          observers(base::MakeUnique<ObserverList>()) {
      AddObserver(observer);
    }

    void AddObserver(Observer<T>* observer) {
      DCHECK(task_runner->RunsTasksInCurrentSequence());
      DCHECK(std::find(observers->begin(), observers->end(), observer) ==
             observers->end());
      observers->push_back(observer);
    }

    bool RemoveObserver(Observer<T>* observer) {
      DCHECK(task_runner->RunsTasksInCurrentSequence());
      DCHECK(std::find(observers->begin(), observers->end(), observer) !=
             observers->end());
      observers->erase(
          std::remove(observers->begin(), observers->end(), observer),
          observers->end());
      if (observers->empty()) {
        // Must post a task to delete the list of observers, since there may
        // still be a pending task to call NotifyObservers().
        // Use manual PostNonNestableTask(), since DeleteSoon() does not
        // guarantee deletion.
        task_runner->PostNonNestableTask(
            FROM_HERE, base::BindOnce(&ObservableInternals::DeleteObserverList,
                                      std::move(observers)));
        return true;
      }
      return false;
    }

    // Operations on |observers| do not need to be synchronized with a lock,
    // since all operations must occur on |task_runner|.
    scoped_refptr<base::SequencedTaskRunner> task_runner;
    std::unique_ptr<ObserverList> observers;
  };

  friend class base::RefCountedThreadSafe<ObservableInternals>;
  ~ObservableInternals() {}

  static void NotifyObservers(const ObserverList* observers,
                              uint32_t update_index,
                              const T& value) {
    for (auto* item : *observers) {
      item->SetValue(update_index, value);
    }
  }

  static void DeleteObserverList(std::unique_ptr<ObserverList> list) {
    // The unique_ptr deletes automatically.
  }

  mutable base::Lock lock_;
  uint32_t next_update_index_;
  T value_;
  std::vector<PerSequenceInfo> per_sequence_;

  DISALLOW_COPY_AND_ASSIGN(ObservableInternals);
};

template <typename T>
Observer<T>::Observer(scoped_refptr<ObservableInternals<T>> internals)
    : internals_(std::move(internals)), value_(internals_->AddObserver(this)) {}

template <typename T>
Observer<T>::Observer(const Observer& other) : Observer(other.internals_) {}

template <typename T>
Observer<T>::~Observer() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  internals_->RemoveObserver(this);
}

template <typename T>
void Observer<T>::SetValue(uint32_t update_index, const T& new_value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (update_index != next_update_index_) {
    // Ignore update from before this observer was added.
    DCHECK(update_index - next_update_index_ > 0x7fffffffu);
    return;
  }
  next_update_index_ = update_index + 1;
  value_ = new_value;
  if (on_update_callback_) {
    on_update_callback_.Run();
  }
}

template <typename T>
Observable<T>::Observable(const T& initial_value)
    : internals_(
          make_scoped_refptr(new ObservableInternals<T>(initial_value))) {}

template <typename T>
Observer<T> Observable<T>::Observe() {
  return Observer<T>(internals_);
}

template <typename T>
void Observable<T>::SetValue(const T& new_value) {
  internals_->SetValue(new_value);
}

template <typename T>
T Observable<T>::GetValue() const {
  return internals_->GetValue();
}

}  // namespace chromecast

#endif  // CHROMECAST_BASE_OBSERVER_H_
