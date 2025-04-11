#pragma once

#include <mutex>

#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"

#include "memoryibstream.h"
#include <pluginterfaces/gui/iplugview.h>
#include <public.sdk/source/vst/hosting/eventlist.h>
#include <public.sdk/source/vst/hosting/parameterchanges.h>
#include <public.sdk/source/vst/hosting/processdata.h>

#include "bindings.h"

struct ParameterChange {
  int id;
  float value;
};

struct ParameterEditState {
  int id;
  float initial_value;
  float current_value;
  bool finished;
};

class PluginInstance {
public:
  PluginInstance();
  ~PluginInstance();

  bool init(const std::string &path);
  void destroy();

  IOConfigutaion _io_config;
  IOConfigutaion get_io_config();

  Steinberg::Vst::ProcessContext *processContext();

  Steinberg::Vst::EventList *eventList(Steinberg::Vst::BusDirection direction,
                                       int which);
  Steinberg::Vst::ParameterChanges *
  parameterChanges(Steinberg::Vst::BusDirection direction, int which);

  bool load_plugin_from_class(VST3::Hosting::PluginFactory &factory,
                              VST3::Hosting::ClassInfo &classInfo);

  Dims createView(void *window_id);

  Steinberg::Vst::HostProcessData _processData = {};

  void _destroy(bool decrementRefCount);

  std::vector<Steinberg::Vst::BusInfo> _inAudioBusInfos, _outAudioBusInfos;
  int _numInAudioBuses = 0, _numOutAudioBuses = 0;

  std::vector<Steinberg::Vst::BusInfo> _inEventBusInfos, _outEventBusInfos;
  int _numInEventBuses = 0, _numOutEventBuses = 0;

  std::vector<Steinberg::Vst::SpeakerArrangement> _inSpeakerArrs,
      _outSpeakerArrs;

  VST3::Hosting::Module::Ptr _module = nullptr;
  Steinberg::IPtr<Steinberg::Vst::PlugProvider> _plugProvider = nullptr;

  Steinberg::IPtr<Steinberg::Vst::IComponent> _vstPlug = nullptr;
  Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> _audioEffect = nullptr;
  Steinberg::IPtr<Steinberg::Vst::IEditController> _editController = nullptr;

  void *component_handler = nullptr;

  Steinberg::Vst::ProcessSetup _processSetup = {};
  Steinberg::Vst::ProcessContext _processContext = {};

  Steinberg::IPtr<Steinberg::IPlugView> _view = nullptr;

  std::vector<ParameterEditState> param_edits;
  std::mutex param_edits_mutex;

  std::string name;
  std::string vendor;
  std::string version;
  std::string id;

  const void *plugin_sent_events_producer = nullptr;

  static Steinberg::Vst::HostApplication *_standardPluginContext;
  static int _standardPluginContextRefCount;
};
