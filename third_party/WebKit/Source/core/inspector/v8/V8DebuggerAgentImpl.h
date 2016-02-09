// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8DebuggerAgentImpl_h
#define V8DebuggerAgentImpl_h

#include "core/CoreExport.h"
#include "core/InspectorBackendDispatcher.h"
#include "core/InspectorFrontend.h"
#include "core/inspector/v8/ScriptBreakpoint.h"
#include "core/inspector/v8/V8DebuggerImpl.h"
#include "core/inspector/v8/public/V8DebuggerAgent.h"
#include "wtf/Forward.h"
#include "wtf/HashMap.h"
#include "wtf/HashSet.h"
#include "wtf/ListHashSet.h"
#include "wtf/PassRefPtr.h"
#include "wtf/Vector.h"
#include "wtf/text/StringHash.h"

namespace blink {

class AsyncCallChain;
class AsyncCallStack;
class DevToolsFunctionInfo;
class InjectedScript;
class InjectedScriptManager;
class JavaScriptCallFrame;
class JSONObject;
class PromiseTracker;
class RemoteCallFrameId;
class ScriptRegexp;
class V8AsyncCallTracker;
class V8StackTraceImpl;

typedef String ErrorString;

class CORE_EXPORT V8DebuggerAgentImpl : public V8DebuggerAgent {
    WTF_MAKE_NONCOPYABLE(V8DebuggerAgentImpl);
    USING_FAST_MALLOC(V8DebuggerAgentImpl);
public:
    enum SkipPauseRequest {
        RequestNoSkip,
        RequestContinue,
        RequestStepInto,
        RequestStepOut,
        RequestStepFrame
    };

    enum BreakpointSource {
        UserBreakpointSource,
        DebugCommandBreakpointSource,
        MonitorCommandBreakpointSource
    };

    V8DebuggerAgentImpl(InjectedScriptManager*, V8DebuggerImpl*, int contextGroupId);
    ~V8DebuggerAgentImpl() override;

    void setInspectorState(PassRefPtr<JSONObject>) override;
    void setFrontend(InspectorFrontend::Debugger* frontend) override { m_frontend = frontend; }
    void clearFrontend() override;
    void restore() override;
    void disable(ErrorString*);

    bool isPaused() override;

    // Part of the protocol.
    void enable(ErrorString*) override;
    void setBreakpointsActive(ErrorString*, bool active) override;
    void setSkipAllPauses(ErrorString*, bool skipped) override;

