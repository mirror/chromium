// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/website_settings/website_settings_popup_view.h"

#include "base/string_number_conversions.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/website_settings/website_settings.h"
#include "chrome/browser/ui/views/collected_cookies_views.h"
#include "chrome/browser/ui/views/website_settings/permission_selector_view.h"
#include "chrome/browser/ui/tab_contents/tab_contents.h"
#include "chrome/common/content_settings_types.h"
#include "content/public/browser/cert_store.h"
#include "googleurl/src/gurl.h"
#include "grit/generated_resources.h"
#include "grit/ui_resources_standard.h"
#include "grit/theme_resources_standard.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font.h"
#include "ui/gfx/image/image.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/controls/button/menu_button_listener.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/tabbed_pane/tabbed_pane.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/view.h"

namespace {

// In order to make the arrow of the bubble point directly at the location icon
// in the Omnibox rather then the bottom border of the Omnibox, the position of
// the bubble must be adjusted. This is the number of pixel the bubble must be
// moved towards the top of the screen (starting from the bottom border of the
// Omnibox).
const int kLocationIconBottomMargin = 5;

// Font size of the label for the site identity.
const int kIdentityNameFontSize = 14;
// The text color that is used for the site identity status text, if the site's
// identity was sucessfully verified.
const int kIdentityVerifiedTextColor = 0xFF298a27;

// Left icon margin.
const int kIconMarginLeft = 6;

// Margin and padding values for the |PopupHeaderView|.
const int kHeaderMarginBottom = 10;
const int kHeaderPaddingBottom = 12;
const int kHeaderPaddingLeft = 10;
const int kHeaderPaddingRight = 8;
const int kHeaderPaddingTop = 8;

// Spacing between the site identity label and the site identity status text in
// the popup header.
const int kHeaderRowSpacing = 4;

// Padding values for sections.
const int kSectionPaddingBottom = 6;
const int kSectionPaddingLeft = 10;
const int kSectionPaddingTop = 14;

// Space between a section headline and the section content.
const int kSectionHeadlineMarginBottom = 10;
// The content of the "Permissions" section and of the "Cookies and Site Data"
// section, is structured in individual rows. |kSectionRowSpaceing| is the
// space between these rows.
const int kSectionRowSpacing = 6;

// The max width of the popup.
const int kPopupWidth = 300;

// The bottom margin of the tabbed pane view.
const int kTabbedPaneMarginBottom = 8;

}  // namespace

// |PopupHeaderView| is the UI element (view) that represents the header of the
// |WebsiteSettingsPopupView|. The header shows the status of the site's
// identity check and the name of the site's identity.
class PopupHeaderView : public views::View {
 public:
  explicit PopupHeaderView(views::ButtonListener* close_button_listener);
  virtual ~PopupHeaderView();

  // Sets the name of the site's identity.
  void SetIdentityName(const string16& name);

  // Sets the |status_text| for the identity check of this site and the
  // |text_color|.
  void SetIdentityStatus(const string16& status_text, SkColor text_color);

 private:
  // The label that displays the name of the site's identity.
  views::Label* name_;
  // The label that displays the status of the identity check for this site.
  views::Label* status_;

  DISALLOW_COPY_AND_ASSIGN(PopupHeaderView);
};

////////////////////////////////////////////////////////////////////////////////
// Popup Header
////////////////////////////////////////////////////////////////////////////////

