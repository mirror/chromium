/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorController_h
#define InspectorController_h

#if ENABLE(JAVASCRIPT_DEBUGGER)
#include "JavaScriptDebugListener.h"
#endif

#include "Console.h"
#include "PlatformString.h"
#include "StringHash.h"
#if USE(JSC)
#include <JavaScriptCore/JSContextRef.h>
#include <profiler/Profiler.h>
#elif USE(V8)
#include <wtf/RefCounted.h>
#include <v8.h>
#endif
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/Vector.h>

#if USE(JSC)
namespace JSC {
    class Profile;
    class UString;
}
#endif

namespace WebCore {

class Database;
class DocumentLoader;
class DOMWindow;
class GraphicsContext;
class HitTestResult;
class InspectorClient;
class JavaScriptCallFrame;
class Node;
class Page;
class ResourceResponse;
class ResourceError;
class ScriptCallContext;
class SharedBuffer;

struct ConsoleMessage;
struct InspectorDatabaseResource;
struct InspectorResource;
class ResourceRequest;

class InspectorController : public RefCounted<InspectorController>
#if ENABLE(JAVASCRIPT_DEBUGGER)
                            , JavaScriptDebugListener
#endif
{
public:
    int m_bug1228513_inspectorState;

    typedef HashMap<unsigned long, RefPtr<InspectorResource> > ResourcesMap;
    typedef HashMap<RefPtr<Frame>, ResourcesMap*> FrameResourcesMap;
    typedef HashSet<RefPtr<InspectorDatabaseResource> > DatabaseResourcesSet;

    typedef enum {
        CurrentPanel,
        ConsolePanel,
        DatabasesPanel,
        ElementsPanel,
        ProfilesPanel,
        ResourcesPanel,
        ScriptsPanel
    } SpecialPanels;

    InspectorController(Page*, InspectorClient*);
    ~InspectorController();

    void inspectedPageDestroyed();
    void pageDestroyed() { m_page = 0; }

    bool enabled() const;

    Page* inspectedPage() const { return m_inspectedPage; }

    String localizedStringsURL();

    void inspect(Node*);
    void highlight(Node*);
    void hideHighlight();

    void show();
    void showPanel(SpecialPanels);
    void close();

#if USE(JSC)
    bool isRecordingUserInitiatedProfile() const { return m_recordingUserInitiatedProfile; }
    void startUserInitiatedProfiling();
    void stopUserInitiatedProfiling();
#endif

    bool windowVisible();
    void setWindowVisible(bool visible = true, bool attached = false);

    void addMessageToConsole(MessageSource, MessageLevel, ScriptCallContext*);
    void addMessageToConsole(MessageSource, MessageLevel, const String& message, unsigned lineNumber, const String& sourceID);

    void clearConsoleMessages();
    void toggleRecordButton(bool);

#if USE(JSC)
    void addProfile(PassRefPtr<JSC::Profile>, unsigned lineNumber, const JSC::UString& sourceURL);
    void addProfileMessageToConsole(PassRefPtr<JSC::Profile> prpProfile, unsigned lineNumber, const JSC::UString& sourceURL);
    void addScriptProfile(JSC::Profile* profile);
    const Vector<RefPtr<JSC::Profile> >& profiles() const { return m_profiles; }
#endif

    void attachWindow();
    void detachWindow();

    void setAttachedWindow(bool); 
    void setAttachedWindowHeight(unsigned height); 

    void toggleSearchForNodeInPage();
    bool searchingForNodeInPage() { return m_searchingForNode; };
    void mouseDidMoveOverElement(const HitTestResult&, unsigned modifierFlags);
    void handleMousePressOnNode(Node*);

#if USE(JSC)
    JSContextRef scriptContext() const { return m_scriptContext; };
    void setScriptContext(JSContextRef context) { m_scriptContext = context; };
#elif USE(V8)
    void setScriptObject(v8::Handle<v8::Object> newScriptObject);
#endif

    void inspectedWindowScriptObjectCleared(Frame*);
    void windowScriptObjectAvailable();

    void scriptObjectReady();

    void populateScriptObjects();
    void resetScriptObjects();

    void didCommitLoad(DocumentLoader*);
    void frameDetachedFromParent(Frame*);

    void didLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int length);