    void setBreakpointByUrl(ErrorString*, int lineNumber, const String* optionalURL, const String* optionalURLRegex, const int* optionalColumnNumber, const String* optionalCondition, TypeBuilder::Debugger::BreakpointId*, RefPtr<TypeBuilder::Array<TypeBuilder::Debugger::Location>>& locations) override;
    void setBreakpoint(ErrorString*, const RefPtr<JSONObject>& location, const String* optionalCondition, TypeBuilder::Debugger::BreakpointId*, RefPtr<TypeBuilder::Debugger::Location>& actualLocation) override;
    void removeBreakpoint(ErrorString*, const String& breakpointId) override;
    void continueToLocation(ErrorString*, const RefPtr<JSONObject>& location, const bool* interstateLocationOpt) override;
    void getStepInPositions(ErrorString*, const String& callFrameId, RefPtr<TypeBuilder::Array<TypeBuilder::Debugger::Location>>& positions) override;
    void getBacktrace(ErrorString*, RefPtr<TypeBuilder::Array<TypeBuilder::Debugger::CallFrame>>&, RefPtr<TypeBuilder::Debugger::StackTrace>&) override;
    void searchInContent(ErrorString*, const String& scriptId, const String& query, const bool* optionalCaseSensitive, const bool* optionalIsRegex, RefPtr<TypeBuilder::Array<TypeBuilder::Debugger::SearchMatch>>&) override;
    void canSetScriptSource(ErrorString*, bool* result) override { *result = true; }
    void setScriptSource(ErrorString*, RefPtr<TypeBuilder::Debugger::SetScriptSourceError>&, const String& scriptId, const String& newContent, const bool* preview, RefPtr<TypeBuilder::Array<TypeBuilder::Debugger::CallFrame>>& newCallFrames, TypeBuilder::OptOutput<bool>* stackChanged, RefPtr<TypeBuilder::Debugger::StackTrace>& asyncStackTrace) override;
    void restartFrame(ErrorString*, const String& callFrameId, RefPtr<TypeBuilder::Array<TypeBuilder::Debugger::CallFrame>>& newCallFrames, RefPtr<TypeBuilder::Debugger::StackTrace>& asyncStackTrace) override;
    void getScriptSource(ErrorString*, const String& scriptId, String* scriptSource) override;
    void getFunctionDetails(ErrorString*, const String& functionId, RefPtr<TypeBuilder::Debugger::FunctionDetails>&) override;
    void getGeneratorObjectDetails(ErrorString*, const String& objectId, RefPtr<TypeBuilder::Debugger::GeneratorObjectDetails>&) override;
    void getCollectionEntries(ErrorString*, const String& objectId, RefPtr<TypeBuilder::Array<TypeBuilder::Debugger::CollectionEntry>>&) override;
    void pause(ErrorString*) override;
    void resume(ErrorString*) override;
    void stepOver(ErrorString*) override;
    void stepInto(ErrorString*) override;
    void stepOut(ErrorString*) override;
    void stepIntoAsync(ErrorString*) override;
    void setPauseOnExceptions(ErrorString*, const String& pauseState) override;
    void evaluateOnCallFrame(ErrorString*,
        const String& callFrameId,
        const String& expression,
        const String* objectGroup,
        const bool* includeCommandLineAPI,
        const bool* doNotPauseOnExceptionsAndMuteConsole,
        const bool* returnByValue,
        const bool* generatePreview,
        RefPtr<TypeBuilder::Runtime::RemoteObject>& result,
        TypeBuilder::OptOutput<bool>* wasThrown,
        RefPtr<TypeBuilder::Runtime::ExceptionDetails>&) override;
    void setVariableValue(ErrorString*, int in_scopeNumber, const String& in_variableName, const RefPtr<JSONObject>& in_newValue, const String* in_callFrame, const String* in_functionObjectId) override;
    void setAsyncCallStackDepth(ErrorString*, int depth) override;
    void enablePromiseTracker(ErrorString*, const bool* captureStacks) override;
    void disablePromiseTracker(ErrorString*) override;
    void getPromiseById(ErrorString*, int promiseId, const String* objectGroup, RefPtr<TypeBuilder::Runtime::RemoteObject>& promise) override;
    void flushAsyncOperationEvents(ErrorString*) override;
    void setAsyncOperationBreakpoint(ErrorString*, int operationId) override;
    void removeAsyncOperationBreakpoint(ErrorString*, int operationId) override;
    void setBlackboxedRanges(ErrorString*, const String& scriptId, const RefPtr<JSONArray>& positions) override;

    void schedulePauseOnNextStatement(InspectorFrontend::Debugger::Reason::Enum breakReason, PassRefPtr<JSONObject> data) override;
    void cancelPauseOnNextStatement() override;
    bool canBreakProgram() override;
    void breakProgram(InspectorFrontend::Debugger::Reason::Enum breakReason, PassRefPtr<JSONObject> data) override;
    void breakProgramOnException(InspectorFrontend::Debugger::Reason::Enum breakReason, PassRefPtr<JSONObject> data) override;
    void willExecuteScript(int scriptId) override;
    void didExecuteScript() override;

    bool enabled() override;
    V8DebuggerImpl& debugger() override { return *m_debugger; }

    void setBreakpointAt(const String& scriptId, int lineNumber, int columnNumber, BreakpointSource, const String& condition = String());
    void removeBreakpointAt(const String& scriptId, int lineNumber, int columnNumber, BreakpointSource);

    // Async call stacks implementation
    int traceAsyncOperationStarting(const String& description) override;
    void traceAsyncCallbackStarting(int operationId) override;
    void traceAsyncCallbackCompleted() override;
    void traceAsyncOperationCompleted(int operationId) override;
    bool trackingAsyncCalls() const override { return m_maxAsyncCallStackDepth; }

    void didUpdatePromise(InspectorFrontend::Debugger::EventType::Enum, PassRefPtr<TypeBuilder::Debugger::PromiseDetails>);
    void reset() override;

    // Interface for V8DebuggerImpl
    SkipPauseRequest didPause(v8::Local<v8::Context>, v8::Local<v8::Object> callFrames, v8::Local<v8::Value> exception, const Vector<String>& hitBreakpoints, bool isPromiseRejection);
    void didContinue();
    void didParseSource(const V8DebuggerParsedScript&);
    bool v8AsyncTaskEventsEnabled() const;
    void didReceiveV8AsyncTaskEvent(v8::Local<v8::Context>, const String& eventType, const String& eventName, int id);
    bool v8PromiseEventsEnabled() const;
    void didReceiveV8PromiseEvent(v8::Local<v8::Context>, v8::Local<v8::Object> promise, v8::Local<v8::Value> parentPromise, int status);

