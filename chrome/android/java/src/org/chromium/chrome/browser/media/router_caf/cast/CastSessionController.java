package org.chromium.chrome.browser.media.router_caf.cast;

import static org.chromium.chrome.browser.media.router_caf.cast.CastMessageHandler.INVALID_SEQUENCE_NUMBER;

import org.chromium.base.Log;
import com.google.android.gms.cast.ApplicationMetadata;
import com.google.android.gms.cast.CastDevice;
import com.google.android.gms.cast.Cast;
import com.google.android.gms.cast.framework.CastSession;
import com.google.android.gms.common.api.ResultCallback;
import com.google.android.gms.common.api.Status;
import org.json.JSONObject;
import org.json.JSONArray;
import org.json.JSONException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

// Have the same lifecycle with CastSession.
public class CastSessionController {
  private static final String TAG = "CastSessionController";

    // The value is borrowed from the Android Cast SDK code to match their behavior.
    private static final double MIN_VOLUME_LEVEL_DELTA = 1e-7;

  /**
     * The return type for {@link handleVolumeMessage}.
     */
    static class HandleVolumeMessageResult {
        public final boolean mSucceeded;
        public final boolean mShouldWaitForVolumeChange;

        /**
         * Initializes a {@link HandleVolumeMessageResult}.
         */
        public HandleVolumeMessageResult(boolean succeeded, boolean shouldWaitForVolumeChange) {
            mSucceeded = succeeded;
            mShouldWaitForVolumeChange = shouldWaitForVolumeChange;
        }
    }

      private class CastMessagingChannel implements Cast.MessageReceivedCallback {

        public CastMessagingChannel() {}

        @Override
        public void onMessageReceived(CastDevice castDevice, String namespace, String message) {
            Log.d(TAG, "Received message from Cast device: namespace=\"" + namespace
                       + "\" message=\"" + message + "\"");
            mMessageHandler.onMessageReceived(namespace, message);
        }
    }

  private final CastSession mCastSession;
  private final CastMediaRouteProvider mProvider;
  private final MediaSink mSink;
  private final MediaSource mSource;
  private final CastMessageHandler mMessageHandler;
  private final CastMessagingChannel mMessageChannel;
  private final CastListener mCastListener;
  private List<String> mRegisteredNamespaces;

  public CastSessionController(CastSession castSession, CastMediaRouteProvider provider, MediaSink sink, MediaSource source) {
    mCastSession = castSession;
    mProvider = provider;
    mSink = sink;
    mSource = source;
    mMessageHandler = new CastMessageHandler(mProvider, this);
    mMessageChannel = new CastMessagingChannel();
    mCastListener = new CastListener();
    mRegisteredNamespaces = new ArrayList<>();
  }

  public MediaSource getSource() {
    return mSource;
  }

  public MediaSink getSink() {
    return mSink;
  }

  public CastSession getSession() {
    return mCastSession;
  }

  public List<String> getRegisteredNamespaces() {
    return mRegisteredNamespaces;
  }

  public void updateNamespaces() {
    if (!mCastSession.isConnected()) return;
    ApplicationMetadata applicationMetadata = mCastSession.getApplicationMetadata();
    if (applicationMetadata == null) return;

    List<String> supportedNamespaces = applicationMetadata.getSupportedNamespaces();

    List<String> namespacesToRegister = new ArrayList<>(supportedNamespaces);
    namespacesToRegister.removeAll(mRegisteredNamespaces);

    List<String> namespacesToUnregister = new ArrayList<>(mRegisteredNamespaces);
    namespacesToUnregister.removeAll(supportedNamespaces);

    try {
      for (String namespace : namespacesToRegister) {
        mCastSession.setMessageReceivedCallbacks(namespace, mMessageChannel);
      }
      for (String namespace : namespacesToUnregister) {
        mCastSession.removeMessageReceivedCallbacks(namespace);
      }
    } catch (IOException e) {
      Log.e(TAG, "Failed to update message callbacks for supported namespaces", e);
    }
  }

  public boolean handleSessionMessage(JSONObject message) throws JSONException {
    return mMessageHandler.handleSessionMessage(message);
  }

  public void sendClientMessageTo(String clientId, String type, String message, int sequenceNumber) {
    mMessageHandler.sendClientMessageTo(clientId, type, message, sequenceNumber);
  }

