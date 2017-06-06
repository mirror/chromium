// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/web_contents_accessibility_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/feature_list.h"
#include "content/browser/accessibility/browser_accessibility_android.h"
#include "content/browser/accessibility/browser_accessibility_manager_android.h"
#include "content/browser/accessibility/browser_accessibility_state_impl.h"
#include "content/browser/accessibility/one_shot_accessibility_tree_search.h"
#include "content/browser/android/render_widget_host_connector.h"
#include "content/browser/renderer_host/render_widget_host_view_android.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/common/content_features.h"
#include "jni/WebContentsAccessibility_jni.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace content {

namespace {

using SearchKeyToPredicateMap =
    base::hash_map<base::string16, AccessibilityMatchPredicate>;
base::LazyInstance<SearchKeyToPredicateMap>::DestructorAtExit
    g_search_key_to_predicate_map = LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<base::string16>::DestructorAtExit g_all_search_keys =
    LAZY_INSTANCE_INITIALIZER;

bool SectionPredicate(BrowserAccessibility* start, BrowserAccessibility* node) {
  switch (node->GetRole()) {
    case ui::AX_ROLE_ARTICLE:
    case ui::AX_ROLE_APPLICATION:
    case ui::AX_ROLE_BANNER:
    case ui::AX_ROLE_COMPLEMENTARY:
    case ui::AX_ROLE_CONTENT_INFO:
    case ui::AX_ROLE_HEADING:
    case ui::AX_ROLE_MAIN:
    case ui::AX_ROLE_NAVIGATION:
    case ui::AX_ROLE_SEARCH:
    case ui::AX_ROLE_REGION:
      return true;
    default:
      return false;
  }
}

bool AllInterestingNodesPredicate(BrowserAccessibility* start,
                                  BrowserAccessibility* node) {
  BrowserAccessibilityAndroid* android_node =
      static_cast<BrowserAccessibilityAndroid*>(node);
  return android_node->IsInterestingOnAndroid();
}

void AddToPredicateMap(const char* search_key_ascii,
                       AccessibilityMatchPredicate predicate) {
  base::string16 search_key_utf16 = base::ASCIIToUTF16(search_key_ascii);
  g_search_key_to_predicate_map.Get()[search_key_utf16] = predicate;
  if (!g_all_search_keys.Get().empty())
    g_all_search_keys.Get() += base::ASCIIToUTF16(",");
  g_all_search_keys.Get() += search_key_utf16;
}

// These are special unofficial strings sent from TalkBack/BrailleBack
// to jump to certain categories of web elements.
void InitSearchKeyToPredicateMapIfNeeded() {
  if (!g_search_key_to_predicate_map.Get().empty())
    return;

  AddToPredicateMap("ARTICLE", AccessibilityArticlePredicate);
  AddToPredicateMap("BUTTON", AccessibilityButtonPredicate);
  AddToPredicateMap("CHECKBOX", AccessibilityCheckboxPredicate);
  AddToPredicateMap("COMBOBOX", AccessibilityComboboxPredicate);
  AddToPredicateMap("CONTROL", AccessibilityControlPredicate);
  AddToPredicateMap("FOCUSABLE", AccessibilityFocusablePredicate);
  AddToPredicateMap("FRAME", AccessibilityFramePredicate);
  AddToPredicateMap("GRAPHIC", AccessibilityGraphicPredicate);
  AddToPredicateMap("H1", AccessibilityH1Predicate);
  AddToPredicateMap("H2", AccessibilityH2Predicate);
  AddToPredicateMap("H3", AccessibilityH3Predicate);
  AddToPredicateMap("H4", AccessibilityH4Predicate);
  AddToPredicateMap("H5", AccessibilityH5Predicate);
  AddToPredicateMap("H6", AccessibilityH6Predicate);
  AddToPredicateMap("HEADING", AccessibilityHeadingPredicate);
  AddToPredicateMap("LANDMARK", AccessibilityLandmarkPredicate);
  AddToPredicateMap("LINK", AccessibilityLinkPredicate);
  AddToPredicateMap("LIST", AccessibilityListPredicate);
  AddToPredicateMap("LIST_ITEM", AccessibilityListItemPredicate);
  AddToPredicateMap("MAIN", AccessibilityMainPredicate);
  AddToPredicateMap("MEDIA", AccessibilityMediaPredicate);
  AddToPredicateMap("RADIO", AccessibilityRadioButtonPredicate);
  AddToPredicateMap("SECTION", SectionPredicate);
  AddToPredicateMap("TABLE", AccessibilityTablePredicate);
  AddToPredicateMap("TEXT_FIELD", AccessibilityTextfieldPredicate);
  AddToPredicateMap("UNVISITED_LINK", AccessibilityUnvisitedLinkPredicate);
  AddToPredicateMap("VISITED_LINK", AccessibilityVisitedLinkPredicate);
}

AccessibilityMatchPredicate PredicateForSearchKey(
    const base::string16& element_type) {
  InitSearchKeyToPredicateMapIfNeeded();
  const auto& iter = g_search_key_to_predicate_map.Get().find(element_type);
  if (iter != g_search_key_to_predicate_map.Get().end())
    return iter->second;

  // If we don't recognize the selector, return any element that a
  // screen reader should navigate to.
  return AllInterestingNodesPredicate;
}

// The element in the document for which we may be displaying an autofill popup.
int32_t g_element_hosting_autofill_popup_unique_id = -1;

// The element in the document that is the next element after
// |g_element_hosting_autofill_popup_unique_id|.
int32_t g_element_after_element_hosting_autofill_popup_unique_id = -1;

// Autofill popup will not be part of the |AXTree| that is sent by renderer.
// Hence, we need a proxy |AXNode| to represent the autofill popup.
BrowserAccessibility* g_autofill_popup_proxy_node = nullptr;
ui::AXNode* g_autofill_popup_proxy_node_ax_node = nullptr;

void DeleteAutofillPopupProxy() {
  if (g_autofill_popup_proxy_node) {
    g_autofill_popup_proxy_node->Destroy();
    delete g_autofill_popup_proxy_node_ax_node;
    g_autofill_popup_proxy_node = nullptr;
  }
}

}  // anonymous namespace

