#!/bin/bash

SERVER="$1"
PORT="$2"

if [ "$SERVER" ==  "" ]; then
    echo Please specify Server IP.
    exit -1
fi

if [ "$PORT" ==  "" ]; then
    echo Please specify Server port.
    exit -1
fi

RENDER_RES=7680x4320

output_png='/tmp/testremote-bptrender.png'

tcpclient -vRHl0 "$SERVER" "$PORT" ./fetch_imghalf - "$RENDER_RES" | convert -size 7680x2160 rgb:- -depth 32 "$output_png"

exit 0

