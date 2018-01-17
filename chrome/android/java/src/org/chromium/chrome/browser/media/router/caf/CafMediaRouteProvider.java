package org.chromium.chrome.browser.media.router.caf;

public class CafMediaRouteProvider extends BaseMediaRouteProvider {
    private static final String TAG = "CafMediaRouter";

    public static CafMediaRouteProvider create(MediaRouteManager manager) {
        return new CastMediaRouteProvider(ChromeMediaRouter.getAndroidMediaRouter(), manager);
    }

    @Override
    public void onSessionStartFailed() {}

    @Override
    public void onSessionStarted(CastSession session) {}

    @Override
    public void onSessionEnded() {}

    public void onMessageSentResult(boolean success, int callbackId) {
        mManager.onMessageSentResult(success, callbackId);
    }

    public void onMessage(String clientId, String message) {
        ClientRecord clientRecord = mClientRecords.get(clientId);
    }
}
