// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/try_chrome_dialog.h"

#include <shellapi.h>

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "cc/paint/paint_flags.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/browser/win/taskbar_icon_finder.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "chrome/installer/util/experiment.h"
#include "chrome/installer/util/experiment_storage.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/compositor/dip_util.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/gfx/win/singleton_hwnd_observer.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace {

std::ostream& operator<<(std::ostream& out, const gfx::Vector2d& distance) {
  return out << "(" << distance.x() << ", " << distance.y() << ")";
}

std::ostream& operator<<(std::ostream& out, const gfx::Vector2dF& distance) {
  return out << "(" << distance.x() << ", " << distance.y() << ")";
}

std::ostream& operator<<(std::ostream& out, const gfx::Point& point) {
  return out << "(" << point.x() << ", " << point.y() << ")";
}

std::ostream& operator<<(std::ostream& out, const gfx::PointF& point) {
  return out << "(" << point.x() << ", " << point.y() << ")";
}

std::ostream& operator<<(std::ostream& out, const gfx::Size& size) {
  return out << "(" << size.width() << ", " << size.height() << ")";
}

std::ostream& operator<<(std::ostream& out, const gfx::Rect& rect) {
  return out << "(" << rect.x() << ", " << rect.y() << ", " << rect.width()
             << ", " << rect.height() << ")";
}

std::ostream& operator<<(std::ostream& out, const gfx::RectF& rect) {
  return out << "(" << rect.x() << ", " << rect.y() << ", " << rect.width()
             << ", " << rect.height() << ")";
}

constexpr unsigned int kToastWidth = 360;
constexpr int kHoverAboveNotificationHeight = 24;
constexpr int kOffsetFromTaskbar = 1;
constexpr int kArrowHeight = 14;
constexpr int kArrowWidth = 24;

// The number of points the caret icon is inset from the outer edge of the
// dialog's border.
constexpr int kArrowInset = 3;

const SkColor kBackgroundColor = SkColorSetRGB(0x1F, 0x1F, 0x1F);
const SkColor kHeaderColor = SkColorSetRGB(0xFF, 0xFF, 0xFF);
const SkColor kBodyColor = SkColorSetARGB(0xAD, 0xFF, 0xFF, 0xFF);
const SkColor kBorderColor = SkColorSetARGB(0x80, 0x80, 0x80, 0x80);
const SkColor kButtonTextColor = SkColorSetRGB(0xFF, 0xFF, 0xFF);
const SkColor kButtonAcceptColor = SkColorSetRGB(0x00, 0x78, 0xDA);
const SkColor kButtonNoThanksColor = SkColorSetARGB(0x33, 0xFF, 0xFF, 0xFF);

enum class ButtonTag { CLOSE_BUTTON, OK_BUTTON, NO_THANKS_BUTTON };

// Experiment specification information needed for layout.
struct ExperimentVariations {
  enum class CloseStyle {
    kNoThanksButton,
    kCloseX,
    kNoThanksButtonAndCloseX,
  };

  // Resource ID for header message string.
  int heading_id;
  // Resource ID for body message string, or 0 for no body text.
  int body_id;
  // Set of dismissal controls.
  CloseStyle close_style;
  // Which action to take on acceptance of the dialog.
  TryChromeDialog::Result result;
};

constexpr ExperimentVariations kExperiments[] = {
    {IDS_WIN10_TOAST_RECOMMENDATION, 0,
     ExperimentVariations::CloseStyle::kCloseX,
     TryChromeDialog::OPEN_CHROME_DEFAULT},
    {IDS_WIN10_TOAST_RECOMMENDATION, 0,
     ExperimentVariations::CloseStyle::kNoThanksButtonAndCloseX,
     TryChromeDialog::OPEN_CHROME_DEFAULT},
    {IDS_WIN10_TOAST_RECOMMENDATION, 0,
     ExperimentVariations::CloseStyle::kNoThanksButton,
     TryChromeDialog::OPEN_CHROME_DEFAULT},
    {IDS_WIN10_TOAST_RECOMMENDATION, 0,
     ExperimentVariations::CloseStyle::kCloseX,
     TryChromeDialog::OPEN_CHROME_WELCOME_WIN10},
    {IDS_WIN10_TOAST_RECOMMENDATION, 0,
     ExperimentVariations::CloseStyle::kCloseX,
     TryChromeDialog::OPEN_CHROME_WELCOME},
    {IDS_WIN10_TOAST_RECOMMENDATION, IDS_WIN10_TOAST_SWITCH_FAST,
     ExperimentVariations::CloseStyle::kCloseX,
     TryChromeDialog::OPEN_CHROME_DEFAULT},
    {IDS_WIN10_TOAST_RECOMMENDATION, IDS_WIN10_TOAST_SWITCH_SECURE,
     ExperimentVariations::CloseStyle::kCloseX,
     TryChromeDialog::OPEN_CHROME_DEFAULT},
    {IDS_WIN10_TOAST_RECOMMENDATION, IDS_WIN10_TOAST_SWITCH_SMART,
     ExperimentVariations::CloseStyle::kCloseX,
     TryChromeDialog::OPEN_CHROME_DEFAULT},
    {IDS_WIN10_TOAST_SWITCH_FAST, IDS_WIN10_TOAST_RECOMMENDATION,
     ExperimentVariations::CloseStyle::kCloseX,
     TryChromeDialog::OPEN_CHROME_DEFAULT},
    {IDS_WIN10_TOAST_SWITCH_SECURE, IDS_WIN10_TOAST_RECOMMENDATION,
     ExperimentVariations::CloseStyle::kCloseX,
     TryChromeDialog::OPEN_CHROME_DEFAULT},
    {IDS_WIN10_TOAST_SWITCH_SMART, IDS_WIN10_TOAST_RECOMMENDATION,
     ExperimentVariations::CloseStyle::kCloseX,
     TryChromeDialog::OPEN_CHROME_DEFAULT},
    {IDS_WIN10_TOAST_BROWSE_FAST, 0, ExperimentVariations::CloseStyle::kCloseX,
     TryChromeDialog::OPEN_CHROME_DEFAULT},
    {IDS_WIN10_TOAST_BROWSE_SAFELY, 0,
     ExperimentVariations::CloseStyle::kCloseX,
     TryChromeDialog::OPEN_CHROME_DEFAULT},
    {IDS_WIN10_TOAST_BROWSE_SMART, 0, ExperimentVariations::CloseStyle::kCloseX,
     TryChromeDialog::OPEN_CHROME_DEFAULT},
    {IDS_WIN10_TOAST_SWITCH_SMART_AND_SECURE, IDS_WIN10_TOAST_RECOMMENDATION,
     ExperimentVariations::CloseStyle::kNoThanksButtonAndCloseX,
     TryChromeDialog::OPEN_CHROME_DEFAULT},
    {IDS_WIN10_TOAST_SWITCH_SMART_AND_SECURE, IDS_WIN10_TOAST_RECOMMENDATION,
     ExperimentVariations::CloseStyle::kNoThanksButton,
     TryChromeDialog::OPEN_CHROME_DEFAULT}};

