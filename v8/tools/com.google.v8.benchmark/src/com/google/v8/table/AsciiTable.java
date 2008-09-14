// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.table;

import java.io.IOException;
import java.io.Writer;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Stack;

/**
 * A table that can be "rendered" as text on the console.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
@SuppressWarnings("nls")
public class AsciiTable {
  
  static final int INDENT_SIZE = 2;
  final List<AsciiTableColumn> columns = new ArrayList<AsciiTableColumn>();
  final List<AsciiTableRow> rows = new ArrayList<AsciiTableRow>();
  private char columnChar = '|', sspace = ' ', dspace = ' ';
  final Map<Attribute, Object> attribs = new HashMap<Attribute, Object>();
  
  public void addColumn(AsciiTableColumn column) {
    column.names.addFirst(Integer.toString(columns.size()));
    this.columns.add(column);
  }
  
  public void addRow(AsciiTableRow row) {
    rows.add(row);
  }

  public void render(Writer out) throws IOException {
    RenderContext context = new RenderContext(this, out);
    for (AsciiTableColumn col : columns) {
      String str = col.label;
      printTableEntry(out, context, col, str, 0);
    }
    out.write('\n');
    printLine(out, context, LineType.THICK);
    LineType nextLine = null;
    Stack<Iterator<AsciiTableRow>> iter = new Stack<Iterator<AsciiTableRow>>();
    iter.push(rows.iterator());
    while (!iter.isEmpty()) {
      while (iter.peek().hasNext()) {
        AsciiTableRow row = iter.peek().next();
        LineType lineBefore = (LineType) getAttribute(Attribute.LINE_BEFORE, row, LineType.NONE);
        if (nextLine != null) { // false when printing the first line
          nextLine = lineBefore.max(nextLine);
          if (nextLine != LineType.NONE) printLine(out, context, nextLine);
        }
        int indent = (iter.size() - 1) * INDENT_SIZE;
        for (int i = 0; i < indent; i++)
          out.write(' ');
        boolean first = true;
        for (AsciiTableColumn col : columns) {
          String str = getEntry(row, col, context);
          printTableEntry(out, context, col, str, first ? indent : 0);
          if (first) first = false;
        }
        out.write('\n');
        nextLine = (LineType) getAttribute(Attribute.LINE_AFTER, row, LineType.NONE);
        if (row.rows != null) iter.push(row.rows.iterator());
      }
      iter.pop();
    }
    if (nextLine != null && nextLine != LineType.NONE) printLine(out, context, nextLine);
    out.flush();
  }

  private void printLine(Writer out, RenderContext context, LineType type) 
      throws IOException {
    for (AsciiTableColumn col : columns) {
      repeat(out, context.getColumnWidth(col) + 2, type.chr);
      out.write('+');
    }
    out.write('\n');
  }

  private void printTableEntry(Writer out, RenderContext context, 
      AsciiTableColumn col, String str, int indent) throws IOException {
    out.write(this.sspace);
    Alignment align = (Alignment) getAttribute(Attribute.ALIGN, col, Alignment.LEFT);
    int columnWidth = context.getColumnWidth(col) - indent;
    String shown = str;
    if (shown.length() > columnWidth)
      shown = shown.substring(0, columnWidth - 1) + ".";
    if (align == Alignment.CENTER) {
      int dist = columnWidth - shown.length();
      repeat(out, dist / 2, this.dspace);
      out.write(shown);
      repeat(out, dist - (dist / 2), this.dspace);
    } else if (align == Alignment.RIGHT) {
      repeat(out, columnWidth - shown.length(), this.dspace);
      out.write(shown);      
    } else {
      out.write(shown);
      repeat(out, columnWidth - shown.length(), this.dspace);      
    }
    out.write(this.sspace);
    out.write(this.columnChar);
  }

  String getEntry(AsciiTableRow row, AsciiTableColumn col,
      RenderContext context) {
    String str = ""; //$NON-NLS-1$
    findValue: for (String name : col.names) {
      Object value = row.values.get(name);
      if (value != null) {
        str = toString(value, col, context);
        break findValue;
      }
    }
    return str;
  }
  
  private static final NumberFormat DEFAULT_NUMBER_FORMAT;
  static {
    DEFAULT_NUMBER_FORMAT = NumberFormat.getNumberInstance();
    DEFAULT_NUMBER_FORMAT.setMaximumFractionDigits(2);
    DEFAULT_NUMBER_FORMAT.setMinimumFractionDigits(0);
    DEFAULT_NUMBER_FORMAT.setGroupingUsed(false);
  }
  
  private String toString(Object value, AsciiTableColumn col,
      RenderContext context) {
    if (value instanceof Double || value instanceof Float) {
      NumberFormat format = (NumberFormat) getAttribute(
          Attribute.NUM_FORMAT, col, DEFAULT_NUMBER_FORMAT);
      return format.format(value);
    }
    return "" + value;
  }

  private static void repeat(Writer out, int count, char chr) throws IOException {
    for (int i = 0; i < count; i++)
      out.write(chr);
  }
  
  /**
   * Looks up an attribute.  If the attribute is defined in the
   * specified column, that value is returned.  Otherwise, if it is
   * defined in the table the table's value is returned.  Otherwise
   * the default value is returned.
   */
  Object getAttribute(Attribute key, AsciiTableColumn col, Object def) {
    Object result = col.attribs.get(key);
    if (result == null) result = this.attribs.get(key);
    if (result == null) result = def;
    return result;
  }
  
  /**
   * Looks up an attribute.  If the attribute is defined in the
   * specified row, that value is returned.  Otherwise, if it is
   * defined in the table the table's value is returned.  Otherwise
   * the default value is returned.
   */
  Object getAttribute(Attribute key, AsciiTableRow row, Object def) {
    Object result = row.attribs.get(key);
    if (result == null) result = this.attribs.get(key);
    if (result == null) result = def;
    return result;
  }

  public AsciiTable setAlignment(Alignment value) {
    this.attribs.put(Attribute.ALIGN, value);
    return this;
  }

  public AsciiTable setMinimumColumnWidth(int value) {
    this.attribs.put(Attribute.MIN_COLUMN_WIDTH, value);
    return this;
  }

  public AsciiTable setMaximumColumnWidth(int value) {
    this.attribs.put(Attribute.MAX_COLUMN_WIDTH, value);
    return this;
  }
  
  public AsciiTable setNumberFormat(NumberFormat format) {
    this.attribs.put(Attribute.NUM_FORMAT, format);
    return this;
  }

  public AsciiTable setLineBetween(LineType type) {
    this.attribs.put(Attribute.LINE_BEFORE, type);
    return this;
  }

}
