// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

import java.io.PrintStream;

/**
 * An output listener that simply prints the output on a print stream.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class OutputListener implements IOutputListener {

  private final PrintStream out, err;

  public OutputListener(PrintStream out, PrintStream err) {
    this.out = out;
    this.err = err;
  }

  public void output(String str) {
    out.print(str);
  }
  
  public void error(String str) {
    err.print(str);
  }

}
