#include "componenthandler.h"
#include <iostream>
#include "pluginterfaces/vst/ivsteditcontroller.h"

ComponentHandler::ComponentHandler() {}

Steinberg::tresult ComponentHandler::beginEdit(Steinberg::Vst::ParamID id) {
  // TODO
  return Steinberg::kResultOk;
}

Steinberg::tresult
ComponentHandler::performEdit(Steinberg::Vst::ParamID id,
                              Steinberg::Vst::ParamValue valueNormalized) {
  std::lock_guard<std::mutex> guard(param_edits_mutex);

  for (ParameterEditState &param : param_edits) {
    if (param.id != id)
      continue;

    param.current_value = valueNormalized;

    send_param_change_event(id, valueNormalized, param.initial_value);

    return Steinberg::kResultOk;
  }

  ParameterEditState state = {};
  state.id = id;
  state.finished = false;
  state.current_value = valueNormalized;
  state.initial_value = valueNormalized;

  param_edits.push_back(state);

  send_param_change_event(id, valueNormalized, valueNormalized);

  return Steinberg::kResultOk;
}

Steinberg::tresult ComponentHandler::endEdit(Steinberg::Vst::ParamID id) {
  std::lock_guard<std::mutex> guard(param_edits_mutex);

  for (int i = 0; i < param_edits.size(); i++) {
    auto param = param_edits.at(i);
    if (param.id != id)
      continue;

    send_param_change_event(param.id, param.current_value, param.initial_value,
                            true);

    param_edits.erase(std::next(param_edits.begin(), i));

    return Steinberg::kResultOk;
  }

  send_param_change_event(id, NAN, NAN, true);

  return Steinberg::kResultOk;
}

void log(const char *msg) {
  // TODO
}

// [UI-thread & Connected] 
Steinberg::tresult ComponentHandler::restartComponent(Steinberg::int32 flags) {
  // TODO

  // https://steinbergmedia.github.io/vst3_doc/vstinterfaces/namespaceSteinberg_1_1Vst.html#a35f71d02b79cbd11942a149e651373e6


  if ((flags & Steinberg::Vst::kReloadComponent) != 0) {
    log("kReloadComponent");
  }

  if ((flags & Steinberg::Vst::kIoChanged) != 0) {
    log("kIoChanged");
    PluginIssuedEvent event = {};
    event.tag = PluginIssuedEvent::Tag::IOChanged;
    send_event_to_host(&event, rust_side_vst3_instance_object);
  }

  if ((flags & Steinberg::Vst::kParamValuesChanged) != 0) {
    log("kParamValuesChanged");
  }

  if ((flags & Steinberg::Vst::kLatencyChanged) != 0) {
    log("kLatencyChanged");
  }

  if ((flags & Steinberg::Vst::kParamTitlesChanged) != 0) {
    log("kParamTitlesChanged");
  }

  if ((flags & Steinberg::Vst::kMidiCCAssignmentChanged) != 0) {
    log("kMidiCCAssignmentChanged");
  }

  if ((flags & Steinberg::Vst::kNoteExpressionChanged) != 0) {
    log("kNoteExpressionChanged");
  }

  if ((flags & Steinberg::Vst::kIoTitlesChanged) != 0) {
    log("kIoTitlesChanged");
  }

  if ((flags & Steinberg::Vst::kPrefetchableSupportChanged) != 0) {
    log("kPrefetchableSupportChanged");
  }

  if ((flags & Steinberg::Vst::kRoutingInfoChanged) != 0) {
    log("kRoutingInfoChanged");
  }

  if ((flags & Steinberg::Vst::kKeyswitchChanged) != 0) {
    log("kKeyswitchChanged");
  }

  // if ((flags & Steinberg::Vst::kParamIDMappingChanged) != 0) {
  // }

  return Steinberg::kResultOk;
}

Steinberg::tresult
ComponentHandler::queryInterface(const Steinberg::TUID /*_iid*/,
                                 void ** /*obj*/) {
  return Steinberg::kNoInterface;
}
// we do not care here of the ref-counting. A plug-in call of release should
// not destroy this class!
Steinberg::uint32 ComponentHandler::addRef() { return 1000; }
Steinberg::uint32 ComponentHandler::release() { return 1000; }

void ComponentHandler::send_param_change_event(int32_t id, float value,
                                               float initial_value,
                                               bool end_edit) {
  PluginIssuedEvent event = {};
  event.tag = PluginIssuedEvent::Tag::Parameter;
  event.parameter = {};
  event.parameter._0 = {};

  int32_t param_index = -1;
  auto it = parameter_indicies.find(id);
  if (it != parameter_indicies.end()) {
    param_index = (int32_t)it->second;
  }

  event.parameter._0.parameter_id = id,
  event.parameter._0.parameter_index = param_index,
  event.parameter._0.current_value = value,
  event.parameter._0.end_edit = end_edit,
  event.parameter._0.initial_value = initial_value,
  send_event_to_host(&event, rust_side_vst3_instance_object);
}
