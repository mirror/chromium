// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.table;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A row in an ASCII table.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class AsciiTableRow {
  
  final Map<String, Object> values = new HashMap<String, Object>();
  final Map<Attribute, Object> attribs = new HashMap<Attribute, Object>();
  List<AsciiTableRow> rows;
  
  public AsciiTableRow(Object... values) {
    for (int i = 0; i < values.length; i++)
      this.values.put(Integer.toString(i), values[i]);
  }
  
  public AsciiTableRow setValue(String key, Object value) {
    values.put(key, value);
    return this;
  }
  
  public AsciiTableRow setLineBefore(LineType value) {
    attribs.put(Attribute.LINE_BEFORE, value);
    return this;
  }
  
  public AsciiTableRow setLineAfter(LineType value) {
    attribs.put(Attribute.LINE_AFTER, value);
    return this;
  }

  public void addRow(AsciiTableRow row) {
    if (rows == null) rows = new ArrayList<AsciiTableRow>();
    this.rows.add(row);
  }  

}
