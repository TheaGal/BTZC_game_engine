#include "audio_engine.h"

#include "audio_impl_fmod.h"
#include "btlogger.h"
#include "sound_key.h"

#include <memory>
#include <unordered_map>


namespace
{

using namespace BT::audio;

class Audio_engine
{
public:
    /// Singleton instance.
    static Audio_engine& instance()
    {
        static Audio_engine s_inst;
        return s_inst;
    }

    /// Updates impl's audio thread and state.
    void update()
    {
        m_pimpl->update();
    }

    /// Gets or registers new sound.
    snd_key_t get_or_emplace_sound(std::string const& snd_name,
                                   bool is_3d,
                                   bool is_looping,
                                   bool stream)
    {
        if (m_snd_name_to_key.find(snd_name) != m_snd_name_to_key.end())
        {   // Get the found sound in cache.
            auto key{ m_snd_name_to_key.at(snd_name) };
            auto const& snd_meta{ m_snd_metadatas.at(key) };
            assert(snd_meta.snd_name == snd_name);
            assert(snd_meta.is_3d == is_3d);
            assert(snd_meta.is_looping == is_looping);
            assert(snd_meta.stream == stream);

            return key;
        }

        // Create new sound metadata entry.
        auto key{ m_next_key++ };

        m_snd_name_to_key.emplace(snd_name, key);
        m_snd_metadatas.emplace(key,
                                Sound_metadata{ .snd_name = snd_name,
                                                .is_3d = is_3d,
                                                .is_looping = is_looping,
                                                .stream = stream,
                                                .refcount = 0 });

        return key;
    }

    /// Increments reference count of sound.
    void incr_requires(snd_key_t key)
    {
        auto& snd_meta{ m_snd_metadatas.at(key) };
        snd_meta.refcount++;

        if (snd_meta.refcount == 1)
        {   // Load sound.
            m_pimpl->load_snd(key,
                              snd_meta.snd_name,
                              snd_meta.is_3d,
                              snd_meta.is_looping,
                              snd_meta.stream);
        }
    }

    /// Decrements reference count of sound.
    void decr_requires(snd_key_t key)
    {
        auto& snd_meta{ m_snd_metadatas.at(key) };
        snd_meta.refcount--;

        if (snd_meta.refcount == 0)
        {   // Unload sound.
            m_pimpl->unload_snd(key);
        }
        else if (snd_meta.refcount < 0)
        {
            BT_ERRORF("Sound %d refcount has dropped below 0. Something is wrong.", key);
            assert(false);
        }
    }

    /// Plays sound in 3D (sound does not have to be registered as a 3D sound).
    channel_key_t play_sound_3d(snd_key_t snd_key, vec3s const& pos, float_t db)
    {
        auto chan_key{ m_pimpl->play_snd_paused(snd_key) };

        if (m_pimpl->is_snd_3d(snd_key))
        {   // Setup 3D properties.
            m_pimpl->set_channel_3d_props(chan_key, pos, vec3s{ 0, 0, 0 });
        }
        m_pimpl->set_channel_volume(chan_key, db);
        m_pimpl->set_channel_paused(chan_key, false);

        BT_TRACEF("Started playing snd %i", snd_key);

        return chan_key;
    }

    /// Sets 3D listener transform.
    void set_3d_listener_trans(vec3s const& pos, vec3s const& forward)
    {
        m_pimpl->set_3d_listener_trans(pos, forward);
    }

private:
    /// Ctor.
    Audio_engine()
        : m_pimpl{ std::make_unique<impl::Audio_impl_FMOD>() }
    {
    }

    std::unique_ptr<impl::Audio_impl_FMOD> m_pimpl;

    snd_key_t m_next_key{ 69420 };
    std::unordered_map<std::string, snd_key_t> m_snd_name_to_key;

    struct Sound_metadata
    {
        std::string const& snd_name;
        bool is_3d;
        bool is_looping;
        bool stream;

        int32_t refcount;
    };
    std::unordered_map<snd_key_t, Sound_metadata> m_snd_metadatas;
};

}  // namespace


void BT::audio::initialize()
{
    (void)Audio_engine::instance();
}

void BT::audio::update()
{
    Audio_engine::instance().update();
}

snd_key_t BT::audio::mark_snd_required(std::string const& snd_name, bool is_3d, bool is_looping, bool stream)
{
    auto& eng{ Audio_engine::instance() };

    auto key{ eng.get_or_emplace_sound(snd_name, is_3d, is_looping, stream) };
    eng.incr_requires(key);

    return key;
}

void BT::audio::unmark_snd_required(snd_key_t key)
{
    Audio_engine::instance().decr_requires(key);
}

channel_key_t BT::audio::play_sound(snd_key_t key, float_t db)
{
    return play_sound_3d(key, vec3s{ 0, 0, 0 }, db);
}

channel_key_t BT::audio::play_sound_3d(snd_key_t key, vec3s const& pos, float_t db)
{
    return Audio_engine::instance().play_sound_3d(key, pos, db);
}

void BT::audio::set_3d_listener_trans(vec3s const& pos, vec3s const& forward)
{
    Audio_engine::instance().set_3d_listener_trans(pos, forward);
}