PopupHeaderView::PopupHeaderView(views::ButtonListener* close_button_listener)
  : name_(NULL), status_(NULL) {
  views::GridLayout* layout = new views::GridLayout(this);
  SetLayoutManager(layout);

  const int label_column = 0;
  views::ColumnSet* column_set = layout->AddColumnSet(label_column);
  column_set->AddPaddingColumn(0, kHeaderPaddingLeft);
  column_set->AddColumn(views::GridLayout::FILL,
                        views::GridLayout::FILL,
                        1,
                        views::GridLayout::USE_PREF,
                        0,
                        0);
  column_set->AddPaddingColumn(1,0);
  column_set->AddColumn(views::GridLayout::FILL,
                        views::GridLayout::FILL,
                        1,
                        views::GridLayout::USE_PREF,
                        0,
                        0);
  column_set->AddPaddingColumn(0, kHeaderPaddingRight);

  layout->AddPaddingRow(0, kHeaderPaddingTop);

  layout->StartRow(0, label_column);
  name_ = new views::Label(string16());
  gfx::Font headline_font(name_->font().GetFontName(), kIdentityNameFontSize);
  name_->SetFont(headline_font.DeriveFont(0, gfx::Font::BOLD));
  layout->AddView(name_, 1, 1, views::GridLayout::LEADING,
                  views::GridLayout::TRAILING);
  views::ImageButton* close_button =
      new views::ImageButton(close_button_listener);
  ResourceBundle& rb = ResourceBundle::GetSharedInstance();
  close_button->SetImage(views::CustomButton::BS_NORMAL,
                         rb.GetImageNamed(IDR_CLOSE_BAR).ToImageSkia());
  close_button->SetImage(views::CustomButton::BS_HOT,
                         rb.GetImageNamed(IDR_CLOSE_BAR_H).ToImageSkia());
  close_button->SetImage(views::CustomButton::BS_PUSHED,
                         rb.GetImageNamed(IDR_CLOSE_BAR_P).ToImageSkia());
  layout->AddView(close_button, 1, 1, views::GridLayout::TRAILING,
                  views::GridLayout::LEADING);

  layout->AddPaddingRow(0, kHeaderRowSpacing);

  layout->StartRow(0, label_column);
  status_ = new views::Label(string16());
  layout->AddView(status_,
                  1,
                  1,
                  views::GridLayout::LEADING,
                  views::GridLayout::CENTER);

  layout->AddPaddingRow(0, kHeaderPaddingBottom);
}

PopupHeaderView::~PopupHeaderView() {
}

void PopupHeaderView::SetIdentityName(const string16& name) {
  name_->SetText(name);
}

void PopupHeaderView::SetIdentityStatus(const string16& status,
                                    SkColor text_color) {
  status_->SetText(status);
  status_->SetEnabledColor(text_color);
}

///////////////////////////////////////////////////////////////////////////////
// WebsiteSettingsPopupView
///////////////////////////////////////////////////////////////////////////////

WebsiteSettingsPopupView::~WebsiteSettingsPopupView() {
}

// static
void WebsiteSettingsPopupView::ShowPopup(views::View* anchor_view,
                                         Profile* profile,
                                         TabContents* tab_contents,
                                         const GURL& url,
                                         const content::SSLStatus& ssl) {
  new WebsiteSettingsPopupView(anchor_view, profile, tab_contents, url, ssl);
}

WebsiteSettingsPopupView::WebsiteSettingsPopupView(
    views::View* anchor_view,
    Profile* profile,
    TabContents* tab_contents,
    const GURL& url,
    const content::SSLStatus& ssl)
    : BubbleDelegateView(anchor_view, views::BubbleBorder::TOP_LEFT),
      tab_contents_(tab_contents),
      header_(NULL),
      tabbed_pane_(NULL),
      site_data_content_(NULL),
      cookie_dialog_link_(NULL),
      permissions_content_(NULL),
      identity_info_content_(NULL),
      connection_info_content_(NULL),
      page_info_content_(NULL) {
  views::GridLayout* layout = new views::GridLayout(this);
  SetLayoutManager(layout);
  const int content_column = 0;
  views::ColumnSet* column_set = layout->AddColumnSet(content_column);
  column_set->AddColumn(views::GridLayout::FILL,
                        views::GridLayout::FILL,
                        1,
                        views::GridLayout::USE_PREF,
                        0,
                        0);

  header_ = new PopupHeaderView(this);
  layout->StartRow(1, content_column);
  layout->AddView(header_);

  layout->AddPaddingRow(1, kHeaderMarginBottom);

  tabbed_pane_ = new views::TabbedPane();
  layout->StartRow(1, content_column);
  layout->AddView(tabbed_pane_);
  // Tabs must be added after the tabbed_pane_ was added to the views hierachy.
  // Adding the |tabbed_pane_| to the views hierachy triggers the
  // initialization of the native tab UI element. If the native tab UI element
  // is not initalized adding a tab will result in a NULL pointer excetion.
  tabbed_pane_->AddTab(
      l10n_util::GetStringUTF16(IDS_WEBSITE_SETTINGS_TAB_LABEL_PERMISSIONS),
      CreatePermissionsTab());
  tabbed_pane_->AddTab(
      l10n_util::GetStringUTF16(IDS_WEBSITE_SETTINGS_TAB_LABEL_IDENTITY),
      CreateIdentityTab());
  tabbed_pane_->SelectTabAt(0);
  tabbed_pane_->set_listener(this);

  layout->AddPaddingRow(0, kTabbedPaneMarginBottom);

  views::BubbleDelegateView::CreateBubble(this);
  this->Show();
  SizeToContents();

  presenter_.reset(new WebsiteSettings(this, profile,
                                       tab_contents->content_settings(), url,
                                       ssl, content::CertStore::GetInstance()));
}

