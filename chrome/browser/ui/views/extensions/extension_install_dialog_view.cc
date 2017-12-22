// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/extensions/extension_install_dialog_view.h"

#include <string>

#include "base/callback_helpers.h"
#include "base/i18n/message_formatter.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/api/experience_sampling_private/experience_sampling.h"
#include "chrome/browser/extensions/extension_install_prompt_show_params.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/scoped_tabbed_browser_displayer.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_urls.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"

using content::OpenURLParams;
using content::Referrer;
using extensions::ExperienceSamplingEvent;

namespace {

// Width of the bullet column in BulletedView.
constexpr int kBulletWidth = 20;

// Size of extension icon in top left of dialog.
constexpr int kIconSize = 32;

// Time delay before the install button is enabled after initial display.
int g_install_delay_in_ms = 500;

// A custom view to contain the ratings information (stars, ratings count, etc).
// With screen readers, this will handle conveying the information properly
// (i.e., "Rated 4.2 stars by 379 reviews" rather than "image image...379").
class RatingsView : public views::View {
 public:
  RatingsView(double rating, int rating_count)
      : rating_(rating), rating_count_(rating_count) {
    set_id(ExtensionInstallDialogView::kRatingsViewId);
    SetLayoutManager(
        std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal));
  }
  ~RatingsView() override {}

  void GetAccessibleNodeData(ui::AXNodeData* node_data) override {
    node_data->role = ui::AX_ROLE_STATIC_TEXT;
    base::string16 accessible_text;
    if (rating_count_ == 0) {
      accessible_text = l10n_util::GetStringUTF16(
          IDS_EXTENSION_PROMPT_NO_RATINGS_ACCESSIBLE_TEXT);
    } else {
      accessible_text = base::i18n::MessageFormatter::FormatWithNumberedArgs(
          l10n_util::GetStringUTF16(
              IDS_EXTENSION_PROMPT_RATING_ACCESSIBLE_TEXT),
          rating_, rating_count_);
    }
    node_data->SetName(accessible_text);
  }

 private:
  double rating_;
  int rating_count_;

  DISALLOW_COPY_AND_ASSIGN(RatingsView);
};

// A custom view for the ratings star image that will be ignored by screen
// readers (since the RatingsView handles the context).
class RatingStar : public views::ImageView {
 public:
  explicit RatingStar(const gfx::ImageSkia& image) { SetImage(image); }
  ~RatingStar() override {}

  // views::ImageView:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override {
    node_data->role = ui::AX_ROLE_IGNORED;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(RatingStar);
};

// A custom view for the ratings label that will be ignored by screen readers
// (since the RatingsView handles the context).
class RatingLabel : public views::Label {
 public:
  RatingLabel(const base::string16& text, int text_context)
      : views::Label(text, text_context, views::style::STYLE_PRIMARY) {}
  ~RatingLabel() override {}

  // views::Label:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override {
    node_data->role = ui::AX_ROLE_IGNORED;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(RatingLabel);
};

// Get the appropriate indentation for an item if its parent is using bullet
// points. If the parent is using bullets for its items, then a padding of one
// unit will make the child item (which has no bullet) look like a sibling of
// its parent. Therefore increase the indentation by one more unit to show that
// it is in fact a child item (with no missing bullet) and not a sibling.
int GetLeftPaddingForBulletedItems(bool parent_bulleted) {
  return ChromeLayoutProvider::Get()->GetDistanceMetric(
             views::DISTANCE_RELATED_CONTROL_HORIZONTAL) *
         (parent_bulleted ? 2 : 1);
}

void AddResourceIcon(const gfx::ImageSkia* skia_image, void* data) {
  views::View* parent = static_cast<views::View*>(data);
  parent->AddChildView(new RatingStar(*skia_image));
}

// Creates a string for displaying |message| to the user. If it has to look
// like a entry in a bullet point list, one is added.
base::string16 PrepareForDisplay(const base::string16& message,
                                 bool bullet_point) {
  return bullet_point ? l10n_util::GetStringFUTF16(
                            IDS_EXTENSION_PERMISSION_LINE, message)
                      : message;
}

