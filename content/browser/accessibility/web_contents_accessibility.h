// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_ROOT_ANDROID_H_
#define CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_ROOT_ANDROID_H_

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "content/browser/accessibility/browser_accessibility_manager_android.h"
#include "content/browser/android/render_widget_host_connector.h"

namespace content {

class WebContents;

// Owned by RenderFrameHostImpl

class CONTENT_EXPORT WebContentsAccessibility
    : public BrowserAccessibilityManagerAndroid,
      public RenderWidgetHostConnector {
 public:
  WebContentsAccessibility(JNIEnv* env,
                           const base::android::JavaParamRef<jobject>& obj,
                           WebContents* web_contents);
  ~WebContentsAccessibility() override;

  static ui::AXTreeUpdate GetEmptyDocument();

  // RendetWidgetHostConnector implementation.
  void UpdateRenderProcessConnection(
      RenderWidgetHostViewAndroid* old_rwhva,
      RenderWidgetHostViewAndroid* new_rhwva) override;

  void HandlePageLoaded(int32_t id);
  void HandleContentChanged(int32_t id);
  void HandleFocusChanged(int32_t id);
  void HandleCheckStateChanged(int32_t id);
  void HandleClicked(int32_t id);
  void HandleScrollPositionChanged(int32_t id);
  void HandleScrolledToAnchor(int32_t id);
  void AnnounceLiveRegionText(const base::string16& text);
  void HandleTextSelectionChanged(int32_t id);
  void HandleEditableTextChanged(int32_t id);
  void HandleSliderChanged(int32_t id);
  void SendDelayedWindowContentChangedEvent();
  void HandleHover(int32_t id);
  void HandleNavigate();
  bool ShouldRespectDisplayedPasswordTextJni();
  bool ShouldExposePasswordTextJni();

  // --------------------------------------------------------------------------
  // Methods called from Java via JNI
  // --------------------------------------------------------------------------

  // Global methods.
  base::android::ScopedJavaLocalRef<jstring> GetSupportedHtmlElementTypes(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);

  // Tree methods.
  jint GetRootId(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  jboolean IsNodeValid(JNIEnv* env,
                       const base::android::JavaParamRef<jobject>& obj,
                       jint id);

  void HitTest(JNIEnv* env,
               const base::android::JavaParamRef<jobject>& obj,
               jint x,
               jint y);

  // Methods to get information about a specific node.
  jboolean IsEditableText(JNIEnv* env,
                          const base::android::JavaParamRef<jobject>& obj,
                          jint id);
  jboolean IsFocused(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& obj,
                     jint id);
  jint GetEditableTextSelectionStart(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jint id);
  jint GetEditableTextSelectionEnd(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jint id);

  // Populate Java accessibility data structures with info about a node.
  jboolean PopulateAccessibilityNodeInfo(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& info,
      jint id);
  jboolean PopulateAccessibilityEvent(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& event,
      jint id,
      jint event_type);

  // Perform actions.
  void Click(JNIEnv* env,
             const base::android::JavaParamRef<jobject>& obj,
             jint id);
  void Focus(JNIEnv* env,
             const base::android::JavaParamRef<jobject>& obj,
             jint id);
  void Blur(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void ScrollToMakeNodeVisible(JNIEnv* env,
                               const base::android::JavaParamRef<jobject>& obj,
                               jint id);
  void SetTextFieldValue(JNIEnv* env,
                         const base::android::JavaParamRef<jobject>& obj,
                         jint id,
                         const base::android::JavaParamRef<jstring>& value);
  void SetSelection(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj,
                    jint id,
                    jint start,
                    jint end);
  jboolean AdjustSlider(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& obj,
                        jint id,
                        jboolean increment);
  void ShowContextMenu(JNIEnv* env,
                       const base::android::JavaParamRef<jobject>& obj,
                       jint id);

  // Return the id of the next node in tree order in the direction given by
  // |forwards|, starting with |start_id|, that matches |element_type|,
  // where |element_type| is a special uppercase string from TalkBack or
  // BrailleBack indicating general categories of web content like
  // "SECTION" or "CONTROL".  Return 0 if not found.
  jint FindElementType(JNIEnv* env,
                       const base::android::JavaParamRef<jobject>& obj,
                       jint start_id,
                       const base::android::JavaParamRef<jstring>& element_type,
                       jboolean forwards);

  // Respond to a ACTION_[NEXT/PREVIOUS]_AT_MOVEMENT_GRANULARITY action
  // and move the cursor/selection within the given node id. We keep track
  // of our own selection in BrowserAccessibilityManager.java for static
  // text, but if this is an editable text node, updates the selected text
  // in Blink, too, and either way calls
  // Java_BrowserAccessibilityManager_finishGranularityMove with the
  // result.
  jboolean NextAtGranularity(JNIEnv* env,
                             const base::android::JavaParamRef<jobject>& obj,
                             jint granularity,
                             jboolean extend_selection,
                             jint id,
                             jint cursor_index);
  jboolean PreviousAtGranularity(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jint granularity,
      jboolean extend_selection,
      jint id,
      jint cursor_index);

  // Helper functions to compute the next start and end index when moving
  // forwards or backwards by character, word, or line. This part is
  // unit-tested; the Java interfaces above are just wrappers. Both of these
  // take a single cursor index as input and return the boundaries surrounding
  // the next word or line. If moving by character, the output start and
  // end index will be the same.
  bool NextAtGranularity(int32_t granularity,
                         int cursor_index,
                         BrowserAccessibilityAndroid* node,
                         int32_t* start_index,
                         int32_t* end_index);
  bool PreviousAtGranularity(int32_t granularity,
                             int cursor_index,
                             BrowserAccessibilityAndroid* node,
                             int32_t* start_index,
                             int32_t* end_index);

  // Set accessibility focus. This sends a message to the renderer to
  // asynchronously load inline text boxes for this node only, enabling more
  // accurate movement by granularities on this node.
  void SetAccessibilityFocus(JNIEnv* env,
                             const base::android::JavaParamRef<jobject>& obj,
                             jint id);

  // Returns true if the object is a slider.
  bool IsSlider(JNIEnv* env,
                const base::android::JavaParamRef<jobject>& obj,
                jint id);

  // Accessibility methods to support navigation for autofill popup.
  void OnAutofillPopupDisplayed(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void OnAutofillPopupDismissed(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  jint GetIdForElementAfterElementHostingAutofillPopup(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  jboolean IsAutofillPopupNode(JNIEnv* env,
                               const base::android::JavaParamRef<jobject>& obj,
                               jint id);

  // Scrolls any scrollable container by about 80% of one page in the
  // given direction.
  bool Scroll(JNIEnv* env,
              const base::android::JavaParamRef<jobject>& obj,
              jint id,
              int direction);

  void UpdateFrameInfo();

 private:
  BrowserAccessibilityAndroid* GetFromUniqueID(int32_t unique_id);

  WebContentsAccessibility* GetManagerJni() override;

  // This gives BrowserAccessibilityManager::Create access to the class
  // constructor.
  friend class BrowserAccessibilityManager;

  // A weak reference to the Java BrowserAccessibilityManager object.
  JavaObjectWeakGlobalRef java_ref_;

  bool frame_info_initialized_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsAccessibility);
};

bool RegisterWebContentsAccessibility(JNIEnv* env);
}

#endif  // CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_ROOT_ANDROID_H_
