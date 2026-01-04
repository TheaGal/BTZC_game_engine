#include "audio_impl_fmod.h"

#include "api/core/inc/fmod.hpp"
#include "api/core/inc/fmod_common.h"
#include "api/core/inc/fmod_errors.h"
#include "btlogger.h"
#include "btzc_game_engine.h"
#include "util.h"

#include <cassert>

#define ERRCHECK(_result) _error_check(_result, __FILE__, __LINE__)


namespace
{

void _error_check(FMOD_RESULT result, char const* file, int32_t line)
{
    if (result != FMOD_OK)
    {
        BT_ERRORF("FMOD :: %s:%d\n\t%s", file, line, FMOD_ErrorString(result));
        assert(false);
    }
}

}  // namespace


BT::audio::impl::Audio_impl_FMOD::Audio_impl_FMOD()
{
    ERRCHECK(FMOD::System_Create(&m_system));

    auto init_mode{ FMOD_INIT_NORMAL };
    if constexpr (false)
    {   // Enable profiling.
        init_mode |= FMOD_INIT_PROFILE_ENABLE;
    }

    ERRCHECK(m_system->init(k_max_channels, init_mode, nullptr));
}

BT::audio::impl::Audio_impl_FMOD::~Audio_impl_FMOD()
{
    ERRCHECK(m_system->release());
}

void BT::audio::impl::Audio_impl_FMOD::update()
{
    // Unload unused channels.
    std::vector<Alive_channels_iterator_t> stopped_channels;
    for (auto it = m_alive_channels.begin(); it != m_alive_channels.end(); it++)
    {
        bool is_playing;
        ERRCHECK(it->second->isPlaying(&is_playing));

        if (!is_playing)
        {
            stopped_channels.emplace_back(it);
        }
    }
    for (auto& it : stopped_channels)
        m_alive_channels.erase(it);

    // Tick update for FMOD.
    ERRCHECK(m_system->update());
}

void BT::audio::impl::Audio_impl_FMOD::set_3d_listener_trans(vec3s const& pos, vec3s const& forward)
{
    FMOD_VECTOR fmod_pos{ pos.x, pos.y, pos.z };
    static FMOD_VECTOR const k_fmod_velo{ 0, 0, 0 };
    FMOD_VECTOR fmod_forward{ forward.x, forward.y, forward.z };
    static FMOD_VECTOR const k_fmod_up{ 0, 1, 0 };

    ERRCHECK(m_system->set3DListenerAttributes(0,
                                               &fmod_pos,
                                               &k_fmod_velo,
                                               &fmod_forward,
                                               &k_fmod_up));
}

void BT::audio::impl::Audio_impl_FMOD::load_snd(snd_key_t key,
                                                std::string const& snd_name,
                                                bool is_3d,
                                                bool is_looping,
                                                bool stream)
{
    FMOD_MODE mode{ FMOD_DEFAULT };
    mode |= is_3d ? FMOD_3D : FMOD_2D;
    mode |= is_looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
    mode |= stream ? FMOD_CREATESTREAM : FMOD_CREATECOMPRESSEDSAMPLE;

    FMOD::Sound* sound{ nullptr };
    ERRCHECK(m_system->createSound((BTZC_GAME_ENGINE_ASSET_AUDIO_PATH + snd_name).c_str(),
                                   mode,
                                   nullptr,
                                   &sound));

    if (!sound)
    {
        BT_ERRORF("Could not create sound \"%s\"", snd_name.c_str());
        assert(false);
    }

    auto [_, success] = m_loaded_snds.emplace(key, sound);

    if (!success)
    {
        BT_ERRORF("Created sound \"%s\", however emplace failed.", snd_name.c_str());
        assert(false);
    }
}

void BT::audio::impl::Audio_impl_FMOD::unload_snd(snd_key_t key)
{
    ERRCHECK(m_loaded_snds.at(key)->release());
    m_loaded_snds.erase(key);
}

bool BT::audio::impl::Audio_impl_FMOD::is_snd_3d(snd_key_t key)
{
    FMOD_MODE mode;
    ERRCHECK(m_loaded_snds.at(key)->getMode(&mode));
    return (mode & FMOD_3D);
}

BT::audio::channel_key_t BT::audio::impl::Audio_impl_FMOD::play_snd_paused(snd_key_t key)
{
    FMOD::Channel* channel{ nullptr };
    ERRCHECK(m_system->playSound(m_loaded_snds.at(key), nullptr, true, &channel));
    
    auto channel_key{ m_next_key++ };
    m_alive_channels.emplace(channel_key, channel);

    return channel_key;
}

void BT::audio::impl::Audio_impl_FMOD::set_channel_3d_props(channel_key_t key,
                                                            vec3s const& pos,
                                                            vec3s const& velo)
{
    FMOD_VECTOR fmod_pos{ pos.x, pos.y, pos.z };
    FMOD_VECTOR fmod_velo{ velo.x, velo.y, velo.z };
    ERRCHECK(m_alive_channels.at(key)->set3DAttributes(&fmod_pos, &fmod_velo));
}

void BT::audio::impl::Audio_impl_FMOD::set_channel_volume(channel_key_t key, float_t db)
{
    ERRCHECK(m_alive_channels.at(key)->setVolume(db_to_volume(db)));
}

void BT::audio::impl::Audio_impl_FMOD::set_channel_paused(channel_key_t key, bool is_paused)
{
    ERRCHECK(m_alive_channels.at(key)->setPaused(is_paused));
}