void ShowExtensionInstallDialogImpl(
    ExtensionInstallPromptShowParams* show_params,
    const ExtensionInstallPrompt::DoneCallback& done_callback,
    std::unique_ptr<ExtensionInstallPrompt::Prompt> prompt) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  bool use_tab_modal_dialog = prompt->ShouldUseTabModalDialog();
  ExtensionInstallDialogView* dialog = new ExtensionInstallDialogView(
      show_params->profile(), show_params->GetParentWebContents(),
      done_callback, std::move(prompt));
  if (use_tab_modal_dialog) {
    content::WebContents* parent_web_contents =
        show_params->GetParentWebContents();
    if (parent_web_contents)
      constrained_window::ShowWebModalDialogViews(dialog, parent_web_contents);
  } else {
    constrained_window::CreateBrowserModalDialogViews(
        dialog, show_params->GetParentWindow())
        ->Show();
  }
}

// A custom scrollable view implementation for the dialog.
class CustomScrollableView : public views::View {
 public:
  explicit CustomScrollableView(ExtensionInstallDialogView* parent)
      : parent_(parent) {}
  ~CustomScrollableView() override {}

  gfx::Size CalculatePreferredSize() const override {
    const int content_width = ChromeLayoutProvider::Get()->GetDistanceMetric(
                                  DISTANCE_MODAL_DIALOG_PREFERRED_WIDTH) -
                              GetInsets().width();
    return gfx::Size(content_width, GetHeightForWidth(content_width));
  }

  void ChildPreferredSizeChanged(views::View* child) override {
    PreferredSizeChanged();
    parent_->ResizeWidget();
  }

 private:
  ExtensionInstallDialogView* parent_;

  DISALLOW_COPY_AND_ASSIGN(CustomScrollableView);
};

}  // namespace

struct ExtensionInfoSection {
  base::string16 header;
  views::View* contents_view;
};

void ExtensionInstallDialogView::ResizeWidget() {
  GetWidget()->SetSize(GetWidget()->non_client_view()->GetPreferredSize());
}

BulletedView::BulletedView(views::View* view) {
  views::GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));
  views::ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(views::GridLayout::CENTER, views::GridLayout::LEADING,
                        0, views::GridLayout::FIXED, kBulletWidth, 0);
  column_set->AddColumn(views::GridLayout::LEADING, views::GridLayout::LEADING,
                        0, views::GridLayout::USE_PREF,
                        0,  // No fixed width.
                        0);
  layout->StartRow(0, 0);
  layout->AddView(new views::Label(PrepareForDisplay(base::string16(), true)));
  layout->AddView(view);
}

ExtensionInstallDialogView::ExtensionInstallDialogView(
    Profile* profile,
    content::PageNavigator* navigator,
    const ExtensionInstallPrompt::DoneCallback& done_callback,
    std::unique_ptr<ExtensionInstallPrompt::Prompt> prompt)
    : profile_(profile),
      navigator_(navigator),
      done_callback_(done_callback),
      prompt_(std::move(prompt)),
      scroll_view_(nullptr),
      handled_result_(false),
      install_button_enabled_(false) {
  CreateContents();

  DCHECK_GE(prompt_->type(), 0);
  UMA_HISTOGRAM_ENUMERATION("Extensions.InstallPrompt.Type", prompt_->type(),
                            ExtensionInstallPrompt::NUM_PROMPT_TYPES);

  std::string event_name = ExperienceSamplingEvent::kExtensionInstallDialog;
  event_name.append(
      ExtensionInstallPrompt::PromptTypeToString(prompt_->type()));
  sampling_event_ = ExperienceSamplingEvent::Create(event_name);

  chrome::RecordDialogCreation(chrome::DialogIdentifier::EXTENSION_INSTALL);
}

ExtensionInstallDialogView::~ExtensionInstallDialogView() {
  if (!handled_result_ && !done_callback_.is_null()) {
    base::ResetAndReturn(&done_callback_)
        .Run(ExtensionInstallPrompt::Result::USER_CANCELED);
  }
}

