/**
 *
 * Kernel Module which creates a device to listen to Tuxs heart.
 *
 * @copyright: GPLv2 (see LICENSE), by Timo Furrer <tuxtimo@gmail.com>
 *
 */

#ifndef DEVHEART_H

// raw sound data
struct devheart_sound_t {
    size_t size;
    char data[];
};

#endif /* DEVHEART_H */