class WebContentsAccessibilityAndroid::Connector
    : public RenderWidgetHostConnector {
 public:
  Connector(WebContents* web_contents,
            WebContentsAccessibilityAndroid* accessibility);
  ~Connector() override;

  // RendetWidgetHostConnector implementation.
  void UpdateRenderProcessConnection(
      RenderWidgetHostViewAndroid* old_rwhva,
      RenderWidgetHostViewAndroid* new_rhwva) override;

 private:
  WebContentsAccessibilityAndroid* const accessibility_;
};

WebContentsAccessibilityAndroid::Connector::Connector(
    WebContents* web_contents,
    WebContentsAccessibilityAndroid* accessibility)
    : RenderWidgetHostConnector(web_contents), accessibility_(accessibility) {
  Initialize();
}

WebContentsAccessibilityAndroid::Connector::~Connector() {
  delete accessibility_;
}

void WebContentsAccessibilityAndroid::Connector::UpdateRenderProcessConnection(
    RenderWidgetHostViewAndroid* old_rwhva,
    RenderWidgetHostViewAndroid* new_rwhva) {
  if (old_rwhva)
    old_rwhva->set_web_contents_accessibility(nullptr);
  if (new_rwhva) {
    new_rwhva->set_web_contents_accessibility(accessibility_);
    accessibility_->set_should_focus_on_page_load(
        new_rwhva->ShouldSetAccessibilityFocusOnPageLoad());
  }
}

WebContentsAccessibilityAndroid::WebContentsAccessibilityAndroid(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    WebContents* web_contents)
    : java_ref_(env, obj),
      frame_info_initialized_(false),
      should_focus_on_page_load_(false),
      connector_(new Connector(web_contents, this)),
      root_manager_(nullptr) {}