void ExtensionInstallDialogView::CreateContents() {
  SetLayoutManager(new views::FillLayout());

  auto extension_info_container = std::make_unique<CustomScrollableView>(this);
  const gfx::Insets content_insets =
      ChromeLayoutProvider::Get()->GetDialogInsetsForContentType(
          views::CONTROL, views::CONTROL);
  extension_info_container->SetBorder(views::CreateEmptyBorder(
      0, content_insets.left(), 0, content_insets.right()));
  views::GridLayout* scroll_layout = extension_info_container->SetLayoutManager(
      std::make_unique<views::GridLayout>(extension_info_container.get()));
  const int content_width =
      extension_info_container->GetPreferredSize().width();

  std::vector<ExtensionInfoSection> sections;

  if (prompt_->ShouldShowPermissions()) {
    bool has_permissions =
        prompt_->GetPermissionCount(
            ExtensionInstallPrompt::PermissionsType::ALL_PERMISSIONS) > 0;
    if (has_permissions) {
      AddPermissions(
          sections, content_width,
          ExtensionInstallPrompt::PermissionsType::REGULAR_PERMISSIONS);
      AddPermissions(
          sections, content_width,
          ExtensionInstallPrompt::PermissionsType::WITHHELD_PERMISSIONS);
    } else {
      sections.push_back(
          {l10n_util::GetStringUTF16(IDS_EXTENSION_NO_SPECIAL_PERMISSIONS),
           nullptr});
    }
  }

  if (prompt_->GetRetainedFileCount()) {
    PermissionDetails details;
    for (size_t i = 0; i < prompt_->GetRetainedFileCount(); ++i) {
      details.push_back(prompt_->GetRetainedFile(i));
    }
    ExpandableContainerView* issue_advice_view =
        new ExpandableContainerView(details, content_width, false);
    sections.push_back({prompt_->GetRetainedFilesHeading(), issue_advice_view});
  }

  if (prompt_->GetRetainedDeviceCount()) {
    PermissionDetails details;
    for (size_t i = 0; i < prompt_->GetRetainedDeviceCount(); ++i) {
      details.push_back(prompt_->GetRetainedDeviceMessageString(i));
    }
    ExpandableContainerView* issue_advice_view =
        new ExpandableContainerView(details, content_width, false);
    sections.push_back(
        {prompt_->GetRetainedDevicesHeading(), issue_advice_view});
  }

  if (sections.size() > 0) {
    set_margins(
        gfx::Insets(content_insets.top(), 0, content_insets.bottom(), 0));

    constexpr int kScrollViewColumnSetId = 0;
    views::ColumnSet* scrollable_column_set =
        scroll_layout->AddColumnSet(kScrollViewColumnSetId);
    scrollable_column_set->AddColumn(views::GridLayout::LEADING,
                                     views::GridLayout::LEADING, 0,
                                     views::GridLayout::USE_PREF, 0, 0);

    bool first_row = true;
    for (ExtensionInfoSection section : sections) {
      if (!first_row) {
        scroll_layout->AddPaddingRow(
            0, ChromeLayoutProvider::Get()->GetDistanceMetric(
                   views::DISTANCE_RELATED_CONTROL_VERTICAL));
      }
      scroll_layout->StartRow(0, kScrollViewColumnSetId);
      views::Label* header_label =
          new views::Label(section.header, CONTEXT_BODY_TEXT_LARGE);
      header_label->SetMultiLine(true);
      header_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      header_label->SizeToFit(content_width);
      scroll_layout->AddView(header_label);

      if (section.contents_view) {
        scroll_layout->StartRow(0, kScrollViewColumnSetId);
        scroll_layout->AddView(section.contents_view);
        first_row = false;
      }
    }

    scroll_view_ = new views::ScrollView();
    scroll_view_->set_hide_horizontal_scrollbar(true);
    scroll_view_->SetContents(extension_info_container.release());
    scroll_view_->ClipHeightTo(0, 192);
    AddChildView(scroll_view_);
  } else {
    // Use a smaller margin between the title area and buttons, since there
    // isn't any content.
    set_margins(gfx::Insets(ChromeLayoutProvider::Get()->GetDistanceMetric(
                                views::DISTANCE_UNRELATED_CONTROL_VERTICAL),
                            0, 0, 0));
  }
}

