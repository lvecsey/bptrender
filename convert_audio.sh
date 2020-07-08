#!/bin/bash

audio_fn='Limit 70.mp3'

if [ "$1" != "" ]; then
    audio_fn="$1"
fi

ffmpeg -i "$audio_fn" converting.wav

sox converting.wav -q --encoding floating-point -t raw --bits 64 --endian little --channels 2 --rate 48000 process_audio.raw

exit 0
