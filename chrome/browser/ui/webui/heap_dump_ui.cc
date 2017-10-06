// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/heap_dump_ui.h"

#include "base/memory/ptr_util.h"
#include "base/process/process_handle.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/components_resources.h"
#include "components/heap_dump/heap_dump.mojom.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace {

using content::RenderProcessHost;

content::WebUIDataSource* CreateHeapDumpHTMLSource() {
  // DO NOT SUBMIT
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create("heap-dump");

  source->SetJsonPath("strings.js");
  source->AddResourcePath("heap_dump.js", IDR_HEAP_DUMP_HEAP_DUMP_JS);
  source->SetDefaultResource(IDR_HEAP_DUMP_INDEX_HTML);

  return source;
}

class HeapDumpMessageHandler : public content::WebUIMessageHandler {
 protected:
  void RegisterMessages() override;

  void HandleRegisterForEvents(const base::ListValue* args);
  void HandleRequestProcessHeapDump(const base::ListValue* args);

  void SendRenderProcessHostList();

  void OnHeapDumpCompleted(heap_dump::mojom::HeapDumpPtr heap_dump,
                           bool success);
};

void HeapDumpMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "registerForEvents",
      base::Bind(&HeapDumpMessageHandler::HandleRegisterForEvents,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "requestProcessHeapDump",
      base::Bind(&HeapDumpMessageHandler::HandleRequestProcessHeapDump,
                 base::Unretained(this)));
}

void HeapDumpMessageHandler::HandleRegisterForEvents(
    const base::ListValue* args) {
  DCHECK(args->empty());
  AllowJavascript();

  SendRenderProcessHostList();
}

void HeapDumpMessageHandler::HandleRequestProcessHeapDump(
    const base::ListValue* args) {
  DCHECK_EQ(args->GetSize(), 1U);

  int render_process_id = 0;
  if (args->GetInteger(0, &render_process_id)) {
    RenderProcessHost* host = RenderProcessHost::FromID(render_process_id);
    if (!host)
      return;

    heap_dump::mojom::HeapDumpPtr heap_dump;
    BindInterface(host, &heap_dump);

    // TODO(siggi): Bounce this off a blocking thread to create a valid file.
    heap_dump->RequestHeapDump(
        mojo::ScopedHandle(),
        base::BindOnce(&HeapDumpMessageHandler::OnHeapDumpCompleted,
                       base::Unretained(this), std::move(heap_dump)));
  }
}

void HeapDumpMessageHandler::SendRenderProcessHostList() {
  base::Value hosts(base::Value::Type::LIST);
  // Send the new permissions to the renderers.
  for (RenderProcessHost::iterator i(RenderProcessHost::AllHostsIterator());
       !i.IsAtEnd(); i.Advance()) {
    RenderProcessHost* render_process_host = i.GetCurrentValue();

    base::Value host(base::Value::Type::DICTIONARY);

    host.SetKey("type", base::Value("renderer"));
    host.SetKey("id", base::Value(render_process_host->GetID()));
    host.SetKey("process_id", base::Value(static_cast<int>(base::GetProcId(
                                  render_process_host->GetHandle()))));

    hosts.GetList().push_back(std::move(host));
  }

  CallJavascriptFunction("getAllProcessesCallback", hosts);
}

void HeapDumpMessageHandler::OnHeapDumpCompleted(
    heap_dump::mojom::HeapDumpPtr heap_dump,
    bool success) {
  DebugBreak();
}

}  // namespace

HeapDumpUI::HeapDumpUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, CreateHeapDumpHTMLSource());

  web_ui->AddMessageHandler(base::MakeUnique<HeapDumpMessageHandler>());
}

HeapDumpUI::~HeapDumpUI() {
  DebugBreak();
}