void ExtensionInstallDialogView::AddPermissions(
    std::vector<ExtensionInfoSection>& sections,
    int available_width,
    ExtensionInstallPrompt::PermissionsType perm_type) {
  if (prompt_->GetPermissionCount(perm_type) == 0)
    return;

  sections.push_back({prompt_->GetPermissionsHeading(perm_type), nullptr});

  for (size_t i = 0; i < prompt_->GetPermissionCount(perm_type); ++i) {
    // If we have more details to provide, show them in collapsed form.
    ExtensionInfoSection permission(
        {prompt_->GetPermission(i, perm_type), nullptr});
    if (!prompt_->GetPermissionsDetails(i, perm_type).empty()) {
      PermissionDetails details;
      details.push_back(PrepareForDisplay(
          prompt_->GetPermissionsDetails(i, perm_type), false));
      ExpandableContainerView* details_container =
          new ExpandableContainerView(details, available_width, true);
      permission.contents_view = details_container;
    }
    sections.push_back(permission);
  }
}


int ExtensionInstallDialogView::GetDialogButtons() const {
  int buttons = prompt_->GetDialogButtons();
  // Simply having just an OK button is *not* supported. See comment on function
  // GetDialogButtons in dialog_delegate.h for reasons.
  DCHECK_GT(buttons & ui::DIALOG_BUTTON_CANCEL, 0);
  return buttons;
}

base::string16 ExtensionInstallDialogView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  switch (button) {
    case ui::DIALOG_BUTTON_OK:
      return prompt_->GetAcceptButtonLabel();
    case ui::DIALOG_BUTTON_CANCEL:
      return prompt_->GetAbortButtonLabel();
    default:
      NOTREACHED();
      return base::string16();
  }
}

int ExtensionInstallDialogView::GetDefaultDialogButton() const {
  return ui::DIALOG_BUTTON_CANCEL;
}

bool ExtensionInstallDialogView::Cancel() {
  if (handled_result_)
    return true;

  handled_result_ = true;
  UpdateInstallResultHistogram(false);
  if (sampling_event_)
    sampling_event_->CreateUserDecisionEvent(ExperienceSamplingEvent::kDeny);
  base::ResetAndReturn(&done_callback_)
      .Run(ExtensionInstallPrompt::Result::USER_CANCELED);
  return true;
}

bool ExtensionInstallDialogView::Accept() {
  DCHECK(!handled_result_);

  handled_result_ = true;
  UpdateInstallResultHistogram(true);
  if (sampling_event_)
    sampling_event_->CreateUserDecisionEvent(ExperienceSamplingEvent::kProceed);
  base::ResetAndReturn(&done_callback_)
      .Run(ExtensionInstallPrompt::Result::ACCEPTED);
  return true;
}

ui::ModalType ExtensionInstallDialogView::GetModalType() const {
  return prompt_->ShouldUseTabModalDialog() ? ui::MODAL_TYPE_CHILD
                                            : ui::MODAL_TYPE_WINDOW;
}

void ExtensionInstallDialogView::LinkClicked(views::Link* source,
                                             int event_flags) {
  GURL store_url(extension_urls::GetWebstoreItemDetailURLPrefix() +
                 prompt_->extension()->id());
  OpenURLParams params(store_url, Referrer(),
                       WindowOpenDisposition::NEW_FOREGROUND_TAB,
                       ui::PAGE_TRANSITION_LINK, false);

  if (navigator_) {
    navigator_->OpenURL(params);
  } else {
    chrome::ScopedTabbedBrowserDisplayer displayer(profile_);
    displayer.browser()->OpenURL(params);
  }
  GetWidget()->Close();
}

views::View* ExtensionInstallDialogView::CreateExtraView() {
  if (!prompt_->has_webstore_data())
    return nullptr;

  views::Link* store_link = new views::Link(
      l10n_util::GetStringUTF16(IDS_EXTENSION_PROMPT_STORE_LINK));
  store_link->set_listener(this);
  return store_link;
}

