#include "vst3wrapper.h"

#include <cstdio>
#include <iostream>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;

const char *alloc_string(const char *str) {
  if (str == nullptr) {
    return nullptr;
  }
  size_t len = strlen(str);
  char *copy = new char[len + 1];
  strcpy(copy, str);
  return copy;
}

void send_param_change_event(const void *plugin_sent_events_producer,
                             int32_t id, float value, float initial_value,
                             bool end_edit = false) {
  PluginIssuedEvent event = {};
  event.tag = PluginIssuedEvent::Tag::Parameter;
  event.parameter = {};
  event.parameter._0 = {};
  event.parameter._0.parameter_id = (int32_t)id,
  event.parameter._0.current_value = value,
  event.parameter._0.end_edit = end_edit,
  event.parameter._0.initial_value = initial_value,
  send_event_to_host(&event, plugin_sent_events_producer);
}

class PlugFrame : public Steinberg::IPlugFrame {
public:
  const void *plugin_sent_events_producer = nullptr;

  PlugFrame(const void *_plugin_sent_events_producer) {
    plugin_sent_events_producer = _plugin_sent_events_producer;
  }

  Steinberg::tresult resizeView(Steinberg::IPlugView *view,
                                Steinberg::ViewRect *newSize) override {
    PluginIssuedEvent event = {};
    event.tag = PluginIssuedEvent::Tag::ResizeWindow;
    event.resize_window = {};
    event.resize_window._0 = (uintptr_t)newSize->getWidth();
    event.resize_window._1 = (uintptr_t)newSize->getHeight();

    send_event_to_host(&event, plugin_sent_events_producer);

    return Steinberg::kResultOk;
  }

  Steinberg::tresult queryInterface(const Steinberg::TUID /*_iid*/,
                                    void ** /*obj*/) override {
    return Steinberg::kNoInterface;
  }
  // we do not care here of the ref-counting. A plug-in call of release should
  // not destroy this class!
  Steinberg::uint32 addRef() override { return 1000; }
  Steinberg::uint32 release() override { return 1000; }
};

class ComponentHandler : public Steinberg::Vst::IComponentHandler {
public:
  std::vector<ParameterEditState> *param_edits = nullptr;
  std::mutex *param_edits_mutex = nullptr;
  const void *plugin_sent_events_producer = nullptr;

  ComponentHandler(std::vector<ParameterEditState> *_param_edits,
                   std::mutex *_param_edits_mutex,
                   const void *_plugin_sent_events_producer) {
    param_edits = _param_edits;
    param_edits_mutex = _param_edits_mutex;
    plugin_sent_events_producer = _plugin_sent_events_producer;
  }

  Steinberg::tresult beginEdit(Steinberg::Vst::ParamID id) override {
    // TODO
    return Steinberg::kResultOk;
  }

  Steinberg::tresult
  performEdit(Steinberg::Vst::ParamID id,
              Steinberg::Vst::ParamValue valueNormalized) override {
    if (!param_edits || !param_edits_mutex) {
      std::cout << "Param editing state was no initilaized" << std::endl;
      return Steinberg::kResultFalse;
    }

    std::lock_guard<std::mutex> guard(*param_edits_mutex);

    for (ParameterEditState &param : *param_edits) {
      if (param.id != id)
        continue;

      param.current_value = valueNormalized;

      send_param_change_event(plugin_sent_events_producer, id, valueNormalized,
                              param.initial_value);

      return Steinberg::kResultOk;
    }

    ParameterEditState state = {};
    state.id = id;
    state.finished = false;
    state.current_value = valueNormalized;
    state.initial_value = valueNormalized;

    param_edits->push_back(state);

    send_param_change_event(plugin_sent_events_producer, id, valueNormalized,
                            valueNormalized);

    return Steinberg::kResultOk;
  }

  Steinberg::tresult endEdit(Steinberg::Vst::ParamID id) override {
    std::lock_guard<std::mutex> guard(*param_edits_mutex);

    for (int i = 0; i < param_edits->size(); i++) {
      auto param = param_edits->at(i);
      if (param.id != id)
        continue;

      send_param_change_event(plugin_sent_events_producer, param.id,
                              param.current_value, param.initial_value, true);

      param_edits->erase(std::next(param_edits->begin(), i));

      return Steinberg::kResultOk;
    }

    send_param_change_event(plugin_sent_events_producer, id, NAN, NAN, true);

    return Steinberg::kResultOk;
  }

