#include "audio_impl_fmod.h"

#include "api/core/inc/fmod.hpp"
#include "api/core/inc/fmod_errors.h"
#include "btlogger.h"
#include "btzc_game_engine.h"

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
    ERRCHECK(m_system->init(k_max_channels, FMOD_INIT_NORMAL, nullptr));
    ERRCHECK(m_system->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0));
}

BT::audio::impl::Audio_impl_FMOD::~Audio_impl_FMOD()
{
    ERRCHECK(m_system->release());
}

void BT::audio::impl::Audio_impl_FMOD::update()
{
    // Unload unused channels.
    assert(false);

    // Tick update for FMOD.
    ERRCHECK(m_system->update());
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
