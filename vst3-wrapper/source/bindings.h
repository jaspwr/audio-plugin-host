#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

enum class PlayingState : uint8_t {
  Stopped,
  Playing,
  Recording,
  OfflineRendering,
};

struct Dims {
  int width;
  int height;
};

struct FFIPluginDescriptor {
  const char *name;
  const char *vendor;
  const char *version;
  const char *id;
  int initial_latency;
};

struct AudioBusDescriptor {
  uintptr_t channels;
};

template<typename T>
union MaybeUninit {
  T value;
};

/// Real-time safe, fixed-size, FFI friendly vector.
template<typename T, uintptr_t N>
struct HeaplessVec {
  uintptr_t count;
  MaybeUninit<T> data[N];
};

/// Input and output configuration for the plugin.
struct IOConfigutaion {
  HeaplessVec<AudioBusDescriptor, 16> audio_inputs;
  HeaplessVec<AudioBusDescriptor, 16> audio_outputs;
  int32_t event_inputs_count;
};

using SampleRate = uintptr_t;

using BlockSize = uintptr_t;

using Tempo = double;

using PpqTime = double;

struct ProcessDetails {
  SampleRate sample_rate;
  BlockSize block_size;
  Tempo tempo;
  PpqTime player_time;
  uintptr_t time_signature_numerator;
  uintptr_t time_signature_denominator;
  bool cycle_enabled;
  PpqTime cycle_start;
  PpqTime cycle_end;
  PlayingState playing_state;
  PpqTime bar_start_pos;
  double nanos;
};

using Samples = uintptr_t;

struct MidiEvent {
  Samples note_length;
  uint8_t midi_data[3];
  float detune;
};

struct ParameterUpdate {
  int32_t parameter_id;
  int32_t parameter_index;
  float current_value;
  /// Value at start of edit. For example, the value before the user started dragging a knob
  /// in the plugin editor. Not required to be set when sending events to the plugin; just
  /// used for implementing undo/redo in the host.
  float initial_value;
  ///  If `true`, the user has just released the control and this is the final value.
  bool end_edit;
};

struct HostIssuedEventType {
  enum class Tag {
    Midi,
    Parameter,
  };

  struct Midi_Body {
    MidiEvent _0;
  };

  struct Parameter_Body {
    ParameterUpdate _0;
  };

  Tag tag;
  union {
    Midi_Body midi;
    Parameter_Body parameter;
  };
};

/// Events sent to the plugin from the host. Can be passed into the `process` function or queued
/// for the next process call with `queue_event`.
struct HostIssuedEvent {
  HostIssuedEventType event_type;
  /// Time in samples from start of next block.
  Samples block_time;
  PpqTime ppq_time;
  uintptr_t bus_index;
};

struct ParameterFFI {
  int id;
  const char *name;
  int index;
  float value;
  const char *formatted_value;
  bool hidden;
  bool can_automate;
  bool is_wrap_around;
  bool read_only;
};

/// Events sent to the host from the plugin. Queued in the plugin and the consumed from the `get_events` function.
struct PluginIssuedEvent {
  enum class Tag {
    /// Plugin changed it's latency. New latency is in samples.
    ChangeLatency,
    /// Plugin changed its editor window size. 0 is width, 1 is height.
    ResizeWindow,
    Parameter,
    UpdateDisplay,
    IOChanged,
  };

  struct ChangeLatency_Body {
    uintptr_t _0;
  };

  struct ResizeWindow_Body {
    uintptr_t _0;
    uintptr_t _1;
  };

  struct Parameter_Body {
    ParameterUpdate _0;
  };

  Tag tag;
  union {
    ChangeLatency_Body change_latency;
    ResizeWindow_Body resize_window;
    Parameter_Body parameter;
  };
};

extern "C" {

extern const void *load_plugin(const char *s, const void *plugin_sent_events_producer);

extern Dims show_gui(const void *app, const void *window_id);

extern void hide_gui(const void *app);

extern FFIPluginDescriptor descriptor(const void *app);

extern IOConfigutaion io_config(const void *app);

extern uintptr_t parameter_count(const void *app);

extern void process(const void *app,
                    const ProcessDetails *data,
                    float ***input,
                    float ***output,
                    HostIssuedEvent *events,
                    int32_t events_len);

extern void set_param_in_edit_controller(const void *app, int32_t id, float value);

extern ParameterFFI get_parameter(const void *app, int32_t id);

extern const void *get_data(const void *app, int32_t *data_len, const void **stream);

extern void free_data_stream(const void *stream);

extern void set_data(const void *app, const void *data, int32_t data_len);

extern void set_processing(const void *app, bool processing);

extern void free_string(const char *str);

void send_event_to_host(const PluginIssuedEvent *event, const void *plugin_sent_events_producer);

}  // extern "C"
