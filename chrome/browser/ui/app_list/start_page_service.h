// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_START_PAGE_SERVICE_H_
#define CHROME_BROWSER_UI_APP_LIST_START_PAGE_SERVICE_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "base/time/default_clock.h"
#include "chrome/browser/ui/app_list/speech_recognizer_delegate.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/app_list/speech_ui_model_observer.h"

namespace content {
struct SpeechRecognitionSessionPreamble;
}

namespace extensions {
class Extension;
}

class Profile;

namespace app_list {

class RecommendedApps;
class SpeechAuthHelper;
class SpeechRecognizer;
class StartPageObserver;

// StartPageService collects data to be displayed in app list's start page
// and hosts the start page contents.
class StartPageService : public KeyedService,
                         public SpeechRecognizerDelegate {
 public:
  typedef std::vector<scoped_refptr<const extensions::Extension> >
      ExtensionList;
  // Gets the instance for the given profile.
  static StartPageService* Get(Profile* profile);

  void AddObserver(StartPageObserver* observer);
  void RemoveObserver(StartPageObserver* observer);

  void AppListShown();
  void AppListHidden();
  void ToggleSpeechRecognition(
      const scoped_refptr<content::SpeechRecognitionSessionPreamble>& preamble);

  // Called when the WebUI has finished loading.
  void WebUILoaded();

  // Returns true if the hotword is enabled in the app-launcher.
  bool HotwordEnabled();

  // They return essentially the same web contents but might return NULL when
  // some flag disables the feature.
  content::WebContents* GetStartPageContents();
  content::WebContents* GetSpeechRecognitionContents();

  RecommendedApps* recommended_apps() { return recommended_apps_.get(); }
  Profile* profile() { return profile_; }
  SpeechRecognitionState state() { return state_; }

  // Overridden from app_list::SpeechRecognizerDelegate:
  void OnSpeechResult(const base::string16& query, bool is_final) override;
  void OnSpeechSoundLevelChanged(int16_t level) override;
  void OnSpeechRecognitionStateChanged(
      SpeechRecognitionState new_state) override;
  void GetSpeechAuthParameters(std::string* auth_scope,
                               std::string* auth_token) override;

 protected:
  // Protected for testing.
  explicit StartPageService(Profile* profile);
  ~StartPageService() override;

 private:
  friend class StartPageServiceFactory;

  // ProfileDestroyObserver to shutdown the service on exiting. WebContents
  // depends on the profile and needs to be closed before the profile and its
  // keyed service shutdown.
  class ProfileDestroyObserver;

  // The WebContentsDelegate implementation for the start page. This allows
  // getUserMedia() request from the web contents.
  class StartPageWebContentsDelegate;

#if defined(OS_CHROMEOS)
  // This class observes the change of audio input device availability and
  // checks if currently the system has valid audio input.
  class AudioStatus;
#endif

  // This class observes network change events and disables/enables voice search
  // based on network connectivity.
  class NetworkChangeObserver;

  void LoadContents();
  void UnloadContents();

  // KeyedService overrides:
  void Shutdown() override;

  // Change the known microphone availability. |available| should be true if
  // the microphone exists and is available for use.
  void OnMicrophoneChanged(bool available);
  // Change the known network connectivity state. |available| should be true if
  // at least one network is connected to.
  void OnNetworkChanged(bool available);
  // Enables/disables voice recognition based on network and microphone state.
  void UpdateRecognitionState();

  Profile* profile_;
  scoped_ptr<content::WebContents> contents_;
  scoped_ptr<StartPageWebContentsDelegate> contents_delegate_;
  scoped_ptr<ProfileDestroyObserver> profile_destroy_observer_;
  scoped_ptr<RecommendedApps> recommended_apps_;
  SpeechRecognitionState state_;
  ObserverList<StartPageObserver> observers_;
  bool speech_button_toggled_manually_;
  bool speech_result_obtained_;

  bool webui_finished_loading_;
  std::vector<base::Closure> pending_webui_callbacks_;

  base::DefaultClock clock_;
  scoped_ptr<SpeechRecognizer> speech_recognizer_;
  scoped_ptr<SpeechAuthHelper> speech_auth_helper_;

  bool network_available_;
  bool microphone_available_;
#if defined(OS_CHROMEOS)
  scoped_ptr<AudioStatus> audio_status_;
#endif
  scoped_ptr<NetworkChangeObserver> network_change_observer_;

  base::WeakPtrFactory<StartPageService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(StartPageService);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_START_PAGE_SERVICE_H_
