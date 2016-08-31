// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/midi/midi_manager_winrt.h"

#include <comdef.h>
#include <robuffer.h>
#include <windows.devices.enumeration.h>
#include <windows.devices.midi.h>
#include <wrl/event.h>

#include <iomanip>
#include <unordered_map>
#include <unordered_set>

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_checker.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/timer/timer.h"
#include "base/win/scoped_comptr.h"
#include "base/win/windows_version.h"
#include "media/midi/midi_scheduler.h"

namespace media {
namespace midi {
namespace {

namespace WRL = Microsoft::WRL;

using namespace ABI::Windows::Devices::Enumeration;
using namespace ABI::Windows::Devices::Midi;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage::Streams;

using base::win::ScopedComPtr;

// Helpers for printing HRESULTs.
struct PrintHr {
  PrintHr(HRESULT hr) : hr(hr) {}
  HRESULT hr;
};

std::ostream& operator<<(std::ostream& os, const PrintHr& phr) {
  std::ios_base::fmtflags ff = os.flags();
  os << _com_error(phr.hr).ErrorMessage() << " (0x" << std::hex
     << std::uppercase << std::setfill('0') << std::setw(8) << phr.hr << ")";
  os.flags(ff);
  return os;
}

// Factory functions that activate and create WinRT components. The caller takes
// ownership of the returning ComPtr.
template <typename InterfaceType, base::char16 const* runtime_class_id>
ScopedComPtr<InterfaceType> WrlStaticsFactory() {
  ScopedComPtr<InterfaceType> com_ptr;

  HRESULT hr = GetActivationFactory(
      WRL::Wrappers::HStringReference(runtime_class_id).Get(),
      com_ptr.Receive());
  if (FAILED(hr)) {
    VLOG(1) << "GetActivationFactory failed: " << PrintHr(hr);
    com_ptr = nullptr;
  }

  return com_ptr;
}

template <typename T, HRESULT (T::*method)(HSTRING*)>
std::string GetStringFromObjectMethod(T* obj) {
  HSTRING result;
  HRESULT hr = (obj->*method)(&result);
  if (FAILED(hr)) {
    VLOG(1) << "GetStringFromObjectMethod failed: " << PrintHr(hr);
    return std::string();
  }

  // Note: empty HSTRINGs are represent as nullptr, and instantiating
  // std::string with nullptr (in base::WideToUTF8) is undefined behavior.
  const base::char16* buffer = WindowsGetStringRawBuffer(result, nullptr);
  if (buffer)
    return base::WideToUTF8(buffer);
  return std::string();
}

template <typename T>
std::string GetIdString(T* obj) {
  return GetStringFromObjectMethod<T, &T::get_Id>(obj);
}

template <typename T>
std::string GetDeviceIdString(T* obj) {
  return GetStringFromObjectMethod<T, &T::get_DeviceId>(obj);
}

std::string GetNameString(IDeviceInformation* info) {
  return GetStringFromObjectMethod<IDeviceInformation,
                                   &IDeviceInformation::get_Name>(info);
}

HRESULT GetPointerToBufferData(IBuffer* buffer, uint8_t** out) {
  ScopedComPtr<Windows::Storage::Streams::IBufferByteAccess> buffer_byte_access;

  HRESULT hr = buffer_byte_access.QueryFrom(buffer);
  if (FAILED(hr)) {
    VLOG(1) << "QueryInterface failed: " << PrintHr(hr);
    return hr;
  }

  // Lifetime of the pointing buffer is controlled by the buffer object.
  hr = buffer_byte_access->Buffer(out);
  if (FAILED(hr)) {
    VLOG(1) << "Buffer failed: " << PrintHr(hr);
    return hr;
  }

  return S_OK;
}

// Checks if given DeviceInformation represent a Microsoft GS Wavetable Synth
// instance.
bool IsMicrosoftSynthesizer(IDeviceInformation* info) {
  auto midi_synthesizer_statics =
      WrlStaticsFactory<IMidiSynthesizerStatics,
                        RuntimeClass_Windows_Devices_Midi_MidiSynthesizer>();
  boolean result = FALSE;
  HRESULT hr = midi_synthesizer_statics->IsSynthesizer(info, &result);
  VLOG_IF(1, FAILED(hr)) << "IsSynthesizer failed: " << PrintHr(hr);
  return result != FALSE;
}

// Tokens with value = 0 are considered invalid (as in <wrl/event.h>).
const int64_t kInvalidTokenValue = 0;

template <typename InterfaceType>
struct MidiPort {
  MidiPort() = default;