WebContentsAccessibilityAndroid::~WebContentsAccessibilityAndroid() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  // Clean up autofill popup proxy node in case the popup was not dismissed.
  DeleteAutofillPopupProxy();

  Java_WebContentsAccessibility_onNativeObjectDestroyed(env, obj);
}

void WebContentsAccessibilityAndroid::SetAccessibilityEnabled(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    bool enabled) {
  BrowserAccessibilityStateImpl* accessibility_state =
      BrowserAccessibilityStateImpl::GetInstance();
  if (enabled) {
    // Don't do anything if we already have a BrowserAccessibilityManager
    // that needs to be connected to content layer.
    if (root_manager_)
      return;

    // Otherwise, enable accessibility globally unless it was
    // explicitly disallowed by a command-line flag, then enable it for
    // this WebContents if that succeeded.
    accessibility_state->OnScreenReaderDetected();
    auto* web_contents =
        static_cast<WebContentsImpl*>(connector_->web_contents());
    if (accessibility_state->IsAccessibleBrowser() && web_contents) {
      web_contents->AddAccessibilityMode(kAccessibilityModeComplete);
    }
  } else {
    accessibility_state->ResetAccessibilityMode();
    auto* web_contents =
        static_cast<WebContentsImpl*>(connector_->web_contents());
    if (web_contents) {
      web_contents->SetAccessibilityMode(
          accessibility_state->accessibility_mode());
    }
  }
}

bool WebContentsAccessibilityAndroid::ShouldRespectDisplayedPasswordText() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return false;
  return Java_WebContentsAccessibility_shouldRespectDisplayedPasswordText(env,
                                                                          obj);
}

bool WebContentsAccessibilityAndroid::ShouldExposePasswordText() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return false;
  return Java_WebContentsAccessibility_shouldExposePasswordText(env, obj);
}

void WebContentsAccessibilityAndroid::HandlePageLoaded(int32_t unique_id) {
  if (!should_focus_on_page_load_)
    return;
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_handlePageLoaded(env, obj, unique_id);
}

void WebContentsAccessibilityAndroid::HandleContentChanged(int32_t unique_id) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_handleContentChanged(env, obj, unique_id);
}

void WebContentsAccessibilityAndroid::HandleFocusChanged(int32_t unique_id) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_handleFocusChanged(env, obj, unique_id);
}

void WebContentsAccessibilityAndroid::HandleCheckStateChanged(
    int32_t unique_id) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_handleCheckStateChanged(env, obj, unique_id);
}

void WebContentsAccessibilityAndroid::HandleClicked(int32_t unique_id) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_handleClicked(env, obj, unique_id);
}

void WebContentsAccessibilityAndroid::HandleScrollPositionChanged(
    int32_t unique_id) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_handleScrollPositionChanged(env, obj,
                                                            unique_id);
}

void WebContentsAccessibilityAndroid::HandleScrolledToAnchor(
    int32_t unique_id) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_handleScrolledToAnchor(env, obj, unique_id);
}

void WebContentsAccessibilityAndroid::AnnounceLiveRegionText(
    const base::string16& text) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_announceLiveRegionText(
      env, obj, base::android::ConvertUTF16ToJavaString(env, text));
}

void WebContentsAccessibilityAndroid::HandleTextSelectionChanged(
    int32_t unique_id) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_handleTextSelectionChanged(env, obj, unique_id);
}

void WebContentsAccessibilityAndroid::HandleEditableTextChanged(
    int32_t unique_id) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_handleEditableTextChanged(env, obj, unique_id);
}

void WebContentsAccessibilityAndroid::HandleSliderChanged(int32_t unique_id) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_handleSliderChanged(env, obj, unique_id);
}

void WebContentsAccessibilityAndroid::SendDelayedWindowContentChangedEvent() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_sendDelayedWindowContentChangedEvent(env, obj);
}

void WebContentsAccessibilityAndroid::HandleHover(int32_t unique_id) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_handleHover(env, obj, unique_id);
}

