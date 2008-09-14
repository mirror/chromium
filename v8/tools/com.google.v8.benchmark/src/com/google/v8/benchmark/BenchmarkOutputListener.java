// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

/**
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class BenchmarkOutputListener implements IOutputListener {

  private final IBenchmarkMonitor monitor;
  private final StringBuilder output = new StringBuilder();
  private final StringBuilder error = new StringBuilder();
  
  public BenchmarkOutputListener(IBenchmarkMonitor monitor) {
    this.monitor = monitor;
  }

  public void error(String str) {
    this.monitor.printErr(str);
    this.error.append(str);
  }

  public void output(String str) {
    this.monitor.printOut(str);
    this.output.append(str);
  }
  
  public String getOutput() {
    return this.output.toString();
  }
  
  public String getError() {
    return this.error.toString();
  }

}
