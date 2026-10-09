#include "audio_engine.h"

#include "audio_impl_fmod.h"
#include "btdatecheck.h"
#include "btlogger.h"
#include "util.h"

#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
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
        // Do garbage collection.
        constexpr uint32_t k_garbage_collect_check_interval{ 1024 };
        static_assert((k_garbage_collect_check_interval & (k_garbage_collect_check_interval - 1)) ==
                          0,
                      "This number is not a power of 2");

        if ((m_garbage_collection_timer++ & (k_garbage_collect_check_interval - 1)) == 0)
        {
            for (auto const& [snd_name, snd_key] : m_snd_name_to_key)
            {
                if (m_pimpl->is_snd_loaded(snd_key) && m_snd_metadatas.at(snd_key).refcount == 0 &&
                    !m_pimpl->is_snd_used_anywhere(snd_key))
                {
                    // Unload sound!!
                    m_pimpl->unload_snd(snd_key);
                    BT_TRACEF("Unloaded sound \"%s\"", snd_name.c_str());
                }
            }
        }

        // Update backend.
        m_pimpl->update();
    }

    /// Sets global volume.
    void set_master_db(float_t const db)
    {
        m_pimpl->set_master_db(db);
    }

    /// Get global volume in dB.
    float_t get_master_db() const
    {
        return m_pimpl->get_master_db();
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
            // no snd_name_str comparison bc slow.
            assert(snd_meta.snd_name_hash == std::hash<std::string>{}(snd_name));
            assert(snd_meta.is_3d == is_3d);
            assert(snd_meta.is_looping == is_looping);
            assert(snd_meta.stream == stream);

            return key;
        }

        // Create new sound metadata entry.
        auto key{ m_next_key++ };

        m_snd_name_to_key.emplace(snd_name, key);

        Sound_metadata snd_meta{ .snd_name_str = { '\0' },
                                 .snd_name_hash = std::hash<std::string>{}(snd_name),
                                 .is_3d = is_3d,
                                 .is_looping = is_looping,
                                 .stream = stream,
                                 .refcount = 0 };

        if (snd_name.length() >= sizeof(snd_meta.snd_name_str))
            throw std::runtime_error("`snd_name` length is too large.");

        std::strncpy(snd_meta.snd_name_str, snd_name.c_str(), sizeof(snd_meta.snd_name_str));

        m_snd_metadatas.emplace(key, std::move(snd_meta));

        return key;
    }

    /// Increments reference count of sound.
    void incr_requires(snd_key_t key)
    {
        auto& snd_meta{ m_snd_metadatas.at(key) };
        snd_meta.refcount++;

        if (snd_meta.refcount == 1 && !m_pimpl->is_snd_loaded(key))
        {   // Load sound.
            m_pimpl->load_snd(key,
                              std::string(snd_meta.snd_name_str),
                              snd_meta.is_3d,
                              snd_meta.is_looping,
                              snd_meta.stream);
            BT_TRACEF("Loaded sound \"%s\"", snd_meta.snd_name_str);
        }
    }

    /// Decrements reference count of sound.
    void decr_requires(snd_key_t key)
    {
        auto& snd_meta{ m_snd_metadatas.at(key) };
        snd_meta.refcount--;

        if (snd_meta.refcount < 0)
        {
            BT_ERRORF("Sound %d refcount has dropped below 0. Something is wrong.", key);
            assert(false);
        }
    }

    /// Plays sound in 3D (sound does not have to be registered as a 3D sound).
    channel_key_t play_sound_3d(snd_key_t snd_key, vec3 const pos, float_t db)
    {
        auto chan_key{ m_pimpl->play_snd_paused(snd_key) };

        if (m_pimpl->is_snd_3d(snd_key))
        {   // Setup 3D properties.
            m_pimpl->set_channel_3d_props(chan_key, pos, vec3{ 0, 0, 0 });
        }
        m_pimpl->set_channel_volume(chan_key, db);
        m_pimpl->set_channel_paused(chan_key, false);

        BT_TRACEF("Started playing snd %i", snd_key);

        return chan_key;
    }

    /// Sets 3D listener transform.
    void set_3d_listener_trans(vec3 const pos, vec3 const forward)
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
        char snd_name_str[64];
        size_t snd_name_hash;
        bool is_3d;
        bool is_looping;
        bool stream;

        int32_t refcount;
    };
    std::unordered_map<snd_key_t, Sound_metadata> m_snd_metadatas;

    uint32_t m_garbage_collection_timer{ 0 };
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

void BT::audio::set_master_db(float_t const db)
{
    Audio_engine::instance().set_master_db(db);
}

float_t BT::audio::get_master_db()
{
    return Audio_engine::instance().get_master_db();
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
    return play_sound_3d(key, vec3{ 0, 0, 0 }, db);
}

channel_key_t BT::audio::play_sound_3d(snd_key_t key, vec3 const pos, float_t db)
{
    return Audio_engine::instance().play_sound_3d(key, pos, db);
}

void BT::audio::set_3d_listener_trans(vec3 const pos, vec3 const forward)
{
    Audio_engine::instance().set_3d_listener_trans(pos, forward);
}
