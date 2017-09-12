#ifndef EXTENSIONS_RENDERER_EVENT_BOOKKEEPER_H_
#define EXTENSIONS_RENDERER_EVENT_BOOKKEEPER_H_

#include <map>
#include <set>
#include <string>

#include "base/macros.h"
#include "extensions/common/extension_id.h"

namespace base {
class DictionaryValue;
}
namespace extensions {

class ScriptContext;
class EventFilter;
class ValueCounter;

class EventBookkeeper {
 public:
  EventBookkeeper();
  ~EventBookkeeper();

  static EventBookkeeper* GetForContext(ScriptContext* script_context);

  // Increments the number of event-listeners for the given |event_name| and
  // ScriptContext. Returns the count after the increment.
  int IncrementEventListenerCount(ScriptContext* script_context,
                                  const std::string& event_name);
  // Decrements the number of event-listeners for the given |event_name| and
  // ScriptContext. Returns the count after the increment.
  int DecrementEventListenerCount(ScriptContext* script_context,
                                  const std::string& event_name);

  EventFilter& GetEventFilter();

  // Adds a filter to |event_name| in |extension_id|, returning true if it
  // was the first filter for that event in that extension.
  bool AddFilter(const std::string& event_name,
                 const ExtensionId& extension_id,
                 const base::DictionaryValue& filter);
  // Removes a filter from |event_name| in |extension_id|, returning true if it
  // was the last filter for that event in that extension.
  bool RemoveFilter(const std::string& event_name,
                    const ExtensionId& extension_id,
                    base::DictionaryValue* filter);
  bool HasUnmanagedListener(
      ScriptContext* script_context,
      const std::string& event_name);
  bool HasManagedListener(
      ScriptContext* script_context,
      const std::string& event_name);

  void AddUnmanagedEvent(ScriptContext* context,
                         const std::string& event_name);
  void RemoveUnmanagedEvent(ScriptContext* context,
                            const std::string& event_name);
  void RemoveAllUnmanagedListeners(ScriptContext* context);

 private:
  using EventListenerCounts = std::map<std::string, int>;
  using AllCounts = std::map<std::string, EventListenerCounts>;


  // Lazily constructs.
  AllCounts& listener_counts_lazy();

  // A map of (extension ID, event name) pairs to the filtered listener counts
  // for that pair. The map is used to keep track of which filters are in effect
  // for which events.  We notify the browser about filtered event listeners when
  // we transition between 0 and 1.
  using FilteredEventListenerKey = std::pair<std::string, std::string>;
  using FilteredEventListenerCounts =
      std::map<FilteredEventListenerKey, std::unique_ptr<ValueCounter>>;

  using UnmanagedListenerMap =
    std::map<ScriptContext*, std::set<std::string>>;

  // A map of event names to the number of contexts listening to that event.
  // We notify the browser about event listeners when we transition between 0
  // and 1.
  std::unique_ptr<AllCounts> listener_counts_;

  // Lazily constructs.
  FilteredEventListenerCounts& filtered_listener_counts();

  // Lazily constructs.
  UnmanagedListenerMap& unmanaged_listeners_lazy();

  // unique_ptr for lazy behavior.
  // Not leaky.
  std::unique_ptr<EventFilter> event_filter_;

  // Lazy.
  // Not leaky.
  std::unique_ptr<FilteredEventListenerCounts> filtered_listener_counts_;

  // Lazy, leaky.
  // A collection of the unmanaged events (i.e., those for which the browser is
  // not notified of changes) that have listeners, by context.
  UnmanagedListenerMap* unmanaged_listeners_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(EventBookkeeper);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_EVENT_BOOKKEEPER_H_
