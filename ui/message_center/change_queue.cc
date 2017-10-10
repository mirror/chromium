// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/change_queue.h"

#include "ui/message_center/message_center_impl.h"
#include "ui/message_center/notification.h"

namespace message_center {

// Change represents an operation made on a notification. Since it contains
// the final state of the notification, except complex cases, we generally
// optimize the list and keep only the last change for a particular notification
// that is in the notification list around. There are two ids; |id_| is the
// post-update notification id that has been assigned by an update, and
// |previous_id| is the previous id of the notification before the Change.
// The two ids are same unless the Change changes the id of the notification.
// See the comments of id() and previous_id() for reference.
class ChangeQueue::Change {
 public:
  Change(ChangeType type,
         const std::string& id,
         std::unique_ptr<Notification> notification);
  ~Change();

  // Used to transfer ownership of the contained notification.
  std::unique_ptr<Notification> PassNotification();

  Notification* notification() const { return notification_.get(); }
  // Returns the post-update ID. It means:
  // - ADD event: ID of the notification to be added. In this case, this must be
  //   same as |previous_id()|.
  // - UPDATE event: ID of the notification after the change. If the change
  //   doesn't update its ID, this value is same as |previous_id()|.
  // - DELETE event: ID of the notification to be deleted. This must be same as
  //   |previous_id()|.
  const std::string& id() const { return id_; }
  ChangeType type() const { return type_; }
  bool by_user() const { return by_user_; }
  void set_by_user(bool by_user) { by_user_ = by_user; }
  // Returns the ID which is used in the notification list. In other word, it
  // means the ID before the change.
  const std::string& previous_id() const { return previous_id_; }
  void set_type(const ChangeType new_type) { type_ = new_type; }
  void ReplaceNotification(std::unique_ptr<Notification> new_notification);

 private:
  ChangeType type_;
  std::string id_;
  std::string previous_id_;
  bool by_user_;
  std::unique_ptr<Notification> notification_;

  DISALLOW_COPY_AND_ASSIGN(Change);
};

////////////////////////////////////////////////////////////////////////////////
// ChangeFinder

struct ChangeFinder {
  explicit ChangeFinder(const std::string& id) : id(id) {}
  bool operator()(const std::unique_ptr<ChangeQueue::Change>& change) {
    return change->id() == id;
  }