  uint32_t index;
  ScopedComPtr<InterfaceType> handle;
  EventRegistrationToken token_MessageReceived;

 private:
  DISALLOW_COPY_AND_ASSIGN(MidiPort);
};

}  // namespace

template <typename InterfaceType,
          typename RuntimeType,
          typename StaticsInterfaceType,
          base::char16 const* runtime_class_id>
class MidiManagerWinrt::MidiPortManager {
 public:
  // MidiPortManager instances should be constructed on the COM thread.
  MidiPortManager(MidiManagerWinrt* midi_manager)
      : midi_manager_(midi_manager),
        task_runner_(base::ThreadTaskRunnerHandle::Get()) {}

  virtual ~MidiPortManager() { DCHECK(thread_checker_.CalledOnValidThread()); }

  bool StartWatcher() {
    DCHECK(thread_checker_.CalledOnValidThread());

    HRESULT hr;

    midi_port_statics_ =
        WrlStaticsFactory<StaticsInterfaceType, runtime_class_id>();
    if (!midi_port_statics_)
      return false;

    HSTRING device_selector = nullptr;
    hr = midi_port_statics_->GetDeviceSelector(&device_selector);
    if (FAILED(hr)) {
      VLOG(1) << "GetDeviceSelector failed: " << PrintHr(hr);
      return false;
    }

    auto dev_info_statics = WrlStaticsFactory<
        IDeviceInformationStatics,
        RuntimeClass_Windows_Devices_Enumeration_DeviceInformation>();
    if (!dev_info_statics)
      return false;

    hr = dev_info_statics->CreateWatcherAqsFilter(device_selector,
                                                  watcher_.Receive());
    if (FAILED(hr)) {
      VLOG(1) << "CreateWatcherAqsFilter failed: " << PrintHr(hr);
      return false;
    }

    // Register callbacks to WinRT that post state-modifying jobs back to COM
    // thread. |weak_ptr| and |task_runner| are captured by lambda callbacks for
    // posting jobs. Note that WinRT callback arguments should not be passed
    // outside the callback since the pointers may be unavailable afterwards.
    base::WeakPtr<MidiPortManager> weak_ptr = GetWeakPtrFromFactory();
    scoped_refptr<base::SingleThreadTaskRunner> task_runner = task_runner_;

    hr = watcher_->add_Added(
        WRL::Callback<ITypedEventHandler<DeviceWatcher*, DeviceInformation*>>(
            [weak_ptr, task_runner](IDeviceWatcher* watcher,
                                    IDeviceInformation* info) {
              // Disable Microsoft GS Wavetable Synth due to security reasons.
              // http://crbug.com/499279
              if (IsMicrosoftSynthesizer(info))
                return S_OK;

              std::string dev_id = GetIdString(info),
                          dev_name = GetNameString(info);

              task_runner->PostTask(
                  FROM_HERE, base::Bind(&MidiPortManager::OnAdded, weak_ptr,
                                        dev_id, dev_name));

              return S_OK;
            })
            .Get(),
        &token_Added_);
    if (FAILED(hr)) {
      VLOG(1) << "add_Added failed: " << PrintHr(hr);
      return false;
    }

    hr = watcher_->add_EnumerationCompleted(
        WRL::Callback<ITypedEventHandler<DeviceWatcher*, IInspectable*>>(
            [weak_ptr, task_runner](IDeviceWatcher* watcher,
                                    IInspectable* insp) {
              task_runner->PostTask(
                  FROM_HERE,
                  base::Bind(&MidiPortManager::OnEnumerationCompleted,
                             weak_ptr));

              return S_OK;
            })
            .Get(),
        &token_EnumerationCompleted_);
    if (FAILED(hr)) {
      VLOG(1) << "add_EnumerationCompleted failed: " << PrintHr(hr);
      return false;
    }

    hr = watcher_->add_Removed(
        WRL::Callback<
            ITypedEventHandler<DeviceWatcher*, DeviceInformationUpdate*>>(
            [weak_ptr, task_runner](IDeviceWatcher* watcher,
                                    IDeviceInformationUpdate* update) {
              std::string dev_id = GetIdString(update);

              task_runner->PostTask(
                  FROM_HERE,
                  base::Bind(&MidiPortManager::OnRemoved, weak_ptr, dev_id));

              return S_OK;
            })
            .Get(),
        &token_Removed_);
    if (FAILED(hr)) {
      VLOG(1) << "add_Removed failed: " << PrintHr(hr);
      return false;
    }

    hr = watcher_->add_Stopped(
        WRL::Callback<ITypedEventHandler<DeviceWatcher*, IInspectable*>>(
            [](IDeviceWatcher* watcher, IInspectable* insp) {
              // Placeholder, does nothing for now.
              return S_OK;
            })
            .Get(),
        &token_Stopped_);
    if (FAILED(hr)) {
      VLOG(1) << "add_Stopped failed: " << PrintHr(hr);
      return false;
    }

    hr = watcher_->add_Updated(
        WRL::Callback<
            ITypedEventHandler<DeviceWatcher*, DeviceInformationUpdate*>>(
            [](IDeviceWatcher* watcher, IDeviceInformationUpdate* update) {
              // TODO(shaochuan): Check for fields to be updated here.
              return S_OK;
            })
            .Get(),
        &token_Updated_);
    if (FAILED(hr)) {
      VLOG(1) << "add_Updated failed: " << PrintHr(hr);
      return false;
    }

    hr = watcher_->Start();
    if (FAILED(hr)) {
      VLOG(1) << "Start failed: " << PrintHr(hr);
      return false;
    }

    is_initialized_ = true;
    return true;
  }