  Steinberg::tresult restartComponent(Steinberg::int32 flags) override {
    // TODO

    PluginIssuedEvent event = {};
    event.tag = PluginIssuedEvent::Tag::IOChanged;
    send_event_to_host(&event, plugin_sent_events_producer);

    return Steinberg::kResultOk;
  }

private:
  Steinberg::tresult queryInterface(const Steinberg::TUID /*_iid*/,
                                    void ** /*obj*/) override {
    return Steinberg::kNoInterface;
  }
  // we do not care here of the ref-counting. A plug-in call of release should
  // not destroy this class!
  Steinberg::uint32 addRef() override { return 1000; }
  Steinberg::uint32 release() override { return 1000; }
};

Steinberg::Vst::HostApplication *PluginInstance::_standardPluginContext =
    nullptr;
int PluginInstance::_standardPluginContextRefCount = 0;

PluginInstance::PluginInstance() {}

PluginInstance::~PluginInstance() { destroy(); }

const int MAX_BLOCK_SIZE = 4096 * 2;

bool PluginInstance::init(const std::string &path) {
  _destroy(false);

  ++_standardPluginContextRefCount;
  if (!_standardPluginContext) {
    _standardPluginContext = owned(NEW HostApplication());
    PluginContextFactory::instance().setPluginContext(_standardPluginContext);
  }

  _processSetup.symbolicSampleSize = 0;
  _processSetup.sampleRate = 44100;
  _processSetup.maxSamplesPerBlock = MAX_BLOCK_SIZE;

  _processData.numSamples = 0;
  _processData.processContext = &_processContext;

  std::string error;
  _module = VST3::Hosting::Module::create(path, error);
  if (!_module) {
    std::cout << "Failed to load VST3 module: " << error << std::endl;
    return false;
  }

  VST3::Hosting::PluginFactory factory = _module->getFactory();
  for (auto &classInfo : factory.classInfos()) {
    if (classInfo.category() == kVstAudioEffectClass) {
      return this->load_plugin_from_class(factory, classInfo);
    }
  }

  return true;
}

bool PluginInstance::load_plugin_from_class(
    VST3::Hosting::PluginFactory &factory,
    VST3::Hosting::ClassInfo &classInfo) {
  _plugProvider = owned(NEW PlugProvider(factory, classInfo, true));
  if (!_plugProvider) {
    printf("No PlugProvider found");
    return false;
  }

  _vstPlug = _plugProvider->getComponent();

  _audioEffect = FUnknownPtr<IAudioProcessor>(_vstPlug);
  if (!_audioEffect) {
    printf("Could not get audio processor from VST");
    return false;
  }

  _editController = _plugProvider->getController();
  if (_editController->initialize(_standardPluginContext) != kResultOk) {
    std::cout << "Failed to initialize editor context" << std::endl;
  }

  param_edits = {};

  component_handler = new ComponentHandler(&param_edits, &param_edits_mutex,
                                           plugin_sent_events_producer);
  _editController->setComponentHandler((ComponentHandler *)component_handler);

  Vst::IConnectionPoint *iConnectionPointComponent = nullptr;
  Vst::IConnectionPoint *iConnectionPointController = nullptr;

  _audioEffect->queryInterface(Vst::IConnectionPoint::iid,
                               (void **)&iConnectionPointComponent);
  _editController->queryInterface(Vst::IConnectionPoint::iid,
                                  (void **)&iConnectionPointController);

  if (iConnectionPointComponent && iConnectionPointController) {
    iConnectionPointComponent->connect(iConnectionPointController);
    iConnectionPointController->connect(iConnectionPointComponent);
  } else {
    std::cout << "Failed to get connection points." << std::endl;
  }

  auto stream = ResizableMemoryIBStream();

  if (_vstPlug->getState(&stream) == kResultTrue) {
    stream.rewind();
    _editController->setComponentState(&stream);
  }

  name = classInfo.name();
  vendor = classInfo.vendor();
  version = classInfo.version();
  id = classInfo.ID().toString();

  // TODO: Set bus arrangement

  tresult res = _audioEffect->setupProcessing(_processSetup);
  if (res == kResultOk) {
    _processData.prepare(*_vstPlug, MAX_BLOCK_SIZE,
                         _processSetup.symbolicSampleSize);
    if (_numInEventBuses > 0) {
      _processData.inputEvents = new EventList[_numInEventBuses];
    }
    if (_numOutEventBuses > 0) {
      _processData.outputEvents = new EventList[_numOutEventBuses];
    }
  } else {
    printf("Failed to setup VST processing");
  }

  if (_vstPlug->setActive(true) != kResultTrue) {
    printf("Failed to activate VST component");
  }

  get_io_config();

  return true;
}