    void identifierForInitialRequest(unsigned long identifier, DocumentLoader*, const ResourceRequest&);
    void willSendRequest(DocumentLoader*, unsigned long identifier, ResourceRequest&, const ResourceResponse& redirectResponse);
    void didReceiveResponse(DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    void didReceiveContentLength(DocumentLoader*, unsigned long identifier, int lengthReceived);
    void didFinishLoading(DocumentLoader*, unsigned long identifier);
    void didFailLoading(DocumentLoader*, unsigned long identifier, const ResourceError&);
#if USE(JSC)
    void resourceRetrievedByXMLHttpRequest(unsigned long identifier, JSC::UString& sourceString);
#elif USE(V8)
    void resourceRetrievedByXMLHttpRequest(unsigned long identifier, String& sourceString);
#endif

#if ENABLE(DATABASE)
    void didOpenDatabase(Database*, const String& domain, const String& name, const String& version);
#endif

    const ResourcesMap& resources() const { return m_resources; }

    void moveWindowBy(float x, float y) const;
    void closeWindow();

#if ENABLE(JAVASCRIPT_DEBUGGER)
    void startDebugging();
    void stopDebugging();

    bool debuggerAttached() const { return m_debuggerAttached; }

    JavaScriptCallFrame* currentCallFrame() const;

    void addBreakpoint(intptr_t sourceID, unsigned lineNumber);
    void removeBreakpoint(intptr_t sourceID, unsigned lineNumber);

    bool pauseOnExceptions();
    void setPauseOnExceptions(bool pause);

    void pauseInDebugger();
    void resumeDebugger();

    void stepOverStatementInDebugger();
    void stepIntoStatementInDebugger();
    void stepOutOfFunctionInDebugger();
#endif

    void drawNodeHighlight(GraphicsContext&) const;

#if USE(JSC)
    void count(const JSC::UString& title, unsigned lineNumber, const String& sourceID);

    void startTiming(const JSC::UString& title);
    bool stopTiming(const JSC::UString& title, double& elapsed);
#endif

    void startGroup(MessageSource source, ScriptCallContext*);
    void endGroup(MessageSource source, unsigned lineNumber, const String& sourceURL);

#if USE(V8)
    // InspectorController.idl
    void addSourceToFrame(unsigned long identifier, Node* frame);
    Node* getResourceDocumentNode(unsigned long identifier);
    void highlightDOMNode(Node* node);
    void hideDOMNodeHighlight();
    void loaded();
    void attach();
    void detach();
    // TODO(jackson): search should return an array of JSRanges
    void search(Node* node, const String& query);
    DOMWindow* inspectedWindow();
    String platform() const;
#endif

private:
    void focusNode();

    // TODO(dglazkov): Implement argument array comparison in ScriptCallContext
    // and return method signature back to WebKit's original
    void addConsoleMessage(ScriptCallContext*, ConsoleMessage*);
    void addScriptConsoleMessage(const ConsoleMessage*);

    void addResource(InspectorResource*);
    void removeResource(InspectorResource*);

#if USE(JSC)
    JSObjectRef addScriptResource(InspectorResource*);
#elif USE(V8)
    void addScriptResource(InspectorResource*);
#endif
    void removeScriptResource(InspectorResource*);

#if USE(JSC)
    JSObjectRef addAndUpdateScriptResource(InspectorResource*);
#elif USE(V8)
    void addAndUpdateScriptResource(InspectorResource*);
#endif
    void updateScriptResourceRequest(InspectorResource*);
    void updateScriptResourceResponse(InspectorResource*);
    void updateScriptResourceType(InspectorResource*);
    void updateScriptResource(InspectorResource*, int length);
    void updateScriptResource(InspectorResource*, bool finished, bool failed = false);
    void updateScriptResource(InspectorResource*, double startTime, double responseReceivedTime, double endTime);

    void pruneResources(ResourcesMap*, DocumentLoader* loaderToKeep = 0);
    void removeAllResources(ResourcesMap* map) { pruneResources(map); }

    // Return true if the inspector should track request/response activity.
    // Chrome's policy is to only log requests if the inspector is already open.
    // This reduces the passive bloat from InspectorController: http://b/1113875
    bool trackResources() const { return m_trackResources; }

    // Start/stop resource tracking.
    void enableTrackResources(bool trackResources);

    bool m_trackResources;

    // Helper function to determine when the script object is initialized
#if USE(JSC)
    inline bool hasScriptObject() const { return m_scriptObject; }
#elif USE(V8)
    inline bool hasScriptObject() { return !m_scriptObject.IsEmpty(); }
#endif

#if ENABLE(DATABASE)
#if USE(JSC)
    JSObjectRef addDatabaseScriptResource(InspectorDatabaseResource*);
#elif USE(V8)
    void addDatabaseScriptResource(InspectorDatabaseResource*);
#endif
    void removeDatabaseScriptResource(InspectorDatabaseResource*);
#endif

#if USE(JSC)
    JSValueRef callSimpleFunction(JSContextRef, JSObjectRef thisObject, const char* functionName) const;
    JSValueRef callFunction(JSContextRef, JSObjectRef thisObject, const char* functionName, size_t argumentCount, const JSValueRef arguments[], JSValueRef& exception) const; 

    bool handleException(JSContextRef, JSValueRef exception, unsigned lineNumber) const; 
#endif

    void showWindow();

#if ENABLE(JAVASCRIPT_DEBUGGER)
    virtual void didParseSource(JSC::ExecState*, const JSC::SourceCode&);
    virtual void failedToParseSource(JSC::ExecState*, const JSC::SourceCode&, int errorLine, const JSC::UString& errorMessage);
    virtual void didPause();
#endif

    Page* m_inspectedPage;
    InspectorClient* m_client;
    Page* m_page;
    RefPtr<Node> m_nodeToFocus;
    RefPtr<InspectorResource> m_mainResource;
    ResourcesMap m_resources;
    HashSet<String> m_knownResources;
    FrameResourcesMap m_frameResources;
    Vector<ConsoleMessage*> m_consoleMessages;
#if USE(JSC)
    Vector<RefPtr<JSC::Profile> > m_profiles;
#endif
    HashMap<String, double> m_times;
    HashMap<String, unsigned> m_counts;
#if ENABLE(DATABASE)
    DatabaseResourcesSet m_databaseResources;
#endif
#if USE(JSC)
    JSObjectRef m_scriptObject;
    JSObjectRef m_controllerScriptObject;
    JSContextRef m_scriptContext;
#elif USE(V8)
    v8::Persistent<v8::Object> m_scriptObject;
#endif
    bool m_windowVisible;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    bool m_debuggerAttached;
    bool m_attachDebuggerWhenShown;
#endif
    bool m_recordingUserInitiatedProfile;
    SpecialPanels m_showAfterVisible;
    unsigned long m_nextIdentifier;
    RefPtr<Node> m_highlightedNode;
    unsigned m_groupLevel;
    bool m_searchingForNode;
    int m_currentUserInitiatedProfileNumber;
    unsigned m_nextUserInitiatedProfileNumber;
    ConsoleMessage* m_previousMessage;
};

} // namespace WebCore

#endif // !defined(InspectorController_h)
