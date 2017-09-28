package org.chromium.android_webview.pacservice;

public interface PacProcessorBackend {
  void start();
  void stop();
  String resolve(String host, String url);
  void setPacScript(String content);
}
