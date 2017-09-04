// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/input_method_event_router.h"

#include <algorithm>
#include <utility>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "chrome/browser/chromeos/extensions/input_method_api.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/input_method_private.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_system.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/events/platform/platform_event_source.h"

namespace input_method_private = extensions::api::input_method_private;
namespace OnChanged = extensions::api::input_method_private::OnChanged;
namespace OnTouchEvent = extensions::api::input_method_private::OnTouchEvent;

namespace chromeos {

ExtensionInputMethodEventRouter::ExtensionInputMethodEventRouter(
    content::BrowserContext* context)
    : context_(context) {
  input_method::InputMethodManager::Get()->AddObserver(this);
  ui::PlatformEventSource* event_source =
      ui::PlatformEventSource::GetInstance();
  if (event_source) {
    event_source->AddPlatformEventObserver(this);
  }
}

ExtensionInputMethodEventRouter::~ExtensionInputMethodEventRouter() {
  input_method::InputMethodManager::Get()->RemoveObserver(this);
  ui::PlatformEventSource* event_source =
      ui::PlatformEventSource::GetInstance();
  if (event_source)
    event_source->RemovePlatformEventObserver(this);
}

void ExtensionInputMethodEventRouter::InputMethodChanged(
    input_method::InputMethodManager* manager,
    Profile* profile,
    bool show_message) {
  // This should probably be CHECK, as delivering event to a wrong
  // profile means delivering it to a wrong extension instance.
  DCHECK(profile->IsSameProfile(Profile::FromBrowserContext(context_)));
  extensions::EventRouter* router = extensions::EventRouter::Get(context_);

  if (!router->HasEventListener(OnChanged::kEventName))
    return;

  std::unique_ptr<base::ListValue> args(new base::ListValue());
  args->AppendString(
      manager->GetActiveIMEState()->GetCurrentInputMethod().id());

  // The router will only send the event to extensions that are listening.
  auto event = base::MakeUnique<extensions::Event>(
      extensions::events::INPUT_METHOD_PRIVATE_ON_CHANGED,
      OnChanged::kEventName, std::move(args), context_);
  router->BroadcastEvent(std::move(event));
}

void ExtensionInputMethodEventRouter::DidProcessEvent(
    const ui::PlatformEvent& platform_event) {
  std::unique_ptr<ui::Event> ui_event(ui::EventFromNative(platform_event));
  if (!ui_event || !ui_event->IsMouseEvent())
    return;

  extensions::EventRouter* router = extensions::EventRouter::Get(context_);

  if (!router->HasEventListener(OnTouchEvent::kEventName))
    return;

  input_method_private::Touch touch_event;
  touch_event.x = static_cast<ui::LocatedEvent*>(ui_event.get())->x();
  touch_event.y = static_cast<ui::LocatedEvent*>(ui_event.get())->y();

  std::unique_ptr<base::ListValue> args = OnTouchEvent::Create(touch_event);
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::INPUT_METHOD_PRIVATE_ON_TOUCH_EVENT,
      OnTouchEvent::kEventName, std::move(args)));
  event->restrict_to_browser_context = context_;
  router->BroadcastEvent(std::move(event));
}

}  // namespace chromeos
