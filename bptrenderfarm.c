/*
    Render a billion points across a cluster to an image or movie.
    Copyright (C) 2020  Lester Vecsey

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <math.h>
#include <time.h>

#include "mini_gxkit.h"

#include <sys/mman.h>
#include <errno.h>

#include <string.h>

#include <vector.h>

#include <pthread.h>

#include "pointcol.h"

#include "render.h"

#include "render_work.h"

#include "audio_sample.h"

#include <endian.h>

#include "params.h"

#include "norm3d.h"

#include "ga.h"

#include "recvfile.h"
#include "sendfile.h"

#include "readfile.h"
#include "writefile.h"

#include "region.h"

#include "framestatus.h"

#include "compress_zlib.h"

#include "image_stats.h"

typedef struct {

  int fd;
  struct stat buf;
  void *m;
  
} mpack;

const long int def_numthreads = 8;

int main(int argc, char *argv[]) {

  long int input_xres, input_yres;

  double duration;

  double fps;
  
  long int num_threads;

  pthread_t *threads;

  double *drgb;

  size_t drgb_sz;
  
  pointcol *pc;

  long int num_pointcols;

  long int num_pixels;
  size_t img_sz;

  void *compressed_drgb;

  size_t compressed_len;
  
  long int threadno;

  int retval;

  char *dat_fn;

  int fd;
  
  struct stat buf;
  
  void *m;

  long int region;

  render_work *rws;

  audio_sample *audio_samples;

  long int num_samples;

  mpack aud;

  framestatus fs;
  
  ssize_t bytes_read;

  params par;

  struct timespec start, now;

  char *rstr;

  double elapsed_renderlastframe;
  double elapsed_compresslastframe;
  double elapsed_transferlastframe;

  double vf;

  vec3d vFrom, vTo;
  
  rot3 rot;

  double matrix[16];

  double radians;

  double freq;

  int level;

  long int pixelno;

  retval = argc>1 ? sscanf(argv[1],"%ldx%ld",&input_xres,&input_yres) : -1;

  num_threads = argc>2 ? strtol(argv[2],NULL,10) : def_numthreads;
  
  region = RNONE;

  if (argc>3) {
    if (!strncmp(argv[3], "TOP", 3)) {
      region = RTOP;
    }
    else {
      region = RBOTTOM;
    }
  }

  rstr = regstr(region);
  
  duration = argc>4 ? strtod(argv[4],NULL) : 20.0;

  fps = argc>5 ? strtod(argv[5],NULL) : 60;
  
  threads = malloc(num_threads * sizeof(pthread_t));
  if (threads == NULL) {
    perror("malloc");
    return -1;
  }

  rws = calloc(num_threads, sizeof(render_work));
  if (rws == NULL) {
    perror("calloc");
    return -1;
  }

  num_pixels = input_xres * (input_yres >> 1);
  
  drgb_sz = num_pixels * sizeof(double) * 3;
  
  drgb = malloc(drgb_sz);
  if  (drgb == NULL) {
    perror("malloc");
    return -1;
  }
  
  compressed_drgb = malloc(drgb_sz >> 1);
  if (compressed_drgb == NULL) {
    perror("malloc");
    return -1;
  }
  
  fd = open("/tmp/pointcols.dat", O_RDWR);
  if (fd == -1) {
    perror("open");
    return -1;
  }

  retval = fstat(fd, &buf);
  if (retval == -1) {
    perror("fstat");
    return -1;
  }

  num_pointcols = buf.st_size / sizeof(pointcol);

  fprintf(stderr, "%s: (Region %s) num_pointcols %ld\n", __FUNCTION__, rstr, num_pointcols);
  
  {
    off_t offset;

    offset = 0;
      
    m = mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, offset);
    if (m == MAP_FAILED) {
      perror("mmap");
      return -1;
    }
  }
    
  pc = (pointcol*) m;

  aud.fd = open("/tmp/process_audio.raw", O_RDWR);
  if (aud.fd == -1) {
    perror("open");
    return -1;
  }

  retval = fstat(aud.fd, &(aud.buf));
  if (retval == -1) {
    perror("fstat");
    return -1;
  }

  aud.m = mmap(NULL, aud.buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, aud.fd, 0);
  if (aud.m == MAP_FAILED) {
    perror("mmap");
    return -1;
  }

  audio_samples = (audio_sample*) aud.m;

  num_samples = aud.buf.st_size / sizeof(audio_sample);

  fprintf(stderr, "%s: (Region %s) num_samples %ld\n", __FUNCTION__, rstr, num_samples);
  
  fs = (framestatus) { .num_frames = duration * fps };

  freq = 3.0;
  
  fprintf(stderr, "%s: (Region %s) Rendering %ld frames.\n", __FUNCTION__, rstr, fs.num_frames);

  for (fs.frameno = 0; fs.frameno < fs.num_frames; fs.frameno++) {

    fprintf(stderr, "%s: (Region %s) Rendering %ld/%ld\n", __FUNCTION__, rstr, fs.frameno, fs.num_frames);

    clock_gettime(CLOCK_REALTIME, &start);

    for (pixelno = 0; pixelno < num_pixels; pixelno++) {
      drgb[3*pixelno+0] = 0.0;
      drgb[3*pixelno+1] = 0.0;
      drgb[3*pixelno+2] = 0.0;      
    }
    
    if (fs.num_frames == 1 && fps <= 1.0) {
      vf = 0.35;
    }
    else {
      vf = fs.frameno; vf /= (fs.num_frames - 1);
    }

    {

      radians = 2.0 * M_PI * freq * vf;
      
      vFrom[0] = cos(radians);
      vFrom[1] = 0.0;
      vFrom[2] = sin(radians);
      norm3d(&vFrom);

      vTo[0] = 0.0;
      vTo[1] = 1.0;
      vTo[2] = 0.0;
      norm3d(&vTo);
    
      rot = create_rotor(vFrom, vTo);
  
      toMatrix3(&rot, matrix);

    }
    
    for (threadno = 0; threadno < num_threads; threadno++) {

      rws[threadno].xres = input_xres;
      rws[threadno].yres = (input_yres >> 1);
      rws[threadno].drgb = drgb;      
      rws[threadno].region = region;
      rws[threadno].ypos_start = threadno * (input_yres >> 1) / num_threads;
      rws[threadno].ypos_end = (threadno + 1) * (input_yres >> 1) / num_threads;
      rws[threadno].pc = pc;
      rws[threadno].num_pointcols = num_pointcols;
      rws[threadno].audio_samples = audio_samples;
      rws[threadno].num_samples = num_samples;
      rws[threadno].matrix = matrix;
      rws[threadno].vf = vf;
      
    }

    for (threadno = 0; threadno < num_threads - 1; threadno++) {
    
      retval = pthread_create(threads+threadno, NULL, render, rws + threadno);
      if (retval != 0) {
	fprintf(stderr, "%s: Trouble creating thread, errno %d\n", __FUNCTION__, errno);
	perror("pthread_create");
	return -1;
      }

    }

    render(rws + (num_threads - 1));
    
    for (threadno = 0; threadno < num_threads - 1; threadno++) {
      retval = pthread_join(threads[threadno], NULL);
      if (retval != 0) {
	fprintf(stderr, "%s: Trouble joining thread, errno %d\n", __FUNCTION__, errno);
	perror("pthread_join");
	return -1;
      }
    }
      
    clock_gettime(CLOCK_REALTIME, &now);

    {

      elapsed_renderlastframe = now.tv_sec - start.tv_sec;
      
      fprintf(stderr, "%s: (Region %s) Last frame renderered in %gs\n", __FUNCTION__, rstr, elapsed_renderlastframe);

    }
        
    fprintf(stderr, "%s: (Region %s) Compressing image frame.\n", __FUNCTION__, rstr);
    
    clock_gettime(CLOCK_REALTIME, &start);

    level = 9;
    
    retval = compress_zlib(drgb, drgb_sz, compressed_drgb, drgb_sz >> 1, level, &compressed_len);
    if (retval == -1) {
      fprintf(stderr, "%s: Trouble with call to compress_zlib.\n", __FUNCTION__);
      return -1;
    }
    
    clock_gettime(CLOCK_REALTIME, &now);

    {

      elapsed_compresslastframe = now.tv_sec - start.tv_sec;
      
      fprintf(stderr, "%s: (Region %s) Last frame compressed in %gs to size %lu\n", __FUNCTION__, rstr, elapsed_compresslastframe, compressed_len);

    }
    
    fprintf(stderr, "%s: (Region %s) Transferring frame %ld\n", __FUNCTION__, rstr, fs.frameno);

    clock_gettime(CLOCK_REALTIME, &start);
    
    {
      int out_fd;
      ssize_t bytes_written;

      uint64_t len;

      long int num_iters;
      
      out_fd = 1;

      len = htobe64(compressed_len);

      fprintf(stderr, "%s: (Region %s) Sending file header.\n", __FUNCTION__, rstr);

      num_iters = 0;
      
      bytes_written = sendfile(out_fd, &len, sizeof(uint64_t));
      if (bytes_written != sizeof(uint64_t)) {
	perror("write");
	return -1;
      }

      fprintf(stderr, "%s: (Region %s) Sending file data.\n", __FUNCTION__, rstr);

      num_iters = 0;
      
      bytes_written = sendfile(out_fd, compressed_drgb, compressed_len);
      if (bytes_written != compressed_len) {
	fprintf(stderr, "%s: (Region %s) Mismatch, bytes_written %ld\n", __FUNCTION__, rstr, bytes_written);
	perror("write");
	return -1;
      }

      fprintf(stderr, "%s: (Region %s) Waiting for file ack.\n", __FUNCTION__, rstr);
      
      {
	ssize_t bytes_read;

	uint64_t ack;

	bytes_read = readfile(0, &ack, sizeof(uint64_t));
	if (bytes_read == 0) {
	  fprintf(stderr, "%s: (Region %s) Did not get ack after sending file.\n", __FUNCTION__, rstr);
	  return -1;
	}

	if (bytes_read == -1) {
	  fprintf(stderr, "%s: (Region %s) Failure on read of ack.\n", __FUNCTION__, rstr);
	  perror("read");
	  return -1;
	}
	
	if (bytes_read != sizeof(uint64_t)) {
	  fprintf(stderr, "%s: (Region %s) Read of ack returned just bytes_read %ld.\n", __FUNCTION__, rstr, bytes_read);
	  perror("read");
	  return -1;
	}

	if (be64toh(ack) != 0x100) {
	  fprintf(stderr, "%s: Error, did not get clean ack of file transfer.\n", __FUNCTION__);
	  return -1;
	}
    
      }

      clock_gettime(CLOCK_REALTIME, &now);
      
      {

	elapsed_transferlastframe = now.tv_sec - start.tv_sec;
      
	fprintf(stderr, "%s: (Region %s) Last frame transferred in %gs\n", __FUNCTION__, rstr, elapsed_transferlastframe);

      }
      
      fprintf(stderr, "%s: (Region %s) Frame complete.\n", __FUNCTION__, rstr);

      fprintf(stderr, "%s: (Region %s) Estimated time remaining %gs\n", __FUNCTION__, rstr, (fs.num_frames - fs.frameno) * (elapsed_renderlastframe + elapsed_transferlastframe));
      
    }
    
  }

  retval = munmap(aud.m, aud.buf.st_size);
  if (retval == -1) {
    perror("munmap");
    return -1;
  }
  
  retval = close(aud.fd);
  if (retval == -1) {
    perror("close");
    return -1;
  }

  retval = munmap(m, buf.st_size);
  if (retval == -1) {
    perror("munmap");
    return -1;
  }
  
  retval = close(fd);
  if (retval == -1) {
    perror("close");
    return -1;
  }

  free(compressed_drgb);
  
  free(drgb);
  
  free(rws);
  
  free(threads);
  
  return 0;

}
