#include "content/browser/media/session/audio_focus_delegate.h"

namespace content {

// AudioFocusDelegateDefault is the default implementation of
// AudioFocusDelegate which only handles audio focus between WebContents.
class AudioFocusDelegateDefault : public AudioFocusDelegate {
 public:
  explicit AudioFocusDelegateDefault(MediaSessionImpl* media_session);
  ~AudioFocusDelegateDefault() override;

  // AudioFocusDelegate implementation.
  bool RequestAudioFocus(
      AudioFocusManager::AudioFocusType audio_focus_type) override;
  void AbandonAudioFocus() override;

 private:
  // Weak pointer because |this| is owned by |media_session_|.
  MediaSessionImpl* media_session_;
};

}  // namespace content
