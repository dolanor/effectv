/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * diff.c: color independant differencing.  Just a little test.
 *  copyright (c) 2001 Sam Mertens.  This code is subject to the provisions of
 *  the GNU Public License.
 *
 * Controls:
 *      c   -   lower tolerance (threshhold)
 *      v   -   increase tolerance (threshhold)
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../EffecTV.h"
#include "utils.h"

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#define TOLERANCE_STEP  4

int diffStart();
int diffStop();
int diffDraw();
int diffEvent();
void diffUpdate();
void diffSave();
static char *effectname = "DiffTV";
static int state = 0;

static RGB32* prevbuf;

static int g_tolerance[3] = {10, 10, 10};

effect *diffRegister()
{
	effect *entry;

	sharedbuffer_reset();
	prevbuf = (RGB32*)sharedbuffer_alloc(video_height * video_width * sizeof(RGB32));
	if(prevbuf == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}

	entry->name = effectname;
	entry->start = diffStart;
	entry->stop = diffStop;
	entry->draw = diffDraw;
	entry->event = diffEvent;

	return entry;
}

int diffStart()
{
    
#ifdef DEBUG 
    v4lprint(&vd);
#endif

    if (video_grabstart())
    {
        return -1;
    }
    
    state = 1;
	return 0;
}

int diffStop()
{
	if(state) {
        video_grabstop();
	  state = 0;
	}

	return 0;
}

int diffDraw()
{
    int i;
    int x,y;
    unsigned int src_red, src_grn, src_blu;
    unsigned int old_red, old_grn, old_blu;
    unsigned int red_val, red_diff, grn_val, grn_diff, blu_val, blu_diff;
    RGB32* src;
    RGB32* dest;
    

    if (video_syncframe())
    {
        return -1;
    }
    src = (RGB32*)video_getaddress();

    if (stretch) {
        dest = stretching_buffer;
    } else {
        dest = (RGB32*)screen_getaddress();
    }

    if (video_grabframe())
        return -1;
    if (screen_mustlock()) {
        if (0 > screen_lock()) {
            return 0;
        }
    }
    
    i = 0;
    for (y = 0; y < video_height; y++)
    {
        for (x = 0; x < video_width; x++)
        {
            // MMX would just eat this algorithm up
            src_red = (src[i] & 0x00FF0000) >> 16;
            src_grn = (src[i] & 0x0000FF00) >> 8;
            src_blu = (src[i] & 0x000000FF);

            old_red = (prevbuf[i] & 0x00FF0000) >> 16;
            old_grn = (prevbuf[i] & 0x0000FF00) >> 8;
            old_blu = (prevbuf[i] & 0x000000FF);

            // RED
            if (src_red > old_red)
            {
                red_val = 0xFF;
                red_diff = src_red - old_red;
            }
            else
            {
                red_val = 0x7F;
                red_diff = old_red - src_red;
            }
            red_val = (red_diff >= g_tolerance[0]) ? red_val : 0x00;

            // GREEN
            if (src_grn > old_grn)
            {
                grn_val = 0xFF;
                grn_diff = src_grn - old_grn;
            }
            else
            {
                grn_val = 0x7F;
                grn_diff = old_grn - src_grn;
            }
            grn_val = (grn_diff >= g_tolerance[1]) ? grn_val : 0x00;
            
            // BLUE
            if (src_blu > old_blu)
            {
                blu_val = 0xFF;
                blu_diff = src_blu - old_blu;
            }
            else
            {
                blu_val = 0x7F;
                blu_diff = old_blu - src_blu;
            }
            blu_val = (blu_diff >= g_tolerance[2]) ? blu_val : 0x00;

            prevbuf[i] = (( ((src_red + old_red) >> 1) << 16) +
                          ( ((src_grn + old_grn) >> 1) << 8) +
                          ( ((src_blu + old_blu) >> 1) ) );

#if 0            
            if ((0x00 != blu_val) && (0x00 != grn_val) && (0x00 != red_val))
            {
                dest[i] = src[i];
            }
            else
            {
                dest[i] = prevbuf[i];
            }
#else            
            dest[i] = (red_val << 16) + (grn_val << 8) + blu_val;
#endif
            
 //x            dest[i] = (red_diff << 17) + (grn_diff << 9) + (blu_diff << 1);

            
//            prevbuf[i] = src[i];    // Since we're already iterating through both of them...

            i++;
        }
    }
            
    if (stretch) {
        image_stretch_to_screen();
    }

    if (screen_mustlock()) {
        screen_unlock();
    }
    
	return 0;
}


int diffEvent(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
        case SDLK_c:
            g_tolerance[0] -= TOLERANCE_STEP;
            g_tolerance[0] &= 0xFF;
            
            g_tolerance[1] -= TOLERANCE_STEP;
            g_tolerance[1] &= 0xFF;
            
            g_tolerance[2] -= TOLERANCE_STEP;
            g_tolerance[2] &= 0xFF;
            
            fprintf(stderr, "tol: %d,%d,%d\n", g_tolerance[0], g_tolerance[1], g_tolerance[2]);
            break;
        case SDLK_v:
            g_tolerance[0] += TOLERANCE_STEP;
            g_tolerance[0] &= 0xFF;
            
            g_tolerance[1] += TOLERANCE_STEP;
            g_tolerance[1] &= 0xFF;
            
            g_tolerance[2] += TOLERANCE_STEP;
            g_tolerance[2] &= 0xFF;
            
            fprintf(stderr, "tol: %d,%d,%d\n", g_tolerance[0], g_tolerance[1], g_tolerance[2]);
            break;
		case SDLK_SPACE:
			break;
            
		default:
			break;
		}
	}
    
	return 0;
}