    v8::Isolate* isolate() { return m_isolate; }
    PassOwnPtr<V8StackTraceImpl> currentAsyncStackTraceForRuntime();

private:
    bool checkEnabled(ErrorString*);
    void enable();

    SkipPauseRequest shouldSkipExceptionPause();
    SkipPauseRequest shouldSkipStepPause();
    bool isMuteBreakpointInstalled();

    void schedulePauseOnNextStatementIfSteppingInto();

    PassRefPtr<TypeBuilder::Array<TypeBuilder::Debugger::CallFrame>> currentCallFrames();
    PassRefPtr<TypeBuilder::Debugger::StackTrace> currentAsyncStackTrace();
    bool callStackForId(ErrorString*, const RemoteCallFrameId&, v8::Local<v8::Object>* callStack, bool* isAsync);

    void clearCurrentAsyncOperation();
    void resetAsyncCallTracker();

    void changeJavaScriptRecursionLevel(int step);

    void setPauseOnExceptionsImpl(ErrorString*, int);

    PassRefPtr<TypeBuilder::Debugger::Location> resolveBreakpoint(const String& breakpointId, const String& scriptId, const ScriptBreakpoint&, BreakpointSource);
    void removeBreakpoint(const String& breakpointId);
    void clearStepIntoAsync();
    bool assertPaused(ErrorString*);
    void clearBreakDetails();

    String sourceMapURLForScript(const V8DebuggerScript&, bool success);

    bool isCallStackEmptyOrBlackboxed();
    bool isTopCallFrameBlackboxed();
    bool isCallFrameWithUnknownScriptOrBlackboxed(PassRefPtr<JavaScriptCallFrame>);

    void internalSetAsyncCallStackDepth(int);
    void increaseCachedSkipStackGeneration();

    using ScriptsMap = HashMap<String, V8DebuggerScript>;
    using BreakpointIdToDebuggerBreakpointIdsMap = HashMap<String, Vector<String>>;
    using DebugServerBreakpointToBreakpointIdAndSourceMap = HashMap<String, std::pair<String, BreakpointSource>>;
    using MuteBreakpoins = HashMap<String, std::pair<String, int>>;

    enum DebuggerStep {
        NoStep = 0,
        StepInto,
        StepOver,
        StepOut
    };

    InjectedScriptManager* m_injectedScriptManager;
    V8DebuggerImpl* m_debugger;
    int m_contextGroupId;
    bool m_enabled;
    RefPtr<JSONObject> m_state;
    InspectorFrontend::Debugger* m_frontend;
    v8::Isolate* m_isolate;
    v8::Global<v8::Context> m_pausedContext;
    v8::Global<v8::Object> m_currentCallStack;
    ScriptsMap m_scripts;
    BreakpointIdToDebuggerBreakpointIdsMap m_breakpointIdToDebuggerBreakpointIds;
    DebugServerBreakpointToBreakpointIdAndSourceMap m_serverBreakpoints;
    MuteBreakpoins m_muteBreakpoints;
    String m_continueToLocationBreakpointId;
    InspectorFrontend::Debugger::Reason::Enum m_breakReason;
    RefPtr<JSONObject> m_breakAuxData;
    DebuggerStep m_scheduledDebuggerStep;
    bool m_skipNextDebuggerStepOut;
    bool m_javaScriptPauseScheduled;
    bool m_steppingFromFramework;
    bool m_pausingOnNativeEvent;
    bool m_pausingOnAsyncOperation;

    int m_skippedStepFrameCount;
    int m_recursionLevelForStepOut;
    int m_recursionLevelForStepFrame;
    bool m_skipAllPauses;

    // This field must be destroyed before the listeners set above.
    OwnPtr<V8AsyncCallTracker> m_v8AsyncCallTracker;
    OwnPtr<PromiseTracker> m_promiseTracker;

    using AsyncOperationIdToAsyncCallChain = HashMap<int, RefPtr<AsyncCallChain>>;
    AsyncOperationIdToAsyncCallChain m_asyncOperations;
    int m_lastAsyncOperationId;
    ListHashSet<int> m_asyncOperationNotifications;
    HashSet<int> m_asyncOperationBreakpoints;
    HashSet<int> m_pausingAsyncOperations;
    unsigned m_maxAsyncCallStackDepth;
    RefPtr<AsyncCallChain> m_currentAsyncCallChain;
    unsigned m_nestedAsyncCallCount;
    int m_currentAsyncOperationId;
    bool m_pendingTraceAsyncOperationCompleted;
    bool m_startingStepIntoAsync;
    HashMap<String, Vector<std::pair<int, int>>> m_blackboxedPositions;
};

} // namespace blink


#endif // V8DebuggerAgentImpl_h
