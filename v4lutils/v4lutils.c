/*
 * v4lutils - utility library for Video4Linux
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * v4lutils.c: utility functions
 *
 */

#include <stdio.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/videodev.h>
#include <pthread.h>
#include <errno.h>
#include "v4lutils.h"

#define DEFAULT_DEVICE "/dev/video"

/*
 * v4lopen - open the v4l device.
 *
 * name: device file
 * vd: v4l device object
 */
int v4lopen(char *name, v4ldevice *vd)
{
	int i;

	if(name == NULL)
		name = DEFAULT_DEVICE;
	if((vd->fd = open(name,O_RDWR)) < 0) {
		perror("v4lopen:open");
		return -1;
	}
	if(v4lgetcapability(vd))
		return -1;
	for(i=0;i<vd->capability.channels;i++) {
		vd->channel[i].channel = i;
		if(ioctl(vd->fd, VIDIOCGCHAN, &(vd->channel[i])) < 0) {
			perror("v4lopen:VIDIOCGCHAN");
			return -1;
		}
	}
	v4lgetpicture(vd);
	pthread_mutex_init(&vd->mutex, NULL);
	return 0;
}

/*
 * v4lclose - close v4l device
 *
 * vd: v4l device object
 */
int v4lclose(v4ldevice *vd)
{
	close(vd->fd);
	return 0;
}

/*
 * v4lgetcapability - get the capability of v4l device
 *
 * vd: v4l device object
 */
int v4lgetcapability(v4ldevice *vd)
{
	if(ioctl(vd->fd, VIDIOCGCAP, &(vd->capability)) < 0) {
		perror("v4lopen:VIDIOCGCAP");
		return -1;
	}
	return 0;
}

/*
 * v4lsetdefaultnorm - set default norm and reset parameters
 *
 * vd: v4l device object
 * norm: default norm
 */
int v4lsetdefaultnorm(v4ldevice *vd, int norm)
{
	int i;

	for(i=0;i<vd->capability.channels;i++) {
		v4lsetchannelnorm(vd, i, norm);
	}
	if(v4lgetcapability(vd))
		return -1;
	if(v4lgetpicture(vd))
		return -1;
	return 0;
}

/*
 * v4lgetsubcapture - get current status of subfield capturing
 *
 * vd: v4l device object
 */
int v4lgetsubcapture(v4ldevice *vd)
{
	if(ioctl(vd->fd, VIDIOCGCAPTURE, &(vd->capture)) < 0) {
		perror("v4lgetsubcapture:VIDIOCGCAPTURE");
		return -1;
	}
	return 0;
}

/*
 * v4lsetsubcapture - set parameters for subfield capturing
 *
 * vd: v4l device object
 * x,y: coordinate of source rectangle to grab
 * width: width of source rectangle to grab
 * height: height of source rectangle to grab
 * decimation: decimation to apply
 * flags: flag setting for grabbing odd/even frames
 */
int v4lsetsubcapture(v4ldevice *vd, int x, int y, int width, int height, int decimation, int flags)
{
	vd->capture.x = x;
	vd->capture.y = y;
	vd->capture.width = width;
	vd->capture.height = height;
	vd->capture.decimation = decimation;
	vd->capture.flags = flags;
	if(ioctl(vd->fd, VIDIOCGCAPTURE, &(vd->capture)) < 0) {
		perror("v4lsetsubcapture:VIDIOCSCAPTURE");
		return -1;
	}
	return 0;
}

/*
 * v4lgetframebuffer - get current status of frame buffer
 *
 * vd: v4l device object
 */
int v4lgetframebuffer(v4ldevice *vd)
{
	if(ioctl(vd->fd, VIDIOCGFBUF, &(vd->buffer)) < 0) {
		perror("v4lgetframebuffer:VIDIOCGFBUF");
		return -1;
	}
	return 0;
}

/*
 * v4lsetframebuffer - set parameters of frame buffer
 *
 * vd: v4l device object
 * base: base PHYSICAL address of the frame buffer
 * width: width of the frame buffer
 * height: height of the frame buffer
 * depth: color depth of the frame buffer
 * bpl: number of bytes of memory between the start of two adjacent lines
 */
int v4lsetframebuffer(v4ldevice *vd, void *base, int width, int height, int depth, int bpl)
{
	vd->buffer.base = base;
	vd->buffer.width = width;
	vd->buffer.height = height;
	vd->buffer.depth = depth;
	vd->buffer.bytesperline = bpl;
	if(ioctl(vd->fd, VIDIOCSFBUF, &(vd->buffer)) < 0) {
		perror("v4lsetframebuffer:VIDIOCSFBUF");
		return -1;
	}
	return 0;
}