  void StopWatcher() {
    DCHECK(thread_checker_.CalledOnValidThread());

    HRESULT hr;

    for (const auto& entry : ports_)
      RemovePortEventHandlers(entry.second.get());

    if (token_Added_.value != kInvalidTokenValue) {
      hr = watcher_->remove_Added(token_Added_);
      VLOG_IF(1, FAILED(hr)) << "remove_Added failed: " << PrintHr(hr);
      token_Added_.value = kInvalidTokenValue;
    }
    if (token_EnumerationCompleted_.value != kInvalidTokenValue) {
      hr = watcher_->remove_EnumerationCompleted(token_EnumerationCompleted_);
      VLOG_IF(1, FAILED(hr)) << "remove_EnumerationCompleted failed: "
                             << PrintHr(hr);
      token_EnumerationCompleted_.value = kInvalidTokenValue;
    }
    if (token_Removed_.value != kInvalidTokenValue) {
      hr = watcher_->remove_Removed(token_Removed_);
      VLOG_IF(1, FAILED(hr)) << "remove_Removed failed: " << PrintHr(hr);
      token_Removed_.value = kInvalidTokenValue;
    }
    if (token_Stopped_.value != kInvalidTokenValue) {
      hr = watcher_->remove_Stopped(token_Stopped_);
      VLOG_IF(1, FAILED(hr)) << "remove_Stopped failed: " << PrintHr(hr);
      token_Stopped_.value = kInvalidTokenValue;
    }
    if (token_Updated_.value != kInvalidTokenValue) {
      hr = watcher_->remove_Updated(token_Updated_);
      VLOG_IF(1, FAILED(hr)) << "remove_Updated failed: " << PrintHr(hr);
      token_Updated_.value = kInvalidTokenValue;
    }

    if (is_initialized_) {
      hr = watcher_->Stop();
      VLOG_IF(1, FAILED(hr)) << "Stop failed: " << PrintHr(hr);
      is_initialized_ = false;
    }
  }

  MidiPort<InterfaceType>* GetPortByDeviceId(std::string dev_id) {
    DCHECK(thread_checker_.CalledOnValidThread());
    CHECK(is_initialized_);

    auto it = ports_.find(dev_id);
    if (it == ports_.end())
      return nullptr;
    return it->second.get();
  }

  MidiPort<InterfaceType>* GetPortByIndex(uint32_t port_index) {
    DCHECK(thread_checker_.CalledOnValidThread());
    CHECK(is_initialized_);

    return GetPortByDeviceId(port_ids_[port_index]);
  }