void PluginInstance::destroy() { _destroy(true); }

void set_processing(const void *app, bool processing) {
  PluginInstance *vst = (PluginInstance *)app;
  vst->_audioEffect->setProcessing(processing);
}

Steinberg::Vst::ProcessContext *PluginInstance::processContext() {
  return &_processContext;
}

Steinberg::Vst::EventList *
PluginInstance::eventList(Steinberg::Vst::BusDirection direction, int which) {
  if (direction == kInput) {
    return static_cast<Steinberg::Vst::EventList *>(
        &_processData.inputEvents[which]);
  } else if (direction == kOutput) {
    return static_cast<Steinberg::Vst::EventList *>(
        &_processData.outputEvents[which]);
  } else {
    return nullptr;
  }
}

Steinberg::Vst::ParameterChanges *
PluginInstance::parameterChanges(Steinberg::Vst::BusDirection direction,
                                 int which) {
  if (direction == kInput) {
    return static_cast<Steinberg::Vst::ParameterChanges *>(
        &_processData.inputParameterChanges[which]);
  } else if (direction == kOutput) {
    return static_cast<Steinberg::Vst::ParameterChanges *>(
        &_processData.outputParameterChanges[which]);
  } else {
    return nullptr;
  }
}

Dims PluginInstance::createView(void *window_id) {
  if (!_editController) {
    printf("VST does not provide an edit controller");
    return {};
  }

  if (!_view) {
    _view = _editController->createView(ViewType::kEditor);
    if (!_view) {
      printf("EditController does not provide its own view");
      return {};
    }

    _view->setFrame(owned(new PlugFrame(plugin_sent_events_producer)));
  }

#ifdef _WIN32
  if (_view->isPlatformTypeSupported(Steinberg::kPlatformTypeHWND) !=
      Steinberg::kResultTrue) {
    printf("Editor view does not support HWND");
    return {};
  }
#else
  printf("Platform is not supported yet");
  return false;
#endif

#ifdef _WIN32
  if (_view->attached(window_id, Steinberg::kPlatformTypeHWND) !=
      Steinberg::kResultOk) {
    printf("Failed to attach editor view to HWND");
    return {};
  }
#endif

  ViewRect viewRect = {};
  if (_view->getSize(&viewRect) != kResultOk) {
    printf("Failed to get editor view size");
    return {};
  }

  return {
      viewRect.getWidth(),
      viewRect.getHeight(),
  };
}

IOConfigutaion PluginInstance::get_io_config() {
  IOConfigutaion io_config = {};
  io_config.audio_inputs = {};
  io_config.audio_outputs = {};

  auto audio_inputs =
      _vstPlug->getBusCount(MediaTypes::kAudio, BusDirections::kInput);
  auto audio_outputs =
      _vstPlug->getBusCount(MediaTypes::kAudio, BusDirections::kOutput);

  for (int i = 0; i < audio_inputs; i++) {
    BusInfo info;
    _vstPlug->getBusInfo(MediaTypes::kAudio, BusDirections::kInput, i, info);
    io_config.audio_inputs.count++;
    io_config.audio_inputs.data[i] = {};
    io_config.audio_inputs.data[i].value.channels = info.channelCount;
  }

  for (int i = 0; i < audio_outputs; i++) {
    BusInfo info;
    _vstPlug->getBusInfo(MediaTypes::kAudio, BusDirections::kOutput, i, info);
    io_config.audio_outputs.count++;
    io_config.audio_outputs.data[i] = {};
    io_config.audio_outputs.data[i].value.channels = info.channelCount;
  }

  // vst->_vstPlug->getBusCount(MediaTypes::kEvent, BusDirections::kInput);
  // vst->_vstPlug->getBusCount(MediaTypes::kEvent, BusDirections::kOutput);

  _io_config = io_config;

  return io_config;
}

