// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_UI_SCENE_MANAGER_H_
#define CHROME_BROWSER_VR_UI_SCENE_MANAGER_H_

#include "base/macros.h"
#include "base/values.h"
#include "chrome/browser/vr/browser_ui_interface.h"
#include "chrome/browser/vr/ui_element_renderer.h"
#include "chrome/browser/vr/ui_unsupported_mode.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace vr {

class ContentInputDelegate;
class TransientElement;
class UiBrowserInterface;
class UiScene;
struct Model;

// The scene manager creates our scene hierarchy.
class UiSceneManager {
 public:
  UiSceneManager(UiBrowserInterface* browser,
                 UiScene* scene,
                 ContentInputDelegate* content_input_delegate,
                 Model* model);
  ~UiSceneManager();

 private:
  void Create2dBrowsingSubtreeRoots(Model* model);
  void CreateWebVrRoot(Model* model);
  void CreateWebVRExitWarning(Model* model);
  void CreateSystemIndicators(Model* model);
  void CreateContentQuad(ContentInputDelegate* delegate, Model* model);
  void CreateSplashScreen(Model* model);
  void CreateUnderDevelopmentNotice(Model* model);
  void CreateBackground(Model* model);
  void CreateViewportAwareRoot();
  void CreateUrlBar(Model* model);
  void CreateSuggestionList(Model* model);
  void CreateWebVrUrlToast(Model* model);
  void CreateCloseButton(Model* model);
  void CreateExitPrompt(Model* model);
  void CreateAudioPermissionPrompt(Model* model);
  void CreateToasts(Model* model);
  void CreateVoiceSearchUiGroup(Model* model);
  void CreateController(Model* model);

  TransientElement* AddTransientParent(UiElementName name,
                                       UiElementName parent_name,
                                       int timeout_seconds,
                                       bool animate_opacity);

  UiBrowserInterface* browser_;
  UiScene* scene_;

  DISALLOW_COPY_AND_ASSIGN(UiSceneManager);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_UI_SCENE_MANAGER_H_
