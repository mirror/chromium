// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_TEMPLATE_BUILDER_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_TEMPLATE_BUILDER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/string16.h"

class GURL;
class XmlWriter;

namespace base {
class Time;
}

namespace message_center {
struct ButtonInfo;
class Notification;
}

class NotificationImageRetainer;

// Builds XML-based notification templates for displaying a given notification
// in the Windows Action Center.
//
// https://docs.microsoft.com/en-us/windows/uwp/controls-and-patterns/tiles-and-notifications-adaptive-interactive-toasts
//
// libXml was preferred (over WinXml, which the samples tend to use) because it
// is used frequently in Chrome, is nicer to use and has already been vetted.
class NotificationTemplateBuilder {
 public:
  // Builds the notification template for the given |notification|. The given
  // |notification_id| will be available when the user interacts with the toast.
  static std::unique_ptr<NotificationTemplateBuilder> Build(
      const std::string& notification_id,
      NotificationImageRetainer* notification_image_retainer,
      const std::string& profile_id,
      const message_center::Notification& notification);

  ~NotificationTemplateBuilder();

  // Gets the XML template that was created by this builder.
  base::string16 GetNotificationTemplate() const;

 private:
  // The different types of text nodes to output.
  enum class TextType { NORMAL, ATTRIBUTION };

  NotificationTemplateBuilder(
      NotificationImageRetainer* notification_image_retainer,
      const std::string& profile_id);

  // Formats the |origin| for display in the notification template.
  std::string FormatOrigin(const GURL& origin) const;

  // Writes the <toast> element with the |notification_id| as the launch string.
  // Also closes the |xml_writer_| for writing as the toast is now complete.
  void StartToastElement(const std::string& notification_id,
                         const base::Time& timastamp);
  void EndToastElement();

  // Writes the <visual> element.
  void StartVisualElement();
  void EndVisualElement();

  // Writes the <binding> element with the given |template_name|.
  void StartBindingElement(const std::string& template_name);
  void EndBindingElement();

  // Writes the <text> element with the given |content|. If |text_type| is
  // ATTRIBUTION then |content| is treated as the source that the notification
  // is attributed to.
  void WriteTextElement(const std::string& content, TextType text_type);
  // Writes the <image> element for the notification icon.
  void WriteIconElement(const message_center::Notification& notification);
  // Writes the <image> element for showing an image within the notification
  // body.
  void WriteImageElement(const message_center::Notification& notification);

  // Writes the <actions> element.
  void StartActionsElement();
  void EndActionsElement();

  // Writes the <audio silent="true"> element.
  void WriteAudioSilentElement();

  // Fills in the details for the actions (the buttons the notification
  // contains).
  void AddActions(const message_center::Notification& notification);
  void WriteActionElement(const message_center::ButtonInfo& button,
                          int index,
                          const GURL& origin);

  // The XML writer to which the template will be written.
  std::unique_ptr<XmlWriter> xml_writer_;

  // The image retainer. Weak, not owned by us.
  NotificationImageRetainer* image_retainer_;

  // The id of the profile the notification is intended for.
  std::string profile_id_;

  DISALLOW_COPY_AND_ASSIGN(NotificationTemplateBuilder);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_TEMPLATE_BUILDER_H_
