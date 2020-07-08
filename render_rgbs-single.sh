#!/bin/bash

RENDER_RES=7680x2160

tcpclient -vRHl0 "$SERVER1" 5295 ./fetch_imghalf /tmp/renderfarm_image-7680x2160-top.rgb "$RENDER_RES" &
tcpclient -vRHl0 "$SERVER2" 5295 ./fetch_imghalf /tmp/renderfarm_image-7680x2160-bottom.rgb "$RENDER_RES"

echo Showing files:
ls -ltr /tmp/renderfarm_*.rgb

exit 0