void WebsiteSettingsPopupView::OnPermissionChanged(
    PermissionSelectorView* permission_selector) {
  DCHECK(permission_selector);
  presenter_->OnSitePermissionChanged(
      permission_selector->GetPermissionType(),
      permission_selector->GetSelectedSetting());
}

gfx::Rect WebsiteSettingsPopupView::GetAnchorRect() {
  // Compensate for some built-in padding in the icon. This will make the arrow
  // point to the middle of the icon.
  gfx::Rect anchor(BubbleDelegateView::GetAnchorRect());
  anchor.Inset(0, anchor_view() ? kLocationIconBottomMargin : 0);
  return anchor;
}

void WebsiteSettingsPopupView::ButtonPressed(
    views::Button* button,
    const views::Event& event) {
  GetWidget()->Close();
}

void WebsiteSettingsPopupView::LinkClicked(views::Link* source,
                                           int event_flags) {
  DCHECK_EQ(cookie_dialog_link_, source);
  new CollectedCookiesViews(tab_contents_);
  // The popup closes automatically when the collected cookies dialog opens.
}

void WebsiteSettingsPopupView::TabSelectedAt(int index) {
  tabbed_pane_->GetSelectedTab()->Layout();
  SizeToContents();
}

gfx::Size WebsiteSettingsPopupView::GetPreferredSize() {
  int height = 0;
  if (header_)
    height += header_->GetPreferredSize().height();
  if (tabbed_pane_)
    height += tabbed_pane_->GetPreferredSize().height();

  return gfx::Size(kPopupWidth, height);
}

void WebsiteSettingsPopupView::SetCookieInfo(
    const CookieInfoList& cookie_info_list) {
  site_data_content_->RemoveAllChildViews(true);

  views::GridLayout* layout = new views::GridLayout(site_data_content_);
  site_data_content_->SetLayoutManager(layout);

  const int site_data_content_column = 0;
  views::ColumnSet* column_set =
      layout->AddColumnSet(site_data_content_column);
  column_set->AddColumn(views::GridLayout::FILL,
                        views::GridLayout::FILL,
                        1,
                        views::GridLayout::USE_PREF,
                        0,
                        0);
  column_set->AddPaddingColumn(0, kIconMarginLeft);
  column_set->AddColumn(views::GridLayout::FILL,
                        views::GridLayout::FILL,
                        1,
                        views::GridLayout::USE_PREF,
                        0,
                        0);

  for (CookieInfoList::const_iterator i(cookie_info_list.begin());
       i != cookie_info_list.end();
       ++i) {
    string16 label_text = l10n_util::GetStringFUTF16(
        IDS_WEBSITE_SETTINGS_SITE_DATA_STATS_LINE,
        UTF8ToUTF16(i->cookie_source),
        base::IntToString16(i->allowed),
        base::IntToString16(i->blocked));
    layout->StartRow(1, site_data_content_column);
    views::ImageView* icon = new views::ImageView();
    const gfx::Image& image = WebsiteSettingsUI::GetPermissionIcon(
        CONTENT_SETTINGS_TYPE_COOKIES, CONTENT_SETTING_ALLOW);
    icon->SetImage(image.ToImageSkia());
    layout->AddView(icon);
    layout->AddView(new views::Label(label_text),
                    1,
                    1,
                    views::GridLayout::LEADING,
                    views::GridLayout::CENTER);

    layout->AddPaddingRow(1, kSectionRowSpacing);
  }

  layout->Layout(site_data_content_);
  SizeToContents();
}