 protected:
  // Points to the MidiManagerWinrt instance, which is expected to outlive the
  // MidiPortManager instance.
  MidiManagerWinrt* midi_manager_;

  // Task runner of the COM thread.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Ensures all methods are called on the COM thread.
  base::ThreadChecker thread_checker_;

 private:
  // DeviceWatcher callbacks:
  void OnAdded(std::string dev_id, std::string dev_name) {
    DCHECK(thread_checker_.CalledOnValidThread());
    CHECK(is_initialized_);

    port_names_[dev_id] = dev_name;

    WRL::Wrappers::HString dev_id_hstring;
    HRESULT hr = dev_id_hstring.Set(base::UTF8ToWide(dev_id).c_str());
    if (FAILED(hr)) {
      VLOG(1) << "Set failed: " << PrintHr(hr);
      return;
    }

    IAsyncOperation<RuntimeType*>* async_op;

    hr = midi_port_statics_->FromIdAsync(dev_id_hstring.Get(), &async_op);
    if (FAILED(hr)) {
      VLOG(1) << "FromIdAsync failed: " << PrintHr(hr);
      return;
    }

    base::WeakPtr<MidiPortManager> weak_ptr = GetWeakPtrFromFactory();
    scoped_refptr<base::SingleThreadTaskRunner> task_runner = task_runner_;

    hr = async_op->put_Completed(
        WRL::Callback<IAsyncOperationCompletedHandler<RuntimeType*>>(
            [weak_ptr, task_runner](IAsyncOperation<RuntimeType*>* async_op,
                                    AsyncStatus status) {
              InterfaceType* handle;
              HRESULT hr = async_op->GetResults(&handle);
              if (FAILED(hr)) {
                VLOG(1) << "GetResults failed: " << PrintHr(hr);
                return hr;
              }

              // A reference to |async_op| is kept in |async_ops_|, safe to pass
              // outside.
              task_runner->PostTask(
                  FROM_HERE,
                  base::Bind(&MidiPortManager::OnCompletedGetPortFromIdAsync,
                             weak_ptr, handle, async_op));

              return S_OK;
            })
            .Get());
    if (FAILED(hr)) {
      VLOG(1) << "put_Completed failed: " << PrintHr(hr);
      return;
    }

    // Keep a reference to incompleted |async_op| for releasing later.
    async_ops_.insert(async_op);
  }

  void OnEnumerationCompleted() {
    DCHECK(thread_checker_.CalledOnValidThread());
    CHECK(is_initialized_);

    if (async_ops_.empty())
      midi_manager_->OnPortManagerReady();
    else
      enumeration_completed_not_ready_ = true;
  }

  void OnRemoved(std::string dev_id) {
    DCHECK(thread_checker_.CalledOnValidThread());
    CHECK(is_initialized_);

    // Note: in case Microsoft GS Wavetable Synth triggers this event for some
    // reason, it will be ignored here with log emitted.
    MidiPort<InterfaceType>* port = GetPortByDeviceId(dev_id);
    if (!port) {
      VLOG(1) << "Removing non-existent port " << dev_id;
      return;
    }

    SetPortState(port->index, MIDI_PORT_DISCONNECTED);

    RemovePortEventHandlers(port);
    port->handle = nullptr;
  }

  void OnCompletedGetPortFromIdAsync(InterfaceType* handle,
                                     IAsyncOperation<RuntimeType*>* async_op) {
    DCHECK(thread_checker_.CalledOnValidThread());
    CHECK(is_initialized_);

    EventRegistrationToken token = {kInvalidTokenValue};
    if (!RegisterOnMessageReceived(handle, &token))
      return;

    std::string dev_id = GetDeviceIdString(handle);

    MidiPort<InterfaceType>* port = GetPortByDeviceId(dev_id);

    if (port == nullptr) {
      // TODO(crbug.com/642604): Fill in manufacturer and driver version.
      AddPort(MidiPortInfo(dev_id, std::string("Manufacturer"),
                           port_names_[dev_id], std::string("DriverVersion"),
                           MIDI_PORT_OPENED));

      port = new MidiPort<InterfaceType>;
      port->index = static_cast<uint32_t>(port_ids_.size());

      ports_[dev_id].reset(port);
      port_ids_.push_back(dev_id);
    } else {
      SetPortState(port->index, MIDI_PORT_CONNECTED);
    }

    port->handle = handle;
    port->token_MessageReceived = token;

    // Manually release COM interface to completed |async_op|.
    auto it = async_ops_.find(async_op);
    CHECK(it != async_ops_.end());
    (*it)->Release();
    async_ops_.erase(it);

    if (enumeration_completed_not_ready_ && async_ops_.empty()) {
      midi_manager_->OnPortManagerReady();
      enumeration_completed_not_ready_ = false;
    }
  }