/*
 * v4loverlaystart - activate overlay capturing
 *
 * vd: v4l device object
 */
int v4loverlaystart(v4ldevice *vd)
{
	if(ioctl(vd->fd, VIDIOCCAPTURE, 1) < 0) {
		perror("v4loverlaystart:VIDIOCCAPTURE");
		return -1;
	}
	vd->overlay = 1;
	return 0;
}

/*
 * v4loverlaystop - stop overlay capturing
 *
 * vd: v4l device object
 */
int v4loverlaystop(v4ldevice *vd)
{
	if(ioctl(vd->fd, VIDIOCCAPTURE, 0) < 0) {
		perror("v4loverlaystop:VIDIOCCAPTURE");
		return -1;
	}
	vd->overlay = 0;
	return 0;
}

/*
 * v4lsetchannel - select the video source
 *
 * vd: v4l device object
 * ch: the channel number
 */
int v4lsetchannel(v4ldevice *vd, int ch)
{
	if(ioctl(vd->fd, VIDIOCSCHAN, &(vd->channel[ch])) < 0) {
		perror("v4lsetchannel:VIDIOCSCHAN");
		return -1;
	}
	return 0;
}

/*
 * v4lsetfreq - set the frequency of tuner
 *
 * vd: v4l device object
 * ch: frequency in KHz
 */
int v4lsetfreq(v4ldevice *vd, int freq)
{
	unsigned long longfreq=(freq*16)/1000;
	if(ioctl(vd->fd, VIDIOCSFREQ, &longfreq) < 0) {
		perror("v4lsetchannel:VIDIOCSFREQ");
		return -1;
	}
	return 0;
}

/*
 * v4lsetchannelnorm - set the norm of channel
 *
 * vd: v4l device object
 * ch: the channel number
 * norm: PAL/NTSC/OTHER (see videodev.h)
 */
int v4lsetchannelnorm(v4ldevice *vd, int ch, int norm)
{
	vd->channel[ch].norm = norm;
	return 0;
}

/*
 * v4lgetpicture - get current properties of the picture
 *
 * vd: v4l device object
 */
int v4lgetpicture(v4ldevice *vd)
{
	if(ioctl(vd->fd, VIDIOCGPICT, &(vd->picture)) < 0) {
		perror("v4lgetpicture:VIDIOCGPICT");
		return -1;
	}
	return 0;
}

/*
 * v4lsetpicture - set the image properties of the picture
 *
 * vd: v4l device object
 * br: picture brightness
 * hue: picture hue
 * col: picture color
 * cont: picture contrast
 * white: picture whiteness
 */
int v4lsetpicture(v4ldevice *vd, int br, int hue, int col, int cont, int white)
{
	if(br>=0)
		vd->picture.brightness = br;
	if(hue>=0)
		vd->picture.hue = hue;
	if(col>=0)
		vd->picture.colour = col;
	if(cont>=0)
		vd->picture.contrast = cont;
	if(white>=0)
		vd->picture.whiteness = white;
	if(ioctl(vd->fd, VIDIOCSPICT, &(vd->picture)) < 0) {
		perror("v4lsetpicture:VIDIOCSPICT");
		return -1;
	}
	return 0;
}
 
/*
 * v4lsetpalette - set the palette for the images
 *
 * vd: v4l device object
 * palette: palette
 */
int v4lsetpalette(v4ldevice *vd, int palette)
{
	vd->picture.palette = palette;
	vd->mmap.format = palette;
	if(ioctl(vd->fd, VIDIOCSPICT, &(vd->picture)) < 0) {
		perror("v4lsetpalette:VIDIOCSPICT");
		return -1;
	}
	return 0;
}

/*
 * v4lgetmbuf - get the size of the buffer to mmap
 *
 * vd: v4l device object
 */
int v4lgetmbuf(v4ldevice *vd)
{
	if(ioctl(vd->fd, VIDIOCGMBUF, &(vd->mbuf))<0) {
		perror("v4lgetmbuf:VIDIOCGMBUF");
		return -1;
	}
	return 0;
}

/*
 * v4lmmap - initialize mmap interface
 *
 * vd: v4l device object
 */
int v4lmmap(v4ldevice *vd)
{
	if(v4lgetmbuf(vd)<0)
		return -1;
	if((vd->map = mmap(0, vd->mbuf.size, PROT_READ|PROT_WRITE, MAP_SHARED, vd->fd, 0)) < 0) {
		perror("v4lmmap:mmap");
		return -1;
	}
	return 0;
}

/*
 * v4lmunmap - free memory area for mmap interface
 *
 * vd: v4l device object
 */
