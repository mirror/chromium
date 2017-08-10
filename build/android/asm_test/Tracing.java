package util;

import org.chromium.base.test.TestTraceEvent;

public class Tracing {
    public static void traceBegin(String methodName) {
        System.out.println("BEGINNING " + methodName);
    }

    public static void traceEnd(String methodName) {
        System.out.println("ENDING " + methodName);
    }
}