// Whether a button is an accept or cancel-style button.
enum class TryChromeButtonType { OPEN_CHROME, NO_THANKS };

// Builds a Win10-styled rectangular button, for this toast displayed outside of
// the browser.
std::unique_ptr<views::LabelButton> CreateWin10StyleButton(
    views::ButtonListener* listener,
    const base::string16& text,
    TryChromeButtonType button_type) {
  auto button = base::MakeUnique<views::LabelButton>(listener, text,
                                                     CONTEXT_WINDOWS10_NATIVE);
  button->SetHorizontalAlignment(gfx::ALIGN_CENTER);

  button->SetBackground(views::CreateSolidBackground(
      button_type == TryChromeButtonType::OPEN_CHROME ? kButtonAcceptColor
                                                      : kButtonNoThanksColor));
  button->SetEnabledTextColors(kButtonTextColor);
  // Request specific 32pt height, 160+pt width.
  button->SetMinSize(gfx::Size(166, 32));
  button->SetMaxSize(gfx::Size(166, 32));
  return button;
}

// A View that unconditionally reports that it handles mouse presses. This
// results in the widget capturing the mouse so that it receives a
// ET_MOUSE_CAPTURE_CHANGED event upon button release following a drag out of
// the background of the widget.
class ClickableView : public views::View {
 public:
  ClickableView() = default;

  // views::View:
  bool OnMousePressed(const ui::MouseEvent& event) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ClickableView);
};

bool ClickableView::OnMousePressed(const ui::MouseEvent& event) {
  return true;
}

}  // namespace

// A
class TryChromeDialog::Context {
 public:
  Context();
  const gfx::Rect& display_work_area() const {
    return primary_display_.work_area();
  }
  void SetTaskbarIconBounds(const gfx::Rect& bounds);
  const gfx::Rect& taskbar_icon_rect() const { return taskbar_icon_bounds_; }
  installer::ExperimentMetrics::ToastLocation GetToastLocation() const;
  gfx::Rect ComputePopupBounds(views::Widget* popup, const gfx::Size& size);
  std::unique_ptr<views::Border> MakeBorder() const;

 private:
  class DialogCalculator {
   public:
    virtual ~DialogCalculator() {}
    installer::ExperimentMetrics::ToastLocation toast_location() const {
      return toast_location_;
    }
    virtual std::unique_ptr<views::Border> MakeBorder() = 0;
    virtual gfx::Rect ComputeBounds(const Context& context,
                                    views::Widget* popup,
                                    const gfx::Size& size) = 0;

   protected:
    explicit DialogCalculator(
        installer::ExperimentMetrics::ToastLocation toast_location)
        : toast_location_(toast_location) {}

   private:
    const installer::ExperimentMetrics::ToastLocation toast_location_;
    DISALLOW_COPY_AND_ASSIGN(DialogCalculator);
  };

  class NotificationAreaCalculator : public DialogCalculator {
   public:
    NotificationAreaCalculator()
        : DialogCalculator(
              installer::ExperimentMetrics::kOverNotificationArea) {}
    std::unique_ptr<views::Border> MakeBorder() override;
    gfx::Rect ComputeBounds(const Context& context,
                            views::Widget* popup,
                            const gfx::Size& size) override;

   private:
    DISALLOW_COPY_AND_ASSIGN(NotificationAreaCalculator);
  };

  class TaskbarCalculator : public DialogCalculator {
   public:
    gfx::Rect ComputeBounds(const Context& context,
                            views::Widget* popup,
                            const gfx::Size& size) override;

    // Returns the bounds of the caret in DIP relative to the popup.
    const gfx::Rect& caret_bounds() const { return caret_bounds_; }

   protected:
    class ArrowBorder : public views::Border {
     public:
      enum class RotationAmount { kNone, k90Degrees, k180Degrees, k270Degrees };
      ArrowBorder(TaskbarCalculator* calculator,
                  gfx::Insets insets,
                  RotationAmount rotation_amount)
          : calculator_(calculator),
            insets_(insets),
            caret_(gfx::CreateVectorIcon(kInactiveToastCaretIcon,
                                         kBackgroundColor)) {
        switch (rotation_amount) {
          case RotationAmount::kNone:
            break;
          case RotationAmount::k90Degrees:
            caret_ = gfx::ImageSkiaOperations::CreateRotatedImage(
                caret_, SkBitmapOperations::ROTATION_90_CW);
            break;
          case RotationAmount::k180Degrees:
            caret_ = gfx::ImageSkiaOperations::CreateRotatedImage(
                caret_, SkBitmapOperations::ROTATION_180_CW);
            break;
          case RotationAmount::k270Degrees:
            caret_ = gfx::ImageSkiaOperations::CreateRotatedImage(
                caret_, SkBitmapOperations::ROTATION_270_CW);
            break;
        }
      }

     private:
      // views::Border:
      void Paint(const views::View& view, gfx::Canvas* canvas) override;
      gfx::Insets GetInsets() const override;
      gfx::Size GetMinimumSize() const override;

      TaskbarCalculator* const calculator_;
      const gfx::Insets insets_;
      gfx::ImageSkia caret_;
      DISALLOW_COPY_AND_ASSIGN(ArrowBorder);
    };