int v4lmunmap(v4ldevice *vd)
{
	if(munmap(vd->map, vd->mbuf.size) < 0) {
		perror("v4lmunmap:munmap");
		return -1;
	}
	return 0;
}

/*
 * v4lgrabinit - set parameters for mmap interface
 *
 * vd: v4l device object
 * width: width of the buffer
 * height: height of the buffer
 */
int v4lgrabinit(v4ldevice *vd, int width, int height)
{
	vd->mmap.width = width;
	vd->mmap.height = height;
	vd->mmap.format = vd->picture.palette;
	vd->frame = 0;
	return 0;
}

/*
 * v4lgrabstart - activate mmap capturing
 *
 * vd: v4l device object
 * frame: frame number for storing captured image
 */
int v4lgrabstart(v4ldevice *vd, int frame)
{
	vd->mmap.frame = frame;
	if(ioctl(vd->fd, VIDIOCMCAPTURE, &(vd->mmap)) < 0) {
		perror("v4lgrabstart:VIDIOCMCAPTURE");
		return -1;
	}
	return 0;
}

/*
 * v4lsync - wait until mmap capturing of the frame is finished
 *
 * vd: v4l device object
 * frame: frame number
 */
int v4lsync(v4ldevice *vd, int frame)
{
	if(ioctl(vd->fd, VIDIOCSYNC, &frame) < 0) {
		perror("v4lsync:VIDIOCSYNC");
		return -1;
	}
	return 0;
}

/*
 * v4llock - lock the Video4Linux device object
 *
 * vd: v4l device object
 */
int v4llock(v4ldevice *vd)
{
	return pthread_mutex_lock(&vd->mutex);
}

/*
 * v4lunlock - unlock the Video4Linux device object
 *
 * vd: v4l device object
 */
int v4lunlock(v4ldevice *vd)
{
	return pthread_mutex_unlock(&vd->mutex);
}

/*
 * v4ltrylock - lock the Video4Linux device object (non-blocking mode)
 *
 * vd: v4l device object
 */
int v4ltrylock(v4ldevice *vd)
{
	return pthread_mutex_trylock(&vd->mutex);
}

/*
 * v4lsyncf - flip-flop sync
 *
 * vd: v4l device object
 */
int v4lsyncf(v4ldevice *vd)
{
	return v4lsync(vd, vd->frame);
}

/*
 * v4lgrabf - flip-flop grabbing
 *
 * vd: v4l device object
 */
int v4lgrabf(v4ldevice *vd)
{
	int f;

	f = vd->frame;
	vd->frame = vd->frame ^ 1;
	return v4lgrabstart(vd, f);
}

/*
 * v4lgetaddress - returns a offset addres of buffer for mmap capturing
 *
 * vd: v4l device object
 */
unsigned char *v4lgetaddress(v4ldevice *vd)
{
	return (vd->map + vd->mbuf.offsets[vd->frame]);
}

/*
 * v4lprint - print v4l device object
 *
 * vd: v4l device object
 */
void v4lprint(v4ldevice *vd)
{
	printf("v4l device data\nname: %s\n",vd->capability.name);
	printf("channels: %d\n",vd->capability.channels);
	printf("max size: %dx%d\n",vd->capability.maxwidth, vd->capability.maxheight);
	printf("min size: %dx%d\n",vd->capability.minwidth, vd->capability.minheight);
	printf("device type;\n");
	if(vd->capability.type & VID_TYPE_CAPTURE) printf("VID_TYPE_CAPTURE,");
	if(vd->capability.type & VID_TYPE_OVERLAY) printf("VID_TYPE_OVERLAY,");
	if(vd->capability.type & VID_TYPE_CLIPPING) printf("VID_TYPE_CLIPPING,");
	if(vd->capability.type & VID_TYPE_FRAMERAM) printf("VID_TYPE_FRAMERAM,");
	if(vd->capability.type & VID_TYPE_SCALES) printf("VID_TYPE_SCALES,");
	if(vd->capability.type & VID_TYPE_MONOCHROME) printf("VID_TYPE_MONOCHROME,");
	if(vd->capability.type & VID_TYPE_SUBCAPTURE) printf("VID_TYPE_SUBCAPTURE,");
	printf("\ncurrent status;\n");
	printf("picture.depth: %d\n",vd->picture.depth);
	printf("mbuf.size: %08x\n",vd->mbuf.size);
	printf("mbuf.frames: %d\n",vd->mbuf.frames);
	printf("mbuf.offsets[0]: %08x\n",vd->mbuf.offsets[0]);
	printf("mbuf.offsets[1]: %08x\n",vd->mbuf.offsets[1]);
}
