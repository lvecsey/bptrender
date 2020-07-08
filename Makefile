
CC=gcc

# MEMCHECK=-fsanitize=address,undefined -fno-omit-frame-pointer

CFLAGS=-O3 -Wall -g -pg -I./ $(MEMCHECK)

all : client_binaries server_binaries # tests

client_binaries : gen_points fetch_imghalf combine_rgbs multitcp

server_binaries : bptrenderfarm

tests : test-lzma test-zlib test-zlib_decompress sample.txt.zlib

TMPDIR=/tmp

$(TMPDIR)/test_remoterender-top.png : REMOTE_IP=192.168.1.75

$(TMPDIR)/test_remoterender-top.png : REMOTE_PORT=5295

$(TMPDIR)/test_remoterender-top.png : fetch_imghalf
	@tcpclient -vRHl0 $(REMOTE_IP) $(REMOTE_PORT) ./fetch_imghalf - | convert -size 7680x2160 rgb:- -depth 32 $@

$(TMPDIR)/output_prelastframe.png : $(TMPDIR)/output_prelastframe-7680x4320.rgb
	@convert -size 7680x4320 -depth 64 -define quantum:format=floating-point $^ -resize 3840x2160 $@

$(TMPDIR)/output_postlastframe.png : $(TMPDIR)/output_postlastframe-7680x4320.rgb
	@convert -size 7680x4320 $^ -resize 3840x2160 -depth 32 $@

$(TMPDIR)/output_bptrender.png : RENDER_RES=7680x4320

$(TMPDIR)/output_bptrender.png : OUTPUT_RES=3840x2160

$(TMPDIR)/output_bptrender.png : $(TMPDIR)/renderfarm_image-7680x2160-top.rgb $(TMPDIR)/renderfarm_image-7680x2160-bottom.rgb
	@cat $(TMPDIR)/renderfarm_image-7680x2160-top.rgb $(TMPDIR)/renderfarm_image-7680x2160-bottom.rgb | convert -size $(RENDER_RES) rgb:- -depth 32 -resize $(OUTPUT_RES) $@

/tmp/output_bptrender-fromrgbfiles.mp4 : RENDER_RES=7680x4320

/tmp/output_bptrender-fromrgbfiles.mp4 : FPS=60

/tmp/output_bptrender-fromrgbfiles.mp4 : combine_rgbs
	@./$^ $(TMPDIR)/renderfarm_movie-7680x2160-top.rgb $(TMPDIR)/renderfarm_movie-7680x2160-bottom.rgb | ffmpeg -f rawvideo -pix_fmt rgb48le -r $(FPS) -s $(RENDER_RES) -i - -aspect 16:9 -preset veryslow -vcodec libx264 -filter:v "scale=3840x2160" -y $@

gen_points : LIBS+=-lm

gen_points : gen_points.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

bptrenderfarm : LIBS+=-lm -lpthread -lz

bptrenderfarm : dot.o norm3d.o ga.o stereographic.o render.o region.o recvfile.o sendfile.o compress_zlib.o readfile.o writefile.o bptrenderfarm.o
	@$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

fetch_imghalf : LIBS+=-lz

fetch_imghalf : sendfile.o recvfile.o readfile.o writefile.o uncompress_zlib.o sendack.o fih_core.o image_stats.o postprocess.o fetch_imghalf.o
	@$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

combine_rgbs : readfile.o writefile.o combine_rgbs.o
	@$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

multitcp : LIBS+=-lz

multitcp : readfile.o writefile.o recvfile.o sendfile.o uncompress_zlib.o sendack.o fih_core.o image_stats.o postprocess.o multitcp.o
	@$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

test-lzma : LIBS+=-llzma

test-lzma : readfile.o writefile.o test-lzma.o
	@$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

test-zlib : LIBS+=-lz

test-zlib : readfile.o writefile.o test-zlib.o
	@$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

test-zlib_decompress : LIBS+=-lz

test-zlib_decompress : readfile.o writefile.o test-zlib_decompress.o
	@$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

sample.txt.zlib : sample.txt test-zlib
	@./test-zlib sample.txt sample.txt.zlib

.PHONY:
clean:
	-rm *.o sample.txt.zlib