void WebContentsAccessibilityAndroid::HandleNavigate() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_WebContentsAccessibility_handleNavigate(env, obj);
}

base::android::ScopedJavaLocalRef<jstring>
WebContentsAccessibilityAndroid::GetSupportedHtmlElementTypes(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  InitSearchKeyToPredicateMapIfNeeded();
  return base::android::ConvertUTF16ToJavaString(env, g_all_search_keys.Get());
}

jint WebContentsAccessibilityAndroid::GetRootId(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (root_manager_) {
    auto* root = root_manager_->GetRoot();
    if (root)
      return static_cast<jint>(root->unique_id());
  }
  return -1;
}

jboolean WebContentsAccessibilityAndroid::IsNodeValid(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint unique_id) {
  return GetAXFromUniqueID(unique_id) != NULL;
}

void WebContentsAccessibilityAndroid::HitTest(JNIEnv* env,
                                              const JavaParamRef<jobject>& obj,
                                              jint x,
                                              jint y) {
  if (root_manager_)
    root_manager_->HitTest(gfx::Point(x, y));
}

jboolean WebContentsAccessibilityAndroid::IsEditableText(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint unique_id) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (!node)
    return false;

  return node->IsEditableText();
}

jboolean WebContentsAccessibilityAndroid::IsFocused(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint unique_id) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (!node)
    return false;

  return node->IsFocused();
}

jint WebContentsAccessibilityAndroid::GetEditableTextSelectionStart(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint unique_id) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (!node)
    return false;

  return node->GetIntAttribute(ui::AX_ATTR_TEXT_SEL_START);
}

jint WebContentsAccessibilityAndroid::GetEditableTextSelectionEnd(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint unique_id) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (!node)
    return false;

  return node->GetIntAttribute(ui::AX_ATTR_TEXT_SEL_END);
}

