// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

/**
 * An output listener is notified when new output is generated from a
 * process.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public interface IOutputListener {
  
  public void output(String str);
  public void error(String str);

}