    explicit TaskbarCalculator(const gfx::Insets& caret_inset,
                               const gfx::Size& caret_size,
                               float x_offset_scale,
                               float y_offset_scale)
        : DialogCalculator(installer::ExperimentMetrics::kOverTaskbarPin),
          caret_inset_(caret_inset),
          caret_size_(caret_size),
          x_offset_scale_(x_offset_scale),
          y_offset_scale_(y_offset_scale) {}

   private:
    virtual void MakeWindowRegionPolygon(const gfx::Size& dialog_size,
                                         const gfx::Rect& caret_bounds,
                                         POINT* polygon) = 0;

    const gfx::Insets caret_inset_;
    const gfx::Size caret_size_;
    const float x_offset_scale_;
    const float y_offset_scale_;

    // The bounding rectangle of the caret in DIP relative to the origin of the
    // popup. This rectangle extends kArrowInset within the contents of the
    // popup (one point of which is the popup's border).
    gfx::Rect caret_bounds_;

    DISALLOW_COPY_AND_ASSIGN(TaskbarCalculator);
  };

  class TopTaskbarCalculator : public TaskbarCalculator {
   public:
    TopTaskbarCalculator()
        : TaskbarCalculator({0, 0, kArrowInset, 0},
                            {kArrowWidth, kArrowHeight},
                            0.0f,
                            1.0f) {}
    std::unique_ptr<views::Border> MakeBorder() override;

   private:
    void MakeWindowRegionPolygon(const gfx::Size& dialog_size,
                                 const gfx::Rect& caret_bounds,
                                 POINT* polygon) override;

    DISALLOW_COPY_AND_ASSIGN(TopTaskbarCalculator);
  };

  class LeftTaskbarCalculator : public TaskbarCalculator {
   public:
    LeftTaskbarCalculator()
        : TaskbarCalculator({0, 0, 0, kArrowInset},
                            {kArrowHeight, kArrowWidth},
                            1.0f,
                            0.0f) {}
    std::unique_ptr<views::Border> MakeBorder() override;

   private:
    void MakeWindowRegionPolygon(const gfx::Size& dialog_size,
                                 const gfx::Rect& caret_bounds,
                                 POINT* polygon) override;

    DISALLOW_COPY_AND_ASSIGN(LeftTaskbarCalculator);
  };

  class BottomTaskbarCalculator : public TaskbarCalculator {
   public:
    BottomTaskbarCalculator()
        : TaskbarCalculator({kArrowInset, 0, 0, 0},
                            {kArrowWidth, kArrowHeight},
                            0.0f,
                            -1.0f) {}
    std::unique_ptr<views::Border> MakeBorder() override;

   private:
    void MakeWindowRegionPolygon(const gfx::Size& dialog_size,
                                 const gfx::Rect& caret_bounds,
                                 POINT* polygon) override;

    DISALLOW_COPY_AND_ASSIGN(BottomTaskbarCalculator);
  };

  class RightTaskbarCalculator : public TaskbarCalculator {
   public:
    RightTaskbarCalculator()
        : TaskbarCalculator({0, kArrowInset, 0, 0},
                            {kArrowHeight, kArrowWidth},
                            -1.0f,
                            0.0f) {}
    std::unique_ptr<views::Border> MakeBorder() override;

   private:
    void MakeWindowRegionPolygon(const gfx::Size& dialog_size,
                                 const gfx::Rect& caret_bounds,
                                 POINT* polygon) override;

    DISALLOW_COPY_AND_ASSIGN(RightTaskbarCalculator);
  };

  enum class TaskbarLocation { UNKNOWN, TOP, LEFT, BOTTOM, RIGHT };

  static HWND FindTaskbarWindow();
  static gfx::Rect GetTaskbarRect(HWND taskbar_window);
  static TaskbarLocation FindTaskbarPosition(
      const display::Display& primary_display,
      const gfx::Rect& taskbar_rect);

  const display::Display primary_display_;
  const HWND taskbar_window_;
  const gfx::Rect taskbar_rect_;
  const TaskbarLocation taskbar_location_;
  gfx::Rect taskbar_icon_bounds_;
  std::unique_ptr<DialogCalculator> calculator_;
  DISALLOW_COPY_AND_ASSIGN(Context);
};

// TryChromeDialog::Context::Context -------------------------------------------

TryChromeDialog::Context::Context()
    : primary_display_(display::Screen::GetScreen()->GetPrimaryDisplay()),
      taskbar_window_(FindTaskbarWindow()),
      taskbar_rect_(GetTaskbarRect(taskbar_window_)),
      taskbar_location_(FindTaskbarPosition(primary_display_, taskbar_rect_)) {}

void TryChromeDialog::Context::SetTaskbarIconBounds(const gfx::Rect& bounds) {
  taskbar_icon_bounds_ = bounds;
  TaskbarLocation location = taskbar_location_;

  // Nothing more to do if the taskbar couldn't be found, the icon couldn't be
  // found, or the taskbar doesn't contain the icon (e.g., the icon is scrolled
  // out of view).
  if (bounds.IsEmpty() || (location != TaskbarLocation::UNKNOWN &&
                           !taskbar_rect_.Contains(bounds))) {
    location = TaskbarLocation::UNKNOWN;
  }

  switch (location) {
    case TaskbarLocation::TOP:
      calculator_ = std::make_unique<TopTaskbarCalculator>();
      break;
    case TaskbarLocation::LEFT:
      calculator_ = std::make_unique<LeftTaskbarCalculator>();
      break;
    case TaskbarLocation::BOTTOM:
      calculator_ = std::make_unique<BottomTaskbarCalculator>();
      break;
    case TaskbarLocation::RIGHT:
      calculator_ = std::make_unique<RightTaskbarCalculator>();
      break;
    case TaskbarLocation::UNKNOWN:
      calculator_ = std::make_unique<NotificationAreaCalculator>();
      break;
  }
}

installer::ExperimentMetrics::ToastLocation
TryChromeDialog::Context::GetToastLocation() const {
  return calculator_->toast_location();
}

// static
HWND TryChromeDialog::Context::FindTaskbarWindow() {
  return ::FindWindow(L"Shell_TrayWnd", nullptr);
}

