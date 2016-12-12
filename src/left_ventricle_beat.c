/**
 *
 * Kernel Module which creates a device to listen to Tuxs heart.
 *
 * @copyright: GPLv2 (see LICENSE), by Timo Furrer <tuxtimo@gmail.com>
 *
 */

#include "devheart.h"

#define DEVHEART_LEFT_VENTRICLE_BEAT_SIZE 22112

struct devheart_sound_t left_ventricle_beat_sound = {
    .size = DEVHEART_LEFT_VENTRICLE_BEAT_SIZE,
    .data = {
#include "left_ventricle_beat_data.h"
    }
};
