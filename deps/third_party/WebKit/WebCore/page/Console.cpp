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

#include "config.h"
#include "Console.h"

#include "ChromeClient.h"
#include "CString.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameTree.h"
#include "InspectorController.h"
#if USE(JSC)
#include "JSDOMBinding.h"
#endif
#include "Page.h"
#include "PageGroup.h"
#include "PlatformString.h"
#include "ScriptCallContext.h"
#if USE(JSC)
#include <runtime/ArgList.h>
#include <runtime/Interpreter.h>
#include <runtime/JSObject.h>
#include <profiler/Profiler.h>
#endif
#include <stdio.h>

using namespace JSC;

namespace WebCore {

Console::Console(Frame* frame)
    : m_frame(frame)
{
}

void Console::disconnectFrame()
{
    m_frame = 0;
}

static void printSourceURLAndLine(const String& sourceURL, unsigned lineNumber)
{
    if (!sourceURL.isEmpty()) {
        if (lineNumber > 0)
            printf("%s:%d: ", sourceURL.utf8().data(), lineNumber);
        else
            printf("%s: ", sourceURL.utf8().data());
    }
}

static void printMessageSourceAndLevelPrefix(MessageSource source, MessageLevel level)
{
    const char* sourceString;
    switch (source) {
        case HTMLMessageSource:
            sourceString = "HTML";
            break;
        case XMLMessageSource:
            sourceString = "XML";
            break;
        case JSMessageSource:
            sourceString = "JS";
            break;
        case CSSMessageSource:
            sourceString = "CSS";
            break;
        default:
            ASSERT_NOT_REACHED();
            // Fall thru.
        case OtherMessageSource:
            sourceString = "OTHER";
            break;
    }

    const char* levelString;
    switch (level) {
        case TipMessageLevel:
            levelString = "TIP";
            break;
        default:
            ASSERT_NOT_REACHED();
            // Fall thru.
        case LogMessageLevel:
            levelString = "LOG";
            break;
        case WarningMessageLevel:
            levelString = "WARN";
            break;
        case ErrorMessageLevel:
            levelString = "ERROR";
            break;
    }

    printf("%s %s:", sourceString, levelString);
}

static void printToStandardOut(MessageSource source, MessageLevel level, const String& message, const String& sourceURL, unsigned lineNumber)
{
    if (!Console::shouldPrintExceptions())
        return;

    printSourceURLAndLine(sourceURL, lineNumber);
    printMessageSourceAndLevelPrefix(source, level);

    printf(" %s\n", message.utf8().data());
}

static void printToStandardOut(MessageLevel level, ScriptCallContext* context)
{
    if (!Console::shouldPrintExceptions())
        return;

    printSourceURLAndLine(context->sourceURL().prettyURL(), 0);
    printMessageSourceAndLevelPrefix(JSMessageSource, level);

    for (size_t i = 0; i < context->argumentCount(); ++i) {
        printf(" %s", context->argumentStringAt(i).utf8().data());
    }

    printf("\n");
}

void Console::addMessage(MessageSource source, MessageLevel level, const String& message, unsigned lineNumber, const String& sourceURL)
{
    Page* page = this->page();
    if (!page)
        return;

    if (source == JSMessageSource)
        page->chrome()->client()->addMessageToConsole(message, lineNumber, sourceURL);

    page->inspectorController()->addMessageToConsole(source, level, message, lineNumber, sourceURL);

    printToStandardOut(source, level, message, sourceURL, lineNumber);
}

void Console::debug(ScriptCallContext* context)
{
    // In Firebug, console.debug has the same behavior as console.log. So we'll do the same.
    log(context);
}

void Console::error(ScriptCallContext* context)
{
    if (!context->hasArguments())
        return;

    Page* page = this->page();
    if (!page)
        return;

    page->chrome()->client()->addMessageToConsole(context->argumentStringAt(0), context->lineNumber(), context->sourceURL().prettyURL());
    page->inspectorController()->addMessageToConsole(JSMessageSource, ErrorMessageLevel, context);

    printToStandardOut(ErrorMessageLevel, context);
}

void Console::info(ScriptCallContext* context)
{
    if (!context->hasArguments())
        return;

    Page* page = this->page();
    if (!page)
        return;

    page->chrome()->client()->addMessageToConsole(context->argumentStringAt(0), context->lineNumber(), context->sourceURL().prettyURL());
    page->inspectorController()->addMessageToConsole(JSMessageSource, LogMessageLevel, context);

    printToStandardOut(LogMessageLevel, context);
}

void Console::log(ScriptCallContext* context)
{
    if (!context->hasArguments())
        return;

    Page* page = this->page();
    if (!page)
        return;

    page->chrome()->client()->addMessageToConsole(context->argumentStringAt(0), context->lineNumber(), context->sourceURL().prettyURL());
    page->inspectorController()->addMessageToConsole(JSMessageSource, LogMessageLevel, context);

    printToStandardOut(LogMessageLevel, context);
}

#if USE(JSC)

void Console::dir(ScriptCallContext* context)
{
    if (!context->hasArguments())
        return;

    Page* page = this->page();
    if (!page)
        return;

    page->inspectorController()->addMessageToConsole(JSMessageSource, ObjectMessageLevel, context);
}

void Console::dirxml(ScriptCallContext* context)
{
    if (!context->hasArguments())
        return;

    Page* page = this->page();
    if (!page)
        return;

    page->inspectorController()->addMessageToConsole(JSMessageSource, NodeMessageLevel, context);
}

void Console::trace(ScriptCallContext* context)
{
    Page* page = this->page();
    if (!page)
        return;

    int signedLineNumber;
    intptr_t sourceID;
    UString urlString;
    JSValue* func;

    context->exec()->machine()->retrieveLastCaller(context->exec(), signedLineNumber, sourceID, urlString, func);

    ArgList args;
    while (!func->isNull()) {
        args.append(func);
        func = context->exec()->machine()->retrieveCaller(context->exec(), asInternalFunction(func));
    }
    
    page->inspectorController()->addMessageToConsole(JSMessageSource, TraceMessageLevel, context);
}

void Console::assertCondition(bool condition, ScriptCallContext* context)
{
    if (condition)
        return;

    Page* page = this->page();
    if (!page)
        return;

    // FIXME: <https://bugs.webkit.org/show_bug.cgi?id=19135> It would be nice to prefix assertion failures with a message like "Assertion failed: ".
    // FIXME: <https://bugs.webkit.org/show_bug.cgi?id=19136> We should print a message even when args.isEmpty() is true.

    page->inspectorController()->addMessageToConsole(JSMessageSource, ErrorMessageLevel, context);

    printToStandardOut(ErrorMessageLevel, context);
}

void Console::count(ScriptCallContext* context)
{
    Page* page = this->page();
    if (!page)
        return;

    String title;
    if (context->argumentCount() >= 1)
        title = context->argumentStringAt(0, true);

    page->inspectorController()->count(title, context->lineNumber(),
                                       context->sourceURL().string());
}

void Console::profile(ExecState* exec, const ArgList& args)
{
    Page* page = this->page();
    if (!page)
        return;

    // FIXME: log a console message when profiling is disabled.
    if (!page->inspectorController()->profilerEnabled())
        return;

    UString title = args.at(exec, 0)->toString(exec);
    Profiler::profiler()->startProfiling(exec, title);
}

void Console::profileEnd(ExecState* exec, const ArgList& args)
{
    Page* page = this->page();
    if (!page)
        return;

    if (!page->inspectorController()->profilerEnabled())
        return;

    UString title;
    if (args.size() >= 1)
        title = valueToStringWithUndefinedOrNullCheck(exec, args.at(exec, 0));

    RefPtr<Profile> profile = Profiler::profiler()->stopProfiling(exec, title);
    if (!profile)
        return;

    m_profiles.append(profile);

    if (Page* page = this->page()) {
        ScriptCallContext context(exec, args);

        page->inspectorController()->addProfile(profile, context.lineNumber(),
                                                context.sourceURL().string());
    }
}

void Console::time(const UString& title)
{
    if (title.isNull())
        return;
    
    Page* page = this->page();
    if (!page)
        return;
    
    page->inspectorController()->startTiming(title);
}

void Console::timeEnd(ExecState* exec, const ArgList& args)
{
    UString title;
    if (args.size() >= 1)
        title = valueToStringWithUndefinedOrNullCheck(exec, args.at(exec, 0));
    if (title.isNull())
        return;

    Page* page = this->page();
    if (!page)
        return;

    double elapsed;
    if (!page->inspectorController()->stopTiming(title, elapsed))
        return;

    String message = String(title) + String::format(": %.0fms", elapsed);

    ScriptCallContext context(exec, args);

    page->inspectorController()->addMessageToConsole(JSMessageSource,
        LogMessageLevel, message, context.lineNumber(),
        context.sourceURL().string());
}

#endif

void Console::group(ScriptCallContext* context)
{
    Page* page = this->page();
    if (!page)
        return;

    page->inspectorController()->startGroup(JSMessageSource, context);
}

void Console::groupEnd()
{
    Page* page = this->page();
    if (!page)
        return;

    page->inspectorController()->endGroup(JSMessageSource, 0, String());
}

#if USE(V8)

void Console::time(const String& title)
{
    notImplemented();
}

#endif

void Console::warn(ScriptCallContext* context)
{
    if (!context->hasArguments())
        return;

    Page* page = this->page();
    if (!page)
        return;

    page->chrome()->client()->addMessageToConsole(context->argumentStringAt(0), context->lineNumber(), context->sourceURL().prettyURL());
    page->inspectorController()->addMessageToConsole(JSMessageSource, WarningMessageLevel, context);

    printToStandardOut(WarningMessageLevel, context);
}

#if USE(JSC)

void Console::reportException(ExecState* exec, JSValue* exception)
{
    UString errorMessage = exception->toString(exec);
    JSObject* exceptionObject = exception->toObject(exec);
    int lineNumber = exceptionObject->get(exec, Identifier(exec, "line"))->toInt32(exec);
    UString exceptionSourceURL = exceptionObject->get(exec, Identifier(exec, "sourceURL"))->toString(exec);
    addMessage(JSMessageSource, ErrorMessageLevel, errorMessage, lineNumber, exceptionSourceURL);
    if (exec->hadException())
        exec->clearException();
}

void Console::reportCurrentException(ExecState* exec)
{
    JSValue* exception = exec->exception();
    exec->clearException();
    reportException(exec, exception);
}

#endif

static bool printExceptions = false;

bool Console::shouldPrintExceptions()
{
    return printExceptions;
}

void Console::setShouldPrintExceptions(bool print)
{
    printExceptions = print;
}

Page* Console::page() const
{
    if (!m_frame)
        return 0;
    return m_frame->page();
}

} // namespace WebCore
