// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/midi/midi_manager_mac.h"

#include <stddef.h>

#include <algorithm>
#include <iterator>
#include <string>

#include <CoreAudio/HostTime.h>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "media/midi/midi_service.h"
#include "media/midi/task_service.h"

using base::IntToString;
using base::SysCFStringRefToUTF8;
using midi::mojom::PortState;
using midi::mojom::Result;

// NB: System MIDI types are pointer types in 32-bit and integer types in
// 64-bit. Therefore, the initialization is the simplest one that satisfies both
// (if possible).

namespace midi {

namespace {

// Maximum buffer size that CoreMIDI can handle for MIDIPacketList.
const size_t kCoreMIDIMaxPacketListSize = 65536;
// Pessimistic estimation on available data size of MIDIPacketList.
const size_t kEstimatedMaxPacketDataSize = kCoreMIDIMaxPacketListSize / 2;

enum {
  kDefaultRunnerNotUsedOnMac = TaskService::kDefaultRunnerId,
  kClientTaskRunner,
};

MidiPortInfo GetPortInfoFromEndpoint(MIDIEndpointRef endpoint) {
  std::string manufacturer;
  CFStringRef manufacturer_ref = NULL;
  OSStatus result = MIDIObjectGetStringProperty(
      endpoint, kMIDIPropertyManufacturer, &manufacturer_ref);
  if (result == noErr) {
    manufacturer = SysCFStringRefToUTF8(manufacturer_ref);
  } else {
    // kMIDIPropertyManufacturer is not supported in IAC driver providing
    // endpoints, and the result will be kMIDIUnknownProperty (-10835).
    DLOG(WARNING) << "Failed to get kMIDIPropertyManufacturer with status "
                  << result;
  }

  std::string name;
  CFStringRef name_ref = NULL;
  result = MIDIObjectGetStringProperty(endpoint, kMIDIPropertyDisplayName,
                                       &name_ref);
  if (result == noErr) {
    name = SysCFStringRefToUTF8(name_ref);
  } else {
    DLOG(WARNING) << "Failed to get kMIDIPropertyDisplayName with status "
                  << result;
  }

  std::string version;
  SInt32 version_number = 0;
  result = MIDIObjectGetIntegerProperty(
      endpoint, kMIDIPropertyDriverVersion, &version_number);
  if (result == noErr) {
    version = IntToString(version_number);
  } else {
    // kMIDIPropertyDriverVersion is not supported in IAC driver providing
    // endpoints, and the result will be kMIDIUnknownProperty (-10835).
    DLOG(WARNING) << "Failed to get kMIDIPropertyDriverVersion with status "
                  << result;
  }

  std::string id;
  SInt32 id_number = 0;
  result = MIDIObjectGetIntegerProperty(
      endpoint, kMIDIPropertyUniqueID, &id_number);
  if (result == noErr) {
    id = IntToString(id_number);
  } else {
    // On connecting some devices, e.g., nano KONTROL2, unknown endpoints
    // appear and disappear quickly and they fail on queries.
    // Let's ignore such ghost devices.
    // Same problems will happen if the device is disconnected before finishing
    // all queries.
    DLOG(WARNING) << "Failed to get kMIDIPropertyUniqueID with status "
                  << result;
  }

  const PortState state = PortState::OPENED;
  return MidiPortInfo(id, manufacturer, name, version, state);
}

double MIDITimeStampToSeconds(MIDITimeStamp timestamp) {
  UInt64 nanoseconds = AudioConvertHostTimeToNanos(timestamp);
  return static_cast<double>(nanoseconds) / 1.0e9;
}

MIDITimeStamp SecondsToMIDITimeStamp(double seconds) {
  UInt64 nanos = UInt64(seconds * 1.0e9);
  return AudioConvertNanosToHostTime(nanos);
}

}  // namespace

MidiManager* MidiManager::Create(MidiService* service) {
  return new MidiManagerMac(service);
}

MidiManagerMac::MidiManagerMac(MidiService* service) : MidiManager(service) {}

MidiManagerMac::~MidiManagerMac() = default;

void MidiManagerMac::StartInitialization() {
  if (!service()->task_service()->BindInstance()) {
    NOTREACHED();
    return CompleteInitialization(Result::INITIALIZATION_ERROR);
  }
  service()->task_service()->PostBoundTask(
      kClientTaskRunner, base::BindOnce(&MidiManagerMac::InitializeCoreMIDI,
                                        base::Unretained(this)));
}

void MidiManagerMac::Finalize() {
  if (!service()->task_service()->UnbindInstance()) {
    NOTREACHED();
  }

  // Do not need to dispose |coremidi_input_| and |coremidi_output_| explicitly.
  // CoreMIDI automatically dispose them on the client disposal.
  base::AutoLock lock(lock_);
  if (midi_client_)
    MIDIClientDispose(midi_client_);
  midi_client_ = 0;
}

void MidiManagerMac::DispatchSendMidiData(MidiManagerClient* client,
                                          uint32_t port_index,
                                          const std::vector<uint8_t>& data,
                                          double timestamp) {
  service()->task_service()->PostBoundTask(
      kClientTaskRunner,
      base::BindOnce(&MidiManagerMac::SendMidiData, base::Unretained(this),
                     client, port_index, data, timestamp));
}

void MidiManagerMac::InitializeCoreMIDI() {
  DCHECK(service()->task_service()->IsOnTaskRunner(kClientTaskRunner));

  // CoreMIDI registration.
  MIDIClientRef client = 0;
  OSStatus result = MIDIClientCreate(CFSTR("Chrome"), ReceiveMidiNotifyDispatch,
                                     this, &client);
  if (result != noErr || client == 0)
    return CompleteInitialization(Result::INITIALIZATION_ERROR);

  {
    base::AutoLock lock(lock_);
    midi_client_ = client;
  }

  // Create input and output port.
  MIDIPortRef input = 0;
  result = MIDIInputPortCreate(client, CFSTR("MIDI Input"), ReadMidiDispatch,
                               this, &input);
  if (result != noErr || input == 0)
    return CompleteInitialization(Result::INITIALIZATION_ERROR);

  MIDIPortRef output = 0;
  result = MIDIOutputPortCreate(client, CFSTR("MIDI Output"), &output);
  if (result != noErr || output == 0)
    return CompleteInitialization(Result::INITIALIZATION_ERROR);

  {
    base::AutoLock lock(lock_);
    coremidi_input_ = input;
    coremidi_output_ = output;
  }

  // Following loop may miss some newly attached devices, but such device will
  // be captured by ReceiveMidiNotifyDispatch callback.
  std::vector<MIDIEndpointRef> destinations;
  destinations.resize(MIDIGetNumberOfDestinations());
  for (size_t i = 0; i < destinations.size(); ++i) {
    MIDIEndpointRef destination = MIDIGetDestination(i);
    DCHECK(destination != 0);

    // Keep track of all destinations (known as outputs by the Web MIDI API).
    destinations[i] = destination;
  }
  {
    base::AutoLock lock(lock_);
    std::copy(destinations.begin(), destinations.end(),
              std::back_inserter(destinations_));

    // Allocate maximum size of buffer that CoreMIDI can handle.
    midi_buffer_.resize(kCoreMIDIMaxPacketListSize);
  }
  for (const auto destination : destinations)
    AddOutputPort(GetPortInfoFromEndpoint(destination));

  // Open connections from all sources. This loop also may miss new devices.
  std::vector<MIDIEndpointRef> sources;
  sources.resize(MIDIGetNumberOfSources());
  for (size_t i = 0; i < sources.size(); ++i) {
    MIDIEndpointRef source = MIDIGetSource(i);
    DCHECK(source != 0);

    // Keep track of all sources (known as inputs by the Web MIDI API).
    sources[i] = source;
  }
  {
    base::AutoLock lock(lock_);
    std::copy(sources.begin(), sources.end(), std::back_inserter(sources_));
  }
  for (size_t i = 0; i < sources.size(); ++i) {
    AddInputPort(GetPortInfoFromEndpoint(sources[i]));

    // Start listening.
    MIDIPortConnectSource(input, sources[i], reinterpret_cast<void*>(i));
  }

  CompleteInitialization(Result::OK);
}

// static
void MidiManagerMac::ReceiveMidiNotifyDispatch(const MIDINotification* message,
                                               void* refcon) {
  // This callback function is invoked on |kClientTaskRunner|.
  // |manager| should be valid because we can ensure |midi_client_| is still
  // alive here.
  MidiManagerMac* manager = static_cast<MidiManagerMac*>(refcon);
  manager->ReceiveMidiNotify(message);
}

void MidiManagerMac::ReceiveMidiNotify(const MIDINotification* message) {
  DCHECK(service()->task_service()->IsOnTaskRunner(kClientTaskRunner));

  if (kMIDIMsgObjectAdded == message->messageID) {
    // New device is going to be attached.
    const MIDIObjectAddRemoveNotification* notification =
        reinterpret_cast<const MIDIObjectAddRemoveNotification*>(message);
    MIDIEndpointRef endpoint =
        static_cast<MIDIEndpointRef>(notification->child);
    if (notification->childType == kMIDIObjectType_Source) {
      // Attaching device is an input device.
      bool new_device;
      size_t index;
      {
        base::AutoLock lock(lock_);
        auto it = std::find(sources_.begin(), sources_.end(), endpoint);
        new_device = it == sources_.end();
        index = new_device ? sources_.size() : it - sources_.begin();
      }
      if (new_device) {
        MidiPortInfo info = GetPortInfoFromEndpoint(endpoint);
        // If the device disappears before finishing queries, MidiPortInfo
        // becomes incomplete. Skip and do not cache such information here.
        // On kMIDIMsgObjectRemoved, the entry will be ignored because it
        // will not be found in the pool.
        if (!info.id.empty()) {
          MIDIPortRef input;
          {
            base::AutoLock lock(lock_);
            input = coremidi_input_;
            sources_.push_back(endpoint);
          }
          AddInputPort(info);
          MIDIPortConnectSource(input, endpoint,
                                reinterpret_cast<void*>(index));
        }
      } else {
        SetInputPortState(index, PortState::OPENED);
      }
    } else if (notification->childType == kMIDIObjectType_Destination) {
      // Attaching device is an output device.
      bool new_device;
      size_t index;
      {
        base::AutoLock lock(lock_);
        auto it =
            std::find(destinations_.begin(), destinations_.end(), endpoint);
        new_device = it == destinations_.end();
        index = it - destinations_.begin();
      }
      if (new_device) {
        MidiPortInfo info = GetPortInfoFromEndpoint(endpoint);
        // Skip cases that queries are not finished correctly.
        if (!info.id.empty()) {
          {
            base::AutoLock lock(lock_);
            destinations_.push_back(endpoint);
          }
          AddOutputPort(info);
        }
      } else {
        SetOutputPortState(index, PortState::OPENED);
      }
    }
  } else if (kMIDIMsgObjectRemoved == message->messageID) {
    // Existing device is going to be detached.
    const MIDIObjectAddRemoveNotification* notification =
        reinterpret_cast<const MIDIObjectAddRemoveNotification*>(message);
    MIDIEndpointRef endpoint =
        static_cast<MIDIEndpointRef>(notification->child);
    if (notification->childType == kMIDIObjectType_Source) {
      // Detaching device is an input device.
      bool found;
      size_t index;
      {
        base::AutoLock lock(lock_);
        auto it = std::find(sources_.begin(), sources_.end(), endpoint);
        found = it != sources_.end();
        index = it - sources_.begin();
      }
      if (found)
        SetInputPortState(index, PortState::DISCONNECTED);
    } else if (notification->childType == kMIDIObjectType_Destination) {
      // Detaching device is an output device.
      bool found;
      size_t index;
      {
        base::AutoLock lock(lock_);
        auto it =
            std::find(destinations_.begin(), destinations_.end(), endpoint);
        found = it != destinations_.end();
        index = it - destinations_.begin();
      }
      if (found)
        SetOutputPortState(index, PortState::DISCONNECTED);
    }
  }
}

// static
void MidiManagerMac::ReadMidiDispatch(const MIDIPacketList* packet_list,
                                      void* read_proc_refcon,
                                      void* src_conn_refcon) {
  // This method is called on a separate high-priority thread owned by CoreMIDI.
  // |manager| should be valid because we can ensure |midi_client_| is still
  // alive here.
  MidiManagerMac* manager = static_cast<MidiManagerMac*>(read_proc_refcon);
  DCHECK(manager);
  uint32_t port_index = reinterpret_cast<uint64_t>(src_conn_refcon);

  // Go through each packet and process separately.
  const MIDIPacket* packet = &packet_list->packet[0];
  for (size_t i = 0; i < packet_list->numPackets; i++) {
    // Each packet contains MIDI data for one or more messages (like note-on).
    double timestamp_seconds = MIDITimeStampToSeconds(packet->timeStamp);

    manager->ReceiveMidiData(port_index, packet->data, packet->length,
                             timestamp_seconds);

    packet = MIDIPacketNext(packet);
  }
}

void MidiManagerMac::SendMidiData(MidiManagerClient* client,
                                  uint32_t port_index,
                                  const std::vector<uint8_t>& data,
                                  double timestamp) {
  DCHECK(service()->task_service()->IsOnTaskRunner(kClientTaskRunner));

  MIDIPortRef output;
  MIDIEndpointRef destination;
  MIDIPacketList* packet_list;
  {
    // Lookup the destination based on the port index.
    base::AutoLock lock(lock_);
    if (static_cast<size_t>(port_index) >= destinations_.size())
      return;
    output = coremidi_output_;
    destination = destinations_[port_index];
    packet_list = reinterpret_cast<MIDIPacketList*>(midi_buffer_.data());
  }

  MIDITimeStamp coremidi_timestamp = SecondsToMIDITimeStamp(timestamp);

  size_t send_size;
  for (size_t sent_size = 0; sent_size < data.size(); sent_size += send_size) {
    MIDIPacket* midi_packet = MIDIPacketListInit(packet_list);
    // Limit the maximum payload size to kEstimatedMaxPacketDataSize that is
    // half of midi_buffer data size. MIDIPacketList and MIDIPacket consume
    // extra buffer areas for meta information, and available size is smaller
    // than buffer size. Here, we simply assume that at least half size is
    // available for data payload.
    send_size = std::min(data.size() - sent_size, kEstimatedMaxPacketDataSize);
    midi_packet = MIDIPacketListAdd(
        packet_list,
        kCoreMIDIMaxPacketListSize,
        midi_packet,
        coremidi_timestamp,
        send_size,
        &data[sent_size]);
    DCHECK(midi_packet);

    MIDISend(output, destination, packet_list);
  }

  AccumulateMidiBytesSent(client, data.size());
}

}  // namespace midi
