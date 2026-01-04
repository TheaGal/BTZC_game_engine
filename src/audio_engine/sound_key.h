#pragma once

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

}  // namespace audio
}  // namespace BT