jboolean WebContentsAccessibilityAndroid::PopulateAccessibilityNodeInfo(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& info,
    jint unique_id) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (!node)
    return false;

  if (node->PlatformGetParent()) {
    Java_WebContentsAccessibility_setAccessibilityNodeInfoParent(
        env, obj, info, node->PlatformGetParent()->unique_id());
  }
  for (unsigned i = 0; i < node->PlatformChildCount(); ++i) {
    Java_WebContentsAccessibility_addAccessibilityNodeInfoChild(
        env, obj, info, node->PlatformGetChild(i)->unique_id());
  }
  Java_WebContentsAccessibility_setAccessibilityNodeInfoBooleanAttributes(
      env, obj, info, unique_id, node->IsCheckable(), node->IsChecked(),
      node->IsClickable(), node->IsEnabled(), node->IsFocusable(),
      node->IsFocused(), node->IsPassword(), node->IsScrollable(),
      node->IsSelected(), node->IsVisibleToUser());
  Java_WebContentsAccessibility_addAccessibilityNodeInfoActions(
      env, obj, info, unique_id, node->CanScrollForward(),
      node->CanScrollBackward(), node->CanScrollUp(), node->CanScrollDown(),
      node->CanScrollLeft(), node->CanScrollRight(), node->IsClickable(),
      node->IsEditableText(), node->IsEnabled(), node->IsFocusable(),
      node->IsFocused(), node->IsCollapsed(), node->IsExpanded(),
      node->HasNonEmptyValue());
  Java_WebContentsAccessibility_setAccessibilityNodeInfoClassName(
      env, obj, info,
      base::android::ConvertUTF8ToJavaString(env, node->GetClassName()));
  Java_WebContentsAccessibility_setAccessibilityNodeInfoText(
      env, obj, info,
      base::android::ConvertUTF16ToJavaString(env, node->GetText()),
      node->IsLink(), node->IsEditableText(),
      base::android::ConvertUTF16ToJavaString(
          env, node->GetInheritedString16Attribute(ui::AX_ATTR_LANGUAGE)));
  base::string16 element_id;
  if (node->GetHtmlAttribute("id", &element_id)) {
    Java_WebContentsAccessibility_setAccessibilityNodeInfoViewIdResourceName(
        env, obj, info,
        base::android::ConvertUTF16ToJavaString(env, element_id));
  }

  gfx::Rect absolute_rect = node->GetPageBoundsRect();
  gfx::Rect parent_relative_rect = absolute_rect;
  if (node->PlatformGetParent()) {
    gfx::Rect parent_rect = node->PlatformGetParent()->GetPageBoundsRect();
    parent_relative_rect.Offset(-parent_rect.OffsetFromOrigin());
  }
  bool is_root = node->PlatformGetParent() == NULL;
  Java_WebContentsAccessibility_setAccessibilityNodeInfoLocation(
      env, obj, info, unique_id, absolute_rect.x(), absolute_rect.y(),
      parent_relative_rect.x(), parent_relative_rect.y(), absolute_rect.width(),
      absolute_rect.height(), is_root);

  Java_WebContentsAccessibility_setAccessibilityNodeInfoKitKatAttributes(
      env, obj, info, is_root, node->IsEditableText(),
      base::android::ConvertUTF16ToJavaString(env, node->GetRoleDescription()),
      node->GetIntAttribute(ui::AX_ATTR_TEXT_SEL_START),
      node->GetIntAttribute(ui::AX_ATTR_TEXT_SEL_END));

  Java_WebContentsAccessibility_setAccessibilityNodeInfoLollipopAttributes(
      env, obj, info, node->CanOpenPopup(), node->IsContentInvalid(),
      node->IsDismissable(), node->IsMultiLine(), node->AndroidInputType(),
      node->AndroidLiveRegionType());
  if (node->IsCollection()) {
    Java_WebContentsAccessibility_setAccessibilityNodeInfoCollectionInfo(
        env, obj, info, node->RowCount(), node->ColumnCount(),
        node->IsHierarchical());
  }
  if (node->IsCollectionItem() || node->IsHeading()) {
    Java_WebContentsAccessibility_setAccessibilityNodeInfoCollectionItemInfo(
        env, obj, info, node->RowIndex(), node->RowSpan(), node->ColumnIndex(),
        node->ColumnSpan(), node->IsHeading());
  }
  if (node->IsRangeType()) {
    Java_WebContentsAccessibility_setAccessibilityNodeInfoRangeInfo(
        env, obj, info, node->AndroidRangeType(), node->RangeMin(),
        node->RangeMax(), node->RangeCurrentValue());
  }

  return true;
}