bool ExtensionInstallDialogView::IsDialogButtonEnabled(
    ui::DialogButton button) const {
  if (button == ui::DIALOG_BUTTON_OK)
    return install_button_enabled_;
  return true;
}

void ExtensionInstallDialogView::SetInstallButtonDelayForTesting(
    int delay_in_ms) {
  g_install_delay_in_ms = delay_in_ms;
}

void ExtensionInstallDialogView::AddedToWidget() {
  auto title_container = std::make_unique<views::View>();

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  views::GridLayout* layout = title_container->SetLayoutManager(
      std::make_unique<views::GridLayout>(title_container.get()));
  constexpr int kTitleColumnSetId = 0;
  views::ColumnSet* column_set = layout->AddColumnSet(kTitleColumnSetId);
  column_set->AddColumn(views::GridLayout::CENTER, views::GridLayout::LEADING,
                        0, views::GridLayout::FIXED, kIconSize, 0);
  // Equalize padding on the left and the right of the icon.
  column_set->AddPaddingColumn(
      0, provider->GetInsetsMetric(views::INSETS_DIALOG).left());
  column_set->AddColumn(views::GridLayout::LEADING, views::GridLayout::LEADING,
                        0, views::GridLayout::USE_PREF, 0, 0);

  // Scale down to icon size, but allow smaller icons (don't scale up).
  const gfx::ImageSkia* image = prompt_->icon().ToImageSkia();
  views::ImageView* icon = new views::ImageView();
  gfx::Size size(image->width(), image->height());
  size.SetToMin(gfx::Size(kIconSize, kIconSize));
  icon->SetImageSize(size);
  icon->SetImage(*image);

  layout->StartRow(0, kTitleColumnSetId);
  layout->AddView(icon);

  std::unique_ptr<views::Label> title_label =
      views::BubbleFrameView::CreateDefaultTitleLabel(
          prompt_->GetDialogTitle());
  if (prompt_->has_webstore_data()) {
    auto webstore_data_container = std::make_unique<views::View>();
    webstore_data_container->SetLayoutManager(new views::BoxLayout(
        views::BoxLayout::kVertical, gfx::Insets(),
        provider->GetDistanceMetric(DISTANCE_RELATED_CONTROL_VERTICAL_SMALL)));

    webstore_data_container->AddChildView(title_label.release());

    auto rating_container = std::make_unique<views::View>();
    // TODO: 8??
    rating_container->SetLayoutManager(
        new views::BoxLayout(views::BoxLayout::kHorizontal, gfx::Insets(), 8));
    auto rating = std::make_unique<RatingsView>(prompt_->average_rating(),
                                                prompt_->rating_count());
    prompt_->AppendRatingStars(AddResourceIcon, rating.get());
    rating_container->AddChildView(rating.release());
    int rating_text_context, user_count_text_context;
    if (provider->IsHarmonyMode()) {
      rating_text_context = CONTEXT_BODY_TEXT_LARGE;
      user_count_text_context = CONTEXT_BODY_TEXT_SMALL;
    } else {
      rating_text_context = user_count_text_context = CONTEXT_DEPRECATED_SMALL;
    }
    auto rating_count = std::make_unique<RatingLabel>(prompt_->GetRatingCount(),
                                                      rating_text_context);
    rating_count->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    rating_container->AddChildView(rating_count.release());
    webstore_data_container->AddChildView(rating_container.release());

    auto user_count = std::make_unique<views::Label>(
        prompt_->GetUserCount(), user_count_text_context, STYLE_SECONDARY);
    user_count->SetAutoColorReadabilityEnabled(false);
    user_count->SetEnabledColor(SK_ColorGRAY);
    user_count->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    webstore_data_container->AddChildView(user_count.release());

    layout->AddView(webstore_data_container.release());
  } else {
    layout->AddView(title_label.release());
  }

  views::BubbleFrameView* frame_view = static_cast<views::BubbleFrameView*>(
      GetWidget()->non_client_view()->frame_view());
  frame_view->SetTitleView(std::move(title_container));
}