void WebsiteSettingsPopupView::SetPermissionInfo(
    const PermissionInfoList& permission_info_list) {
  permissions_content_->RemoveAllChildViews(true);

  views::GridLayout* layout =
      new views::GridLayout(permissions_content_);
  permissions_content_->SetLayoutManager(layout);
  const int content_column = 0;
  views::ColumnSet* column_set =
      layout->AddColumnSet(content_column);
  column_set->AddColumn(views::GridLayout::FILL,
                        views::GridLayout::FILL,
                        1,
                        views::GridLayout::USE_PREF,
                        0,
                        0);
  for (PermissionInfoList::const_iterator permission =
           permission_info_list.begin();
       permission != permission_info_list.end();
       ++permission) {
    layout->StartRow(1, content_column);
    PermissionSelectorView* selector = new PermissionSelectorView(
        permission->type,
        permission->default_setting,
        permission->setting);
    selector->AddObserver(this);

    layout->AddView(selector,
                    1,
                    1,
                    views::GridLayout::LEADING,
                    views::GridLayout::CENTER);

    layout->AddPaddingRow(1, kSectionRowSpacing);
  }

  SizeToContents();
}

void WebsiteSettingsPopupView::SetIdentityInfo(
    const IdentityInfo& identity_info) {
  string16 identity_status_text;
  SkColor text_color = SK_ColorBLACK;
  switch (identity_info.identity_status) {
    case WebsiteSettings::SITE_IDENTITY_STATUS_CERT:
    case WebsiteSettings::SITE_IDENTITY_STATUS_DNSSEC_CERT:
    case WebsiteSettings::SITE_IDENTITY_STATUS_EV_CERT:
      identity_status_text =
          l10n_util::GetStringUTF16(IDS_WEBSITE_SETTINGS_IDENTITY_VERIFIED);
      text_color = kIdentityVerifiedTextColor;
      break;
    default:
      identity_status_text =
         l10n_util::GetStringUTF16(IDS_WEBSITE_SETTINGS_IDENTITY_NOT_VERIFIED);
      break;
  }
  header_->SetIdentityName(UTF8ToUTF16(identity_info.site_identity));
  header_->SetIdentityStatus(identity_status_text, text_color);

  ResetContentContainer(
      identity_info_content_,
      WebsiteSettingsUI::GetIdentityIcon(identity_info.identity_status),
      UTF8ToUTF16(identity_info.identity_status_description));
  ResetContentContainer(
      connection_info_content_,
      WebsiteSettingsUI::GetConnectionIcon(identity_info.connection_status),
      UTF8ToUTF16(identity_info.connection_status_description));

  // Layout.
  GetLayoutManager()->Layout(this);
  SizeToContents();
}

void WebsiteSettingsPopupView::SetFirstVisit(const string16& first_visit) {
  // TODO(markusheintz): Display a minor warning icon if the page is visited
  // the first time.
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  ResetContentContainer(page_info_content_, rb.GetImageNamed(IDR_PAGEINFO_INFO),
                        first_visit);

  // Layout.
  GetLayoutManager()->Layout(this);
  SizeToContents();
}

views::View* WebsiteSettingsPopupView::CreatePermissionsTab() {
  views::View* pane = new views::View();
  pane->SetLayoutManager(
      new views::BoxLayout(views::BoxLayout::kVertical, 0, 0, 1));

  cookie_dialog_link_ = new views::Link(
      l10n_util::GetStringUTF16(IDS_WEBSITE_SETTINGS_SHOW_SITE_DATA));
  cookie_dialog_link_->set_listener(this);
  site_data_content_ = new views::View();
  views::View* site_data_section =
      CreateSection(l10n_util::GetStringUTF16(
                        IDS_WEBSITE_SETTINGS_TITLE_SITE_DATA),
                    site_data_content_,
                    cookie_dialog_link_);
  pane->AddChildView(site_data_section);

  permissions_content_ = new views::View();
  views::View* permissions_section =
      CreateSection(l10n_util::GetStringUTF16(
                        IDS_WEBSITE_SETTINGS_TITLE_SITE_PERMISSIONS),
                    permissions_content_,
                    NULL);
  pane->AddChildView(permissions_section);
  return pane;
}

