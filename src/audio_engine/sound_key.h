#pragma once

#include <cmath>
#include <cstdint>


namespace BT
{
namespace audio
{

/// Max number of audio channels.
constexpr int32_t k_max_channels{ 512 };

/// Sound key. For accessing the memory of a sound.
using snd_key_t = std::uint32_t;

/// Channel key. For accessing the memory of a channel.
using channel_key_t = std::uint32_t;

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