  // Overrided by MidiInPortManager to listen to input ports.
  virtual bool RegisterOnMessageReceived(InterfaceType* handle,
                                         EventRegistrationToken* p_token) {
    return true;
  }

  // Overrided by MidiInPortManager to remove MessageReceived event handler.
  virtual void RemovePortEventHandlers(MidiPort<InterfaceType>* port) {}

  // Calls midi_manager_->Add{Input,Output}Port.
  virtual void AddPort(MidiPortInfo info) = 0;

  // Calls midi_manager_->Set{Input,Output}PortState.
  virtual void SetPortState(uint32_t port_index, MidiPortState state) = 0;

  // WeakPtrFactory has to be declared in derived class, use this method to
  // retrieve upcasted WeakPtr for posting tasks.
  virtual base::WeakPtr<MidiPortManager> GetWeakPtrFromFactory() = 0;

  // Midi{In,Out}PortStatics instance.
  ScopedComPtr<StaticsInterfaceType> midi_port_statics_;

  // DeviceWatcher instance and event registration tokens for unsubscribing
  // events in destructor.
  ScopedComPtr<IDeviceWatcher> watcher_;
  EventRegistrationToken token_Added_ = {kInvalidTokenValue},
                         token_EnumerationCompleted_ = {kInvalidTokenValue},
                         token_Removed_ = {kInvalidTokenValue},
                         token_Stopped_ = {kInvalidTokenValue},
                         token_Updated_ = {kInvalidTokenValue};

  // All manipulations to these fields should be done on COM thread.
  std::unordered_map<std::string, std::unique_ptr<MidiPort<InterfaceType>>>
      ports_;
  std::vector<std::string> port_ids_;
  std::unordered_map<std::string, std::string> port_names_;

  // Keeps AsyncOperation references before the operation completes. Note that
  // raw pointers are used here and the COM interfaces should be released
  // manually.
  std::unordered_set<IAsyncOperation<RuntimeType*>*> async_ops_;

  // Set when device enumeration is completed but OnPortManagerReady() is not
  // called since some ports are not yet ready (i.e. |async_ops_| is not empty).
  // In such cases, OnPortManagerReady() will be called in
  // OnCompletedGetPortFromIdAsync() when the last pending port is ready.
  bool enumeration_completed_not_ready_ = false;