views::View* WebsiteSettingsPopupView::CreateIdentityTab() {
  views::View* pane = new views::View();
  pane->SetLayoutManager(
      new views::BoxLayout(views::BoxLayout::kVertical, 0, 0, 1));

  // Add Identity section.
  views::View* section_content = new views::View();
  views::GridLayout* layout = new views::GridLayout(section_content);
  section_content->SetLayoutManager(layout);
  const int content_column = 0;
  views::ColumnSet* column_set =
      layout->AddColumnSet(content_column);
  column_set->AddColumn(views::GridLayout::FILL,
                        views::GridLayout::FILL,
                        1,
                        views::GridLayout::USE_PREF,
                        0,
                        0);
  identity_info_content_ = new views::View();
  layout->StartRow(1, content_column);
  layout->AddView(identity_info_content_, 1, 1, views::GridLayout::LEADING,
                  views::GridLayout::CENTER);
  views::View* section = CreateSection(
      l10n_util::GetStringUTF16(IDS_WEBSITE_SETTINGS_TITEL_IDENTITY),
      section_content,
      NULL);
  pane->AddChildView(section);

  // Add connection section.
  connection_info_content_ = new views::View();
  section = CreateSection(
      l10n_util::GetStringUTF16(IDS_WEBSITE_SETTINGS_TITEL_CONNECTION),
      connection_info_content_,
      NULL);
  pane->AddChildView(section);

  // Add page info section.
  section_content = new views::View();
  layout = new views::GridLayout(section_content);
  section_content->SetLayoutManager(layout);
  column_set = layout->AddColumnSet(content_column);
  column_set->AddColumn(views::GridLayout::FILL,
                        views::GridLayout::FILL,
                        1,
                        views::GridLayout::USE_PREF,
                        0,
                        0);
  page_info_content_ = new views::View();
  layout->StartRow(1, content_column);
  layout->AddView(page_info_content_, 1, 1, views::GridLayout::LEADING,
                  views::GridLayout::CENTER);
  section = CreateSection(
      l10n_util::GetStringUTF16(IDS_PAGE_INFO_SITE_INFO_TITLE),
      section_content,
      NULL);
  pane->AddChildView(section);

  return pane;
}

views::View* WebsiteSettingsPopupView::CreateSection(
    const string16& headline_text,
    views::View* content,
    views::Link* link) {
  views::View* container = new views::View();
  views::GridLayout* layout = new views::GridLayout(container);
  container->SetLayoutManager(layout);
  const int content_column = 0;
  views::ColumnSet* column_set = layout->AddColumnSet(content_column);
  column_set->AddPaddingColumn(0, kSectionPaddingLeft);
  column_set->AddColumn(views::GridLayout::FILL,
                        views::GridLayout::FILL,
                        1,
                        views::GridLayout::USE_PREF,
                        0,
                        0);

  layout->AddPaddingRow(1, kSectionPaddingTop);
  layout->StartRow(1, content_column);
  views::Label* headline = new views::Label(headline_text);
  headline->SetFont(headline->font().DeriveFont(0, gfx::Font::BOLD));
  layout->AddView(headline, 1, 1, views::GridLayout::LEADING,
                  views::GridLayout::CENTER);

  layout->AddPaddingRow(1, kSectionHeadlineMarginBottom);
  layout->StartRow(1, content_column);
  layout->AddView(content, 1, 1, views::GridLayout::LEADING,
                  views::GridLayout::CENTER);

  if (link) {
    layout->AddPaddingRow(1, 4);
    layout->StartRow(1, content_column);
    layout->AddView(link, 1, 1, views::GridLayout::LEADING,
                    views::GridLayout::CENTER);
  }

  layout->AddPaddingRow(1, kSectionPaddingBottom);
  return container;
}

void WebsiteSettingsPopupView::ResetContentContainer(
    views::View* content_container,
    const gfx::Image& icon,
    const string16& text) {
  views::ImageView* icon_view = new views::ImageView();
  icon_view->SetImage(icon.ToImageSkia());
  views::Label* label = new views::Label(text);
  label->SetMultiLine(true);
  label->SetAllowCharacterBreak(true);
  label->SetHorizontalAlignment(views::Label::ALIGN_LEFT);

  content_container->RemoveAllChildViews(true);
  views::GridLayout* layout = new views::GridLayout(content_container);
  content_container->SetLayoutManager(layout);
  views::ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(views::GridLayout::FILL,
                        views::GridLayout::FILL,
                        0,
                        views::GridLayout::USE_PREF,
                        0,
                        0);
  column_set->AddPaddingColumn(0, kIconMarginLeft);
  column_set->AddColumn(views::GridLayout::FILL,
                        views::GridLayout::FILL,
                        1,
                        views::GridLayout::USE_PREF,
                        0,
                        0);

  layout->StartRow(1, 0);
  layout->AddView(icon_view, 1, 1, views::GridLayout::LEADING,
                  views::GridLayout::LEADING);
  layout->AddView(label, 1, 1, views::GridLayout::LEADING,
                  views::GridLayout::LEADING);
}