void PluginInstance::_destroy(bool decrementRefCount) {
  // destroyView();
  _editController = nullptr;
  _audioEffect = nullptr;
  _vstPlug = nullptr;
  _plugProvider = nullptr;
  _module = nullptr;

  _inAudioBusInfos.clear();
  _outAudioBusInfos.clear();
  _numInAudioBuses = 0;
  _numOutAudioBuses = 0;

  _inEventBusInfos.clear();
  _outEventBusInfos.clear();
  _numInEventBuses = 0;
  _numOutEventBuses = 0;

  _inSpeakerArrs.clear();
  _outSpeakerArrs.clear();

  if (_processData.inputEvents) {
    delete[] static_cast<Steinberg::Vst::EventList *>(_processData.inputEvents);
  }
  if (_processData.outputEvents) {
    delete[] static_cast<Steinberg::Vst::EventList *>(
        _processData.outputEvents);
  }
  _processData.unprepare();
  _processData = {};

  _processSetup = {};
  _processContext = {};

  name = "";

  if (decrementRefCount) {
    if (_standardPluginContextRefCount > 0) {
      --_standardPluginContextRefCount;
    }
    if (_standardPluginContext && _standardPluginContextRefCount == 0) {
      PluginContextFactory::instance().setPluginContext(nullptr);
      _standardPluginContext->release();
      delete _standardPluginContext;
      _standardPluginContext = nullptr;
    }
  }
}

const void *load_plugin(const char *s,
                        const void *plugin_sent_events_producer) {
  PluginInstance *vst = new PluginInstance();
  vst->plugin_sent_events_producer = plugin_sent_events_producer;
  vst->init(s);

  auto aud_in = vst->_vstPlug->getBusCount(kAudio, kInput);
  for (int i = 0; i < aud_in; i++) {
    vst->_vstPlug->activateBus(kAudio, kInput, i, true);
  }

  auto aud_out = vst->_vstPlug->getBusCount(kAudio, kOutput);
  for (int i = 0; i < aud_out; i++) {
    vst->_vstPlug->activateBus(kAudio, kOutput, i, true);
  }

  auto evt_in = vst->_vstPlug->getBusCount(kEvent, kInput);
  for (int i = 0; i < evt_in; i++) {
    vst->_vstPlug->activateBus(kEvent, kInput, i, true);
  }

  // NOTE: Output event buses are not supported yet so they are not activated

  vst->_audioEffect->setProcessing(true);

  return vst;
}

Dims show_gui(const void *app, const void *window_id) {
  PluginInstance *vst = (PluginInstance *)app;

  return vst->createView((void *)window_id);
}

void hide_gui(const void *app) {
  PluginInstance *vst = (PluginInstance *)app;
  vst->_view->release();
  vst->_view = nullptr;
}

FFIPluginDescriptor descriptor(const void *app) {
  PluginInstance *vst = (PluginInstance *)app;

  FFIPluginDescriptor desc = {};
  desc.name = alloc_string(vst->name.c_str());
  desc.version = alloc_string(vst->version.c_str());
  desc.vendor = alloc_string(vst->vendor.c_str());
  desc.id = alloc_string(vst->id.c_str());

  return desc;
}

void vst3_set_sample_rate(const void *app, int32_t rate) {
  PluginInstance *vst = (PluginInstance *)app;
  vst->_processData.processContext->sampleRate = rate;
}

const void *get_data(const void *app, int32_t *data_len, const void **stream) {
  PluginInstance *vst = (PluginInstance *)app;

  ResizableMemoryIBStream *stream_ = new ResizableMemoryIBStream();
  *stream = stream_;

  if (vst->_vstPlug->getState(stream_) != kResultOk) {
    std::cout << "Failed to get plugin state. Non ok result." << std::endl;
    return nullptr;
  }

  Steinberg::int64 length = 0;
  stream_->tell(&length);
  *data_len = (int)length;

  return stream_->getData();
}