jboolean WebContentsAccessibilityAndroid::PopulateAccessibilityEvent(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& event,
    jint unique_id,
    jint event_type) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (!node)
    return false;

  Java_WebContentsAccessibility_setAccessibilityEventBooleanAttributes(
      env, obj, event, node->IsChecked(), node->IsEnabled(), node->IsPassword(),
      node->IsScrollable());
  Java_WebContentsAccessibility_setAccessibilityEventClassName(
      env, obj, event,
      base::android::ConvertUTF8ToJavaString(env, node->GetClassName()));
  Java_WebContentsAccessibility_setAccessibilityEventListAttributes(
      env, obj, event, node->GetItemIndex(), node->GetItemCount());
  Java_WebContentsAccessibility_setAccessibilityEventScrollAttributes(
      env, obj, event, node->GetScrollX(), node->GetScrollY(),
      node->GetMaxScrollX(), node->GetMaxScrollY());

  switch (event_type) {
    case ANDROID_ACCESSIBILITY_EVENT_TEXT_CHANGED: {
      base::string16 before_text = node->GetTextChangeBeforeText();
      base::string16 text = node->GetText();
      Java_WebContentsAccessibility_setAccessibilityEventTextChangedAttrs(
          env, obj, event, node->GetTextChangeFromIndex(),
          node->GetTextChangeAddedCount(), node->GetTextChangeRemovedCount(),
          base::android::ConvertUTF16ToJavaString(env, before_text),
          base::android::ConvertUTF16ToJavaString(env, text));
      break;
    }
    case ANDROID_ACCESSIBILITY_EVENT_TEXT_SELECTION_CHANGED: {
      base::string16 text = node->GetText();
      Java_WebContentsAccessibility_setAccessibilityEventSelectionAttrs(
          env, obj, event, node->GetSelectionStart(), node->GetSelectionEnd(),
          node->GetEditableTextLength(),
          base::android::ConvertUTF16ToJavaString(env, text));
      break;
    }
    default:
      break;
  }

  Java_WebContentsAccessibility_setAccessibilityEventLollipopAttributes(
      env, obj, event, node->CanOpenPopup(), node->IsContentInvalid(),
      node->IsDismissable(), node->IsMultiLine(), node->AndroidInputType(),
      node->AndroidLiveRegionType());
  if (node->IsCollection()) {
    Java_WebContentsAccessibility_setAccessibilityEventCollectionInfo(
        env, obj, event, node->RowCount(), node->ColumnCount(),
        node->IsHierarchical());
  }
  if (node->IsHeading()) {
    Java_WebContentsAccessibility_setAccessibilityEventHeadingFlag(env, obj,
                                                                   event, true);
  }
  if (node->IsCollectionItem()) {
    Java_WebContentsAccessibility_setAccessibilityEventCollectionItemInfo(
        env, obj, event, node->RowIndex(), node->RowSpan(), node->ColumnIndex(),
        node->ColumnSpan());
  }
  if (node->IsRangeType()) {
    Java_WebContentsAccessibility_setAccessibilityEventRangeInfo(
        env, obj, event, node->AndroidRangeType(), node->RangeMin(),
        node->RangeMax(), node->RangeCurrentValue());
  }

  return true;
}

void WebContentsAccessibilityAndroid::Click(JNIEnv* env,
                                            const JavaParamRef<jobject>& obj,
                                            jint unique_id) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (node)
    node->manager()->DoDefaultAction(*node);
}

void WebContentsAccessibilityAndroid::Focus(JNIEnv* env,
                                            const JavaParamRef<jobject>& obj,
                                            jint unique_id) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (node)
    node->manager()->SetFocus(*node);
}

void WebContentsAccessibilityAndroid::Blur(JNIEnv* env,
                                           const JavaParamRef<jobject>& obj) {
  if (root_manager_)
    root_manager_->SetFocus(*root_manager_->GetRoot());
}

void WebContentsAccessibilityAndroid::ScrollToMakeNodeVisible(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint unique_id) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (node)
    node->manager()->ScrollToMakeVisible(
        *node, gfx::Rect(node->GetFrameBoundsRect().size()));
}

void WebContentsAccessibilityAndroid::SetTextFieldValue(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint unique_id,
    const JavaParamRef<jstring>& value) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (node) {
    node->manager()->SetValue(
        *node, base::android::ConvertJavaStringToUTF16(env, value));
  }
}

void WebContentsAccessibilityAndroid::SetSelection(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint unique_id,
    jint start,
    jint end) {
  using AXPlatformPositionInstance = AXPlatformPosition::AXPositionInstance;
  using AXPlatformRange = ui::AXRange<AXPlatformPositionInstance::element_type>;

  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (node) {
    node->manager()->SetSelection(AXPlatformRange(node->CreatePositionAt(start),
                                                  node->CreatePositionAt(end)));
  }
}

jboolean WebContentsAccessibilityAndroid::AdjustSlider(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint unique_id,
    jboolean increment) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (!node)
    return false;

  BrowserAccessibilityAndroid* android_node =
      static_cast<BrowserAccessibilityAndroid*>(node);

  if (!android_node->IsSlider() || !android_node->IsEnabled())
    return false;

  float value = node->GetFloatAttribute(ui::AX_ATTR_VALUE_FOR_RANGE);
  float min = node->GetFloatAttribute(ui::AX_ATTR_MIN_VALUE_FOR_RANGE);
  float max = node->GetFloatAttribute(ui::AX_ATTR_MAX_VALUE_FOR_RANGE);
  if (max <= min)
    return false;

  // To behave similarly to an Android SeekBar, move by an increment of
  // approximately 20%.
  float original_value = value;
  float delta = (max - min) / 5.0f;
  value += (increment ? delta : -delta);
  value = std::max(std::min(value, max), min);
  if (value != original_value) {
    node->manager()->SetValue(*node,
                              base::UTF8ToUTF16(base::DoubleToString(value)));
    return true;
  }
  return false;
}

