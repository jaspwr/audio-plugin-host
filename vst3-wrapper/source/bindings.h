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

struct NoteEvent {
  bool on;
  int note;
  float velocity;
  float tuning;
  int channel;
  int samples_offset;
  float time_beats;
};

struct ParameterUpdate {
  int32_t parameter_id;
  float current_value;
  /// Value at start of edit. For example, the value before the user started dragging a knob
  /// in the plugin editor. Not required to be set when sending events to the plugin; just
  /// used for implementing undo/redo in the host.
  float initial_value;
  ///  If `true`, the user has just released the control and this is the final value.
  bool end_edit;
};

struct ParameterEditState {
  int id;
  float initial_value;
  float current_value;
  bool finished;
};

struct ParameterFFI {
  int id;
  const char *name;
  int index;
  float value;
  const char *formatted_value;
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

extern void process(const void *app,
                    ProcessDetails data,
                    float **input,
                    float **output,
                    int32_t note_events_count,
                    const NoteEvent *note_events,
                    int32_t *parameter_change_count,
                    ParameterUpdate *parameter_changes);

extern const char *parameter_names(const void *app);

extern void free_string(const char *str);

extern void set_param_from_ui_thread(const void *app, int32_t id, float value);

extern const void *get_data(const void *app, int32_t *data_len, const void **stream);

extern void free_data_stream(const void *stream);

extern void set_data(const void *app, const void *data, int32_t data_len);

extern bool consume_parameter(const void *app, ParameterEditState *param);

extern ParameterFFI get_parameter(const void *app, int32_t id);

void send_event_to_host(const PluginIssuedEvent *event, const void *plugin_sent_events_producer);

}  // extern "C"