  /**
     * @return A message containing the information of the {@link CastSession}.
     */
    public String buildSessionMessage() {
      if (!mCastSession.isConnected()) return "{}";
      try {
          CastDevice castDevice = mCastSession.getCastDevice();
            // "volume" is a part of "receiver" initialized below.
            JSONObject jsonVolume = new JSONObject();
            jsonVolume.put("level", mCastSession.getVolume());
            jsonVolume.put("muted", mCastSession.isMute());

            // "receiver" is a part of "message" initialized below.
            JSONObject jsonReceiver = new JSONObject();
            jsonReceiver.put("label", castDevice.getDeviceId());
            jsonReceiver.put("friendlyName", castDevice.getFriendlyName());
            jsonReceiver.put("capabilities", toJSONArray(CastUtils.getCapabilities(castDevice)));
            jsonReceiver.put("volume", jsonVolume);
            jsonReceiver.put("isActiveInput", mCastSession.getActiveInputState());
            jsonReceiver.put("displayStatus", null);
            jsonReceiver.put("receiverType", "cast");

            JSONArray jsonNamespaces = new JSONArray();

            ApplicationMetadata applicationMetadata = mCastSession.getApplicationMetadata();
            if (applicationMetadata != null) {
              for (String namespace : applicationMetadata.getSupportedNamespaces()) {
                JSONObject jsonNamespace = new JSONObject();
                jsonNamespace.put("name", namespace);
                jsonNamespaces.put(jsonNamespace);
              }
            }

            JSONObject jsonMessage = new JSONObject();
            jsonMessage.put("sessionId", mCastSession.getSessionId());
            jsonMessage.put("statusText", mCastSession.getApplicationStatus());
            jsonMessage.put("receiver", jsonReceiver);
            jsonMessage.put("namespaces", jsonNamespaces);
            // TODO(zqzhang): update Media?
            jsonMessage.put("media", toJSONArray(new ArrayList<String>()));
            jsonMessage.put("status", "connected");
            jsonMessage.put("transportId", "web-4");
            jsonMessage.put("appId", (applicationMetadata != null)
                ? applicationMetadata.getApplicationId()
                : mSource.getApplicationId());
            jsonMessage.put("displayName", (applicationMetadata != null)
                ? applicationMetadata.getName()
                : castDevice.getFriendlyName());

            return jsonMessage.toString();
        } catch (JSONException e) {
            Log.w(TAG, "Building session message failed", e);
            return "{}";
        }
    }

    private JSONArray toJSONArray(List<String> from) throws JSONException {
        JSONArray result = new JSONArray();
        for (String entry : from) {
            result.put(entry);
        }
        return result;
    }

  public void onSessionStarted() {
        for (ClientRecord client : mProvider.getClientRecords().values()) {
            if (!client.isConnected) continue;

            onClientConnected(client.clientId);
        }
        mCastSession.addCastListener(mCastListener);
  }

  public void onSessionEnded() {
    mCastSession.removeCastListener(mCastListener);
  }

  public void endSession() {
        CastSession currentCastSession = CastUtils.getCastContext().getSessionManager().getCurrentCastSession();
        if (currentCastSession == mCastSession) {
          CastUtils.getCastContext().getSessionManager().endCurrentSession(true);
        }
  }

  private void onClientConnected(String clientId) {
    sendClientMessageTo(clientId, "new_session", buildSessionMessage(),
        INVALID_SEQUENCE_NUMBER);
  }

  public void notifyReceiverAction(
            String routeId, MediaSink sink, String clientId, String action) {
    mMessageHandler.notifyReceiverAction(routeId, sink, clientId, action);
  }

  public boolean sendStringCastMessage(
                  final String message,
            final String namespace,
            final String clientId,
            final int sequenceNumber) {
        if (!mCastSession.isConnected()) return false;
        Log.d(TAG, "Sending message to Cast device in namespace %s: %s", namespace, message);

        try {
            mCastSession.sendMessage(namespace, message)
                    .setResultCallback(
                            new ResultCallback<Status>() {
                                @Override
                                public void onResult(Status result) {
                                    if (!result.isSuccess()) {
                                        // TODO(avayvod): should actually report back to the page.
                                        // See https://crbug.com/550445.
                                        Log.e(TAG, "Failed to send the message: " + result);
                                        return;
                                    }

                                    // Media commands wait for the media status update as a result.
                                    if (CastMessageHandler.MEDIA_NAMESPACE.equals(namespace)) return;

                                    // App messages wait for the empty message with the sequence
                                    // number.
                                    mMessageHandler.onAppMessageSent(clientId, sequenceNumber);
                                }
                            });
        } catch (Exception e) {
            Log.e(TAG, "Exception while sending message", e);
            return false;
        }
    return true;
  }

  public void onMediaMessage(String message) {
  }

  public void onApplicationStopped() {
    // TODO(zqzhang): handle this logic.
  }

    // SET_VOLUME messages have a |level| and |muted| properties. One of them is
    // |null| and the other one isn't. |muted| is expected to be a boolean while
    // |level| is a float from 0.0 to 1.0.
    // Example:
    // {
    //   "volume" {
    //     "level": 0.9,
    //     "muted": null
    //   }
    // }
  // TODO(zqzhang): merge this into CastMessageHandler.
    public HandleVolumeMessageResult handleVolumeMessage(
            JSONObject volume, final String clientId, final int sequenceNumber)
            throws JSONException {
        if (volume == null) return new HandleVolumeMessageResult(false, false);

        if (!mCastSession.isConnected()) return new HandleVolumeMessageResult(false, false);

        boolean waitForVolumeChange = false;
        try {
            if (!volume.isNull("muted")) {
                boolean newMuted = volume.getBoolean("muted");
                if (mCastSession.isMute() != newMuted) {
                    mCastSession.setMute(newMuted);
                    waitForVolumeChange = true;
                }
            }
            if (!volume.isNull("level")) {
                double newLevel = volume.getDouble("level");
                double currentLevel = mCastSession.getVolume();
                if (!Double.isNaN(currentLevel)
                        && Math.abs(currentLevel - newLevel) > MIN_VOLUME_LEVEL_DELTA) {
                    mCastSession.setVolume(newLevel);
                    waitForVolumeChange = true;
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "Failed to send volume command: " + e);
            return new HandleVolumeMessageResult(false, false);
        }

        return new HandleVolumeMessageResult(true, waitForVolumeChange);
    }

  private class CastListener extends Cast.Listener {
    @Override
    public void onApplicationMetadataChanged(ApplicationMetadata applicationMetadata) {
      updateNamespaces();
    }

    @Override
    public void onApplicationStatusChanged() {
      updateNamespaces();
    }
  }
}
