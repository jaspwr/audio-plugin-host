#pragma once

#include <mutex>

#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"

#include <public.sdk/source/vst/hosting/eventlist.h>
#include <public.sdk/source/vst/hosting/parameterchanges.h>
#include <public.sdk/source/vst/hosting/processdata.h>
#include <pluginterfaces/gui/iplugview.h>
// #include "memoryibstream.h"

struct Dims {
	int w;
	int h;
};

struct ParameterEditState {
	int id;
	float inital_value;
	float current_value;
	bool finished;
};

class EasyVst {
public:
	EasyVst();
	~EasyVst();

	bool init(const std::string& path, int sampleRate, int maxBlockSize, int symbolicSampleSize, bool realtime);
	void destroy();

	Steinberg::Vst::ProcessContext* processContext();
	void setProcessing(bool processing);
	bool process(int numSamples);

	const Steinberg::Vst::BusInfo* busInfo(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection direction, int which);
	int numBuses(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection direction);
	void setBusActive(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection direction, int which, bool active);

	Steinberg::Vst::Sample32* channelBuffer32(Steinberg::Vst::BusDirection direction, int which);
	Steinberg::Vst::Sample64* channelBuffer64(Steinberg::Vst::BusDirection direction, int which);

	Steinberg::Vst::EventList* eventList(Steinberg::Vst::BusDirection direction, int which);
	Steinberg::Vst::ParameterChanges* parameterChanges(Steinberg::Vst::BusDirection direction, int which);

	Dims createView(void* window_id);
	// void destroyView();
	// static void processSdlEvent(const SDL_Event &event);

	const std::string& name();
	Steinberg::Vst::HostProcessData _processData = {};
	bool _realtime = true;

	void _destroy(bool decrementRefCount);

	void _printDebug(const std::string& info);
	void _printError(const std::string& error);

	std::vector<Steinberg::Vst::BusInfo> _inAudioBusInfos, _outAudioBusInfos;
	int _numInAudioBuses = 0, _numOutAudioBuses = 0;

	std::vector<Steinberg::Vst::BusInfo> _inEventBusInfos, _outEventBusInfos;
	int _numInEventBuses = 0, _numOutEventBuses = 0;

	std::vector<Steinberg::Vst::SpeakerArrangement> _inSpeakerArrs, _outSpeakerArrs;

	VST3::Hosting::Module::Ptr _module = nullptr;
	Steinberg::IPtr<Steinberg::Vst::PlugProvider> _plugProvider = nullptr;

	Steinberg::IPtr<Steinberg::Vst::IComponent> _vstPlug = nullptr;
	Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> _audioEffect = nullptr;
	Steinberg::IPtr<Steinberg::Vst::IEditController> _editController = nullptr;

	void* component_handler = nullptr;

	Steinberg::Vst::ProcessSetup _processSetup = {};
	Steinberg::Vst::ProcessContext _processContext = {};

	Steinberg::IPtr<Steinberg::IPlugView> _view = nullptr;
	// SDL_Window *_window = nullptr;

	int _sampleRate = 0, _maxBlockSize = 0, _symbolicSampleSize = 0;

	std::vector<ParameterEditState> param_edits;
	std::vector<ParameterEditState> dirty_edits;
	std::mutex param_edits_mutex;

	std::string _path;
	std::string _name;

	static Steinberg::Vst::HostApplication* _standardPluginContext;
	static int _standardPluginContextRefCount;
};

struct FFIProcessData {
	double tempo;
	unsigned int timeSigNumerator;
	unsigned int timeSigDenominator;
	double currentBeat;
	unsigned int sampleRate;
	unsigned int blockSize;
	bool playing;
	bool recording;
	bool cycleActive;
	bool offline;
	double cycleStartBeats;
	double cycleEndBeats;
	double barPosBeats;
	long long systemTime;
};

struct NoteEvent {
	bool on;
	int note;
	float velocity;
	float tuning;
	int channel;
	int samples_offset;
	float time_beats;
};

struct ParameterChange {
	int id;
	float value;
};

extern "C" void* load_plugin(char* s);
extern "C" Dims show_gui(void* app, void* window_id);
extern "C" void hide_gui(void* app);
extern "C" char* name(void* app);
extern "C" void vst3_set_sample_rate(void* app, int rate);
extern "C" void set_block_size(void* app, int size);
extern "C" void process(
	void* app,
	FFIProcessData data,
	float** input,
	float** output,
	int note_events_count,
	NoteEvent* note_events,
	int* parameter_change_count,
	ParameterChange* parameter_changes
);
extern "C" char* parameter_names(void* app);
extern "C" void free_parameter_names(char* names);
extern "C" void set_param_from_ui_thread(void* app, int id, float value);

extern "C" const void* get_data(void* app, int* data_len, void** stream);
extern "C" void free_data_stream(void* stream);
extern "C" void set_data(void* app, void* data, int data_len);

extern "C" bool consume_parameter(void* app, ParameterEditState* param);
