// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.table;

/**
 * Specifies the type of a horizontal line in an ascii table.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public enum LineType {
  
  NONE(' ', 0), THIN('-', 1), THICK('=', 2);
  
  final char chr;
  private final int size;
  
  private LineType(char chr, int size) {
    this.chr = chr;
    this.size = size;
  }
  
  public LineType max(LineType that) {
    if (this.size > that.size) return this;
    else return that;
  }

}
