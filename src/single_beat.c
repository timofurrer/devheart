/**
 *
 * Kernel Module which creates a device to listen to Tuxs heart.
 *
 * @copyright: GPLv2 (see LICENSE), by Timo Furrer <tuxtimo@gmail.com>
 *
 */

#include <linux/kernel.h>
#include "devheart.h"

#define DEVHEART_SINGLE_BEAT_SIZE 13615

struct devheart_sound_t single_beat = {
    .size = DEVHEART_SINGLE_BEAT_SIZE,
    .data = {
#include "single_beat_data.h"
    }
};
