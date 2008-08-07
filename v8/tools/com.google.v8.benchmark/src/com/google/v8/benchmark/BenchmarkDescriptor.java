// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

import com.google.v8.table.Alignment;
import com.google.v8.table.AsciiTable;
import com.google.v8.table.AsciiTableColumn;
import com.google.v8.table.AsciiTableRow;
import com.google.v8.table.LineType;

import java.io.IOException;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.text.NumberFormat;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;

/**
 * A description of a set of benchmarks.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
@SuppressWarnings("nls")
public class BenchmarkDescriptor {
  
  private final List<ScriptDescriptor> scripts;
  private final List<Runtime> runtimes;
  
  public BenchmarkDescriptor(List<ScriptDescriptor> scripts, List<Runtime> runtimes) {
    this.scripts = scripts;
    this.runtimes = runtimes;
  }
  
  public void printResults(PrintStream out, CollectedBenchmarkResult result,
      Runtime compared) throws IOException {
    AsciiTable table = new AsciiTable();
    NumberFormat format = NumberFormat.getNumberInstance();
    format.setMaximumFractionDigits(2);
    format.setMinimumFractionDigits(2);
    table.setNumberFormat(format);
    // Create header
    table.addColumn(new AsciiTableColumn("name").setLabel(compared == null ? "Times (ms.)" : "Relative"));
    for (Runtime runtime : runtimes)
      table.addColumn(new AsciiTableColumn(runtime.getName())
        .setMinimumWidth(8)
        .setAlignment(Alignment.RIGHT));
    // Add values
    for (ScriptDescriptor script : scripts) {
      FileBenchmarkResult fileResult = result.getResult(script.getFile());
      AsciiTableRow row = new AsciiTableRow(fileResult.getFile().getName()).setLineBefore(LineType.THIN);
      table.addRow(row);
      Set<String> cases = new TreeSet<String>();
      for (Runtime runtime : runtimes) {
        RuntimeBenchmarkResults runtimeResults = fileResult.getResult(runtime);
        if (compared == null) {
          row.setValue(runtime.getName(), runtimeResults.getSum());
        } else {
          RuntimeBenchmarkResults compareResults = fileResult.getResult(compared);
          row.setValue(runtime.getName(), runtimeResults.getMean(compareResults));
        }
        cases.addAll(runtimeResults.getCases());
      }
      if (cases.size() > 1) {
        for (String cahse : cases) {
          AsciiTableRow subRow = new AsciiTableRow(cahse);
          row.addRow(subRow);
          for (Runtime runtime : runtimes) {
            RuntimeBenchmarkResults runtimeResults = fileResult.getResult(runtime);
            if (compared == null) {
              SingleBenchmarkResult singleResult = runtimeResults.getResult(cahse);
              if (singleResult != null)
                subRow.setValue(runtime.getName(), singleResult.getValue());
            } else {
              SingleBenchmarkResult singleResult = runtimeResults.getResult(cahse);
              RuntimeBenchmarkResults compareResults = fileResult.getResult(compared);
              SingleBenchmarkResult compareResult = compareResults.getResult(cahse);
              if (singleResult != null && compareResult != null) {
                subRow.setValue(runtime.getName(), 
                    singleResult.getValue() / compareResult.getValue());
              }
            }
          }
        }
      }
    }
    AsciiTableRow mean = new AsciiTableRow("Mean");
    table.addRow(mean);
    mean.setLineBefore(LineType.THICK);
    for (Runtime runtime : runtimes) {
      if (compared != null)
        mean.setValue(runtime.getName(), getMean(result, runtime, compared));
    }
    table.render(new PrintWriter(out));

  }

  public List<ScriptDescriptor> getScripts() {
    return scripts;
  }

  public List<Runtime> getRuntimes() {
    return runtimes;
  }

  private double getMean(CollectedBenchmarkResult result, Runtime runtime,
      Runtime compared) {
    return result.getMean(runtime, compared);
  }

}