// static
gfx::Rect TryChromeDialog::Context::GetTaskbarRect(HWND taskbar_window) {
  RECT temp_rect = {};
  if (!taskbar_window || !::GetWindowRect(taskbar_window, &temp_rect))
    return gfx::Rect();
  return display::win::ScreenWin::ScreenToDIPRect(taskbar_window,
                                                  gfx::Rect(temp_rect));
}

// static
TryChromeDialog::Context::TaskbarLocation
TryChromeDialog::Context::FindTaskbarPosition(
    const display::Display& primary_display,
    const gfx::Rect& taskbar_rect) {
  if (taskbar_rect.IsEmpty())
    return TaskbarLocation::UNKNOWN;

  // The taskbar is always on the primary display.
  const gfx::Rect& monitor_rect = primary_display.bounds();

  // Is the taskbar not on the primary display (e.g., is it hidden)?
  if (!monitor_rect.Contains(taskbar_rect))
    return TaskbarLocation::UNKNOWN;

  // Where is the taskbar? Assume that it's "wider" than it is "tall".
  if (taskbar_rect.width() > taskbar_rect.height()) {
    // Horizonal.
    if (taskbar_rect.y() >= monitor_rect.y() + monitor_rect.height() / 2)
      return TaskbarLocation::BOTTOM;
    return TaskbarLocation::TOP;
  }
  // Vertical.
  if (taskbar_rect.x() < monitor_rect.x() + monitor_rect.width() / 2)
    return TaskbarLocation::LEFT;
  return TaskbarLocation::RIGHT;
}

gfx::Rect TryChromeDialog::Context::ComputePopupBounds(views::Widget* popup,
                                                       const gfx::Size& size) {
  return calculator_->ComputeBounds(*this, popup, size);
}

std::unique_ptr<views::Border> TryChromeDialog::Context::MakeBorder() const {
  return calculator_->MakeBorder();
}

// TryChromeDialog::Context::NotificationAreaCalculator ------------------------

std::unique_ptr<views::Border>
TryChromeDialog::Context::NotificationAreaCalculator::MakeBorder() {
  return views::CreateSolidBorder(1, kBorderColor);
}

gfx::Rect TryChromeDialog::Context::NotificationAreaCalculator::ComputeBounds(
    const Context& context,
    views::Widget* popup,
    const gfx::Size& size) {
  const bool is_RTL = base::i18n::IsRTL();
  const gfx::Rect work_area = popup->GetWorkAreaBoundsInScreen();
  return gfx::Rect(
      is_RTL ? work_area.x() : work_area.right() - size.width(),
      work_area.bottom() - size.height() - kHoverAboveNotificationHeight,
      size.width(), size.height());
}

// TryChromeDialog::Context::TaskbarCalculator ---------------------------------

gfx::Rect TryChromeDialog::Context::TaskbarCalculator::ComputeBounds(
    const Context& context,
    views::Widget* popup,
    const gfx::Size& size) {
  // Center the popup over the icon.
  const gfx::RectF taskbar_icon_rect(context.taskbar_icon_rect());
  gfx::PointF popup_origin(taskbar_icon_rect.CenterPoint());
  popup_origin.Offset(size.width() / -2.0f, size.height() / -2.0f);

  LOG(ERROR) << "popup_origin before shift: " << popup_origin;

  // Move the popup by the orientation-specific offset from the center of the
  // taskbar icon. This will move it along one of the axes to push the popup up
  // (for a bottm-of-screen taskbar), right (for a left-of-screen taskbar), etc.
  popup_origin.Offset(
      ((taskbar_icon_rect.width() + size.width()) / 2.0f + kOffsetFromTaskbar) *
          x_offset_scale_,
      ((taskbar_icon_rect.height() + size.height()) / 2.0f +
       kOffsetFromTaskbar) *
          y_offset_scale_);

  const gfx::Rect desired_bounds(gfx::ToRoundedPoint(popup_origin), size);

  // Adjust the popup to fit in the work area to handle the case where the icon
  // is close to the edge.
  gfx::Rect result = desired_bounds;
  result.AdjustToFit(context.display_work_area());

  LOG(ERROR) << "desired_bounds: " << desired_bounds << " result: " << result;

  // Compute the true caret position based on the popup position and the icon
  // rect. Due to the adjustment above, the caret may not be centered on the
  // popup (but will always be centered on the taskbar icon).
  gfx::PointF caret_origin(taskbar_icon_rect.CenterPoint());
  caret_origin.Offset(caret_size_.width() / -2.0f,
                      caret_size_.height() / -2.0f);

  // See comment above about this orientation-specific shift.
  caret_origin.Offset(
      ((taskbar_icon_rect.width() + caret_size_.width()) / 2.0f +
       kOffsetFromTaskbar) *
          x_offset_scale_,
      ((taskbar_icon_rect.height() + caret_size_.height()) / 2.0f +
       kOffsetFromTaskbar) *
          y_offset_scale_);

  caret_bounds_ = gfx::Rect(gfx::ToRoundedPoint(caret_origin), caret_size_);

  LOG(ERROR) << "caret_bounds_ screen: " << caret_bounds_;

  // Finally, account for an off-axis shift from the work area adjustment above.
  gfx::Vector2d adjustment = result.origin() - desired_bounds.origin();
  if (x_offset_scale_)
    caret_bounds_.Offset(adjustment.x(), 0);
  else
    caret_bounds_.Offset(0, adjustment.y());

  // And get its position relative to the origin of the popup.
  caret_bounds_ -= result.OffsetFromOrigin();

  LOG(ERROR) << "caret_bounds_: " << caret_bounds_;

  // Compute and apply the region to shape the dialog around the caret. The
  // region must be in screen rather than DIP coordinates relative to the
  // content area of the window.
  const HWND hwnd = popup->GetNativeView()->GetHost()->GetAcceleratedWidget();
  const float dsf = display::win::ScreenWin::GetScaleFactorForHWND(hwnd);
  const gfx::Size dialog_size = gfx::ScaleToCeiledSize(result.size(), dsf);

  // Compute the bounding rectangle of the caret outside of the content area of
  // the popup.
  gfx::Rect scaled_caret = gfx::ScaleToEnclosingRect(caret_bounds_, dsf);
  scaled_caret.Inset(caret_inset_.Scale(dsf));

  LOG(ERROR) << "dialog_size: " << dialog_size
             << " scaled_caret: " << scaled_caret;

  POINT polygon[7];
  MakeWindowRegionPolygon(dialog_size, scaled_caret, &polygon[0]);
  HRGN region = ::CreatePolygonRgn(&polygon[0], arraysize(polygon), WINDING);
  ::SetWindowRgn(hwnd, region, FALSE);

  return result;
}