void free_data_stream(const void *stream) {
  ResizableMemoryIBStream *stream_ = (ResizableMemoryIBStream *)stream;
  delete stream_;
}

void set_data(const void *app, const void *data, int32_t data_len) {
  PluginInstance *vst = (PluginInstance *)app;

  ResizableMemoryIBStream stream = {};

  int num_bytes_written = 0;
  stream.write((void *)data, data_len, &num_bytes_written);
  assert(data_len == num_bytes_written);

  if (vst->_vstPlug->setState(&stream) != kResultOk) {
    std::cout << "Failed to set plugin state" << std::endl;
  }

  stream.rewind();
  vst->_editController->setComponentState(&stream);
}

void process(const void *app, const ProcessDetails *data, float ***input,
             float ***output, HostIssuedEvent *events, int32_t events_len) {
  PluginInstance *vst = (PluginInstance *)app;

  auto audio_inputs = vst->_io_config.audio_inputs.count;
  auto audio_outputs = vst->_io_config.audio_outputs.count;

  for (int i = 0; i < audio_inputs; i++) {
    vst->_processData.inputs->channelBuffers32 = input[i];
  }

  for (int i = 0; i < audio_outputs; i++) {
    vst->_processData.outputs[i].channelBuffers32 = output[i];
  }

  // if (vst->numBuses(kAudio, kOutput) > 0) {
  //   vst->_processData.outputs->channelBuffers32 = output;
  // }
  // if (vst->numBuses(kAudio, kInput) > 0) {
  //   vst->_processData.inputs->channelBuffers32 = input;
  // }

  Steinberg::uint32 state = 0;

  Steinberg::Vst::ProcessContext *ctx = vst->_processData.processContext;

  vst->_processData.processContext->tempo = data->tempo;
  state |= ctx->kTempoValid;

  vst->_processData.processContext->timeSigNumerator =
      data->time_signature_numerator;
  vst->_processData.processContext->timeSigDenominator =
      data->time_signature_denominator;
  state |= ctx->kTimeSigValid;

  vst->_processData.processContext->projectTimeMusic = data->player_time;

  vst->_processData.processContext->projectTimeSamples =
      (data->player_time / (data->tempo / 60.0)) * data->sample_rate;

  // TODO
  // vst->_processData.processContext->barPositionMusic = data.barPosBeats;
  state |= ctx->kBarPositionValid;

  vst->_processData.processContext->cycleStartMusic = data->cycle_start;
  vst->_processData.processContext->cycleEndMusic = data->cycle_end;
  state |= ctx->kCycleValid;

  vst->_processData.processContext->systemTime = data->nanos;
  state |= ctx->kSystemTimeValid;

  vst->_processData.processContext->frameRate.framesPerSecond = 60.;
  vst->_processData.processContext->frameRate.flags = 0;

  if (data->cycle_enabled) {
    state |= ctx->kCycleActive;
  }

  if (data->playing_state != PlayingState::Stopped) {
    state |= ctx->kPlaying;
  }

  if (data->playing_state == PlayingState::Recording) {
    state |= ctx->kRecording;
  }

  if (data->playing_state == PlayingState::OfflineRendering) {
    vst->_processData.processMode = kOffline;
  } else {
    vst->_processData.processMode = kRealtime;
  }

  vst->_processData.processContext->state = state;

  // int midi_bus = 0;
  // Steinberg::Vst::EventList *eventList = nullptr;
  // // if (vst->numBuses(kEvent, kInput) > 0) {
  // if (false) {
  //   eventList = vst->eventList(Steinberg::Vst::kInput, midi_bus);
  //   for (int i = 0; i < note_events_count; i++) {
  //     NoteEvent note = note_events[i];
  //
  //     Steinberg::Vst::Event evt = {};
  //     evt.busIndex = midi_bus;
  //     evt.sampleOffset = note.samples_offset;
  //     evt.ppqPosition = note.time_beats;
  //     // evt.flags = Steinberg::Vst::Event::EventFlags::kIsLive;
  //
  //     std::cout << note.note << std::endl;
  //
  //     if (note.on) {
  //       evt.type = Steinberg::Vst::Event::EventTypes::kNoteOnEvent;
  //       evt.noteOn.channel = note.channel;
  //       evt.noteOn.pitch = note.note;
  //       evt.noteOn.tuning = note.tuning;
  //       evt.noteOn.velocity = note.velocity;
  //       evt.noteOn.length = 0;
  //       evt.noteOn.noteId = -1;
  //     } else {
  //       evt.type = Steinberg::Vst::Event::EventTypes::kNoteOffEvent;
  //       evt.noteOff.channel = note.channel;
  //       evt.noteOff.pitch = note.note;
  //       evt.noteOff.tuning = note.tuning;
  //       evt.noteOff.velocity = note.velocity;
  //       evt.noteOff.noteId = -1;
  //     }
  //     eventList->addEvent(evt);
  //   }
  // }
  //
  // if (*parameter_change_count > 0) {
  //   if (!vst->_processData.inputParameterChanges) {
  //     vst->_processData.inputParameterChanges = new ParameterChanges(400);
  //   }
  //
  //   auto changes = vst->_processData.inputParameterChanges;
  //
  //   for (int i = 0; i < *parameter_change_count; i++) {
  //     int queue_index = 0;
  //     auto queue =
  //     changes->addParameterData(parameter_changes[i].parameter_id,
  //                                            queue_index);
  //     auto q = static_cast<ParameterValueQueue *>(queue);
  //     q->clear();
  //     int point_index = 0;
  //     if (queue->addPoint(0, parameter_changes[i].current_value, point_index)
  //     !=
  //         kResultOk) {
  //       std::cout << "Failed to set parameter" << std::endl;
  //     }
  //     std::cout << parameter_changes[i].parameter_id << " <- "
  //               << parameter_changes[i].current_value << std::endl;
  //   }
  // }

  tresult result = vst->_audioEffect->process(vst->_processData);
  if (result != kResultOk) {
    std::cout << "Failed to process" << std::endl;
  }

  // for (int i = 0; i < *parameter_change_count; i++) {
  //   int queue_index = 0;
  //   auto changes = vst->_processData.inputParameterChanges;
  //   auto queue = changes->addParameterData(parameter_changes[i].parameter_id,
  //                                          queue_index);
  //   auto q = static_cast<ParameterValueQueue *>(queue);
  //   q->clear();
  // }
  //
  // if (eventList) {
  //   eventList->clear();
  // }
}

