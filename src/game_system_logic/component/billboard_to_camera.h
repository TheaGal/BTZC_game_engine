#pragma once

#include "btglm.h"
#include "btjson.h"
#include "btuuid.h"


namespace BT
{
namespace component
{

/// Tag for making the transform of an entity face the camera.
struct Billboard_to_camera
{
    vec3s rotation_euler{ 0, 0, 0 };

    UUID copy_position_entity;
    vec3s copy_position_offset;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Billboard_to_camera,
        rotation_euler,
        copy_position_entity,
        copy_position_offset
    );
};

}  // namespace component
}  // namespace BT