// TryChromeDialog::Context::TaskbarCalculator::ArrowBorder --------------------

void TryChromeDialog::Context::TaskbarCalculator::ArrowBorder::Paint(
    const views::View& view,
    gfx::Canvas* canvas) {
  // Undo DSF so that we can be sure to draw an integral number of pixels for
  // the border. Integral scale factors should be unaffected by this, but for
  // fractional scale factors this ensures sharp lines.
  gfx::RectF scaled_bounds;
  gfx::RectF content_bounds;
  {
    gfx::ScopedCanvas scoped(canvas);
    float dsf = canvas->UndoDeviceScaleFactor();

    DCHECK(!view.layer());
    scaled_bounds = gfx::RectF(view.GetLocalBounds());
    scaled_bounds.Scale(dsf);

    // Compute the bounds of the contents within the border (this scaling
    // operation floors the inset values).
    content_bounds = scaled_bounds;
    gfx::Insets scaled_insets = insets_.Scale(dsf);
    content_bounds.Inset(scaled_insets);

    // Subtract out the contents.
    canvas->sk_canvas()->clipRect(gfx::RectFToSkRect(content_bounds),
                                  SkClipOp::kDifference, true);
    // Subtract out the buffer area where the caret is.
    gfx::RectF buffer_bounds(
        scaled_bounds.x(),
        content_bounds.y() + content_bounds.height() + scaled_insets.top(),
        scaled_bounds.width(),
        scaled_bounds.height() - scaled_insets.top() - content_bounds.height() -
            scaled_insets.top());
    canvas->sk_canvas()->clipRect(gfx::RectFToSkRect(buffer_bounds),
                                  SkClipOp::kDifference, true);
    canvas->DrawColor(kBorderColor);
  }
  {
    gfx::ScopedCanvas scoped(canvas);
    float dsf = canvas->UndoDeviceScaleFactor();

    gfx::SizeF caret_size(caret_.size());
    caret_size.Scale(dsf);

    gfx::Rect caret_bounds = gfx::ToEnclosingRect(
        gfx::ScaleRect(gfx::RectF(calculator_->caret_bounds()), dsf));

    // The caret image is square with only the top 14 points (kArrowHeight)
    // containing the relevant drawing. When drawn "toward" the origin (i.e.,
    // for a top or left taskbar), the caret must be offset this extra amount.
    if (caret_bounds.origin().x() < content_bounds.origin().x())
      caret_bounds.Offset(caret_bounds.width() - caret_size.width(), 0);
    else if (caret_bounds.origin().y() < content_bounds.origin().y())
      caret_bounds.Offset(0, caret_bounds.height() - caret_size.height());

    canvas->DrawImageIntInPixel(caret_.GetRepresentation(dsf), caret_bounds.x(),
                                caret_bounds.y(), caret_size.width(),
                                caret_size.height(), false, cc::PaintFlags());
  }
}

gfx::Insets
TryChromeDialog::Context::TaskbarCalculator::ArrowBorder::GetInsets() const {
  return insets_;
}

gfx::Size
TryChromeDialog::Context::TaskbarCalculator::ArrowBorder::GetMinimumSize()
    const {
  return {insets_.width(), insets_.height()};
}

// TryChromeDialog::Context::TopTaskbarCalculator ------------------------------

std::unique_ptr<views::Border>
TryChromeDialog::Context::TopTaskbarCalculator::MakeBorder() {
  return std::make_unique<ArrowBorder>(
      this, gfx::Insets(kArrowHeight - kArrowInset + 1, 1, 1, 1),
      ArrowBorder::RotationAmount::k180Degrees);
}

void TryChromeDialog::Context::TopTaskbarCalculator::MakeWindowRegionPolygon(
    const gfx::Size& dialog_size,
    const gfx::Rect& caret_bounds,
    POINT* polygon) {
  polygon[0] = {0, caret_bounds.height()};
  polygon[1] = {caret_bounds.x() + 2, caret_bounds.height()};
  polygon[2] = {caret_bounds.x() + caret_bounds.width() / 2, 0};
  polygon[3] = {caret_bounds.x() + caret_bounds.width() - 2,
                caret_bounds.height()};
  polygon[4] = {dialog_size.width(), caret_bounds.height()};
  polygon[5] = {dialog_size.width(), dialog_size.height()};
  polygon[6] = {0, dialog_size.height()};
}

// TryChromeDialog::Context::LeftTaskbarCalculator -----------------------------

std::unique_ptr<views::Border>
TryChromeDialog::Context::LeftTaskbarCalculator::MakeBorder() {
  return std::make_unique<ArrowBorder>(
      this, gfx::Insets(1, kArrowHeight - kArrowInset + 1, 1, 1),
      ArrowBorder::RotationAmount::k90Degrees);
}

void TryChromeDialog::Context::LeftTaskbarCalculator::MakeWindowRegionPolygon(
    const gfx::Size& dialog_size,
    const gfx::Rect& caret_bounds,
    POINT* polygon) {
  polygon[0] = {caret_bounds.width(), 0};
  polygon[1] = {dialog_size.width(), 0};
  polygon[2] = {dialog_size.width(), dialog_size.height()};
  polygon[3] = {caret_bounds.width(), dialog_size.height()};
  polygon[4] = {caret_bounds.width(),
                caret_bounds.y() + caret_bounds.height() - 2};
  polygon[5] = {0, caret_bounds.y() + caret_bounds.height() / 2};
  polygon[6] = {caret_bounds.width(), caret_bounds.y() + 2};
}

