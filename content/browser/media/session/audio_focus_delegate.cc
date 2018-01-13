#include "content/browser/media/session/audio_focus_delegate.h"
#include "content/browser/media/session/audio_focus_delegate_android.h"
#include "content/browser/media/session/audio_focus_delegate_default.h"

namespace content {

// static
std::unique_ptr<AudioFocusDelegate> AudioFocusDelegate::Create(
    MediaSessionImpl* media_session) {
  if (true) {
    return std::unique_ptr<AudioFocusDelegate>(
        new AudioFocusDelegateDefault(media_session));
  } else {
    return std::unique_ptr<AudioFocusDelegate>(
        new AudioFocusDelegateAndroid(media_session));
  }
}

}  // content
