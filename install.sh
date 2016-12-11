#!/bin/sh

set -e

printf '[*] Downloading repository from https://github.com/timofurrer/devheart ... '
git clone 'https://github.com/timofurrer/devheart' /tmp/devheart > /dev/null
cd /tmp/devheart
echo '[OK]'

printf '[*] Building and Inserting module into kernel ...'
make > /dev/null
sudo make insert > /dev/null
echo '[OK]'
echo 'Press Ctrl+C to end the Electrocardiography'
aplay -r 44100 -f s16_le /dev/heart

echo '[*] Removing moduel from kernel and source ... '
sudo make remove
rm -rf /tmp/devheart
echo '[OK]'
