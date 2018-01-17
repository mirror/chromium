package org.chromium.chrome.browser.media.router_caf.cast;

import android.os.Handler;
import android.support.v7.media.MediaRouteSelector;
import android.support.v7.media.MediaRouter;
import android.support.v7.media.MediaRouter.RouteInfo;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import org.chromium.base.Log;
import org.chromium.chrome.browser.media.router_caf.DiscoveryCallback;
import org.chromium.chrome.browser.media.router_caf.DiscoveryDelegate;
import org.chromium.chrome.browser.media.router_caf.MediaRoute;
import org.chromium.chrome.browser.media.router_caf.MediaRouteProvider;
import org.chromium.chrome.browser.media.router_caf.MediaRouteManager;
import org.chromium.chrome.browser.media.router_caf.CafMediaRouter;
import org.chromium.chrome.browser.media.router.cast.CastOptionsProvider;

import com.google.android.gms.cast.CastDevice;
import com.google.android.gms.cast.framework.CastContext;
import com.google.android.gms.cast.framework.CastSession;
import com.google.android.gms.cast.framework.SessionManagerListener;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.json.JSONObject;
import org.json.JSONException;

public class CastMediaRouteProvider
        implements MediaRouteProvider, DiscoveryDelegate, SessionManagerListener<CastSession> {
    private static final String TAG = "zqzhang";

    private static final String AUTO_JOIN_PRESENTATION_ID = "auto-join";
    private static final String PRESENTATION_ID_SESSION_ID_PREFIX = "cast-session_";

    protected static final List<MediaSink> NO_SINKS = Collections.emptyList();

    protected final MediaRouter mAndroidMediaRouter;
    protected final MediaRouteManager mManager;
    protected final Map<String, DiscoveryCallback> mDiscoveryCallbacks =
            new HashMap<String, DiscoveryCallback>();
    protected final Map<String, MediaRoute> mRoutes = new HashMap<String, MediaRoute>();
    protected Handler mHandler = new Handler();

    private ClientRecord mLastRemovedRouteRecord;
    private final Map<String, ClientRecord> mClientRecords = new HashMap<String, ClientRecord>();

    private CastSessionController mSessionController;

    private CreateRouteRequestInfo mPendingCreateRouteRequestInfo;

    public static CastMediaRouteProvider create(MediaRouteManager manager) {
        return new CastMediaRouteProvider(CafMediaRouter.getAndroidMediaRouter(), manager);
    }

    CastMediaRouteProvider(MediaRouter androidMediaRouter, MediaRouteManager manager) {
        mAndroidMediaRouter = androidMediaRouter;
        mManager = manager;
        CastUtils.getCastContext().getSessionManager()
            .addSessionManagerListener(this, CastSession.class);
    }

    public Map<String, ClientRecord> getClientRecords() {
        return mClientRecords;
    }

    public Set<String> getClientIds() {
        return mClientRecords.keySet();
    }

    @Override
    public boolean supportsSource(@NonNull String sourceId) {
        return getSourceFromId(sourceId) != null;
    }

    @Override
    public void startObservingMediaSinks(@NonNull String sourceId) {
        Log.d(TAG, "startObservingMediaSinks: " + sourceId);

        if (mAndroidMediaRouter == null) {
            // If the MediaRouter API is not available, report no devices so the page doesn't even
            // try to cast.
            onSinksReceived(sourceId, NO_SINKS);
            return;
        }

        MediaSource source = getSourceFromId(sourceId);
        if (source == null) {
            // If the source is invalid or not supported by this provider, report no devices
            // available.
            onSinksReceived(sourceId, NO_SINKS);
            return;
        }

        MediaRouteSelector routeSelector = source.buildRouteSelector();
        if (routeSelector == null) {
            // If the application invalid, report no devices available.
            onSinksReceived(sourceId, NO_SINKS);
            return;
        }

        // No-op, if already monitoring the application for this source.
        String applicationId = source.getApplicationId();
        DiscoveryCallback callback = mDiscoveryCallbacks.get(applicationId);
        if (callback != null) {
            callback.addSourceUrn(sourceId);
            return;
        }

        List<MediaSink> knownSinks = new ArrayList<MediaSink>();
        for (RouteInfo route : mAndroidMediaRouter.getRoutes()) {
            if (route.matchesSelector(routeSelector)) {
                knownSinks.add(MediaSink.fromRoute(route));
            }
        }

        callback = new DiscoveryCallback(sourceId, knownSinks, this, routeSelector);
        mAndroidMediaRouter.addCallback(
                routeSelector, callback, MediaRouter.CALLBACK_FLAG_REQUEST_DISCOVERY);
        mDiscoveryCallbacks.put(applicationId, callback);
    }

    @Override
    public void stopObservingMediaSinks(@NonNull String sourceId) {
        Log.d(TAG, "stopObservingMediaSinks: " + sourceId);
        if (mAndroidMediaRouter == null) return;

        MediaSource source = getSourceFromId(sourceId);
        if (source == null) return;

        String applicationId = source.getApplicationId();
        DiscoveryCallback callback = mDiscoveryCallbacks.get(applicationId);
        if (callback == null) return;

        callback.removeSourceUrn(sourceId);

        if (callback.isEmpty()) {
            mAndroidMediaRouter.removeCallback(callback);
            mDiscoveryCallbacks.remove(applicationId);
        }
    }

    protected MediaSource getSourceFromId(String sourceId) {
        return CastMediaSource.from(sourceId);
    }

    /**
     * Forward the sinks back to the native counterpart.
     */
    protected void onSinksReceivedInternal(String sourceId, @NonNull List<MediaSink> sinks) {
        Log.d(TAG, "Reporting %d sinks for source: %s", sinks.size(), sourceId);
        mManager.onSinksReceived(sourceId, this, sinks);
    }

    /**
     * {@link DiscoveryDelegate} implementation.
     */
    @Override
    public void onSinksReceived(String sourceId, @NonNull List<MediaSink> sinks) {
        Log.d(TAG, "Received %d sinks for sourceId: %s", sinks.size(), sourceId);
        mHandler.post(() -> { onSinksReceivedInternal(sourceId, sinks); });
    }

    @Override
    public void sendStringMessage(String routeId, String message, int nativeCallbackId) {
        Log.d(TAG, "Received message from client: %s", message);

        if (!mRoutes.containsKey(routeId)) {
            mManager.onMessageSentResult(false, nativeCallbackId);
            return;
        }

        boolean success = false;
        try {
            JSONObject jsonMessage = new JSONObject(message);

            String messageType = jsonMessage.getString("type");
            // TODO(zqzhang): Move the handling of "client_connect", "client_disconnect" and
            // "leave_session" from CastMRP to CastMessageHandler. Also, need to have a
            // ClientManager for client managing.
            if ("client_connect".equals(messageType)) {
                success = handleClientConnectMessage(jsonMessage);
            } else if ("client_disconnect".equals(messageType)) {
                success = handleClientDisconnectMessage(jsonMessage);
            } else if ("leave_session".equals(messageType)) {
                success = handleLeaveSessionMessage(jsonMessage);
            } else if (mSessionController != null) {
                success = mSessionController.handleSessionMessage(jsonMessage);
            }
        } catch (JSONException e) {
            Log.e(TAG, "JSONException while handling internal message: " + e);
            success = false;
        }

        mManager.onMessageSentResult(success, nativeCallbackId);
    }

    private boolean handleClientConnectMessage(JSONObject jsonMessage) throws JSONException {
        String clientId = jsonMessage.getString("clientId");
        if (clientId == null) return false;

        ClientRecord clientRecord = mClientRecords.get(clientId);
        if (clientRecord == null) return false;

        clientRecord.isConnected = true;
        if (mSessionController != null) {
            mSessionController.sendClientMessageTo(clientId, "new_session",
                    mSessionController.buildSessionMessage(),
                    CastMessageHandler.INVALID_SEQUENCE_NUMBER);
            // TODO(zqzhang): Add RemoteMediaClient.
        }

        if (clientRecord.pendingMessages.size() == 0) return true;
        for (String message : clientRecord.pendingMessages) {
            Log.d(TAG, "Deqeueing message for client %s: %s", clientId, message);
            mManager.onMessage(clientRecord.routeId, message);
        }
        clientRecord.pendingMessages.clear();

        return true;
    }

    private boolean handleClientDisconnectMessage(JSONObject jsonMessage) throws JSONException {
        String clientId = jsonMessage.getString("clientId");
        if (clientId == null) return false;

        ClientRecord client = mClientRecords.get(clientId);
        if (client == null) return false;

        mRoutes.remove(client.routeId);
        removeClient(client);

        mManager.onRouteClosed(client.routeId);

        return true;
    }

    private boolean handleLeaveSessionMessage(JSONObject jsonMessage) throws JSONException {
        String clientId = jsonMessage.getString("clientId");
        if (clientId == null || mSessionController == null) return false;

        String sessionId = jsonMessage.getString("message");
        if (!mSessionController.getSession().getSessionId().equals(sessionId)) return false;

        ClientRecord leavingClient = mClientRecords.get(clientId);
        if (leavingClient == null) return false;

        int sequenceNumber = jsonMessage.optInt("sequenceNumber", -1);
        mSessionController.sendClientMessageTo(clientId, "leave_session", null, sequenceNumber);

        // Send a "disconnect_session" message to all the clients that match with the leaving
        // client's auto join policy.
        for (ClientRecord client : mClientRecords.values()) {
            if ((CastMediaSource.AUTOJOIN_TAB_AND_ORIGIN_SCOPED.equals(leavingClient.autoJoinPolicy)
                        && isSameOrigin(client.origin, leavingClient.origin)
                        && client.tabId == leavingClient.tabId)
                    || (CastMediaSource.AUTOJOIN_ORIGIN_SCOPED.equals(leavingClient.autoJoinPolicy)
                               && isSameOrigin(client.origin, leavingClient.origin))) {
                mSessionController.sendClientMessageTo(client.clientId, "disconnect_session",
                        sessionId, CastMessageHandler.INVALID_SEQUENCE_NUMBER);
            }
        }

        return true;
    }

    @Override
    public void detachRoute(String routeId) {
        mRoutes.remove(routeId);

        removeClient(getClientRecordByRouteId(routeId));
    }

    @Override
    public void closeRoute(String routeId) {
        MediaRoute route = mRoutes.get(routeId);
        if (route == null) return;

        if (mSessionController == null) {
            mRoutes.remove(routeId);
            mManager.onRouteClosed(routeId);
            return;
        }

        ClientRecord client = getClientRecordByRouteId(routeId);
        if (client != null && mAndroidMediaRouter != null) {
            MediaSink sink =
                    MediaSink.fromSinkId(mSessionController.getSink().getId(), mAndroidMediaRouter);
            if (sink != null) {
                mSessionController.notifyReceiverAction(routeId, sink, client.clientId, "stop");
            }
        }

        mSessionController.endSession();
    }

    @Override
    public void joinRoute(
            String sourceId, String presentationId, String origin, int tabId, int nativeRequestId) {
        CastMediaSource source = CastMediaSource.from(sourceId);
        if (source == null || source.getClientId() == null) {
            mManager.onRouteRequestError("Unsupported presentation URL", nativeRequestId);
            return;
        }

        if (!hasSession()) {
            mManager.onRouteRequestError("No presentation", nativeRequestId);
            return;
        }

        if (!canJoinExistingSession(presentationId, origin, tabId, source)) {
            mManager.onRouteRequestError("No matching route", nativeRequestId);
            return;
        }

        MediaRoute route =
                new MediaRoute(mSessionController.getSink().getId(), sourceId, presentationId);
        addRoute(route, origin, tabId);
        mManager.onRouteCreated(route.id, route.sinkId, nativeRequestId, this, false);
    }

    @Override
    public void createRoute(String sourceId, String sinkId, String presentationId, String origin,
            int tabId, boolean isIncognito, int nativeRequestId) {
      Log.i(TAG, "createRoute");
      if (mPendingCreateRouteRequestInfo != null) {
            // TODO: do something.
        }
        if (mAndroidMediaRouter == null) {
            mManager.onRouteRequestError("Not supported", nativeRequestId);
            return;
        }

        MediaSink sink = MediaSink.fromSinkId(sinkId, mAndroidMediaRouter);
        if (sink == null) {
            mManager.onRouteRequestError("No sink", nativeRequestId);
            return;
        }

        MediaSource source = CastMediaSource.from(sourceId);
        if (source == null) {
            mManager.onRouteRequestError("Unsupported presentation URL", nativeRequestId);
            return;
        }

        mPendingCreateRouteRequestInfo = new CreateRouteRequestInfo(
                source, sink, presentationId, origin, tabId, isIncognito, nativeRequestId);

        CastUtils.getCastContext().setReceiverApplicationId(source.getApplicationId());

        for (MediaRouter.RouteInfo routeInfo : mAndroidMediaRouter.getRoutes()) {
            if (routeInfo.getId().equals(sink.getId())) {
                // Unselect and then select so that CAF will get notified of the selection.
                mAndroidMediaRouter.unselect(0);
                routeInfo.select();
                break;
            }
        }
    }

    @Override
    public void onSessionStarting(CastSession session) {
      Log.i(TAG, "onSessionStarting");
        mSessionController = new CastSessionController(session, this,
                mPendingCreateRouteRequestInfo.sink, mPendingCreateRouteRequestInfo.source);
        MediaRoute route = new MediaRoute(mPendingCreateRouteRequestInfo.sink.getId(),
                mPendingCreateRouteRequestInfo.source.getSourceId(),
                mPendingCreateRouteRequestInfo.presentationId);
        addRoute(
                route, mPendingCreateRouteRequestInfo.origin, mPendingCreateRouteRequestInfo.tabId);
        mManager.onRouteCreated(
                route.id, route.sinkId, mPendingCreateRouteRequestInfo.nativeRequestId, this, true);

        String clientId = ((CastMediaSource) mPendingCreateRouteRequestInfo.source).getClientId();

        if (clientId != null) {
            ClientRecord clientRecord = mClientRecords.get(clientId);
            if (clientRecord != null) {
                mSessionController.notifyReceiverAction(clientRecord.routeId,
                        mPendingCreateRouteRequestInfo.sink, clientId, "cast");
            }
        }
    }

    @Override
    public void onSessionStarted(CastSession session, String sessionId) {
      Log.i(TAG, "onSessionStarted");
        mPendingCreateRouteRequestInfo = null;
        mSessionController.onSessionStarted();
    }

    @Override
    public void onSessionStartFailed(CastSession session, int error) {
        for (String routeId : mRoutes.keySet()) {
            mManager.onRouteClosedWithError(routeId, "Launch error");
        }
        mRoutes.clear();
        mClientRecords.clear();
    }

    @Override
    public void onSessionEnding(CastSession session) {
        // Ignore.
    }

    public boolean hasSession() {
        return mSessionController != null;
    }

    @Override
    public void onSessionEnded(CastSession session, int error) {
        if (!hasSession()) return;

        if (mClientRecords.isEmpty()) {
            for (String routeId : mRoutes.keySet()) mManager.onRouteClosed(routeId);
            mRoutes.clear();
        } else {
            mLastRemovedRouteRecord = mClientRecords.values().iterator().next();
            for (ClientRecord client : mClientRecords.values()) {
                mManager.onRouteClosed(client.routeId);

                mRoutes.remove(client.routeId);
            }
            mClientRecords.clear();
        }

        detachFromSession();
        if (mAndroidMediaRouter != null) {
            mAndroidMediaRouter.selectRoute(mAndroidMediaRouter.getDefaultRoute());
        }
    }

    @Override
    public void onSessionResuming(CastSession session, String sessionId) {}

    @Override
    public void onSessionResumed(CastSession session, boolean wasSuspended) {}

    @Override
    public void onSessionResumeFailed(CastSession session, int error) {}

    @Override
    public void onSessionSuspended(CastSession session, int reason) {}

    private void detachFromSession() {
        mSessionController.onSessionEnded();
        mSessionController = null;
    }

    private static class CreateRouteRequestInfo {
        public final MediaSource source;
        public final MediaSink sink;
        public final String presentationId;
        public final String origin;
        public final int tabId;
        public final boolean isIncognito;
        public final int nativeRequestId;

        public CreateRouteRequestInfo(MediaSource source, MediaSink sink, String presentationId,
                String origin, int tabId, boolean isIncognito, int nativeRequestId) {
            this.source = source;
            this.sink = sink;
            this.presentationId = presentationId;
            this.origin = origin;
            this.tabId = tabId;
            this.isIncognito = isIncognito;
            this.nativeRequestId = nativeRequestId;
        }
    }

    private void addRoute(MediaRoute route, String origin, int tabId) {
        // TODO(zqzhang): merge mRoutes and mClientRecords?
        mRoutes.put(route.id, route);

        CastMediaSource source = CastMediaSource.from(route.sourceId);
        final String clientId = source.getClientId();

        if (clientId == null || mClientRecords.get(clientId) != null) return;

        mClientRecords.put(clientId,
                new ClientRecord(route.id, clientId, source.getApplicationId(),
                        source.getAutoJoinPolicy(), origin, tabId));
    }

    public void notifyClientMessage(String clientId, String message) {
        ClientRecord clientRecord = mClientRecords.get(clientId);
        if (clientRecord == null) return;

        if (!clientRecord.isConnected) {
            Log.d(TAG, "Queueing message to client %s: %s", clientId, message);
            clientRecord.pendingMessages.add(message);
            return;
        }

        Log.d(TAG, "Sending message to client %s: %s", clientId, message);
        mManager.onMessage(clientRecord.routeId, message);
    }

    private boolean canAutoJoin(CastMediaSource source, String origin, int tabId) {
        if (source.getAutoJoinPolicy().equals(CastMediaSource.AUTOJOIN_PAGE_SCOPED)) return false;

        CastMediaSource currentSource = (CastMediaSource) mSessionController.getSource();
        if (!currentSource.getApplicationId().equals(source.getApplicationId())) return false;

        ClientRecord client = null;
        if (!mClientRecords.isEmpty()) {
            client = mClientRecords.values().iterator().next();
        } else if (mLastRemovedRouteRecord != null) {
            client = mLastRemovedRouteRecord;
            return isSameOrigin(origin, client.origin) && tabId == client.tabId;
        }
        if (client == null) return false;

        boolean sameOrigin = isSameOrigin(origin, client.origin);
        if (source.getAutoJoinPolicy().equals(CastMediaSource.AUTOJOIN_ORIGIN_SCOPED)) {
            return sameOrigin;
        } else if (source.getAutoJoinPolicy().equals(
                           CastMediaSource.AUTOJOIN_TAB_AND_ORIGIN_SCOPED)) {
            return sameOrigin && tabId == client.tabId;
        }

        return false;
    }

    private boolean canJoinExistingSession(
            String presentationId, String origin, int tabId, CastMediaSource source) {
        if (AUTO_JOIN_PRESENTATION_ID.equals(presentationId)) {
            return canAutoJoin(source, origin, tabId);
        } else if (presentationId.startsWith(PRESENTATION_ID_SESSION_ID_PREFIX)) {
            String sessionId = presentationId.substring(PRESENTATION_ID_SESSION_ID_PREFIX.length());
            if (mSessionController.getSession().getSessionId().equals(sessionId)) return true;
        } else {
            for (MediaRoute route : mRoutes.values()) {
                if (route.presentationId.equals(presentationId)) return true;
            }
        }
        return false;
    }

    /**
     * Compares two origins. Empty origin strings correspond to unique origins in
     * url::Origin.
     *
     * @param originA A URL origin.
     * @param originB A URL origin.
     * @return True if originA and originB represent the same origin, false otherwise.
     */
    private static final boolean isSameOrigin(String originA, String originB) {
        if (originA == null || originA.isEmpty() || originB == null || originB.isEmpty())
            return false;
        return originA.equals(originB);
    }

    @Nullable
    private ClientRecord getClientRecordByRouteId(String routeId) {
        for (ClientRecord record : mClientRecords.values()) {
            if (record.routeId.equals(routeId)) return record;
        }
        return null;
    }

    private void removeClient(@Nullable ClientRecord client) {
        if (client == null) return;

        mLastRemovedRouteRecord = client;
        mClientRecords.remove(client.clientId);
    }
}