// TryChromeDialog::Context::BottomTaskbarCalculator ---------------------------

std::unique_ptr<views::Border>
TryChromeDialog::Context::BottomTaskbarCalculator::MakeBorder() {
  return std::make_unique<ArrowBorder>(
      this, gfx::Insets(1, 1, kArrowHeight - kArrowInset + 1, 1),
      ArrowBorder::RotationAmount::kNone);
}

void TryChromeDialog::Context::BottomTaskbarCalculator::MakeWindowRegionPolygon(
    const gfx::Size& dialog_size,
    const gfx::Rect& caret_bounds,
    POINT* polygon) {
  polygon[0] = {0, 0};
  polygon[1] = {dialog_size.width(), 0};
  polygon[2] = {dialog_size.width(),
                dialog_size.height() - caret_bounds.height()};
  polygon[3] = {caret_bounds.x() + caret_bounds.width() - 2,
                dialog_size.height() - caret_bounds.height()};
  polygon[4] = {caret_bounds.x() + caret_bounds.width() / 2,
                dialog_size.height()};
  polygon[5] = {caret_bounds.x() + 2,
                dialog_size.height() - caret_bounds.height()};
  polygon[6] = {0, dialog_size.height() - caret_bounds.height()};
}

// TryChromeDialog::Context::RightTaskbarCalculator ----------------------------

std::unique_ptr<views::Border>
TryChromeDialog::Context::RightTaskbarCalculator::MakeBorder() {
  return std::make_unique<ArrowBorder>(
      this, gfx::Insets(1, 1, 1, kArrowHeight - kArrowInset + 1),
      ArrowBorder::RotationAmount::k270Degrees);
}

void TryChromeDialog::Context::RightTaskbarCalculator::MakeWindowRegionPolygon(
    const gfx::Size& dialog_size,
    const gfx::Rect& caret_bounds,
    POINT* polygon) {
  polygon[0] = {0, 0};
  polygon[1] = {dialog_size.width() - caret_bounds.width(), 0};
  polygon[2] = {dialog_size.width() - caret_bounds.width(),
                caret_bounds.y() + 2};
  polygon[3] = {dialog_size.width(),
                caret_bounds.y() + caret_bounds.height() / 2};
  polygon[4] = {dialog_size.width() - caret_bounds.width(),
                caret_bounds.y() + caret_bounds.height() - 2};
  polygon[5] = {dialog_size.width() - caret_bounds.width(),
                dialog_size.height()};
  polygon[6] = {0, dialog_size.height()};
}

// TryChromeDialog::ModalShowDelegate ------------------------------------------

// A delegate for use by the modal Show() function to update the experiment
// state in the Windows registry and break out of the modal run loop upon
// completion.
class TryChromeDialog::ModalShowDelegate : public TryChromeDialog::Delegate {
 public:
  // Constructs the updater with a closure to be run after the dialog is closed
  // to break out of the modal run loop.
  explicit ModalShowDelegate(base::Closure quit_closure)
      : quit_closure_(std::move(quit_closure)) {}
  ~ModalShowDelegate() override = default;

 protected:
  // TryChromeDialog::Delegate:
  void SetToastLocation(
      installer::ExperimentMetrics::ToastLocation toast_location) override;
  void SetExperimentState(installer::ExperimentMetrics::State state) override;
  void InteractionComplete() override;

 private:
  base::Closure quit_closure_;
  installer::ExperimentStorage storage_;

  // The time at which the toast was shown; used for computing the action delay.
  base::TimeTicks time_shown_;

  DISALLOW_COPY_AND_ASSIGN(ModalShowDelegate);
};

void TryChromeDialog::ModalShowDelegate::SetToastLocation(
    installer::ExperimentMetrics::ToastLocation toast_location) {
  time_shown_ = base::TimeTicks::Now();

  installer::Experiment experiment;
  auto lock = storage_.AcquireLock();
  if (lock->LoadExperiment(&experiment)) {
    experiment.SetDisplayTime(base::Time::Now());
    experiment.SetToastCount(experiment.toast_count() + 1);
    experiment.SetToastLocation(toast_location);
    // TODO(skare): SetUserSessionUptime
    lock->StoreExperiment(experiment);
  }
}

void TryChromeDialog::ModalShowDelegate::SetExperimentState(
    installer::ExperimentMetrics::State state) {
  installer::Experiment experiment;
  auto lock = storage_.AcquireLock();
  if (lock->LoadExperiment(&experiment)) {
    if (!time_shown_.is_null())
      experiment.SetActionDelay(base::TimeTicks::Now() - time_shown_);
    experiment.SetState(state);
    lock->StoreExperiment(experiment);
  }
}

void TryChromeDialog::ModalShowDelegate::InteractionComplete() {
  quit_closure_.Run();
}

// TryChromeDialog -------------------------------------------------------------

// static
TryChromeDialog::Result TryChromeDialog::Show(
    size_t group,
    ActiveModalDialogListener listener) {
  if (group >= arraysize(kExperiments)) {
    // Exit immediately given bogus values; see TryChromeDialogBrowserTest test.
    return NOT_NOW;
  }

  base::RunLoop run_loop;
  ModalShowDelegate delegate(run_loop.QuitWhenIdleClosure());
  TryChromeDialog dialog(group, &delegate);

  dialog.ShowDialogAsync();

  if (listener) {
    listener.Run(base::Bind(&TryChromeDialog::OnProcessNotification,
                            base::Unretained(&dialog)));
  }
  run_loop.Run();
  if (listener)
    listener.Run(base::Closure());

  return dialog.result();
}

TryChromeDialog::TryChromeDialog(size_t group, Delegate* delegate)
    : group_(group),
      delegate_(delegate),
      context_(std::make_unique<Context>()) {
  DCHECK_LT(group, arraysize(kExperiments));
  DCHECK(delegate);
}

TryChromeDialog::~TryChromeDialog() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(my_sequence_checker_);
}