void WebContentsAccessibilityAndroid::ShowContextMenu(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint unique_id) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (node)
    node->manager()->ShowContextMenu(*node);
}

jint WebContentsAccessibilityAndroid::FindElementType(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint start_id,
    const JavaParamRef<jstring>& element_type_str,
    jboolean forwards) {
  BrowserAccessibilityAndroid* start_node = GetAXFromUniqueID(start_id);
  if (!start_node)
    return 0;

  if (!root_manager_)
    return 0;

  BrowserAccessibility* root = root_manager_->GetRoot();
  if (!root)
    return 0;

  AccessibilityMatchPredicate predicate = PredicateForSearchKey(
      base::android::ConvertJavaStringToUTF16(env, element_type_str));

  OneShotAccessibilityTreeSearch tree_search(root);
  tree_search.SetStartNode(start_node);
  tree_search.SetDirection(forwards
                               ? OneShotAccessibilityTreeSearch::FORWARDS
                               : OneShotAccessibilityTreeSearch::BACKWARDS);
  tree_search.SetResultLimit(1);
  tree_search.SetImmediateDescendantsOnly(false);
  tree_search.SetVisibleOnly(false);
  tree_search.AddPredicate(predicate);

  if (tree_search.CountMatches() == 0)
    return 0;

  int32_t element_id = tree_search.GetMatchAtIndex(0)->unique_id();

  // Navigate forwards to the autofill popup's proxy node if focus is currently
  // on the element hosting the autofill popup. Once within the popup, a back
  // press will navigate back to the element hosting the popup. If user swipes
  // past last suggestion in the popup, or swipes left from the first suggestion
  // in the popup, we will navigate to the element that is the next element in
  // the document after the element hosting the popup.
  if (forwards && start_id == g_element_hosting_autofill_popup_unique_id &&
      g_autofill_popup_proxy_node) {
    g_element_after_element_hosting_autofill_popup_unique_id = element_id;
    return g_autofill_popup_proxy_node->unique_id();
  }

  return element_id;
}

jboolean WebContentsAccessibilityAndroid::NextAtGranularity(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint granularity,
    jboolean extend_selection,
    jint unique_id,
    jint cursor_index) {
  if (!root_manager_)
    return false;
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (!node)
    return false;

  jint start_index = -1;
  int end_index = -1;
  if (root_manager_->NextAtGranularity(granularity, cursor_index, node,
                                       &start_index, &end_index)) {
    base::string16 text = node->GetText();
    Java_WebContentsAccessibility_finishGranularityMove(
        env, obj, base::android::ConvertUTF16ToJavaString(env, text),
        extend_selection, start_index, end_index, true);
    return true;
  }
  return false;
}

jboolean WebContentsAccessibilityAndroid::PreviousAtGranularity(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint granularity,
    jboolean extend_selection,
    jint unique_id,
    jint cursor_index) {
  if (!root_manager_)
    return false;
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (!node)
    return false;

  jint start_index = -1;
  int end_index = -1;
  if (root_manager_->PreviousAtGranularity(granularity, cursor_index, node,
                                           &start_index, &end_index)) {
    Java_WebContentsAccessibility_finishGranularityMove(
        env, obj, base::android::ConvertUTF16ToJavaString(env, node->GetText()),
        extend_selection, start_index, end_index, false);
    return true;
  }
  return false;
}

void WebContentsAccessibilityAndroid::SetAccessibilityFocus(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint unique_id) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (node)
    node->manager()->SetAccessibilityFocus(*node);
}

