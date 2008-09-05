// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.table;

import java.text.NumberFormat;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;

/**
 * Describes a column in an ASCII table.  A table column can not be
 * shared between tables, but can be added to only one.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class AsciiTableColumn {
  
  final LinkedList<String> names = new LinkedList<String>();
  String label;
  final Map<Attribute, Object> attribs = new HashMap<Attribute, Object>();
  
  public AsciiTableColumn(String name) {
    this.names.add(name);
    this.label = name;
  }
  
  public AsciiTableColumn setLabel(String value) {
    this.label = value;
    return this;
  }
  
  public AsciiTableColumn setAlignment(Alignment value) {
    this.attribs.put(Attribute.ALIGN, value);
    return this;
  }
  
  public AsciiTableColumn setMinimumWidth(int value) {
    this.attribs.put(Attribute.MIN_COLUMN_WIDTH, value);
    return this;
  }
  
  public AsciiTableColumn setMaximumWidth(int value) {
    this.attribs.put(Attribute.MAX_COLUMN_WIDTH, value);
    return this;
  }
  
  public AsciiTableColumn setNumberFormat(NumberFormat format) {
    this.attribs.put(Attribute.NUM_FORMAT, format);
    return this;
  }


}
