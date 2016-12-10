/**
 *
 * Kernel Module which creates a device to listen to Tuxs heart.
 *
 * @copyright: GPLv2 (see LICENSE), by Timo Furrer <tuxtimo@gmail.com>
 *
 */

#include <linux/kernel.h>
#include "devheart.h"

#define DEVHEART_PAUSE_SIZE 64

struct devheart_sound_t pause_sound = {
    .size = DEVHEART_PAUSE_SIZE,
    .data = {
#include "pause_data.h"
    }
};