  // Set if the instance is initialized without error. Should be checked in all
  // methods on COM thread except StartWatcher().
  bool is_initialized_ = false;
};

class MidiManagerWinrt::MidiInPortManager final
    : public MidiPortManager<IMidiInPort,
                             MidiInPort,
                             IMidiInPortStatics,
                             RuntimeClass_Windows_Devices_Midi_MidiInPort> {
 public:
  MidiInPortManager(MidiManagerWinrt* midi_manager)
      : MidiPortManager(midi_manager), weak_factory_(this) {}

 private:
  // MidiPortManager overrides:
  bool RegisterOnMessageReceived(IMidiInPort* handle,
                                 EventRegistrationToken* p_token) override {
    DCHECK(thread_checker_.CalledOnValidThread());

    base::WeakPtr<MidiInPortManager> weak_ptr = weak_factory_.GetWeakPtr();
    scoped_refptr<base::SingleThreadTaskRunner> task_runner = task_runner_;

    HRESULT hr = handle->add_MessageReceived(
        WRL::Callback<
            ITypedEventHandler<MidiInPort*, MidiMessageReceivedEventArgs*>>(
            [weak_ptr, task_runner](IMidiInPort* handle,
                                    IMidiMessageReceivedEventArgs* args) {
              const base::TimeTicks now = base::TimeTicks::Now();

              std::string dev_id = GetDeviceIdString(handle);

              ScopedComPtr<IMidiMessage> message;
              HRESULT hr = args->get_Message(message.Receive());
              if (FAILED(hr)) {
                VLOG(1) << "get_Message failed: " << PrintHr(hr);
                return hr;
              }

              ScopedComPtr<IBuffer> buffer;
              hr = message->get_RawData(buffer.Receive());
              if (FAILED(hr)) {
                VLOG(1) << "get_RawData failed: " << PrintHr(hr);
                return hr;
              }

              uint8_t* p_buffer_data = nullptr;
              hr = GetPointerToBufferData(buffer.get(), &p_buffer_data);
              if (FAILED(hr))
                return hr;

              uint32_t data_length = 0;
              hr = buffer->get_Length(&data_length);
              if (FAILED(hr)) {
                VLOG(1) << "get_Length failed: " << PrintHr(hr);
                return hr;
              }

              std::vector<uint8_t> data(p_buffer_data,
                                        p_buffer_data + data_length);

              task_runner->PostTask(
                  FROM_HERE, base::Bind(&MidiInPortManager::OnMessageReceived,
                                        weak_ptr, dev_id, data, now));

              return S_OK;
            })
            .Get(),
        p_token);
    if (FAILED(hr)) {
      VLOG(1) << "add_MessageReceived failed: " << PrintHr(hr);
      return false;
    }

    return true;
  }

  void RemovePortEventHandlers(MidiPort<IMidiInPort>* port) override {
    if (!(port->handle &&
          port->token_MessageReceived.value != kInvalidTokenValue))
      return;

    HRESULT hr =
        port->handle->remove_MessageReceived(port->token_MessageReceived);
    VLOG_IF(1, FAILED(hr)) << "remove_MessageReceived failed: " << PrintHr(hr);
    port->token_MessageReceived.value = kInvalidTokenValue;
  }

  void AddPort(MidiPortInfo info) final { midi_manager_->AddInputPort(info); }

  void SetPortState(uint32_t port_index, MidiPortState state) final {
    midi_manager_->SetInputPortState(port_index, state);
  }

  base::WeakPtr<MidiPortManager> GetWeakPtrFromFactory() final {
    DCHECK(thread_checker_.CalledOnValidThread());

    return weak_factory_.GetWeakPtr();
  }

  // Callback on receiving MIDI input message.
  void OnMessageReceived(std::string dev_id,
                         std::vector<uint8_t> data,
                         base::TimeTicks time) {
    DCHECK(thread_checker_.CalledOnValidThread());

    MidiPort<IMidiInPort>* port = GetPortByDeviceId(dev_id);
    CHECK(port);

    midi_manager_->ReceiveMidiData(port->index, &data[0], data.size(), time);
  }

  // Last member to ensure destructed first.
  base::WeakPtrFactory<MidiInPortManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MidiInPortManager);
};

class MidiManagerWinrt::MidiOutPortManager final
    : public MidiPortManager<IMidiOutPort,
                             IMidiOutPort,
                             IMidiOutPortStatics,
                             RuntimeClass_Windows_Devices_Midi_MidiOutPort> {
 public:
  MidiOutPortManager(MidiManagerWinrt* midi_manager)
      : MidiPortManager(midi_manager), weak_factory_(this) {}

 private:
  // MidiPortManager overrides:
  void AddPort(MidiPortInfo info) final { midi_manager_->AddOutputPort(info); }

  void SetPortState(uint32_t port_index, MidiPortState state) final {
    midi_manager_->SetOutputPortState(port_index, state);
  }

  base::WeakPtr<MidiPortManager> GetWeakPtrFromFactory() final {
    DCHECK(thread_checker_.CalledOnValidThread());

    return weak_factory_.GetWeakPtr();
  }

  // Last member to ensure destructed first.
  base::WeakPtrFactory<MidiOutPortManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MidiOutPortManager);
};

MidiManagerWinrt::MidiManagerWinrt() : com_thread_("Windows MIDI COM Thread") {}

MidiManagerWinrt::~MidiManagerWinrt() {
  base::AutoLock auto_lock(lazy_init_member_lock_);

  CHECK(!com_thread_checker_);
  CHECK(!port_manager_in_);
  CHECK(!port_manager_out_);
  CHECK(!scheduler_);
}

