#include "vst3wrapper.h"

#include <cstdio>
#include <iostream>
#include <vector>

class ComponentHandler : public Steinberg::Vst::IComponentHandler
{
public:
	std::vector<ParameterEditState>* param_edits = nullptr;
	std::vector<ParameterEditState>* dirty_edits = nullptr;
	std::mutex* param_edits_mutex = nullptr;

	ComponentHandler(std::vector<ParameterEditState>* _param_edits, std::vector<ParameterEditState>* _dirty_edits, std::mutex* _param_edits_mutex) {
		param_edits = _param_edits;
		dirty_edits = _dirty_edits;
		param_edits_mutex = _param_edits_mutex;
	}

	Steinberg::tresult beginEdit(Steinberg::Vst::ParamID id) override {
		printf("beginEdit called (%d)\n", id);
		return Steinberg::kResultOk;
	}

	Steinberg::tresult performEdit(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized) override {
		printf("performEdit called (%d, %f)\n", id, valueNormalized);

		if (!param_edits || !dirty_edits || !param_edits_mutex) {
			std::cout << "Did not have something" << std::endl;
			return Steinberg::kResultFalse;
		}

		std::lock_guard<std::mutex> guard(*param_edits_mutex);

		for (ParameterEditState& param : *param_edits) {
			if (param.id != id) continue;
		
			param.current_value = valueNormalized;

			for (int i = (int)dirty_edits->size() - 1; i >= 0; i--) {
				if ((*dirty_edits)[i].id == param.id) {
					dirty_edits->erase(std::next(dirty_edits->begin(), i));
				}
			}

			dirty_edits->push_back(param);

			return Steinberg::kResultOk;
		}
			
		ParameterEditState state = {};
		state.id = id;
		state.finished = false;
		state.current_value = valueNormalized;
		state.inital_value = valueNormalized;

		param_edits->push_back(state);
		dirty_edits->push_back(state);

		return Steinberg::kResultOk;
	}

	Steinberg::tresult endEdit(Steinberg::Vst::ParamID id) override {
		printf("endEdit called (%d)\n", id);

		std::lock_guard<std::mutex> guard(*param_edits_mutex);

		for (int i = 0; i < param_edits->size(); i++) {
			auto param = param_edits->at(i);
			if (param.id != id) continue;

			param.finished = true;

			dirty_edits->push_back(param);

			param_edits->erase(std::next(param_edits->begin(), i));

			return Steinberg::kResultOk;
		}

		ParameterEditState state = {};
		state.id = id;
		state.current_value = NAN;
		state.inital_value = NAN;
		state.finished = true;

		dirty_edits->push_back(state);

		return Steinberg::kResultOk;
	}

	Steinberg::tresult restartComponent(Steinberg::int32 flags) override {
		printf("restartComponent called (%d)\n", flags);
		return Steinberg::kNotImplemented;
	}

private:

	Steinberg::tresult queryInterface(const Steinberg::TUID /*_iid*/, void** /*obj*/) override
	{
		return Steinberg::kNoInterface;
	}
	// we do not care here of the ref-counting. A plug-in call of release should not destroy this
	// class!
	Steinberg::uint32 addRef() override { return 1000; }
	Steinberg::uint32 release() override { return 1000; }
};

Steinberg::Vst::HostApplication* EasyVst::_standardPluginContext = nullptr;
int EasyVst::_standardPluginContextRefCount = 0;

using namespace Steinberg;
using namespace Steinberg::Vst;

EasyVst::EasyVst() {}

EasyVst::~EasyVst() { destroy(); }

#define NEW new

