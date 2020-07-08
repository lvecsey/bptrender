Billion point render

Create a 3D point based image, in this case using two servers and 8 cores each.
Each server has 48GB of memory, and will keep the point data in memory.
So we can actually have 1.5 billion points in memory.

This release depends on Linux epoll, and also Image Magick, and ffmpeg.

The first server renders onto a 7680x2160 image area (top), and the second
server renders onto the same size for the bottom. The individual half images are compressed with zlib and transferred to a front-end machine. The image sections decompressed there, combined, and downsampled to 3840x2160.

```console
[                   ]
[                   ]
[                   ]
[                   ]
[                   ]
---------------------
[                   ]
[                   ]
[                   ]
[                   ]
[                   ]
```

The front end machine does some post processing of the image halfs, and then acknowledges the backend cluster so they can continue computing the next frame.

If you are just rendering a still image The front end machine needs sufficient memory to combine the decompressed image halfs and convert to .png

*Generating the point data*

First, generate the point and color data which will be copied to each server.

```console
make gen_points
time ./gen_points /tmp/pointcols.dat 1.5e9
```

For 1.5 billion points, this can sometimes take 25 minutes. It's basically a
48GB file.

*Prepare the audio*

Retrieve an audio file for example from freepd.com and convert it to raw format.
64bit floating-point at 48000hz sample rate, stereo.

```console
./convert_audio.sh incoming.mp3
```

*Pushing the point data and audio to the servers*

Copy the files to each server:

```console
rsync /tmp/pointcols.dat ./process_audio.raw 192.168.1.98:/tmp
rsync /tmp/pointcols.dat ./process_audio.raw 192.168.1.99:/tmp   
```

*Setting up the render servers in the cluster*

You can specify the number of threads that each server will use (for example 8), using an even value that will neatly divide the image height.

Decide if you want to render a single image, or movie.
For single images set environment variables DUR=1 and FPS=1
For movies, try DUR=20.0 and FPS=60

On the first server, run:

```console
tcpserver -vRHl0 0.0.0.0 5295 ./bptrenderfarm 7680x4320 8 TOP $DUR $FPS
```

The second server:

```console
tcpserver -vRHl0 0.0.0.0 5295 ./bptrenderfarm 7680x4320 8 BOTTOM $DUR $FPS
```

*Rendering a test frame (half image) from a node in the cluster*

Of the two servers in the cluster, one renders the TOP half, other is BOTTOM.
You can test that each one is functioning by rendering a single frame:

```console
./testremote.sh 192.168.1.98 5295
feh --fullscreen /tmp/testremote-bptrender.png
```

or

```console
./testremote.sh 192.168.1.99 5295
feh --fullscreen /tmp/testremote-bptrender.png
```

To retrieve and combine the half images:

```console
SERVER1=192.168.1.98 SERVER2=192.168.1.99 ./render_rgbs-single.sh
make /tmp/output_bptrender.png
```

*Rendering a movie*

The front end machine would start the rendering pipeline and encode:

```console
    ./multitcp - 7680x4320 0.0.0.0 4950 192.168.1.98 5295 \
        0.0.0.0 4951 192.168.1.99 5296 \
	| ffmpeg -loglevel quiet -f rawvideo -pix_fmt rgb48le -r 60 -s 7680x4320 -i - -preset veryslow -vcodec libx264 -filter:v "scale=3840x2160" -pix_fmt yuv420p -profile:v baseline -level 3.0 -y /tmp/output_bptrender.mp4
```

The format for multitcp is to firstly take either a directory path for where .rgb files would be stored from each server, or specify "-" to output a reconstructed image to stdout. Then the full frame render size (i.e., the two halfs combined). Next a bind address and bind port is followed by a corresponding server address and server port. This is repeated again for the second bind address and server. Combined, raw rgb frames are output to stdout for consumption by ffmpeg.

There is also a combine_rgbs client binary which will take streams of .rgb data that reside in files, and combine them into full frames that can be sent on to ffmpeg.


[Sample image](https://phrasep.com/~lvecsey/software/bptrender/output_bptrender.png)

![Image of output](https://phrasep.com/~lvecsey/software/bptrender/output_bptrender.png)

[Sample movie](https://phrasep.com/~lvecsey/software/bptrender/output_bptrender.mp4)
