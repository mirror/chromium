# Preference Service User Guide

[TOC]

## What is the Preference Service?

Preferences, also known as "prefs", are key-value pairs stored by
Chrome. Examples include the settins in chrome://settings, all per-extension
metadata, the list of plugins and so on. Individual prefs are keyed by a string
and have a type. E.g., the “browser.enable_spellchecking” pref stores a boolean
indicating whether spell-checking is enabled.

The prefs service persists prefs to disk and communicates updates to them
between services, including Chrome itself. There's one service instance per
profile and prefs are persisted on a pre-profile basis.

## Using the service

The service is used through a client library that offers clients fast and
synchronous access to prefs. To connect to the pref service and start reading
and writing prefs simply use the `ConnectToPrefService` factory function:

``` cpp
#include "services/preferences/public/cpp/pref_service_factory.h"

class ExampleService : public service_manager::Service {
  void OnStart() {
    auto* connector = context()->connector();
    auto pref_registry = base::MakeRefCounted<PrefRegistrySimple>();
    // Register any preferences you intend to use in |pref_registry|.
    prefs::ConnectToPrefService(
        connector, std::move(pref_registry), {},
        base::Bind(&ExampleService::OnPrefServiceConnected,
        base::Unretained(this)));
  }
  
  void OnPrefServiceConnected(std::unique_ptr<::PrefService> pref_service) {
    // Use |pref_service|.
  }
};
```

## Semantics

Updates made on the `PrefService` object are reflected immediately in the
originating service and eventually in all other services. In other words,
updates are eventually consistent.

## Design

The design doc is here:

https://docs.google.com/document/d/1JU8QUWxMEXWMqgkvFUumKSxr7Z-nfq0YvreSJTkMVmU/edit?usp=sharing
