// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TRY_CHROME_DIALOG_H_
#define CHROME_BROWSER_UI_VIEWS_TRY_CHROME_DIALOG_H_

#include <stddef.h>

#include "base/callback.h"
#include "base/macros.h"
#include "base/sequence_checker.h"
#include "chrome/installer/util/experiment_metrics.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class Widget;
}

// This class displays a modal dialog using the views system. The dialog asks
// the user to give Chrome another try. This class only handles the UI so the
// resulting actions are up to the caller.
//
// The layout is as follows:
//
//   +-----------------------------------------------+
//   | |icon| Header text.                       [x] |
//   |                                               |
//   |        Body text.                             |
//   |        [ Open Chrome ] [No Thanks]            |
//   +-----------------------------------------------+
//
// Some variants do not have body text, or only have one button.
class TryChromeDialog : public views::ButtonListener,
                        public views::WidgetObserver {
 public:
  // Receives a closure to run upon process singleton notification when the
  // modal dialog is open, or a null closure when the active dialog is
  // dismissed.
  using ActiveModalDialogListener = base::Callback<void(base::Closure)>;

  enum Result {
    NOT_NOW,                    // Don't launch chrome. Exit now.
    OPEN_CHROME_WELCOME,        // Launch Chrome to the standard Welcome page.
    OPEN_CHROME_WELCOME_WIN10,  // Launch Chrome to the Win10 Welcome page.
    OPEN_CHROME_DEFAULT,        // Launch Chrome to the default page.
    OPEN_CHROME_DEFER,          // Launch Chrome on account of a rendezvous,
                                // deferring to the caller's command line.
  };

  // Shows a modal dialog asking the user to give Chrome another try. See
  // above for the possible outcomes of the function.
  // |group| selects what strings to present and what controls are shown.
  // |listener| will be provided with a closure when the modal event loop is
  // |started and when it completes.
  // Note that the dialog has no parent and it will position itself in a lower
  // corner of the screen or near the Chrome taskbar button.
  // The dialog does not steal focus and does not have an entry in the taskbar.
  static Result Show(size_t group, ActiveModalDialogListener listener);

 private:
  class Delegate {
   public:
    // Called when the dialog is shown. |toast_location| indicates the dialog's
    // location.
    virtual void RecordLocationOnShow(
        installer::ExperimentMetrics::ToastLocation toast_location) {}

    // Called when the dialog is closed. |result| is the value to be returned to
    // the caller, while |state| is the state to be recorded for metrics.
    virtual void ReportMetricsOnClose(
        Result result,
        installer::ExperimentMetrics::State state) {}

   protected:
    virtual ~Delegate() {}
  };

  class MetricsUpdater;

  friend class TryChromeDialogTest;

  // Creates a Try Chrome toast dialog. |group| signifies an experiment group
  // which dictactes messaging text and presence of ui elements. |delegate|
  // must outlive the instance.
  TryChromeDialog(size_t group, Delegate* delegate);
  ~TryChromeDialog() override;

  // Starts the process of presenting the dialog by initiating an asychronous
  // search for Chrome's taskbar icon. The delegate's RecordLocationOnShow
  // method will be called once the dialog is shown.
  void ShowDialogAsync();

  // Receives the bounds of Chrome's taskbar icon in |icon_rect| and proceeds
  // to show the dialog to the user, calling the delegate's RecordLocationOnShow
  // method when complete.
  void OnTaskbarIconRect(const gfx::Rect& icon_rect);

  // Returns the bounding rectangle of a popup of size |size| centered "over"
  // |icon_rect|, taking into account the orientation of the taskbar. Returns an
  // empty rect if |icon_rect| is empty or in case of error.
  gfx::Rect ComputePopupBoundsOverTaskbarIcon(const gfx::Size& size,
                                              const gfx::Rect& icon_rect);

  // Returns the bounding rectangle of a popup of size |size| to be positioned
  // over the notification area of the screen, taking into account LTR vs RTL
  // text orientation.
  gfx::Rect ComputePopupBoundsOverNoficationArea(const gfx::Size& size);

  // Invoked upon notification from another process by way of the process
  // singleton. Closes the dialog, allowing Chrome startup to resume.
  void OnProcessNotification();

  // views::ButtonListener:
  // We have two buttons and according to what the user clicked we set |result_|
  // and we should always close and end the modal loop.
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // views::WidgetObserver:
  // Calls the delegate's ReportMetricsOnClose method.
  void OnWidgetDestroyed(views::Widget* widget) override;

  // Controls which experiment group to use for varying the layout and controls.
  const size_t group_;

  // The delegate to be used to handle show and close events.
  Delegate* const delegate_;

  // The pessimistic result that will prevent launching Chrome.
  Result result_ = NOT_NOW;

  // The pessimistic state indicating that the dialog was closed via some means
  // other than its intended UX.
  installer::ExperimentMetrics::State state_ =
      installer::ExperimentMetrics::kOtherClose;

  // Unowned; |popup_| owns itself.
  views::Widget* popup_ = nullptr;

  SEQUENCE_CHECKER(my_sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(TryChromeDialog);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TRY_CHROME_DIALOG_H_