void TryChromeDialog::ShowDialogAsync() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(my_sequence_checker_);

  endsession_observer_ = std::make_unique<gfx::SingletonHwndObserver>(
      base::Bind(&TryChromeDialog::OnWindowMessage, base::Unretained(this)));

  // Get the bounding rectangle of Chrome's taskbar icon on the primary monitor.
  FindTaskbarIcon(base::BindOnce(&TryChromeDialog::OnTaskbarIconRect,
                                 base::Unretained(this)));
}

void TryChromeDialog::OnTaskbarIconRect(const gfx::Rect& icon_rect) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(my_sequence_checker_);

  // It's possible that a rendezvous from another browser process arrived while
  // searching for the taskbar icon (see OnProcessNotification). In this case,
  // report the DEFER result and bail out immediately.
  if (result_ == OPEN_CHROME_DEFER) {
    DCHECK_EQ(state_, installer::ExperimentMetrics::kOtherLaunch);
    CompleteInteraction();
    return;
  }

  // It's also possible that a WM_ENDSESSION arrived while searching (see
  // OnWindowMessage). In this case, continue processing since it's possible
  // that the logoff was cancelled. The toast may as well be shown.

  context_->SetTaskbarIconBounds(icon_rect);

  // Create the popup.
  auto icon = base::MakeUnique<views::ImageView>();
  icon->SetImage(gfx::CreateVectorIcon(kInactiveToastLogoIcon, kHeaderColor));
  gfx::Size icon_size = icon->GetPreferredSize();

  views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
  params.activatable = views::Widget::InitParams::ACTIVATABLE_YES;
  // An approximate window size. Layout() can adjust.
  params.bounds = gfx::Rect(kToastWidth, 120);
  popup_ = new views::Widget;
  popup_->AddObserver(this);
  popup_->Init(params);

  auto contents_view = std::make_unique<ClickableView>();
  contents_view->SetBackground(views::CreateSolidBackground(kBackgroundColor));
  views::GridLayout* layout =
      views::GridLayout::CreateAndInstall(contents_view.get());
  layout->set_minimum_size(gfx::Size(kToastWidth, 0));
  views::ColumnSet* columns;

  // Note the right padding is smaller than other dimensions,
  // to acommodate the close 'x' button.
  //  contents_view->SetBorder(MakeBorder(position));
  contents_view->SetBorder(context_->MakeBorder());

  static constexpr int kLabelSpacing = 10;
  static constexpr int kSpaceBetweenButtons = 4;
  static constexpr int kSpacingAfterHeadingHorizontal = 9;
  static constexpr int kCloseButtonRightPadding = 4;

  // 9pt between the border and the first row.
  layout->AddPaddingRow(0, 9);

  // First row: [pad][icon][pad][text][pad][close button].
  columns = layout->AddColumnSet(0);
  columns->AddPaddingColumn(0, 9);
  columns->AddColumn(views::GridLayout::LEADING, views::GridLayout::LEADING, 0,
                     views::GridLayout::FIXED, icon_size.width(),
                     icon_size.height());
  columns->AddPaddingColumn(0, kLabelSpacing);
  columns->AddColumn(views::GridLayout::FILL, views::GridLayout::LEADING, 1,
                     views::GridLayout::USE_PREF, 0, 0);
  columns->AddPaddingColumn(0, kSpacingAfterHeadingHorizontal);
  columns->AddColumn(views::GridLayout::TRAILING, views::GridLayout::FILL, 0,
                     views::GridLayout::USE_PREF, 0, 0);
  columns->AddPaddingColumn(0, kCloseButtonRightPadding);
  int icon_padding = icon_size.width() + kLabelSpacing;

  // Optional second row: [pad][text].
  columns = layout->AddColumnSet(1);
  columns->AddPaddingColumn(0, icon_padding + 9);
  columns->AddColumn(views::GridLayout::LEADING, views::GridLayout::FILL, 1,
                     views::GridLayout::USE_PREF, 0, 0);

  // Third row: [pad][optional button][pad][button].
  columns = layout->AddColumnSet(2);
  columns->AddPaddingColumn(0, 11);
  columns->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 1,
                     views::GridLayout::USE_PREF, 0, 0);
  columns->AddPaddingColumn(0, kSpaceBetweenButtons);
  columns->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 0,
                     views::GridLayout::USE_PREF, 0, 0);
  columns->AddPaddingColumn(0, 11);

  // Padding between the top of the toast and first row is handled via border.
  // First row.
  layout->StartRow(0, 0);
  layout->AddView(icon.release());
  // All variants have a main header.
  auto header = base::MakeUnique<views::Label>(
      l10n_util::GetStringUTF16(kExperiments[group_].heading_id),
      CONTEXT_WINDOWS10_NATIVE);
  header->SetBackgroundColor(kBackgroundColor);
  header->SetEnabledColor(kHeaderColor);
  header->SetMultiLine(true);
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  layout->AddView(header.release());

  // Close button if included in the variant.
  if (kExperiments[group_].close_style ==
          ExperimentVariations::CloseStyle::kCloseX ||
      kExperiments[group_].close_style ==
          ExperimentVariations::CloseStyle::kNoThanksButtonAndCloseX) {
    auto close_button = base::MakeUnique<views::ImageButton>(this);
    close_button->SetImage(
        views::Button::STATE_NORMAL,
        gfx::CreateVectorIcon(kInactiveToastCloseIcon, kBodyColor));
    close_button->set_tag(static_cast<int>(ButtonTag::CLOSE_BUTTON));
    close_button_ = close_button.get();
    layout->AddView(close_button.release());
    close_button_->SetVisible(false);
  } else {
    layout->SkipColumns(1);
  }

  // Second row: May have text or may be blank.
  layout->StartRow(0, 1);
  const int body_string_id = kExperiments[group_].body_id;
  if (body_string_id) {
    auto body_text = base::MakeUnique<views::Label>(
        l10n_util::GetStringUTF16(body_string_id), CONTEXT_WINDOWS10_NATIVE);
    body_text->SetBackgroundColor(kBackgroundColor);
    body_text->SetEnabledColor(kBodyColor);
    layout->AddView(body_text.release());
  }

  // Third row: one or two buttons depending on group.
  layout->StartRowWithPadding(0, 2, 0, 12);
  bool has_no_thanks_button =
      kExperiments[group_].close_style ==
          ExperimentVariations::CloseStyle::kNoThanksButton ||
      kExperiments[group_].close_style ==
          ExperimentVariations::CloseStyle::kNoThanksButtonAndCloseX;
  if (!has_no_thanks_button)
    layout->SkipColumns(1);
  auto accept_button = CreateWin10StyleButton(
      this, l10n_util::GetStringUTF16(IDS_WIN10_TOAST_OPEN_CHROME),
      TryChromeButtonType::OPEN_CHROME);
  accept_button->set_tag(static_cast<int>(ButtonTag::OK_BUTTON));
  layout->AddView(accept_button.release());

  if (has_no_thanks_button) {
    auto no_thanks_button = CreateWin10StyleButton(
        this, l10n_util::GetStringUTF16(IDS_WIN10_TOAST_NO_THANKS),
        TryChromeButtonType::NO_THANKS);
    no_thanks_button->set_tag(static_cast<int>(ButtonTag::NO_THANKS_BUTTON));
    layout->AddView(no_thanks_button.release());
  }

  layout->AddPaddingRow(0, 11);

  // Padding between buttons and the edge of the view is via the border.
  gfx::Size preferred = layout->GetPreferredSize(contents_view.get());

  popup_->SetBounds(context_->ComputePopupBounds(popup_, preferred));
  popup_->SetContentsView(contents_view.release());

  popup_->Show();
  delegate_->SetToastLocation(context_->GetToastLocation());
}