bool WebContentsAccessibilityAndroid::IsSlider(JNIEnv* env,
                                               const JavaParamRef<jobject>& obj,
                                               jint unique_id) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (!node)
    return false;

  return node->GetRole() == ui::AX_ROLE_SLIDER;
}

void WebContentsAccessibilityAndroid::OnAutofillPopupDisplayed(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (!root_manager_ ||
      !base::FeatureList::IsEnabled(features::kAndroidAutofillAccessibility))
    return;

  BrowserAccessibility* current_focus = root_manager_->GetFocus();
  if (current_focus == nullptr) {
    return;
  }

  DeleteAutofillPopupProxy();

  g_autofill_popup_proxy_node = BrowserAccessibility::Create();
  g_autofill_popup_proxy_node_ax_node = new ui::AXNode(nullptr, -1, -1);
  ui::AXNodeData ax_node_data;
  ax_node_data.role = ui::AX_ROLE_MENU;
  ax_node_data.SetName("Autofill");
  ax_node_data.AddState(ui::AX_STATE_READ_ONLY);
  ax_node_data.AddState(ui::AX_STATE_FOCUSABLE);
  ax_node_data.AddState(ui::AX_STATE_SELECTABLE);
  g_autofill_popup_proxy_node_ax_node->SetData(ax_node_data);
  g_autofill_popup_proxy_node->Init(root_manager_,
                                    g_autofill_popup_proxy_node_ax_node);

  g_element_hosting_autofill_popup_unique_id = current_focus->unique_id();
}

void WebContentsAccessibilityAndroid::OnAutofillPopupDismissed(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  g_element_hosting_autofill_popup_unique_id = -1;
  g_element_after_element_hosting_autofill_popup_unique_id = -1;
  DeleteAutofillPopupProxy();
}

jint WebContentsAccessibilityAndroid::
    GetIdForElementAfterElementHostingAutofillPopup(
        JNIEnv* env,
        const JavaParamRef<jobject>& obj) {
  if (!base::FeatureList::IsEnabled(features::kAndroidAutofillAccessibility) ||
      g_element_after_element_hosting_autofill_popup_unique_id == -1 ||
      GetAXFromUniqueID(
          g_element_after_element_hosting_autofill_popup_unique_id) == nullptr)
    return 0;

  return g_element_after_element_hosting_autofill_popup_unique_id;
}

jboolean WebContentsAccessibilityAndroid::IsAutofillPopupNode(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint unique_id) {
  return g_autofill_popup_proxy_node &&
         g_autofill_popup_proxy_node->unique_id() == unique_id;
}

bool WebContentsAccessibilityAndroid::Scroll(JNIEnv* env,
                                             const JavaParamRef<jobject>& obj,
                                             jint unique_id,
                                             int direction) {
  BrowserAccessibilityAndroid* node = GetAXFromUniqueID(unique_id);
  if (!node)
    return false;

  return node->Scroll(direction);
}

BrowserAccessibilityAndroid* WebContentsAccessibilityAndroid::GetAXFromUniqueID(
    int32_t unique_id) {
  return static_cast<BrowserAccessibilityAndroid*>(
      BrowserAccessibility::GetFromUniqueID(unique_id));
}

void WebContentsAccessibilityAndroid::UpdateFrameInfo() {
  if (frame_info_initialized_)
    return;

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  Java_WebContentsAccessibility_notifyFrameInfoInitialized(env, obj);
  frame_info_initialized_ = true;
}

bool RegisterWebContentsAccessibilityAndroid(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

jlong Init(JNIEnv* env,
           const JavaParamRef<jobject>& obj,
           const JavaParamRef<jobject>& jweb_contents) {
  WebContents* web_contents = WebContents::FromJavaWebContents(jweb_contents);
  DCHECK(web_contents);

  return reinterpret_cast<intptr_t>(
      new WebContentsAccessibilityAndroid(env, obj, web_contents));
}

}  // namespace content
