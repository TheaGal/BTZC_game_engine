#pragma once

#include "btglm.h"
#include "sound_key.h"

#include <cmath>
#include <string>


namespace BT
{
namespace audio
{

/// OPTIONAL. Creates audio engine object. Omitting this will lazy-load the audio engine.
void initialize();

/// Marks a sound as required. If the first one to mark a sound as required, audio engine will load
/// this sound into its memory.
snd_key_t mark_snd_required(std::string const& snd_name, bool is_3d, bool is_looping, bool stream);

/// Unmarks a sound as required. After this point there's a promise to not use this sound anymore.
void unmark_snd_required(snd_key_t key);

/// Plays a sound. Must be marked as required first.
channel_key_t play_sound(snd_key_t key, float_t db = 0);

/// Plays a sound in 3D space. Must be marked as required first.
channel_key_t play_sound_3d(snd_key_t key, vec3s const& pos, float_t db = 0);

/// Sets the position of the 3D listener.
void set_3d_listener_trans(vec3s const& pos, vec3s const& forward);

/// Helper for db -> volume.
inline float_t db_to_volume(float_t db)
{
    return std::powf(10.0f, 0.05f * db);
}

/// Helper for volume -> db.
inline float_t volume_to_db(float_t volume)
{
    return 20.0f * std::log10f(volume);
}

}  // namespace audio
}  // namespace BT