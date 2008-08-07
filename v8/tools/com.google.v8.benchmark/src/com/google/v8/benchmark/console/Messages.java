// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark.console;

import java.util.MissingResourceException;
import java.util.ResourceBundle;

/**
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class Messages {
  private static final String BUNDLE_NAME = "com.google.v8.benchmark.console.messages"; //$NON-NLS-1$

  private static final ResourceBundle RESOURCE_BUNDLE = ResourceBundle
      .getBundle(BUNDLE_NAME);

  private Messages() {
  }

  public static String getString(String key) {
    try {
      return RESOURCE_BUNDLE.getString(key);
    } catch (MissingResourceException e) {
      return '!' + key + '!';
    }
  }
}
