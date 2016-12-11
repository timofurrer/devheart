/**
 *
 * Kernel Module which creates a device to listen to Tuxs heart.
 *
 * @copyright: GPLv2 (see LICENSE), by Timo Furrer <tuxtimo@gmail.com>
 *
 */

#include <linux/kernel.h>
#include "devheart.h"

#define DEVHEART_RIGHT_VENTRICLE_BEAT_SIZE 17710

struct devheart_sound_t right_ventricle_beat_sound = {
    .size = DEVHEART_RIGHT_VENTRICLE_BEAT_SIZE,
    .data = {
#include "right_ventricle_beat_data.h"
    }
};