bool EasyVst::init(const std::string& path, int sampleRate, int maxBlockSize,
	int symbolicSampleSize, bool realtime) {
	_destroy(false);

	++_standardPluginContextRefCount;
	if (!_standardPluginContext) {
		_standardPluginContext = owned(NEW HostApplication());
		PluginContextFactory::instance().setPluginContext(_standardPluginContext);
	}

	_path = path;
	_sampleRate = sampleRate;
	_maxBlockSize = maxBlockSize;
	_symbolicSampleSize = symbolicSampleSize;
	_realtime = realtime;

	_processSetup.processMode = _realtime;
	_processSetup.symbolicSampleSize = _symbolicSampleSize;
	_processSetup.sampleRate = _sampleRate;
	_processSetup.maxSamplesPerBlock = _maxBlockSize;

	_processData.numSamples = 0;
	_processData.symbolicSampleSize = _symbolicSampleSize;
	_processData.processContext = &_processContext;

	std::string error;
	_module = VST3::Hosting::Module::create(path, error);
	if (!_module) {
		_printError(error);
		return false;
	}

	VST3::Hosting::PluginFactory factory = _module->getFactory();
	for (auto& classInfo : factory.classInfos()) {
		if (classInfo.category() == kVstAudioEffectClass) {
			_plugProvider = owned(NEW PlugProvider(factory, classInfo, true));
			if (!_plugProvider) {
				_printError("No PlugProvider found");
				return false;
			}

			_vstPlug = _plugProvider->getComponent();

			_audioEffect = FUnknownPtr<IAudioProcessor>(_vstPlug);
			if (!_audioEffect) {
				_printError("Could not get audio processor from VST");
				return false;
			}

			_editController = _plugProvider->getController();
			if (_editController->initialize(_standardPluginContext) != kResultOk) {
				std::cout << "Failed to initialize editor context" << std::endl;
			}

			param_edits = {};
			dirty_edits = {};

			component_handler = new ComponentHandler(&param_edits, &dirty_edits, &param_edits_mutex);
			_editController->setComponentHandler((ComponentHandler*) component_handler);
				
			Vst::IConnectionPoint* iConnectionPointComponent = nullptr;
			Vst::IConnectionPoint* iConnectionPointController = nullptr;

			_audioEffect->queryInterface(Vst::IConnectionPoint::iid, (void**)&iConnectionPointComponent);
			_editController->queryInterface(Vst::IConnectionPoint::iid, (void**)&iConnectionPointController);

			if (iConnectionPointComponent && iConnectionPointController) {
				iConnectionPointComponent->connect(iConnectionPointController);
				iConnectionPointController->connect(iConnectionPointComponent);
			} else {
				std::cout << "Failed to get connection points." << std::endl;
			}

			// auto stream = ResizableMemoryIBStream();

			// if (_vstPlug->getState(&stream) == kResultTrue) {
			//	stream.rewind();
			//	_editController->setComponentState(&stream);
			//}

			_name = classInfo.name();

			FUnknownPtr<IProcessContextRequirements> contextRequirements(
				_audioEffect);
			if (contextRequirements) {
				auto flags = contextRequirements->getProcessContextRequirements();

#define PRINT_FLAG(x)                                                          \
  if (flags & IProcessContextRequirements::Flags::x) {                         \
    _printDebug(#x);                                                           \
  }
				PRINT_FLAG(kNeedSystemTime)
					PRINT_FLAG(kNeedContinousTimeSamples)
					PRINT_FLAG(kNeedProjectTimeMusic)
					PRINT_FLAG(kNeedBarPositionMusic)
					PRINT_FLAG(kNeedCycleMusic)
					PRINT_FLAG(kNeedSamplesToNextClock)
					PRINT_FLAG(kNeedTempo)
					PRINT_FLAG(kNeedTimeSignature)
					PRINT_FLAG(kNeedChord)
					PRINT_FLAG(kNeedFrameRate)
					PRINT_FLAG(kNeedTransportState)
#undef PRINT_FLAG
			}

			_numInAudioBuses =
				_vstPlug->getBusCount(MediaTypes::kAudio, BusDirections::kInput);
			_numOutAudioBuses =
				_vstPlug->getBusCount(MediaTypes::kAudio, BusDirections::kOutput);
			_numInEventBuses =
				_vstPlug->getBusCount(MediaTypes::kEvent, BusDirections::kInput);
			_numOutEventBuses =
				_vstPlug->getBusCount(MediaTypes::kEvent, BusDirections::kOutput);

			std::cout << "Buses: " << _numInAudioBuses << " audio and "
				<< _numInEventBuses << " event inputs; " << std::endl;
			std::cout << _numOutAudioBuses << " audio and " << _numOutEventBuses
				<< " event outputs" << std::endl;


			for (int i = 0; i < _numInAudioBuses; ++i) {
				BusInfo info;
				_vstPlug->getBusInfo(kAudio, kInput, i, info);
				_inAudioBusInfos.push_back(info);
				setBusActive(kAudio, kInput, i, false);

				SpeakerArrangement speakerArr;
				_audioEffect->getBusArrangement(kInput, i, speakerArr);
				_inSpeakerArrs.push_back(speakerArr);
			}

			for (int i = 0; i < _numInEventBuses; ++i) {
				BusInfo info;
				_vstPlug->getBusInfo(kEvent, kInput, i, info);
				_inEventBusInfos.push_back(info);
				setBusActive(kEvent, kInput, i, false);
			}

			for (int i = 0; i < _numOutAudioBuses; ++i) {
				BusInfo info;
				_vstPlug->getBusInfo(kAudio, kOutput, i, info);
				_outAudioBusInfos.push_back(info);
				setBusActive(kAudio, kOutput, i, false);

				SpeakerArrangement speakerArr;
				_audioEffect->getBusArrangement(kOutput, i, speakerArr);
				_outSpeakerArrs.push_back(speakerArr);
			}

			for (int i = 0; i < _numOutEventBuses; ++i) {
				BusInfo info;
				_vstPlug->getBusInfo(kEvent, kOutput, i, info);
				_outEventBusInfos.push_back(info);
				setBusActive(kEvent, kOutput, i, false);
			}

			tresult res = _audioEffect->setBusArrangements(
				_inSpeakerArrs.data(), _numInAudioBuses, _outSpeakerArrs.data(),
				_numOutAudioBuses);
			if (res != kResultTrue) {
				_printError("Failed to set bus arrangements");
				return false;
			}

			res = _audioEffect->setupProcessing(_processSetup);
			if (res == kResultOk) {
				_processData.prepare(*_vstPlug, _maxBlockSize,
					_processSetup.symbolicSampleSize);
				if (_numInEventBuses > 0) {
					_processData.inputEvents = new EventList[_numInEventBuses];
				}
				if (_numOutEventBuses > 0) {
					_processData.outputEvents = new EventList[_numOutEventBuses];
				}
			} else {
				_printError("Failed to setup VST processing");
				return false;
			}

			if (_vstPlug->setActive(true) != kResultTrue) {
				_printError("Failed to activate VST component");
				return false;
			}
		}
	}

	return true;
}

void EasyVst::destroy() { _destroy(true); }

bool EasyVst::process(int numSamples) {

	_processData.numSamples = numSamples;
	tresult result = _audioEffect->process(_processData);
	if (result != kResultOk) {
#ifdef _DEBUG
		std::cerr << "VST process failed" << std::endl;
#endif
		return false;
	}

	return true;
}

const Steinberg::Vst::BusInfo*
EasyVst::busInfo(Steinberg::Vst::MediaType type,
	Steinberg::Vst::BusDirection direction, int which) {
	if (type == kAudio) {
		if (direction == kInput) {
			return &_inAudioBusInfos[which];
		}
		else if (direction == kOutput) {
			return &_outAudioBusInfos[which];
		}
		else {
			return nullptr;
		}
	}
	else if (type == kEvent) {
		if (direction == kInput) {
			return &_inEventBusInfos[which];
		}
		else if (direction == kOutput) {
			return &_outEventBusInfos[which];
		}
		else {
			return nullptr;
		}
	}
	else {
		return nullptr;
	}
}

int EasyVst::numBuses(Steinberg::Vst::MediaType type,
	Steinberg::Vst::BusDirection direction) {
	if (type == kAudio) {
		if (direction == kInput) {
			return _numInAudioBuses;
		}
		else if (direction == kOutput) {
			return _numOutAudioBuses;
		}
		else {
			return 0;
		}
	}
	else if (type == kEvent) {
		if (direction == kInput) {
			return _numInEventBuses;
		}
		else if (direction == kOutput) {
			return _numOutEventBuses;
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}
}

void EasyVst::setBusActive(MediaType type, BusDirection direction, int which,
	bool active) {
	_vstPlug->activateBus(type, direction, which, active);
}

void EasyVst::setProcessing(bool processing) {
	_audioEffect->setProcessing(processing);
}

Steinberg::Vst::ProcessContext* EasyVst::processContext() {
	return &_processContext;
}

Steinberg::Vst::Sample32* EasyVst::channelBuffer32(BusDirection direction,
	int which) {
	if (direction == kInput) {
		return _processData.inputs->channelBuffers32[which];
	}
	else if (direction == kOutput) {
		return _processData.outputs->channelBuffers32[which];
	}
	else {
		return nullptr;
	}
}

Steinberg::Vst::Sample64* EasyVst::channelBuffer64(BusDirection direction,
	int which) {
	if (direction == kInput) {
		return _processData.inputs->channelBuffers64[which];
	}
	else if (direction == kOutput) {
		return _processData.outputs->channelBuffers64[which];
	}
	else {
		return nullptr;
	}
}

Steinberg::Vst::EventList*
EasyVst::eventList(Steinberg::Vst::BusDirection direction, int which) {
	if (direction == kInput) {
		return static_cast<Steinberg::Vst::EventList*>(
			&_processData.inputEvents[which]);
	}
	else if (direction == kOutput) {
		return static_cast<Steinberg::Vst::EventList*>(
			&_processData.outputEvents[which]);
	}
	else {
		return nullptr;
	}
}

Steinberg::Vst::ParameterChanges*
EasyVst::parameterChanges(Steinberg::Vst::BusDirection direction, int which) {
	if (direction == kInput) {
		return static_cast<Steinberg::Vst::ParameterChanges*>(
			&_processData.inputParameterChanges[which]);
	}
	else if (direction == kOutput) {
		return static_cast<Steinberg::Vst::ParameterChanges*>(
			&_processData.outputParameterChanges[which]);
	}
	else {
		return nullptr;
	}
}


Dims EasyVst::createView(void* window_id) {
	if (!_editController) {
		_printError("VST does not provide an edit controller");
		return {};
	}

	// if (_view || _window) {
	//   _printDebug("Editor view or window already exists");
	//   return false;
	// }

	if (!_view) {
		_view = _editController->createView(ViewType::kEditor);
		if (!_view) {
			_printError("EditController does not provide its own view");
			return {};
		}
	}

#ifdef _WIN32
		if (_view->isPlatformTypeSupported(Steinberg::kPlatformTypeHWND) !=
			Steinberg::kResultTrue) {
			_printError("Editor view does not support HWND");
			return {};
		}
#else
		_printError("Platform is not supported yet");
		return false;
#endif

		// _window = SDL_CreateWindow(_name.data(), SDL_WINDOWPOS_UNDEFINED,
		//                            SDL_WINDOWPOS_UNDEFINED, viewRect.getWidth(),
		//                            viewRect.getHeight(), SDL_WINDOW_SHOWN);
		// SDL_SetWindowData(_window, "EasyVstInstance", this);
			  //
		// SDL_SysWMinfo wmInfo = {};
		// SDL_VERSION(&wmInfo.version);
		// SDL_GetWindowWMInfo(_window, &wmInfo);

#ifdef _WIN32
  // if (_view->attached(wmInfo.info.win.window, Steinberg::kPlatformTypeHWND) !=
  // 	  Steinberg::kResultOk) {
		if (_view->attached(window_id, Steinberg::kPlatformTypeHWND) !=
			Steinberg::kResultOk) {
			_printError("Failed to attach editor view to HWND");
			return {};
		}
#endif

	ViewRect viewRect = {};
	if (_view->getSize(&viewRect) != kResultOk) {
	  _printError("Failed to get editor view size");
	  return {};
	}

	return {
		viewRect.getWidth(),
		viewRect.getHeight(),
	};
}

// void EasyVst::destroyView() {
//   if (_window) {
//     SDL_DestroyWindow(_window);
//     _window = nullptr;
//   }
//
//   if (_view) {
//     _view = nullptr;
//   }
// }
//
// void EasyVst::processSdlEvent(const SDL_Event &event) {
//   if (event.type == SDL_WINDOWEVENT) {
//     if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
//       SDL_Window *window = SDL_GetWindowFromID(event.window.windowID);
//       EasyVst *target =
//           static_cast<EasyVst *>(SDL_GetWindowData(window, "EasyVstInstance"));
//       if (target) {
//         target->destroyView();
//       }
//     }
//   }
// }

const std::string& EasyVst::name() { return _name; }

void EasyVst::_destroy(bool decrementRefCount) {
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
		delete[] static_cast<Steinberg::Vst::EventList*>(_processData.inputEvents);
	}
	if (_processData.outputEvents) {
		delete[] static_cast<Steinberg::Vst::EventList*>(
			_processData.outputEvents);
	}
	_processData.unprepare();
	_processData = {};

	_processSetup = {};
	_processContext = {};

	_sampleRate = 0;
	_maxBlockSize = 0;
	_symbolicSampleSize = 0;
	_realtime = true;

	_path = "";
	_name = "";

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

void EasyVst::_printDebug(const std::string& info) {
	std::cout << "Debug info for VST3 plugin \"" << _path << "\": " << info
		<< std::endl;
}

void EasyVst::_printError(const std::string& error) {
	std::cerr << "Error loading VST3 plugin \"" << _path << "\": " << error
		<< std::endl;
}

void* load_plugin(char* s) {
	EasyVst* vst = new EasyVst();
	vst->init(s, 44100, 2048, kSample32, true);

	if (vst->numBuses(kAudio, kInput) > 0)
		vst->setBusActive(kAudio, kInput, 0, true);

	if (vst->numBuses(kAudio, kOutput) > 0)
		vst->setBusActive(kAudio, kOutput, 0, true);

	if (vst->numBuses(kEvent, kInput) > 0)
		vst->setBusActive(kEvent, kInput, 0, true);

	vst->setProcessing(true);
	return vst;
}

Dims show_gui(void* app, void* window_id) {
	EasyVst* vst = (EasyVst*) app;

	return vst->createView(window_id);
}

void hide_gui(void* app) {
	EasyVst* vst = (EasyVst*)app;
	vst->_view->release();
	vst->_view = nullptr;
}

char* name(void* app) {
	EasyVst* vst = (EasyVst*)app;
	return (char*)vst->name().c_str();
}

void vst3_set_sample_rate(void* app, int rate) {
	EasyVst* vst = (EasyVst*)app;
	vst->_processData.processContext->sampleRate = rate;
}

const void* get_data(void* app, int* data_len, void** stream) {
	EasyVst* vst = (EasyVst*)app;

	// ResizableMemoryIBStream* stream_ = new ResizableMemoryIBStream();
	// *stream = stream_;

	// if (vst->_vstPlug->getState(stream_) != kResultOk) {
	//	std::cout << "Failed to get plugin state. Non ok result." << std::endl;
	//	return nullptr;
	//}

	//Steinberg::int64 length = 0;
	// stream_->tell(&length);
	//*data_len = (int)length;

	//return stream_->getData();
	return nullptr;
}

void free_data_stream(void* stream) {
	// ResizableMemoryIBStream* stream_ = (ResizableMemoryIBStream*) stream;
	// delete stream_;
}

void set_data(void* app, void* data, int data_len) {
	EasyVst* vst = (EasyVst*)app;

	//ResizableMemoryIBStream stream = {};

	int num_bytes_written = 0;
	//stream.write(data, data_len, &num_bytes_written);
	assert(data_len == num_bytes_written);

	//if (vst->_vstPlug->setState(&stream) != kResultOk) {
	//	std::cout << "Failed to set plugin state" << std::endl;
	//}

	//stream.rewind();
	//vst->_editController->setComponentState(&stream);
}

void set_block_size(void* app, int size) {}

void process(
	void* app,
	FFIProcessData data,
	float** input,
	float** output,
	int note_events_count,
	NoteEvent* note_events,
	int* parameter_change_count,
	ParameterChange* parameter_changes
) {
	EasyVst* vst = (EasyVst*)app;

	if (vst->numBuses(kAudio, kInput) > 0) {
		vst->_processData.inputs->channelBuffers32 = input;
	}
	if (vst->numBuses(kAudio, kOutput) > 0) {
		vst->_processData.outputs->channelBuffers32 = output;
	}

	Steinberg::uint32 state = 0;

	Steinberg::Vst::ProcessContext* ctx = vst->_processData.processContext;

	vst->_processData.processContext->tempo = data.tempo;
	state |= ctx->kTempoValid;

	vst->_processData.processContext->timeSigNumerator = data.timeSigNumerator;
	vst->_processData.processContext->timeSigDenominator = data.timeSigDenominator;
	state |= ctx->kTimeSigValid;

	vst->_processData.processContext->projectTimeMusic = data.currentBeat;

	vst->_processData.processContext->projectTimeSamples = (data.currentBeat / (data.tempo / 60.0)) * data.sampleRate;

	vst->_processData.processContext->barPositionMusic = data.barPosBeats;
	state |= ctx->kBarPositionValid;

	vst->_processData.processContext->cycleStartMusic = data.cycleStartBeats;
	vst->_processData.processContext->cycleEndMusic = data.cycleEndBeats;
	state |= ctx->kCycleValid;

	vst->_processData.processContext->systemTime = data.systemTime;
	state |= ctx->kSystemTimeValid;

	vst->_processData.processContext->frameRate.framesPerSecond = 60.;
	vst->_processData.processContext->frameRate.flags = 0;

	if (data.cycleActive) {
		state |= ctx->kCycleActive;
	}

	if (data.playing) {
		state |= ctx->kPlaying;
	}

	if (data.recording) {
		state |= ctx->kRecording;
	}

	if (data.offline) {
		vst->_processData.processMode = kOffline;
	} else {
		vst->_processData.processMode = kRealtime;
	}

	vst->_processData.processContext->state = state;

	int midi_bus = 0;
	Steinberg::Vst::EventList* eventList = nullptr;
	if (vst->numBuses(kEvent, kInput) > 0) {
		eventList = vst->eventList(Steinberg::Vst::kInput, midi_bus);
		for (int i = 0; i < note_events_count; i++) {
			NoteEvent note = note_events[i];

			Steinberg::Vst::Event evt = {};
			evt.busIndex = midi_bus;
			evt.sampleOffset = note.samples_offset;
			evt.ppqPosition = note.time_beats;
			// evt.flags = Steinberg::Vst::Event::EventFlags::kIsLive;

			std::cout << note.note << std::endl;

			if (note.on) {
				evt.type = Steinberg::Vst::Event::EventTypes::kNoteOnEvent;
				evt.noteOn.channel = note.channel;
				evt.noteOn.pitch = note.note;
				evt.noteOn.tuning = note.tuning;
				evt.noteOn.velocity = note.velocity;
				evt.noteOn.length = 0;
				evt.noteOn.noteId = -1;
			} else {
				evt.type = Steinberg::Vst::Event::EventTypes::kNoteOffEvent;
				evt.noteOff.channel = note.channel;
				evt.noteOff.pitch = note.note;
				evt.noteOff.tuning = note.tuning;
				evt.noteOff.velocity = note.velocity;
				evt.noteOff.noteId = -1;
			}
			eventList->addEvent(evt);
		}
	}

	if (*parameter_change_count > 0) {
		if (!vst->_processData.inputParameterChanges) {
			vst->_processData.inputParameterChanges = new ParameterChanges(400);
		}

		auto changes = vst->_processData.inputParameterChanges;
		

		for (int i = 0; i < *parameter_change_count; i++) {
			int queue_index = 0;
			auto queue = changes->addParameterData(parameter_changes[i].id, queue_index);
			auto q = static_cast<ParameterValueQueue*>(queue);
			q->clear();
			int point_index = 0;
			if (queue->addPoint(0, parameter_changes[i].value, point_index) != kResultOk) {
				std::cout << "Failed to set parameter" << std::endl;
			}
			std::cout << parameter_changes[i].id << " <- " << parameter_changes[i].value << std::endl;
		}
	}

	vst->process(data.blockSize);

	for (int i = 0; i < *parameter_change_count; i++) {
		int queue_index = 0;
		auto changes = vst->_processData.inputParameterChanges;
		auto queue = changes->addParameterData(parameter_changes[i].id, queue_index);
		auto q = static_cast<ParameterValueQueue*>(queue);
		q->clear();
	}

	if (eventList) {
		eventList->clear();
	}
}

void set_param_from_ui_thread(void* app, int id, float value) {
	EasyVst* vst = (EasyVst*)app;
	std::cout << "setting " << id << " -> " << value << std::endl;

	if (vst->_editController->setParamNormalized(id, value) != kResultOk) {
		std::cout << "Failed to set parameter normalized" << std::endl;
	}
}

char* parameter_names(void* app) {
	EasyVst* vst = (EasyVst*)app;

	int params_count = vst->_editController->getParameterCount();

	std::string names = {};

	for (int i = 0; i < params_count; i++) {
		ParameterInfo param_info = {};
		vst->_editController->getParameterInfo(i, param_info);
		for (TChar c : param_info.title) {
			if (c != '\0') {
				names += c;
			}
		}
		names += ",";
	}

	char* c_str = new char[names.size() + 1];
	std::strcpy(c_str, names.c_str());

	return c_str;
}

void free_parameter_names(char* names) {
	delete[] names;
}

bool consume_parameter(void* app, ParameterEditState* param) {
	EasyVst* vst = (EasyVst*)app;

	std::lock_guard<std::mutex> guard(vst->param_edits_mutex);

	if (vst->dirty_edits.empty()) {
		return false;
	}

	*param = vst->dirty_edits.back();
	vst->dirty_edits.pop_back();

	return true;
}
