#include "extensions/renderer/event_bookkeeper.h"

#include "base/values.h"
#include "base/lazy_instance.h"
#include "components/crx_file/id_util.h"
#include "extensions/common/constants.h"
#include "extensions/common/event_filter.h"
#include "extensions/common/value_counter.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/worker_thread_dispatcher.h"

namespace extensions {

namespace {

base::LazyInstance<EventBookkeeper>::DestructorAtExit g_main_thread_event_bookkeeper =
    LAZY_INSTANCE_INITIALIZER;

// Gets a unique string key identifier for a ScriptContext.
// TODO(kalman): Just use pointer equality...?
std::string GetKeyForScriptContext(ScriptContext* script_context) {
  const std::string& extension_id = script_context->GetExtensionID();
  CHECK(crx_file::id_util::IdIsValid(extension_id) ||
        script_context->url().is_valid());
  return crx_file::id_util::IdIsValid(extension_id)
             ? extension_id
             : script_context->url().spec();
}

}  // namespace

EventBookkeeper::~EventBookkeeper() {}
EventBookkeeper::EventBookkeeper() {}

// static
EventBookkeeper* EventBookkeeper::Get() {
  if (content::WorkerThread::GetCurrentId() == kNonWorkerThreadId)
    return &g_main_thread_event_bookkeeper.Get();
  return WorkerThreadDispatcher::Get()->GetEventBookkeeper();
}

int EventBookkeeper::IncrementEventListenerCount(
    ScriptContext* script_context,
    const std::string& event_name) {
  return ++listener_counts_lazy()[GetKeyForScriptContext(script_context)][event_name];
}

int EventBookkeeper::DecrementEventListenerCount(
    ScriptContext* script_context,
    const std::string& event_name) {
  return --listener_counts_lazy()
               [GetKeyForScriptContext(script_context)][event_name];
}

EventFilter& EventBookkeeper::GetEventFilter() {
  if (!event_filter_)
    event_filter_ = std::make_unique<EventFilter>();
  return *event_filter_;
}

EventBookkeeper::AllCounts& EventBookkeeper::listener_counts_lazy() {
  if (!listener_counts_)
    listener_counts_ = std::make_unique<AllCounts>();
  return *listener_counts_;
}

EventBookkeeper::FilteredEventListenerCounts& EventBookkeeper::filtered_listener_counts() {
  if (!filtered_listener_counts_)
    filtered_listener_counts_ = std::make_unique<FilteredEventListenerCounts>();
  return *filtered_listener_counts_;
}

EventBookkeeper::UnmanagedListenerMap& EventBookkeeper::unmanaged_listeners_lazy() {
  if (!unmanaged_listeners_)
    unmanaged_listeners_ = new UnmanagedListenerMap();
  return *unmanaged_listeners_;
}

bool EventBookkeeper::AddFilter(const std::string& event_name,
                                const ExtensionId& extension_id,
                                const base::DictionaryValue& filter) {
  FilteredEventListenerKey key(extension_id, event_name);
  FilteredEventListenerCounts& all_counts = filtered_listener_counts();
  FilteredEventListenerCounts::const_iterator counts = all_counts.find(key);
  if (counts == all_counts.end()) {
    counts =
        all_counts.insert(std::make_pair(key, std::make_unique<ValueCounter>()))
            .first;
  }
  return counts->second->Add(filter);
}

bool EventBookkeeper::RemoveFilter(const std::string& event_name,
                                   const ExtensionId& extension_id,
                                   base::DictionaryValue* filter) {
  FilteredEventListenerKey key(extension_id, event_name);
  FilteredEventListenerCounts& all_counts = filtered_listener_counts();
  FilteredEventListenerCounts::const_iterator counts = all_counts.find(key);
  if (counts == all_counts.end())
    return false;
  // Note: Remove() returns true if it removed the last filter equivalent to
  // |filter|. If there are more equivalent filters, or if there weren't any in
  // the first place, it returns false.
  if (counts->second->Remove(*filter)) {
    if (counts->second->is_empty())
      all_counts.erase(counts);  // Clean up if there are no more filters.
    return true;
  }
  return false;
}

bool EventBookkeeper::HasListener(ScriptContext* script_context,
                                  const std::string& event_name) {
  // Unmanaged event listeners.
  const auto& unmanaged_listeners = unmanaged_listeners_lazy();
  auto unmanaged_iter = unmanaged_listeners.find(script_context);
  if (unmanaged_iter != unmanaged_listeners.end() &&
      base::ContainsKey(unmanaged_iter->second, event_name)) {
    return true;
  }
  // Managed event listeners.
  const auto& managed_listeners = listener_counts_lazy();
  auto managed_iter =
      managed_listeners.find(GetKeyForScriptContext(script_context));
  if (managed_iter != managed_listeners.end()) {
    auto event_iter = managed_iter->second.find(event_name);
    if (event_iter != managed_iter->second.end() && event_iter->second > 0)
      return true;
  }
  return false;
}

void EventBookkeeper::AddUnmanagedEvent(
    ScriptContext* context,
    const std::string& event_name) {
  unmanaged_listeners_lazy()[context].insert(event_name);
}

void EventBookkeeper::RemoveUnmanagedEvent(ScriptContext* context,
                                           const std::string& event_name) {
  unmanaged_listeners_lazy()[context].erase(event_name);
}

void EventBookkeeper::RemoveAllUnmanagedListeners(ScriptContext* context) {
  unmanaged_listeners_lazy().erase(context);
}

}  // namespace extensions