  std::string id;
};

////////////////////////////////////////////////////////////////////////////////
// ChangeQueue::Change

ChangeQueue::Change::Change(ChangeType type,
                            const std::string& id,
                            std::unique_ptr<Notification> notification)
    : type_(type),
      previous_id_(id),
      by_user_(false),
      notification_(std::move(notification)) {
  DCHECK(!id.empty());
  DCHECK(type != CHANGE_TYPE_DELETE || !notification_);

  id_ = notification_ ? notification_->id() : previous_id_;
}

ChangeQueue::Change::~Change() {}

std::unique_ptr<Notification> ChangeQueue::Change::PassNotification() {
  return std::move(notification_);
}

void ChangeQueue::Change::ReplaceNotification(
    std::unique_ptr<Notification> new_notification) {
  id_ = new_notification ? new_notification->id() : previous_id_;
  notification_.swap(new_notification);
}

////////////////////////////////////////////////////////////////////////////////
// ChangeQueue

ChangeQueue::ChangeQueue() = default;

ChangeQueue::~ChangeQueue() = default;

void ChangeQueue::ApplyChanges(MessageCenterImpl* message_center) {
  while (!changes_.empty()) {
    auto iter = changes_.begin();
    std::unique_ptr<Change> change(std::move(*iter));
    changes_.erase(iter);
    ApplyChangeInternal(message_center, std::move(change));
  }
}

void ChangeQueue::AddNotification(std::unique_ptr<Notification> notification) {
  std::string id = notification->id();
  changes_.push_back(
      std::make_unique<Change>(CHANGE_TYPE_ADD, id, std::move(notification)));
}

void ChangeQueue::UpdateNotification(
    const std::string& old_id,
    std::unique_ptr<Notification> notification) {
  // Pick up the previous change with the same ID to try optimizing the queue.
  auto iter =
      std::find_if(changes_.rbegin(), changes_.rend(), ChangeFinder(old_id));
  if (iter == changes_.rend()) {
    changes_.push_back(std::make_unique<Change>(CHANGE_TYPE_UPDATE, old_id,
                                                std::move(notification)));
    return;
  }

  Change* previous_change = iter->get();
  switch (previous_change->type()) {
    case CHANGE_TYPE_ADD: {
      std::string id = notification->id();
      // Needs to add the change at the last, because if this change updates
      // its ID, some previous changes may affect new ID.
      // (eg. Add A, Update B->C, and This update A->B).
      changes_.erase(--(iter.base()));
      changes_.push_back(std::make_unique<Change>(CHANGE_TYPE_ADD, id,
                                                  std::move(notification)));
      break;
    }
    case CHANGE_TYPE_UPDATE:
      if (notification->id() == old_id) {
        // Case 1:
        // - This update doesn't change the own ID.
        // Safe to place the change at the previous place.
        previous_change->ReplaceNotification(std::move(notification));
      } else if (previous_change->id() == previous_change->previous_id()) {
        // Case 2:
        // - This update changes the own ID.
        // - The previous update doesn't change the own ID.
        std::string id = previous_change->previous_id();
        // Safe to place the change at the last.
        changes_.erase(--(iter.base()));
        changes_.push_back(std::make_unique<Change>(CHANGE_TYPE_UPDATE, id,
                                                    std::move(notification)));
      } else {
        // Case 3:
        // - This update changes the own ID.
        // - The previous update also changes the own ID.
        // Complex case: gives up to optimize.
        changes_.push_back(std::make_unique<Change>(CHANGE_TYPE_UPDATE, old_id,
                                                    std::move(notification)));
      }
      break;
    case CHANGE_TYPE_DELETE:
      // DELETE -> UPDATE. Something is wrong. Treats the UPDATE as ADD.
      changes_.push_back(std::make_unique<Change>(CHANGE_TYPE_ADD, old_id,
                                                  std::move(notification)));
      break;
    default:
      NOTREACHED();
  }
}

void ChangeQueue::RemoveNotification(const std::string& id, bool by_user) {
  // Pick up the previous change with the same ID to try optimizing the queue.
  auto iter =
      std::find_if(changes_.rbegin(), changes_.rend(), ChangeFinder(id));
  if (iter == changes_.rend()) {
    auto change = std::make_unique<Change>(CHANGE_TYPE_DELETE, id, nullptr);
    change->set_by_user(by_user);
    changes_.push_back(std::move(change));
    return;
  }

  Change* change = iter->get();
  switch (change->type()) {
    case CHANGE_TYPE_ADD:
      // ADD -> DELETE. Just removes both.
      changes_.erase(--(iter.base()));
      break;
    case CHANGE_TYPE_UPDATE:
      // UPDATE -> DELETE. Changes the previous UPDATE to DELETE.
      change->set_type(CHANGE_TYPE_DELETE);
      change->set_by_user(by_user);
      change->ReplaceNotification(nullptr);
      break;
    case CHANGE_TYPE_DELETE:
      // DELETE -> DELETE. Something is wrong. Combines them with overriding
      // the |by_user| flag.
      change->set_by_user(change->by_user() || by_user);
      break;
    default:
      NOTREACHED();
  }
}

Notification* ChangeQueue::GetLatestNotification(const std::string& id) const {
  auto iter = std::find_if(changes_.begin(), changes_.end(), ChangeFinder(id));
  if (iter == changes_.end())
    return nullptr;

  if ((*iter)->type() == CHANGE_TYPE_DELETE)
    return nullptr;

  return (*iter)->notification();
}

void ChangeQueue::ApplyChangeInternal(MessageCenterImpl* message_center,
                                      std::unique_ptr<Change> change) {
  switch (change->type()) {
    case CHANGE_TYPE_ADD:
      message_center->AddNotificationImmediately(change->PassNotification());
      break;
    case CHANGE_TYPE_UPDATE:
      message_center->UpdateNotificationImmediately(change->previous_id(),
                                                    change->PassNotification());
      break;
    case CHANGE_TYPE_DELETE:
      message_center->RemoveNotificationImmediately(change->previous_id(),
                                                    change->by_user());
      break;
    default:
      NOTREACHED();
  }
}

}  // namespace message_center
