// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.table;

import java.io.Writer;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A render context is used to hold information about an ascii table
 * during rendering.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class RenderContext {
  
  
  private final AsciiTable table;
  final Writer out;
  private Map<AsciiTableColumn, ColumnInfo> columnInfo = null;
  
  public RenderContext(AsciiTable table, Writer out) {
    this.out = out;
    this.table = table;
  }

  public int getColumnWidth(AsciiTableColumn col) {
    if (columnInfo == null)
      this.columnInfo = buildColumnWidths();
    return columnInfo.get(col).width;
  }
  
  private Map<AsciiTableColumn, ColumnInfo> buildColumnWidths() {
    Map<AsciiTableColumn, ColumnInfo> result = new HashMap<AsciiTableColumn, ColumnInfo>();
    for (AsciiTableColumn col : table.columns)
      result.put(col, new ColumnInfo(col.label.length()));
    boolean first = true;
    for (AsciiTableColumn col : table.columns) {
      scanRows(result, col, table.rows, 0, first);
      if (first) first = false;
    }
    for (AsciiTableColumn col : table.columns) {
      ColumnInfo info = result.get(col);      
      int min = (Integer) table.getAttribute(Attribute.MIN_COLUMN_WIDTH,
          col, info.width);
      info.width = Math.max(info.width, min);
    }
    for (AsciiTableColumn col : table.columns) {
      ColumnInfo info = result.get(col);
      int min = (Integer) table.getAttribute(Attribute.MAX_COLUMN_WIDTH,
          col, info.width);
      info.width = Math.min(info.width, min);
    }
    return result;
  }

  private void scanRows(Map<AsciiTableColumn, ColumnInfo> result, 
      AsciiTableColumn col, List<AsciiTableRow> rows, int indent,
      boolean countChildren) {
    for (AsciiTableRow row : rows) {
      ColumnInfo info = result.get(col);
      String str = table.getEntry(row, col, this);
      info.width = Math.max(info.width, indent + str.length());
      if (countChildren && row.rows != null)
        scanRows(result, col, row.rows, indent + AsciiTable.INDENT_SIZE,
            true);
    }
  }
  
  private static class ColumnInfo {
    
    public int width;

    public ColumnInfo(int width) {
      this.width = width;
    }
    
  }

}
