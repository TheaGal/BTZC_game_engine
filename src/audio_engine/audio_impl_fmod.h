#pragma once

#include "api/core/inc/fmod.hpp"
#include "btglm.h"
#include "sound_key.h"

#include <string>
#include <unordered_map>


namespace BT
{
namespace audio
{
namespace impl
{

class Audio_impl_FMOD
{
public:
    /// Ctor.
    Audio_impl_FMOD();

    Audio_impl_FMOD(Audio_impl_FMOD const&)            = delete;
    Audio_impl_FMOD(Audio_impl_FMOD&&)                 = delete;
    Audio_impl_FMOD& operator=(Audio_impl_FMOD const&) = delete;
    Audio_impl_FMOD& operator=(Audio_impl_FMOD&&)      = delete;

    /// Dtor.
    ~Audio_impl_FMOD();

    /// Update audio engine thread.
    void update();

    /// Sets audio listener's 3D transform.
    void set_3d_listener_trans(vec3s const& pos, vec3s const& forward);

    /// Load sound.
    void load_snd(snd_key_t key,
                  std::string const& snd_name,
                  bool is_3d,
                  bool is_looping,
                  bool stream);

    /// Unload sound.
    void unload_snd(snd_key_t key);

    /// Gets whether a sound is 3D or not.
    bool is_snd_3d(snd_key_t key);

    /// Reserves a channel and starts playing a sound but paused.
    channel_key_t play_snd_paused(snd_key_t key);

    /// Sets a channel's 3D properties.
    void set_channel_3d_props(channel_key_t key, vec3s const& pos, vec3s const& velo);

    /// Sets a channel's volume.
    void set_channel_volume(channel_key_t key, float_t db);

    /// Sets whether a channel is paused or not.
    void set_channel_paused(channel_key_t key, bool is_paused);

private:
    FMOD::System* m_system{ nullptr };

    std::unordered_map<snd_key_t, FMOD::Sound*> m_loaded_snds;

};

}  // namespace impl
}  // namespace audio
}  // namespace BT