void ExtensionInstallDialogView::VisibilityChanged(views::View* starting_from,
                                                   bool is_visible) {
  if (is_visible && !install_button_enabled_) {
    // This base::Unretained is safe because the task is owned by the timer,
    // which is in turn owned by this object.
    timer_.Start(FROM_HERE,
                 base::TimeDelta::FromMilliseconds(g_install_delay_in_ms),
                 base::Bind(&ExtensionInstallDialogView::EnableInstallButton,
                            base::Unretained(this)));
  }
}

void ExtensionInstallDialogView::EnableInstallButton() {
  install_button_enabled_ = true;
  DialogModelChanged();
}

void ExtensionInstallDialogView::UpdateInstallResultHistogram(bool accepted)
    const {
  if (prompt_->type() == ExtensionInstallPrompt::INSTALL_PROMPT)
    UMA_HISTOGRAM_BOOLEAN("Extensions.InstallPrompt.Accepted", accepted);
}

// ExpandableContainerView::DetailsView ----------------------------------------

ExpandableContainerView::DetailsView::DetailsView(
    const PermissionDetails& details) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kVertical, gfx::Insets(),
      ChromeLayoutProvider::Get()->GetDistanceMetric(
          DISTANCE_RELATED_CONTROL_VERTICAL_SMALL)));
  for (auto& detail : details) {
    views::Label* detail_label =
        new views::Label(PrepareForDisplay(detail, false));
    detail_label->SetMultiLine(true);
    detail_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    AddChildView(detail_label);
  }
}

gfx::Size ExpandableContainerView::DetailsView::CalculatePreferredSize() const {
  return expanded_ ? views::View::CalculatePreferredSize() : gfx::Size();
}

void ExpandableContainerView::DetailsView::ToggleExpanded() {
  expanded_ = !expanded_;
  PreferredSizeChanged();
}

// ExpandableContainerView -----------------------------------------------------

ExpandableContainerView::ExpandableContainerView(
    const PermissionDetails& details,
    int available_width,
    bool parent_bulleted)
    : details_view_(nullptr), details_link_(nullptr), expanded_(false) {
  DCHECK(!details.empty());

  views::GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));
  constexpr int kColumnSetId = 0;
  views::ColumnSet* column_set = layout->AddColumnSet(kColumnSetId);

  const int left_padding = GetLeftPaddingForBulletedItems(parent_bulleted);
  column_set->AddPaddingColumn(0, left_padding);
  column_set->AddColumn(views::GridLayout::LEADING, views::GridLayout::LEADING,
                        0, views::GridLayout::USE_PREF,
                        available_width - left_padding, 0);

  layout->StartRow(0, kColumnSetId);
  details_view_ = new DetailsView(details);
  layout->AddView(details_view_);

  layout->StartRow(0, kColumnSetId);
  details_link_ =
      new views::Link(l10n_util::GetStringUTF16(IDS_EXTENSIONS_SHOW_DETAILS));
  details_link_->set_listener(this);
  details_link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  layout->AddView(details_link_);
}

ExpandableContainerView::~ExpandableContainerView() {
}

void ExpandableContainerView::LinkClicked(
    views::Link* source, int event_flags) {
  ToggleDetailLevel();
}

void ExpandableContainerView::ChildPreferredSizeChanged(views::View* child) {
  PreferredSizeChanged();
}

void ExpandableContainerView::ToggleDetailLevel() {
  expanded_ = !expanded_;
  details_view_->ToggleExpanded();
  details_link_->SetText(
      expanded_ ? l10n_util::GetStringUTF16(IDS_EXTENSIONS_HIDE_DETAILS)
                : l10n_util::GetStringUTF16(IDS_EXTENSIONS_SHOW_DETAILS));
}

// static
ExtensionInstallPrompt::ShowDialogCallback
ExtensionInstallPrompt::GetViewsShowDialogCallback() {
  return base::Bind(&ShowExtensionInstallDialogImpl);
}