void MidiManagerWinrt::StartInitialization() {
  if (base::win::GetVersion() < base::win::VERSION_WIN10) {
    VLOG(1) << "WinRT MIDI backend is only supported on Windows 10 or later.";
    CompleteInitialization(Result::INITIALIZATION_ERROR);
    return;
  }

  com_thread_.init_com_with_mta(true);
  com_thread_.Start();

  com_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&MidiManagerWinrt::InitializeOnComThread,
                            base::Unretained(this)));
}

void MidiManagerWinrt::Finalize() {
  com_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&MidiManagerWinrt::FinalizeOnComThread,
                            base::Unretained(this)));

  // Blocks until FinalizeOnComThread() returns. Delayed MIDI send data tasks
  // will be ignored.
  com_thread_.Stop();
}

void MidiManagerWinrt::DispatchSendMidiData(MidiManagerClient* client,
                                            uint32_t port_index,
                                            const std::vector<uint8_t>& data,
                                            double timestamp) {
  CHECK(scheduler_);

  scheduler_->PostSendDataTask(
      client, data.size(), timestamp,
      base::Bind(&MidiManagerWinrt::SendOnComThread, base::Unretained(this),
                 port_index, data));
}

void MidiManagerWinrt::InitializeOnComThread() {
  base::AutoLock auto_lock(lazy_init_member_lock_);

  com_thread_checker_.reset(new base::ThreadChecker);

  port_manager_in_.reset(new MidiInPortManager(this));
  port_manager_out_.reset(new MidiOutPortManager(this));

  scheduler_.reset(new MidiScheduler(this));

  if (!(port_manager_in_->StartWatcher() &&
        port_manager_out_->StartWatcher())) {
    port_manager_in_->StopWatcher();
    port_manager_out_->StopWatcher();
    CompleteInitialization(Result::INITIALIZATION_ERROR);
  }
}

void MidiManagerWinrt::FinalizeOnComThread() {
  base::AutoLock auto_lock(lazy_init_member_lock_);

  DCHECK(com_thread_checker_->CalledOnValidThread());

  scheduler_.reset();

  port_manager_in_->StopWatcher();
  port_manager_in_.reset();
  port_manager_out_->StopWatcher();
  port_manager_out_.reset();

  com_thread_checker_.reset();
}

void MidiManagerWinrt::SendOnComThread(uint32_t port_index,
                                       const std::vector<uint8_t>& data) {
  DCHECK(com_thread_checker_->CalledOnValidThread());

  MidiPort<IMidiOutPort>* port = port_manager_out_->GetPortByIndex(port_index);
  if (!(port && port->handle)) {
    VLOG(1) << "Port not available: " << port_index;
    return;
  }

  auto buffer_factory =
      WrlStaticsFactory<IBufferFactory,
                        RuntimeClass_Windows_Storage_Streams_Buffer>();
  if (!buffer_factory)
    return;

  ScopedComPtr<IBuffer> buffer;
  HRESULT hr = buffer_factory->Create(static_cast<UINT32>(data.size()),
                                      buffer.Receive());
  if (FAILED(hr)) {
    VLOG(1) << "Create failed: " << PrintHr(hr);
    return;
  }

  hr = buffer->put_Length(static_cast<UINT32>(data.size()));
  if (FAILED(hr)) {
    VLOG(1) << "put_Length failed: " << PrintHr(hr);
    return;
  }

  uint8_t* p_buffer_data = nullptr;
  hr = GetPointerToBufferData(buffer.get(), &p_buffer_data);
  if (FAILED(hr))
    return;

  std::copy(data.begin(), data.end(), p_buffer_data);

  hr = port->handle->SendBuffer(buffer.get());
  if (FAILED(hr)) {
    VLOG(1) << "SendBuffer failed: " << PrintHr(hr);
    return;
  }
}

void MidiManagerWinrt::OnPortManagerReady() {
  DCHECK(com_thread_checker_->CalledOnValidThread());
  DCHECK(port_manager_ready_count_ < 2);

  if (++port_manager_ready_count_ == 2)
    CompleteInitialization(Result::OK);
}

MidiManager* MidiManager::Create() {
  return new MidiManagerWinrt();
}

}  // namespace midi
}  // namespace media
