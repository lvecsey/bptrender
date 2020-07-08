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

#include <time.h>
#include <errno.h>

#include <string.h>

#include <sys/socket.h>
#include <netdb.h>

#include <sys/epoll.h>

#include "recvfile.h"
#include "sendfile.h"

#include "mini_gxkit.h"

#include "fih_core.h"

#include "uncompress_zlib.h"

#include "writefile.h"

#include "imgfill_pack.h"

#include "postprocess.h"

int imgfill_func(void *data, size_t len, void *extra) {

  imgfill_pack *pack;

  pack = (imgfill_pack*) extra;

  memcpy(pack->drgb_start, data, len);

  (*pack->slice_set) = 1;
  
  return 0;

}

int main(int argc, char *argv[]) {

  long int input_address1[4], input_address2[4];
  long int input_port1, input_port2;
  
  unsigned char bind_address1[4], bind_address2[4];
  uint16_t bind_port1, bind_port2;
  
  unsigned char server_address1[4], server_address2[4];
  uint16_t server_port1, server_port2;

  int retval;

  struct protoent *pe;

  int sock1, sock2;
  
  struct sockaddr_in bind_addr1, bind_addr2;

  struct sockaddr_in server_addr1, server_addr2;
  
  socklen_t addrlen; 

  long int colarg;

  char *imghalf_dir;

  long int input_xres, input_yres;
  
  colarg = 1;

  imghalf_dir = argc > colarg ? argv[colarg] : getenv("TMPDIR");

  colarg++;

  retval = argc > colarg ? sscanf(argv[colarg],"%ldx%ld",&input_xres,&input_yres) : -1;
  
  colarg++;
  
  if (argc > colarg) {
    retval = sscanf(argv[colarg], "%ld.%ld.%ld.%ld", input_address1+0, input_address1+1, input_address1+2, input_address1+3);
  }

  colarg++;
  
  input_port1 = argc > colarg ? strtol(argv[colarg],NULL,10) : 4975;

  bind_address1[0] = input_address1[0];
  bind_address1[1] = input_address1[1];
  bind_address1[2] = input_address1[2];
  bind_address1[3] = input_address1[3];  
  
  bind_port1 = input_port1;

  colarg++;
  
  if (argc > colarg) {
    retval = sscanf(argv[colarg], "%ld.%ld.%ld.%ld", input_address1+0, input_address1+1, input_address1+2, input_address1+3);
  }

  colarg++;
  
  input_port1 = argc > colarg ? strtol(argv[colarg],NULL,10) : 5295;

  colarg++;
  
  if (argc > colarg) {
    retval = sscanf(argv[colarg], "%ld.%ld.%ld.%ld", input_address2+0, input_address2+1, input_address2+2, input_address2+3);
  }

  colarg++;
  
  input_port2 = argc > colarg ? strtol(argv[colarg],NULL,10) : 4976;

  bind_address2[0] = input_address2[0];
  bind_address2[1] = input_address2[1];
  bind_address2[2] = input_address2[2];
  bind_address2[3] = input_address2[3];  
  
  bind_port2 = input_port2;

  colarg++;
  
  if (argc > colarg) {
    retval = sscanf(argv[colarg], "%ld.%ld.%ld.%ld", input_address2+0, input_address2+1, input_address2+2, input_address2+3);
  }

  colarg++;
  
  input_port2 = argc > colarg ? strtol(argv[colarg],NULL,10) : 5295;    

  server_address1[0] = input_address1[0];
  server_address1[1] = input_address1[1];
  server_address1[2] = input_address1[2];
  server_address1[3] = input_address1[3];  
  
  server_port1 = input_port1;

  fprintf(stderr, "Server1 set to %u.%u.%u.%u:%u\n", server_address1[0], server_address1[1], server_address1[2], server_address1[3], server_port1);
  
  server_address2[0] = input_address2[0];
  server_address2[1] = input_address2[1];
  server_address2[2] = input_address2[2];
  server_address2[3] = input_address2[3];  

  server_port2 = input_port2;

  fprintf(stderr, "Server2 set to %u.%u.%u.%u:%u\n", server_address2[0], server_address2[1], server_address2[2], server_address2[3], server_port2);
  
  pe = getprotobyname("TCP");
  if (pe == NULL) {
    perror("getprotobyname");
    return -1;
  }
  
  sock1 = socket(AF_INET, SOCK_STREAM, pe->p_proto);
  if (sock1 == -1) {
    perror("socket");
    return -1;
  }

  sock2 = socket(AF_INET, SOCK_STREAM, pe->p_proto);
  if (sock2 == -1) {
    perror("socket");
    return -1;
  }

  memset(&bind_addr1, 0, sizeof(struct sockaddr_in));
  bind_addr1.sin_family = AF_INET;
  memcpy(&(bind_addr1.sin_addr.s_addr), bind_address1, sizeof(bind_address1)); 
  bind_addr1.sin_port = htons(bind_port1);

  addrlen = sizeof(struct sockaddr_in);
  
  retval = bind(sock1, (const struct sockaddr*) &bind_addr1, addrlen);
  if (retval == -1) {
    fprintf(stderr, "%s: Trouble binding to first address, port %d\n", __FUNCTION__, bind_port1);
    perror("bind");
    return -1;
  }
  
  memset(&bind_addr2, 0, sizeof(struct sockaddr_in));  
  bind_addr2.sin_family = AF_INET;
  memcpy(&(bind_addr2.sin_addr.s_addr), bind_address2, sizeof(bind_address2));
  bind_addr2.sin_port = htons(bind_port2);

  retval = bind(sock2, (const struct sockaddr*) &bind_addr2, addrlen);  
  if (retval == -1) {
    fprintf(stderr, "%s: Trouble binding to second address, port %d\n", __FUNCTION__, bind_port2);
    perror("bind");
    return -1;
  }

  memset(&server_addr1, 0, sizeof(struct sockaddr_in));
  server_addr1.sin_family = AF_INET;
  memcpy(&(server_addr1.sin_addr.s_addr), server_address1, sizeof(server_address1));
  server_addr1.sin_port = htons(server_port1);

  fprintf(stderr, "Connecting to %u.%u.%u.%u:%u\n", server_address1[0], server_address1[1], server_address1[2], server_address1[3], server_port1);

  retval = connect(sock1, (const struct sockaddr*) &server_addr1, addrlen);
  if (retval == -1) {
    fprintf(stderr, "%s: Trouble connecting to server1.\n", __FUNCTION__);
    perror("connect");
    return -1;
  }
  
  memset(&server_addr2, 0, sizeof(struct sockaddr_in));
  server_addr2.sin_family = AF_INET;
  memcpy(&(server_addr2.sin_addr.s_addr), server_address2, sizeof(server_address2));
  server_addr2.sin_port = htons(server_port2);
  
  fprintf(stderr, "Connecting to %u.%u.%u.%u:%u\n", server_address2[0], server_address2[1], server_address2[2], server_address2[3], server_port2);  

  retval = connect(sock2, (const struct sockaddr*) &server_addr2, addrlen);
  if (retval == -1) {
    fprintf(stderr, "%s: Trouble connecting to server2.\n", __FUNCTION__);
    perror("connect");
    return -1;
  }
  
#define MAX_EVENTS 10
  
  {

    int epfd;

    struct epoll_event event1, event2;
    
    struct epoll_event events[MAX_EVENTS];

    int nfds;
    
    long int n;

    ssize_t bytes_written;

    fih_state fih1, fih2;

    fih_state *cur_fih;

    int half_fd1, half_fd2;

    imgfill_pack of_fillpack1, of_fillpack2;
    
    long int xres, yres;
    
    long int num_pixels;

    size_t img_sz;

    size_t drgb_sz;
    
    char imghalf_fn[240];

    mode_t mode;

    double *reconstructed_drgb;

    image_t output_img;
    
    long int image_splices[2] = { 0, 0 };

    long int results;

    long int frameno;
    
    epfd = epoll_create1(0);
    if (epfd == -1) {
      perror("epoll_create1");
      return -1;
    }

    if (imghalf_dir[0] == '-' && imghalf_dir[1] == 0) {
      num_pixels = input_xres * input_yres;
      drgb_sz = num_pixels * sizeof(double) * 3;
      reconstructed_drgb = malloc(drgb_sz);
      if (reconstructed_drgb == NULL) {
	perror("malloc");
	return -1;
      }
    }
    else {
      reconstructed_drgb = NULL;
    }
    
    xres = input_xres;
    yres = (input_yres >> 1);
    num_pixels = xres * yres;
    img_sz = num_pixels * sizeof(pixel_t);

    drgb_sz = num_pixels * sizeof(double) * 3;

    output_img.xres = input_xres;
    output_img.yres = input_yres;
    num_pixels = output_img.xres * output_img.yres;
    img_sz = num_pixels * sizeof(pixel_t);
    output_img.rgb = malloc(img_sz);
    if (output_img.rgb == NULL) {
      perror("malloc");
      return -1;
    }
    
    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    
    memset(&fih1, 0, sizeof(fih_state));
    fih1.fd = sock1;
    fih1.out_fd = fih1.fd;
    fih1.len.req_bytes = 8;
    fih1.len.data = malloc(fih1.len.req_bytes);
    if (fih1.len.data == NULL) {
      perror("malloc");
      return -1;
    }
    fih1.xres = xres;
    fih1.yres = yres;
    fih1.compressed_drgb.req_bytes = 4096;
    fih1.compressed_drgb.data = malloc(fih1.compressed_drgb.req_bytes);
    if (fih1.compressed_drgb.data == NULL) {
      perror("malloc");
      return -1;
    }
    fih1.prot_state = FILLING_LEN;
    fih1.unpacked_drgb = malloc(drgb_sz);
    if (fih1.unpacked_drgb == NULL) {
      perror("malloc");
      return -1;
    }

    retval = sprintf(imghalf_fn, "%s/renderfarm_movie-%ldx%ld-top.rgb", imghalf_dir, xres, yres);

    if (imghalf_dir[0] != '-' && imghalf_dir[1] != 0) {
      half_fd1 = open(imghalf_fn, O_CREAT | O_TRUNC | O_WRONLY, mode);
      if (half_fd1 == -1) {
	fprintf(stderr, "%s: Failed to open output file %s\n", __FUNCTION__, imghalf_fn);
	perror("open");
	return -1;
      }

      fih1.output_func = fih_writeout;
      fih1.output_extra = &half_fd1;
      
    }

    else {
      of_fillpack1.drgb_start = reconstructed_drgb;
      of_fillpack1.slice_set = image_splices + 0;
      fih1.output_func = imgfill_func;
      fih1.output_extra = &of_fillpack1;
    }
    
    fih1.active = 1;
    
    memset(&event1, 0, sizeof(struct epoll_event));
    event1.events = EPOLLIN;
    event1.data.ptr = &fih1;
    
    retval = epoll_ctl(epfd, EPOLL_CTL_ADD, sock1, &event1);
    if (retval == -1) {
      perror("epoll_ctl");
      return -1;
    }

    memset(&fih2, 0, sizeof(fih_state));
    fih2.fd = sock2;
    fih2.out_fd = fih2.fd;
    fih2.xres = xres;
    fih2.yres = yres;
    fih2.len.req_bytes = 8;
    fih2.len.data = malloc(fih2.len.req_bytes);
    if (fih2.len.data == NULL) {
      perror("malloc");
      return -1;
    }
    fih2.compressed_drgb.req_bytes = 4096;
    fih2.compressed_drgb.data = malloc(fih2.compressed_drgb.req_bytes);
    if (fih2.compressed_drgb.data == NULL) {
      perror("malloc");
      return -1;
    }
    fih2.prot_state = FILLING_LEN;
    fih2.unpacked_drgb = malloc(drgb_sz);
    if (fih2.unpacked_drgb == NULL) {
      perror("malloc");
      return -1;
    }

    retval = sprintf(imghalf_fn, "%s/renderfarm_movie-%ldx%ld-bottom.rgb", imghalf_dir, xres, yres);
    
    if (imghalf_dir[0] != '-' && imghalf_dir[1] != 0) {
      half_fd2 = open(imghalf_fn, O_CREAT | O_TRUNC | O_WRONLY, mode);
      if (half_fd2 == -1) {
	fprintf(stderr, "%s: Failed to open output file %s\n", __FUNCTION__, imghalf_fn);
	perror("open");
	return -1;
      }

      fih2.output_func = fih_writeout;
      fih2.output_extra = &half_fd2;
      
    }

    else {
      of_fillpack2.drgb_start = reconstructed_drgb + input_xres * (input_yres >> 1) * 3;
      of_fillpack2.slice_set = image_splices + 1;
      fih2.output_func = imgfill_func;
      fih2.output_extra = &of_fillpack2;
    }
    
    fih2.active = 1;
    
    memset(&event2, 0, sizeof(struct epoll_event));
    event2.events = EPOLLIN;
    event2.data.ptr = &fih2;

    retval = epoll_ctl(epfd, EPOLL_CTL_ADD, sock2, &event2);
    if (retval == -1) {
      perror("epoll_ctl");
      return -1;
    }

    frameno = 0;
    
    for (;fih1.active || fih2.active;) {

      fprintf(stderr, "Waiting on epoll events.\n");
      
      nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
      if (nfds == -1) {
	perror("epoll_wait");
	if (errno==EINTR) {
	  continue;
	}
	return -1;
      }

      for (n = 0; n < nfds; ++n) {

	if (events[n].data.ptr == &fih1 || events[n].data.ptr == &fih2) {

	  cur_fih = events[n].data.ptr;

	  results = 0;
	  
	  retval = fih_core(cur_fih, FC_VERBOSE, &results);
	  if (retval == -1) {
	    fprintf(stderr, "%s: Trouble with call to fih_core.\n", __FUNCTION__);
	    return -1;
	  }

	  if (imghalf_dir[0] == '-' && imghalf_dir[1] == 0) {
	    if (results == FCR_OUTPUT && image_splices[0] && image_splices[1]) {
	      size_t len;

	      fprintf(stderr, "%s: Post processing combined frame.\n", __FUNCTION__);

	      retval = postprocess(reconstructed_drgb, &output_img);
	      if (retval == -1) {
		fprintf(stderr, "%s: Trouble calling postprocess from drgb to rgb.\n", __FUNCTION__);
		return -1;
	      }
	      
	      fprintf(stderr, "%s: Writing combined frame.\n", __FUNCTION__);

	      len = input_xres * input_yres * sizeof(pixel_t);
	      
	      bytes_written = writefile(1, output_img.rgb, len);
	      if (bytes_written != len) {
		perror("write");
		return -1;
	      }
	      frameno++;

	      image_splices[0] = 0;
	      image_splices[1] = 0;

	      retval = sendack_restart(fih1.out_fd, &fih1);
	      if (retval == -1) {
		fprintf(stderr, "%s: Trouble sending an ack and restart. fih1\n", __FUNCTION__);
	      }

	      retval = sendack_restart(fih2.out_fd, &fih2);
	      if (retval == -1) {
		fprintf(stderr, "%s: Trouble sending an ack and restart. fih2\n", __FUNCTION__);
	      }
	      
	    }
	  }
	  
	  if (retval == 100) {
	    cur_fih->active = 0;
	  }
	  
	}

      }

    }

    {

      long int num_frames;

      num_frames = frameno;

      fprintf(stderr, "%s: Combined %ld frames.\n", __FUNCTION__, num_frames);
      
    }
    
    retval = close(epfd);
    if (retval == -1) {
      perror("close");
      return -1;
    }

    free(fih1.len.data);
    free(fih1.compressed_drgb.data);
    free(fih1.unpacked_drgb);

    free(fih2.len.data);
    free(fih2.compressed_drgb.data);
    free(fih2.unpacked_drgb);

    if (imghalf_dir[0] == '-' && imghalf_dir[1] == 0) {
      free(reconstructed_drgb);
    }

    free(output_img.rgb);
    
  }

  retval = close(sock1);
  if (retval == -1) {
    perror("close");
    return -1;
  }

  retval = close(sock2);
  if (retval == -1) {
    perror("close");
    return -1;
  }

  return 0;

}