void set_param_in_edit_controller(const void *app, int32_t id, float value) {
  PluginInstance *vst = (PluginInstance *)app;

  if (vst->_editController->setParamNormalized(id, value) != kResultOk) {
    std::cout << "Failed to set parameter normalized" << std::endl;
  }
}

void free_string(const char *str) { delete[] str; }

ParameterFFI get_parameter(const void *app, int32_t id) {
  // TODO: sort out naming confusion with id and index

  PluginInstance *vst = (PluginInstance *)app;

  ParameterInfo param_info = {};
  vst->_editController->getParameterInfo(id, param_info);

  // TODO: Make real-time safe with stack buffers

  std::string name = {};
  for (TChar c : param_info.title) {
    if (c != '\0') {
      name += c;
    }
  }

  Steinberg::Vst::ParamValue value =
      vst->_editController->getParamNormalized(param_info.id);

  TChar formatted_value[128] = {};
  if (vst->_editController->getParamStringByValue(
          param_info.id, value, formatted_value) != kResultOk) {
    std::cout << "Failed to get parameter value by string" << std::endl;
  }

  std::string formatted_value_c_str = {};
  for (TChar c : formatted_value) {
    if (c != '\0') {
      formatted_value_c_str += c;
    }
  }

  ParameterFFI param = {};
  param.id = id;
  param.index = param_info.id;
  param.value = (float)value;
  param.name = alloc_string(name.c_str());
  param.formatted_value = alloc_string(formatted_value_c_str.c_str());

  return param;
}

IOConfigutaion io_config(const void *app) {
  PluginInstance *vst = (PluginInstance *)app;

  return vst->get_io_config();
}