void TryChromeDialog::CompleteInteraction() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(my_sequence_checker_);
  endsession_observer_.reset();
  delegate_->SetExperimentState(state_);
  delegate_->InteractionComplete();
}

void TryChromeDialog::OnProcessNotification() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(my_sequence_checker_);

  // Another browser process is trying to rendezvous with this one, which is
  // either waiting on FindTaskbarIcon to complete or waiting on the user to
  // interact with the dialog. In the former case, no attempt is made to stop
  // the search, as it is expected to complete "quickly". When it does complete
  // (in OnTaskbarIconRect), processing will complete tout de suite. In the
  // latter case, the dialog is closed so that processing will continue in
  // OnWidgetDestroyed. OPEN_CHROME_DEFER conveys to this browser process that
  // it should ignore its own command line and instead handle that provided by
  // the other browser process.
  result_ = OPEN_CHROME_DEFER;
  state_ = installer::ExperimentMetrics::kOtherLaunch;
  if (popup_)
    popup_->Close();
}

void TryChromeDialog::OnWindowMessage(HWND window,
                                      UINT message,
                                      WPARAM wparam,
                                      LPARAM lparam) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(my_sequence_checker_);

  // wparam == FALSE means the system is not shutting down.
  if (message != WM_ENDSESSION || wparam == FALSE)
    return;

  // The ship is going down. Record the endsession event, but don't bother
  // trying to close the window or take any other steps to shut down -- the OS
  // will tear everything down soon enough. It's always possible that the
  // endsession is aborted, in which case the dialog may as well stay onscreen.
  result_ = NOT_NOW;
  state_ = installer::ExperimentMetrics::kUserLogOff;
  delegate_->SetExperimentState(state_);
}

void TryChromeDialog::GainedMouseHover() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(my_sequence_checker_);
  if (close_button_)
    close_button_->SetVisible(true);
}

void TryChromeDialog::LostMouseHover() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(my_sequence_checker_);
  if (close_button_)
    close_button_->SetVisible(false);
}

void TryChromeDialog::ButtonPressed(views::Button* sender,
                                    const ui::Event& event) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(my_sequence_checker_);
  DCHECK_EQ(result_, NOT_NOW);

  // Ignore this press if another press or a rendezvous has already been
  // registered.
  if (state_ != installer::ExperimentMetrics::kOtherClose)
    return;

  // Figure out what the subsequent action and experiment state should be based
  // on which button was pressed.
  switch (sender->tag()) {
    case static_cast<int>(ButtonTag::CLOSE_BUTTON):
      state_ = installer::ExperimentMetrics::kSelectedClose;
      break;
    case static_cast<int>(ButtonTag::OK_BUTTON):
      result_ = kExperiments[group_].result;
      state_ = installer::ExperimentMetrics::kSelectedOpenChromeAndNoCrash;
      break;
    case static_cast<int>(ButtonTag::NO_THANKS_BUTTON):
      state_ = installer::ExperimentMetrics::kSelectedNoThanks;
      break;
    default:
      NOTREACHED();
      break;
  }

  popup_->Close();
}

void TryChromeDialog::OnWidgetClosing(views::Widget* widget) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(my_sequence_checker_);
  DCHECK_EQ(widget, popup_);

  popup_->GetNativeWindow()->RemovePreTargetHandler(this);
}

void TryChromeDialog::OnWidgetCreated(views::Widget* widget) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(my_sequence_checker_);
  DCHECK_EQ(widget, popup_);

  popup_->GetNativeWindow()->AddPreTargetHandler(this);
}

void TryChromeDialog::OnWidgetDestroyed(views::Widget* widget) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(my_sequence_checker_);
  DCHECK_EQ(widget, popup_);

  popup_->RemoveObserver(this);
  popup_ = nullptr;
  close_button_ = nullptr;

  CompleteInteraction();
}

void TryChromeDialog::OnMouseEvent(ui::MouseEvent* event) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(my_sequence_checker_);
  DCHECK(popup_);

  switch (event->type()) {
    // A MOUSE_ENTERED event is received if the mouse is over the dialog when it
    // opens.
    case ui::ET_MOUSE_ENTERED:
    case ui::ET_MOUSE_MOVED:
      if (!has_hover_) {
        has_hover_ = true;
        GainedMouseHover();
      }
      break;
    case ui::ET_MOUSE_EXITED:
      if (has_hover_) {
        has_hover_ = false;
        LostMouseHover();
      }
      break;
    case ui::ET_MOUSE_CAPTURE_CHANGED:
      if (has_hover_ && !display::Screen::GetScreen()->IsWindowUnderCursor(
                            popup_->GetNativeWindow())) {
        has_hover_ = false;
        LostMouseHover();
      }
      break;
    default:
      break;
  }
}
