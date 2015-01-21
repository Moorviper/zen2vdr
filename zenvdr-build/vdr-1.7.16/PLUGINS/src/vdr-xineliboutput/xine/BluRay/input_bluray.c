/*
 * Copyright (C) 2000-2005 the xine project
 *
 * Copyright (C) 2009 Petri Hintukainen <phintuka@users.sourceforge.net>
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * Input plugin for BluRay discs / images
 *
 * Requires libbluray from git://git.videolan.org/libbluray.git
 * Tested with revision 2010-12-10 10:00 UTC
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* asprintf: */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>

#include <libbluray/bluray.h>
#include <libbluray/keys.h>
#include <libbluray/overlay.h>
#include <libbluray/meta_data.h>

#define LOG_MODULE "input_bluray"
#define LOG_VERBOSE

#define LOG

#define LOGMSG(x...)  xine_log (this->stream->xine, XINE_LOG_MSG, "input_bluray: " x);

#define XINE_ENGINE_INTERNAL

#ifdef HAVE_CONFIG_H
# include "xine_internal.h"
# include "input_plugin.h"
#else
# include <xine/xine_internal.h>
# include <xine/input_plugin.h>
#endif

#ifndef XINE_VERSION_CODE
# error XINE_VERSION_CODE undefined !
#endif

#ifndef EXPORTED
#  define EXPORTED __attribute__((visibility("default")))
#endif

#ifndef MIN
# define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
# define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#define ALIGNED_UNIT_SIZE 6144
#define PKT_SIZE          192
#define TICKS_IN_MS       45

typedef struct {

  input_class_t   input_class;

  xine_t         *xine;

  xine_mrl_t    **xine_playlist;
  int             xine_playlist_size;

  /* config */
  char           *mountpoint;
  char           *device;
  char           *language;
  char           *country;
  int             region;
  int             parental;
} bluray_input_class_t;

typedef struct {
  input_plugin_t        input_plugin;

  xine_stream_t        *stream;
  xine_event_queue_t   *event_queue;
  xine_osd_t           *osd;

  bluray_input_class_t *class;

  char                 *mrl;
  char                 *disc_root;
  char                 *disc_name;

  BLURAY               *bdh;

  const BLURAY_DISC_INFO *disc_info;
  const META_DL          *meta_dl; /* disc library meta data */

  int                num_title_idx;     /* number of relevant playlists */
  int                current_title_idx;
  int                num_titles;        /* navigation mode, number of titles in disc index */
  int                current_title;     /* navigation mode, title from disc index */
  BLURAY_TITLE_INFO *title_info;
  int                current_clip;
  int                error;
  int                menu_open;
  int                stream_flushed;
  int                pg_enable;
  int                pg_stream;

  uint32_t       cap_seekable;
  uint8_t        nav_mode;

} bluray_input_plugin_t;

static void close_overlay(bluray_input_plugin_t *this)
{
  if (this->osd) {
    xine_osd_free(this->osd);
    this->osd = NULL;
  }
}

static void overlay_proc(void *this_gen, const BD_OVERLAY * const ov)
{
  bluray_input_plugin_t *this = (bluray_input_plugin_t *) this_gen;
  uint32_t color[256];
  uint8_t  trans[256];
  unsigned i;

  if (!this) {
    return;
  }

  if (!ov || ov->plane == 1)
    this->menu_open = 0;

  if (!ov || !ov->img) {
    /* hide OSD */
    close_overlay(this);
    return;
  }

  /* open xine OSD */

  if (!this->osd) {
    this->osd = xine_osd_new(this->stream, 0, 0, 1920, 1080);
  }
  if (!this->pg_enable)
    _x_select_spu_channel(this->stream, -1);

  /* convert and set palette */

  for(i = 0; i < 256; i++) {
    trans[i] = ov->palette[i].T;
    color[i] = (ov->palette[i].Y << 16) | (ov->palette[i].Cr << 8) | ov->palette[i].Cb;
  }

  xine_osd_set_palette(this->osd, color, trans);

  /* uncompress and draw bitmap */

  const BD_PG_RLE_ELEM *rlep = ov->img;
  uint8_t *img = malloc(ov->w * ov->h);
  unsigned pixels = ov->w * ov->h;

  for (i = 0; i < pixels; i += rlep->len, rlep++) {
    memset(img + i, rlep->color, rlep->len);
  }

  xine_osd_draw_bitmap(this->osd, img, ov->x, ov->y, ov->w, ov->h, NULL);

  free(img);

  /* display */

  xine_osd_show(this->osd, 0);

  if (ov->plane == 1)
    this->menu_open = 1;
}

static void update_stream_info(bluray_input_plugin_t *this)
{
  /* set stream info */

  _x_stream_info_set(this->stream, XINE_STREAM_INFO_DVD_ANGLE_COUNT,    this->title_info->angle_count);
  _x_stream_info_set(this->stream, XINE_STREAM_INFO_DVD_ANGLE_NUMBER,   bd_get_current_angle(this->bdh));
  _x_stream_info_set(this->stream, XINE_STREAM_INFO_HAS_CHAPTERS,       this->title_info->chapter_count > 0);
  _x_stream_info_set(this->stream, XINE_STREAM_INFO_DVD_CHAPTER_COUNT,  this->title_info->chapter_count);
  _x_stream_info_set(this->stream, XINE_STREAM_INFO_DVD_CHAPTER_NUMBER, bd_get_current_chapter(this->bdh) + 1);
}

static void update_title_info(bluray_input_plugin_t *this)
{
  if (this->title_info)
    bd_free_title_info(this->title_info);
  this->title_info = bd_get_title_info(this->bdh, this->current_title_idx);
  if (!this->title_info) {
    LOGMSG("bd_get_title_info(%d) failed\n", this->current_title_idx);
    return;
  }

#ifdef LOG
  int ms = this->title_info->duration / INT64_C(90);
  lprintf("Opened title %d. Length %"PRId64" bytes / %02d:%02d:%02d.%03d\n",
          this->current_title_idx, bd_get_title_size(this->bdh),
          ms / 3600000, (ms % 3600000 / 60000), (ms % 60000) / 1000, ms % 1000);
#endif

  /* set title */

  xine_ui_data_t udata;
  xine_event_t uevent = {
    .type = XINE_EVENT_UI_SET_TITLE,
    .stream = this->stream,
    .data = &udata,
    .data_length = sizeof(udata)
  };

  char title_name[64] = "";

  if (this->meta_dl) {
    unsigned i;
    for (i = 0; i < this->meta_dl->toc_count; i++)
      if (this->meta_dl->toc_entries[i].title_number == (unsigned)this->current_title)
        if (this->meta_dl->toc_entries[i].title_name)
          if (strlen(this->meta_dl->toc_entries[i].title_name) > 2)
            strncpy(title_name, this->meta_dl->toc_entries[i].title_name, sizeof(title_name));
  }

  if (title_name[0]) {
  } else if (this->current_title == BLURAY_TITLE_TOP_MENU) {
    strcpy(title_name, "Top Menu");
  } else if (this->current_title == BLURAY_TITLE_FIRST_PLAY) {
    strcpy(title_name, "First Play");
  } else if (this->nav_mode) {
    snprintf(title_name, sizeof(title_name), "Title %d/%d (PL %d/%d)",
             this->current_title, this->num_titles,
             this->current_title_idx + 1, this->num_title_idx);
  } else {
    snprintf(title_name, sizeof(title_name), "Title %d/%d",
             this->current_title_idx + 1, this->num_title_idx);
  }

  if (this->disc_name && this->disc_name[0]) {
    udata.str_len = snprintf(udata.str, sizeof(udata.str), "%s, %s",
                             this->disc_name, title_name);
  } else {
    udata.str_len = snprintf(udata.str, sizeof(udata.str), "%s",
                             title_name);
  }
  xine_event_send(this->stream, &uevent);

  _x_meta_info_set(this->stream, XINE_META_INFO_TITLE, udata.str);

  /* calculate and set stream rate */

  uint64_t rate = bd_get_title_size(this->bdh) * UINT64_C(8) // bits
                  * INT64_C(90000)
                  / (uint64_t)(this->title_info->duration);
  _x_stream_info_set(this->stream, XINE_STREAM_INFO_BITRATE, rate);

#ifdef LOG
  int krate = (int)(rate / UINT64_C(1024));
  int s = this->title_info->duration / 90000;
  int h = s / 3600; s -= h*3600;
  int m = s / 60;   s -= m*60;
  int f = this->title_info->duration % 90000; f = f * 1000 / 90000;
  LOGMSG("BluRay stream: length:  %d:%d:%d.%03d\n"
         "               bitrate: %d.%03d Mbps\n\n",
         h, m, s, f, krate/1024, (krate%1024)*1000/1024);
#endif

  /* set stream info */

  _x_stream_info_set(this->stream, XINE_STREAM_INFO_DVD_TITLE_COUNT,  this->num_title_idx);
  _x_stream_info_set(this->stream, XINE_STREAM_INFO_DVD_TITLE_NUMBER, this->current_title_idx + 1);

  update_stream_info(this);
}

static int open_title (bluray_input_plugin_t *this, int title)
{
  if (bd_select_title(this->bdh, title) <= 0) {
    LOGMSG("bd_select_title(%d) failed\n", title);
    return 0;
  }

  this->current_title_idx = title;

  update_title_info(this);

  return 1;
}

#ifndef DEMUX_OPTIONAL_DATA_FLUSH
#  define DEMUX_OPTIONAL_DATA_FLUSH 0x10000
#endif

static void stream_flush(bluray_input_plugin_t *this)
{
  if (this->stream_flushed)
    return;

  lprintf("Stream flush\n");

  this->stream_flushed = 1;

  int tmp = 0;
  if (DEMUX_OPTIONAL_SUCCESS !=
      this->stream->demux_plugin->get_optional_data(this->stream->demux_plugin, &tmp, DEMUX_OPTIONAL_DATA_FLUSH)) {
    LOGMSG("stream flush not supported by the demuxer !\n");
    return;
  }
}

static void stream_reset(bluray_input_plugin_t *this)
{
  if (!this || !this->stream || !this->stream->demux_plugin)
    return;

  lprintf("Stream reset\n");

  this->cap_seekable = 0;

  xine_set_param(this->stream, XINE_PARAM_FINE_SPEED, XINE_FINE_SPEED_NORMAL);
  this->stream->demux_plugin->seek(this->stream->demux_plugin, 0, 0, 1);
  _x_demux_control_start(this->stream);

  this->cap_seekable = INPUT_CAP_SEEKABLE;
}

static void wait_secs(bluray_input_plugin_t *this, unsigned seconds)
{
  // infinite still mode ?
  if (!seconds) {
    stream_flush(this);
    xine_usec_sleep(10*1000);
    return;
  }

  // clip to allowed range
  if (seconds > 300) {
    seconds = 300;
  }

  // pause the stream
  int paused = _x_get_fine_speed(this->stream) == XINE_SPEED_PAUSE;
  if (!paused) {
    _x_set_fine_speed(this->stream, XINE_SPEED_PAUSE);
  }

  // wait until interrupted
  int loops = seconds * 25; /* N * 40 ms */
  while (!this->stream->demux_action_pending && loops-- > 0) {
    xine_usec_sleep(40*1000);
  }

  lprintf("paused for %d seconds (%d ms left)\n", seconds - loops/25, loops * 40);

  if (!paused) {
    _x_set_fine_speed(this->stream, XINE_FINE_SPEED_NORMAL);
  }
}

static void handle_libbluray_event(bluray_input_plugin_t *this, BD_EVENT ev)
{
    switch (ev.event) {

      case BD_EVENT_ERROR:
        LOGMSG("BD_EVENT_ERROR\n");
        this->error = 1;
        return;

      case BD_EVENT_ENCRYPTED:
        lprintf("BD_EVENT_ENCRYPTED\n");
        _x_message (this->stream, XINE_MSG_ENCRYPTED_SOURCE,
                    "Media stream scrambled/encrypted", NULL);
        this->error = 1;
        return;

      /* playback control */

      case BD_EVENT_SEEK:
        lprintf("BD_EVENT_SEEK\n");
        stream_reset(this);
        break;

      case BD_EVENT_STILL_TIME:
        lprintf("BD_EVENT_STILL_TIME %d\n", ev.param);
        wait_secs(this, ev.param);
        break;

      case BD_EVENT_STILL:
        lprintf("BD_EVENT_STILL %d\n", ev.param);
        int paused = _x_get_fine_speed(this->stream) == XINE_SPEED_PAUSE;
        if (paused && !ev.param) {
          _x_set_fine_speed(this->stream, XINE_FINE_SPEED_NORMAL);
        }
        if (!paused && ev.param) {
          _x_set_fine_speed(this->stream, XINE_SPEED_PAUSE);
        }
        break;

      /* playback position */

      case BD_EVENT_ANGLE:
        lprintf("BD_EVENT_ANGLE_NUMBER %d\n", ev.param);
        _x_stream_info_set(this->stream, XINE_STREAM_INFO_DVD_ANGLE_NUMBER, ev.param);
        break;

      case BD_EVENT_END_OF_TITLE:
        LOGMSG("BD_EVENT_END_OF_TITLE\n");
        stream_flush(this);
        break;

      case BD_EVENT_TITLE:
        LOGMSG("BD_EVENT_TITLE %d\n", ev.param);
        this->current_title = ev.param;
        break;

      case BD_EVENT_PLAYLIST:
        lprintf("BD_EVENT_PLAYLIST %d\n", ev.param);
        this->current_title_idx = bd_get_current_title(this->bdh);
        this->current_clip = 0;
        update_title_info(this);
        stream_reset(this);
        break;

      case BD_EVENT_PLAYITEM:
        lprintf("BD_EVENT_PLAYITEM %d\n", ev.param);
        if (ev.param < this->title_info->clip_count)
          this->current_clip = ev.param;
        else
          this->current_clip = 0;
        break;

      case BD_EVENT_CHAPTER:
        lprintf("BD_EVENT_CHAPTER %d\n", ev.param);
        _x_stream_info_set(this->stream, XINE_STREAM_INFO_DVD_CHAPTER_NUMBER, ev.param);
        break;

      /* stream selection */

      case BD_EVENT_AUDIO_STREAM:
        lprintf("BD_EVENT_AUDIO_STREAM %d\n", ev.param);
        this->stream->audio_channel_user = ev.param - 1;
        break;

      case BD_EVENT_PG_TEXTST:
        lprintf("BD_EVENT_PG_TEXTST %s\n", ev.param ? "ON" : "OFF");
        this->pg_enable = ev.param;
        if (!this->pg_enable) {
          _x_select_spu_channel(this->stream, -2);
        } else {
          _x_select_spu_channel(this->stream, this->pg_stream);
        }
        break;

      case BD_EVENT_PG_TEXTST_STREAM:
        lprintf("BD_EVENT_PG_TEXTST_STREAM %d\n", ev.param);
        this->pg_stream = ev.param - 1;
        if (this->pg_enable) {
          _x_select_spu_channel(this->stream, this->pg_stream);
        }
        break;

      case BD_EVENT_IG_STREAM:
      case BD_EVENT_SECONDARY_AUDIO:
      case BD_EVENT_SECONDARY_AUDIO_STREAM:
      case BD_EVENT_SECONDARY_VIDEO:
      case BD_EVENT_SECONDARY_VIDEO_SIZE:
      case BD_EVENT_SECONDARY_VIDEO_STREAM:
        // TODO

      case BD_EVENT_NONE:
        break;

      default:
        LOGMSG("unhandled libbluray event %d [param %d]\n", ev.event, ev.param);
        break;
    }
}

static void handle_libbluray_events(bluray_input_plugin_t *this)
{
  BD_EVENT ev;
  while (bd_get_event(this->bdh, &ev)) {
    handle_libbluray_event(this, ev);
    if (this->error || ev.event == BD_EVENT_NONE || ev.event == BD_EVENT_ERROR)
      break;
  }
}

static void handle_events(bluray_input_plugin_t *this)
{
  if (!this->event_queue)
    return;

  xine_event_t *event;
  while (NULL != (event = xine_event_get(this->event_queue))) {

    if (!this->bdh || !this->title_info) {
      xine_event_free(event);
      return;
    }

    int64_t pts = xine_get_current_vpts(this->stream) -
      this->stream->metronom->get_option(this->stream->metronom, METRONOM_VPTS_OFFSET);

    if (this->menu_open) {
      switch (event->type) {
        case XINE_EVENT_INPUT_LEFT:      bd_user_input(this->bdh, pts, BD_VK_LEFT);  break;
        case XINE_EVENT_INPUT_RIGHT:     bd_user_input(this->bdh, pts, BD_VK_RIGHT); break;
      }
    } else {
      switch (event->type) {

        case XINE_EVENT_INPUT_LEFT:
          lprintf("XINE_EVENT_INPUT_LEFT: previous title\n");
          if (!this->nav_mode) {
            open_title(this, MAX(0, this->current_title_idx - 1));
          } else {
            bd_play_title(this->bdh, MAX(1, this->current_title - 1));
          }
          break;

        case XINE_EVENT_INPUT_RIGHT:
          lprintf("XINE_EVENT_INPUT_RIGHT: next title\n");
          if (!this->nav_mode) {
            open_title(this, MIN(this->num_title_idx - 1, this->current_title_idx + 1));
          } else {
            bd_play_title(this->bdh, MIN(this->num_titles, this->current_title + 1));
          }
          break;
      }
    }

    switch (event->type) {

      case XINE_EVENT_INPUT_MOUSE_BUTTON: {
        xine_input_data_t *input = event->data;
        lprintf("mouse click: button %d at (%d,%d)\n", input->button, input->x, input->y);
        if (input->button == 1) {
          bd_mouse_select(this->bdh, pts, input->x, input->y);
          bd_user_input(this->bdh, pts, BD_VK_MOUSE_ACTIVATE);
        }
        break;
      }

      case XINE_EVENT_INPUT_MOUSE_MOVE: {
        xine_input_data_t *input = event->data;
        bd_mouse_select(this->bdh, pts, input->x, input->y);
        break;
      }

      case XINE_EVENT_INPUT_MENU1:
        if (!this->disc_info->top_menu_supported) {
          _x_message (this->stream, XINE_MSG_GENERAL_WARNING,
                      "Can't open Top Menu",
                      "Top Menu title not supported", NULL);
        }
        bd_menu_call(this->bdh, pts);
        break;

      case XINE_EVENT_INPUT_MENU2:     bd_user_input(this->bdh, pts, BD_VK_POPUP); break;
      case XINE_EVENT_INPUT_UP:        bd_user_input(this->bdh, pts, BD_VK_UP);    break;
      case XINE_EVENT_INPUT_DOWN:      bd_user_input(this->bdh, pts, BD_VK_DOWN);  break;
      case XINE_EVENT_INPUT_SELECT:    bd_user_input(this->bdh, pts, BD_VK_ENTER); break;
      case XINE_EVENT_INPUT_NUMBER_0:  bd_user_input(this->bdh, pts, BD_VK_0); break;
      case XINE_EVENT_INPUT_NUMBER_1:  bd_user_input(this->bdh, pts, BD_VK_1); break;
      case XINE_EVENT_INPUT_NUMBER_2:  bd_user_input(this->bdh, pts, BD_VK_2); break;
      case XINE_EVENT_INPUT_NUMBER_3:  bd_user_input(this->bdh, pts, BD_VK_3); break;
      case XINE_EVENT_INPUT_NUMBER_4:  bd_user_input(this->bdh, pts, BD_VK_4); break;
      case XINE_EVENT_INPUT_NUMBER_5:  bd_user_input(this->bdh, pts, BD_VK_5); break;
      case XINE_EVENT_INPUT_NUMBER_6:  bd_user_input(this->bdh, pts, BD_VK_6); break;
      case XINE_EVENT_INPUT_NUMBER_7:  bd_user_input(this->bdh, pts, BD_VK_7); break;
      case XINE_EVENT_INPUT_NUMBER_8:  bd_user_input(this->bdh, pts, BD_VK_8); break;
      case XINE_EVENT_INPUT_NUMBER_9:  bd_user_input(this->bdh, pts, BD_VK_9); break;

      case XINE_EVENT_INPUT_NEXT: {
        unsigned chapter = bd_get_current_chapter(this->bdh) + 1;

        lprintf("XINE_EVENT_INPUT_NEXT: next chapter\n");

        if (chapter >= this->title_info->chapter_count) {
          if (this->current_title_idx < this->num_title_idx - 1) {
            open_title(this, this->current_title_idx + 1);
            stream_reset(this);
          }
        } else {
          bd_seek_chapter(this->bdh, chapter);
          update_stream_info(this);
          stream_reset(this);
        }
        break;
      }

      case XINE_EVENT_INPUT_PREVIOUS: {
        int chapter = bd_get_current_chapter(this->bdh) - 1;

        lprintf("XINE_EVENT_INPUT_PREVIOUS: previous chapter\n");

        if (chapter < 0 && this->current_title_idx > 0) {
          open_title(this, this->current_title_idx - 1);
          stream_reset(this);
        } else {
          chapter = MAX(0, chapter);
          bd_seek_chapter(this->bdh, chapter);
          update_stream_info(this);
          stream_reset(this);
        }
        break;
      }

      case XINE_EVENT_INPUT_ANGLE_NEXT: {
        unsigned curr_angle = bd_get_current_angle(this->bdh);
        unsigned angle      = MIN(8, curr_angle + 1);
        lprintf("XINE_EVENT_INPUT_ANGLE_NEXT: set angle %d --> %d\n", curr_angle, angle);
        bd_seamless_angle_change(this->bdh, angle);
        _x_stream_info_set(this->stream, XINE_STREAM_INFO_DVD_ANGLE_NUMBER, bd_get_current_angle(this->bdh));
        break;
      }

      case XINE_EVENT_INPUT_ANGLE_PREVIOUS: {
        unsigned curr_angle = bd_get_current_angle(this->bdh);
        unsigned angle      = curr_angle ? curr_angle - 1 : 0;
        lprintf("XINE_EVENT_INPUT_ANGLE_PREVIOUS: set angle %d --> %d\n", curr_angle, angle);
        bd_seamless_angle_change(this->bdh, angle);
        _x_stream_info_set(this->stream, XINE_STREAM_INFO_DVD_ANGLE_NUMBER, bd_get_current_angle(this->bdh));
        break;
      }
    }

    xine_event_free(event);
  }
}

/*
 * xine plugin interface
 */

static uint32_t bluray_plugin_get_capabilities (input_plugin_t *this_gen)
{
  bluray_input_plugin_t *this = (bluray_input_plugin_t *) this_gen;
  return this->cap_seekable  |
         INPUT_CAP_BLOCK     |
         INPUT_CAP_AUDIOLANG |
         INPUT_CAP_SPULANG   |
         INPUT_CAP_CHAPTERS;
}

#if XINE_VERSION_CODE >= 10190
static off_t bluray_plugin_read (input_plugin_t *this_gen, void *buf, off_t len)
#else
static off_t bluray_plugin_read (input_plugin_t *this_gen, char *buf, off_t len)
#endif
{
  bluray_input_plugin_t *this = (bluray_input_plugin_t *) this_gen;
  off_t result;

  if (!this || !this->bdh || len < 0 || this->error)
    return -1;

  handle_events(this);

  if (this->nav_mode) {
    do {
      BD_EVENT ev;
      result = bd_read_ext (this->bdh, (unsigned char *)buf, len, &ev);
      handle_libbluray_event(this, ev);
      if (result == 0) {
        handle_events(this);
        if (ev.event == BD_EVENT_NONE) {
          if (this->stream->demux_action_pending) {
            break;
          }
        }
      }
    } while (!this->error && result == 0);
  } else {
    result = bd_read (this->bdh, (unsigned char *)buf, len);
    handle_libbluray_events(this);
  }

  if (result < 0)
    LOGMSG("bd_read() failed: %s (%d of %d)\n", strerror(errno), (int)result, (int)len);

  this->stream_flushed = 0;

#if 0
  if (buf[4] != 0x47) {
    LOGMSG("bd_read(): invalid data ? [%02x %02x %02x %02x %02x ...]\n",
          buf[0], buf[1], buf[2], buf[3], buf[4]);
    return 0;
  }
#endif

  return result;
}

static buf_element_t *bluray_plugin_read_block (input_plugin_t *this_gen, fifo_buffer_t *fifo, off_t todo)
{
  buf_element_t *buf = fifo->buffer_pool_alloc (fifo);

  if (todo > (off_t)buf->max_size)
    todo = buf->max_size;

  if (todo > ALIGNED_UNIT_SIZE)
    todo = ALIGNED_UNIT_SIZE;

  if (todo > 0) {
    bluray_input_plugin_t *this = (bluray_input_plugin_t *) this_gen;

    buf->size = bluray_plugin_read(this_gen, (char*)buf->mem, todo);
    buf->type = BUF_DEMUX_BLOCK;

    if (buf->size > 0) {
      buf->extra_info->input_time = 0;
      buf->extra_info->total_time = this->title_info->duration / 90000;
      return buf;
    }
  }

  buf->free_buffer (buf);
  return NULL;
}

static off_t bluray_plugin_seek (input_plugin_t *this_gen, off_t offset, int origin)
{
  bluray_input_plugin_t *this = (bluray_input_plugin_t *) this_gen;

  if (!this || !this->bdh)
    return -1;
  if (this->current_title_idx < 0)
    return offset;

  /* convert relative seeks to absolute */

  if (origin == SEEK_CUR) {
    offset = bd_tell(this->bdh) + offset;
  }
  else if (origin == SEEK_END) {
    if (offset < (off_t)bd_get_title_size(this->bdh))
      offset = bd_get_title_size(this->bdh) - offset;
    else
      offset = 0;
  }

  lprintf("bluray_plugin_seek() seeking to %lld\n", (long long)offset);

  return bd_seek (this->bdh, offset);
}

static off_t bluray_plugin_seek_time (input_plugin_t *this_gen, int time_offset, int origin)
{
  bluray_input_plugin_t *this = (bluray_input_plugin_t *) this_gen;

  if (!this || !this->bdh || !this->title_info)
    return -1;

  /* convert relative seeks to absolute */

  if (origin == SEEK_CUR) {
    time_offset += this_gen->get_current_time(this_gen);
  }
  else if (origin == SEEK_END) {
    int duration = this->title_info->duration / 90;
    if (time_offset < duration)
      time_offset = duration - time_offset;
    else
      time_offset = 0;
  }

  lprintf("bluray_plugin_seek_time() seeking to %d.%03ds\n", time_offset / 1000, time_offset % 1000);

  return bd_seek_time(this->bdh, time_offset * INT64_C(90));
}

static off_t bluray_plugin_get_current_pos (input_plugin_t *this_gen)
{
  bluray_input_plugin_t *this = (bluray_input_plugin_t *) this_gen;

  return this->bdh ? bd_tell(this->bdh) : 0;
}

static int bluray_plugin_get_current_time (input_plugin_t *this_gen)
{
  bluray_input_plugin_t *this = (bluray_input_plugin_t *) this_gen;

  return this->bdh ? (int)(bd_tell_time(this->bdh) / UINT64_C(90)) : -1;
}

static off_t bluray_plugin_get_length (input_plugin_t *this_gen)
{
  bluray_input_plugin_t *this = (bluray_input_plugin_t *) this_gen;

  return this->bdh ? (off_t)bd_get_title_size(this->bdh) : (off_t)-1;
}

static uint32_t bluray_plugin_get_blocksize (input_plugin_t *this_gen)
{
  (void)this_gen;

  return ALIGNED_UNIT_SIZE;
}

static const char* bluray_plugin_get_mrl (input_plugin_t *this_gen)
{
  bluray_input_plugin_t *this = (bluray_input_plugin_t *) this_gen;

  return this->mrl;
}

static int bluray_plugin_get_optional_data (input_plugin_t *this_gen, void *data, int data_type)
{
  bluray_input_plugin_t *this = (bluray_input_plugin_t *) this_gen;

  if (!this || !this->stream || !data)
    return INPUT_OPTIONAL_UNSUPPORTED;

  switch (data_type) {

    case INPUT_OPTIONAL_DATA_DEMUXER:
#ifdef HAVE_CONFIG_H
      *(const char **)data = "mpeg-ts";
#else
      *(const char **)data = "mpeg-ts-hdmv";
#endif
      return INPUT_OPTIONAL_SUCCESS;

    /*
     * audio track language:
     * - channel number can be mpeg-ts PID (0x1100 ... 0x11ff)
     */
    case INPUT_OPTIONAL_DATA_AUDIOLANG:
      if (this->title_info) {
        int               channel = *((int *)data);
        BLURAY_CLIP_INFO *clip    = &this->title_info->clips[this->current_clip];

        if (channel < clip->audio_stream_count) {
          memcpy(data, clip->audio_streams[channel].lang, 4);
          lprintf("INPUT_OPTIONAL_DATA_AUDIOLANG: %02d [pid 0x%04x]: %s\n",
                  channel, clip->audio_streams[channel].pid, clip->audio_streams[channel].lang);

          return INPUT_OPTIONAL_SUCCESS;
        }
        /* search by pid */
        int i;
        for (i = 0; i < clip->audio_stream_count; i++) {
          if (channel == clip->audio_streams[i].pid) {
            memcpy(data, clip->audio_streams[i].lang, 4);
            lprintf("INPUT_OPTIONAL_DATA_AUDIOLANG: pid 0x%04x -> ch %d: %s\n",
                    channel, i, clip->audio_streams[i].lang);

            return INPUT_OPTIONAL_SUCCESS;
          }
        }
      }
      return INPUT_OPTIONAL_UNSUPPORTED;

    /*
     * SPU track language:
     * - channel number can be mpeg-ts PID (0x1200 ... 0x12ff)
     */
    case INPUT_OPTIONAL_DATA_SPULANG:
      if (this->title_info) {
        int               channel = *((int *)data);
        BLURAY_CLIP_INFO *clip    = &this->title_info->clips[this->current_clip];

        if (channel < clip->pg_stream_count) {
          memcpy(data, clip->pg_streams[channel].lang, 4);
          lprintf("INPUT_OPTIONAL_DATA_SPULANG: %02d [pid 0x%04x]: %s\n",
                  channel, clip->pg_streams[channel].pid, clip->pg_streams[channel].lang);

          return INPUT_OPTIONAL_SUCCESS;
        }
        /* search by pid */
        int i;
        for (i = 0; i < clip->pg_stream_count; i++) {
          if (channel == clip->pg_streams[i].pid) {
            memcpy(data, clip->pg_streams[i].lang, 4);
            lprintf("INPUT_OPTIONAL_DATA_SPULANG: pid 0x%04x -> ch %d: %s\n",
                    channel, i, clip->pg_streams[i].lang);

            return INPUT_OPTIONAL_SUCCESS;
          }
        }
      }
      return INPUT_OPTIONAL_UNSUPPORTED;

    default:
      return DEMUX_OPTIONAL_UNSUPPORTED;
    }

  return INPUT_OPTIONAL_UNSUPPORTED;
}

static void bluray_plugin_dispose (input_plugin_t *this_gen)
{
  bluray_input_plugin_t *this = (bluray_input_plugin_t *) this_gen;

  if (this->bdh)
    bd_register_overlay_proc(this->bdh, NULL, NULL);

  close_overlay(this);

  if (this->event_queue)
    xine_event_dispose_queue(this->event_queue);

  if (this->title_info)
    bd_free_title_info(this->title_info);

  if (this->bdh)
    bd_close(this->bdh);

  free (this->mrl);
  free (this->disc_root);
  free (this->disc_name);

  free (this);
}

static int parse_mrl(const char *mrl_in, char **path, int *title, int *chapter)
{
  int skip = 0;

  if (!strncasecmp(mrl_in, "bluray:", 7))
    skip = 7;
  else if (!strncasecmp(mrl_in, "bd:", 3))
    skip = 3;
  else
    return -1;

  char *mrl = strdup(mrl_in + skip);

  /* title[.chapter] given ? parse and drop it */
  if (mrl[strlen(mrl)-1] != '/') {
    char *end = strrchr(mrl, '/');
    if (end && end[1]) {
      if (sscanf(end, "/%d.%d", title, chapter) < 1)
        *title = -1;
      else
        *end = 0;
    }
  }
  lprintf(" -> title %d, chapter %d, mrl \'%s\'\n", *title, *chapter, mrl);

  if ((mrl[0] == 0) ||
      (mrl[1] == 0 && mrl[0] == '/') ||
      (mrl[2] == 0 && mrl[1] == '/' && mrl[0] == '/') ||
      (mrl[3] == 0 && mrl[2] == '/' && mrl[1] == '/' && mrl[0] == '/')){

    /* default device */
    *path = NULL;

  } else if (*mrl == '/') {

    /* strip extra slashes */
    char *start = mrl;
    while (start[0] == '/' && start[1] == '/')
      start++;

    *path = strdup(start);

    _x_mrl_unescape(*path);

    lprintf("non-defaut mount point \'%s\'\n", *path);

  } else {
    lprintf("invalid mrl \'%s\'\n", mrl_in);
    free(mrl);
    return 0;
  }

  free(mrl);

  return 1;
}

static int get_disc_info(bluray_input_plugin_t *this)
{
  const BLURAY_DISC_INFO *disc_info;

  disc_info = bd_get_disc_info(this->bdh);

  if (!disc_info) {
    LOGMSG("bd_get_disc_info() failed\n");
    return -1;
  }

  if (!disc_info->bluray_detected) {
    LOGMSG("bd_get_disc_info(): BluRay not detected\n");
    this->nav_mode = 0;
    return 0;
  }

  if (disc_info->aacs_detected && !disc_info->aacs_handled) {
    if (!disc_info->libaacs_detected)
      _x_message (this->stream, XINE_MSG_ENCRYPTED_SOURCE,
                  "Media stream scrambled/encrypted with AACS",
                  "libaacs not installed", NULL);
    else
      _x_message (this->stream, XINE_MSG_ENCRYPTED_SOURCE,
                  "Media stream scrambled/encrypted with AACS", NULL);
    return -1;
  }

  if (disc_info->bdplus_detected && !disc_info->bdplus_handled) {
    if (!disc_info->libbdplus_detected)
      _x_message (this->stream, XINE_MSG_ENCRYPTED_SOURCE,
                  "Media scrambled/encrypted with BD+",
                  "libbdplus not installed.", NULL);
    else
      _x_message (this->stream, XINE_MSG_ENCRYPTED_SOURCE,
                  "Media stream scrambled/encrypted with BD+", NULL);
    return -1;
  }

  if (this->nav_mode && !disc_info->first_play_supported) {
    _x_message (this->stream, XINE_MSG_GENERAL_WARNING,
                "Can't play disc in HDMV navigation mode",
                "First Play title not supported", NULL);
    this->nav_mode = 0;
  }

  if (this->nav_mode && disc_info->num_unsupported_titles > 0) {
    _x_message (this->stream, XINE_MSG_GENERAL_WARNING,
                "Unsupported titles found",
                "Some titles can't be played in navigation mode", NULL);
  }

  this->num_titles = disc_info->num_hdmv_titles + disc_info->num_bdj_titles;
  this->disc_info  = disc_info;

  return 1;
}

static int bluray_plugin_open (input_plugin_t *this_gen)
{
  bluray_input_plugin_t *this    = (bluray_input_plugin_t *) this_gen;
  int                    title   = -1;
  int                    chapter = 0;

  lprintf("bluray_plugin_open\n");

  /* validate and parse mrl */
  if (!parse_mrl(this->mrl, &this->disc_root, &title, &chapter))
    return -1;

  if (!strncasecmp(this->mrl, "bd:", 3))
    this->nav_mode = 1;

  if (!this->disc_root)
    this->disc_root = strdup(this->class->mountpoint);

  /* open libbluray */

  if (! (this->bdh = bd_open (this->disc_root, NULL))) {
    LOGMSG("bd_open(\'%s\') failed: %s\n", this->disc_root, strerror(errno));
    return -1;
  }
  lprintf("bd_open(\'%s\') OK\n", this->disc_root);

  if (get_disc_info(this) < 0) {
    return -1;
  }

  /* load title list */

  this->num_title_idx = bd_get_titles(this->bdh, TITLES_RELEVANT);
  LOGMSG("%d titles\n", this->num_title_idx);

  if (this->num_title_idx < 1)
    return -1;

  /* select title */

  /* if title was not in mrl, find the main title */
  if (title < 0) {
    uint64_t duration = 0;
    int i, playlist = 99999;
    for (i = 0; i < this->num_title_idx; i++) {
      BLURAY_TITLE_INFO *info = bd_get_title_info(this->bdh, i);
      if (info->duration > duration) {
        title    = i;
        duration = info->duration;
        playlist = info->playlist;
      }
      bd_free_title_info(info);
    }
    lprintf("main title: %d (%05d.mpls)\n", title, playlist);
  }

  /* update player settings */

  bd_set_player_setting    (this->bdh, BLURAY_PLAYER_SETTING_REGION_CODE,  this->class->region);
  bd_set_player_setting    (this->bdh, BLURAY_PLAYER_SETTING_PARENTAL,     this->class->parental);
  bd_set_player_setting_str(this->bdh, BLURAY_PLAYER_SETTING_AUDIO_LANG,   this->class->language);
  bd_set_player_setting_str(this->bdh, BLURAY_PLAYER_SETTING_PG_LANG,      this->class->language);
  bd_set_player_setting_str(this->bdh, BLURAY_PLAYER_SETTING_MENU_LANG,    this->class->language);
  bd_set_player_setting_str(this->bdh, BLURAY_PLAYER_SETTING_COUNTRY_CODE, this->class->country);

  /* get disc name */

  this->meta_dl = bd_get_meta(this->bdh);

  if (this->meta_dl && this->meta_dl->di_name && strlen(this->meta_dl->di_name) > 1) {
    this->disc_name = strdup(this->meta_dl->di_name);
  }
  else if (strcmp(this->disc_root, this->class->mountpoint)) {
    char *t = strrchr(this->disc_root, '/');
    if (!t[1])
      while (t > this->disc_root && t[-1] != '/') t--;
    else
      while (t[0] == '/') t++;
    this->disc_name = strdup(t);
    char *end = this->disc_name + strlen(this->disc_name) - 1;
    if (*end ==  '/')
      *end = 0;
  }

  /* register overlay (graphics) handler */

  bd_register_overlay_proc(this->bdh, this, overlay_proc);

  /* open */
  this->current_title = -1;
  this->current_title_idx = -1;

  if (this->nav_mode) {
    if (bd_play(this->bdh) <= 0) {
      LOGMSG("bd_play() failed\n");
      return -1;
    }

  } else {
    if (open_title(this, title) <= 0 &&
        open_title(this, 0) <= 0)
      return -1;
  }

  /* jump to chapter */

  if (chapter > 0) {
    chapter = MAX(0, MIN((int)this->title_info->chapter_count, chapter) - 1);
    bd_seek_chapter(this->bdh, chapter);
    _x_stream_info_set(this->stream, XINE_STREAM_INFO_DVD_CHAPTER_NUMBER, chapter + 1);
  }

  return 1;
}

static input_plugin_t *bluray_class_get_instance (input_class_t *cls_gen, xine_stream_t *stream,
                                                  const char *mrl)
{
  bluray_input_plugin_t *this;

  lprintf("bluray_class_get_instance\n");

  if (strncasecmp(mrl, "bluray:", 7) && strncasecmp(mrl, "bd:", 3))
    return NULL;

  this = (bluray_input_plugin_t *) calloc(1, sizeof (bluray_input_plugin_t));

  this->stream = stream;
  this->class  = (bluray_input_class_t*)cls_gen;
  this->mrl    = strdup(mrl);

  this->cap_seekable = INPUT_CAP_SEEKABLE;

  this->input_plugin.open               = bluray_plugin_open;
  this->input_plugin.get_capabilities   = bluray_plugin_get_capabilities;
  this->input_plugin.read               = bluray_plugin_read;
  this->input_plugin.read_block         = bluray_plugin_read_block;
  this->input_plugin.seek               = bluray_plugin_seek;
  this->input_plugin.seek_time          = bluray_plugin_seek_time;
  this->input_plugin.get_current_pos    = bluray_plugin_get_current_pos;
  this->input_plugin.get_current_time   = bluray_plugin_get_current_time;
  this->input_plugin.get_length         = bluray_plugin_get_length;
  this->input_plugin.get_blocksize      = bluray_plugin_get_blocksize;
  this->input_plugin.get_mrl            = bluray_plugin_get_mrl;
  this->input_plugin.get_optional_data  = bluray_plugin_get_optional_data;
  this->input_plugin.dispose            = bluray_plugin_dispose;
  this->input_plugin.input_class        = cls_gen;

  this->event_queue = xine_event_new_queue (this->stream);

  return &this->input_plugin;
}

/*
 * plugin class
 */

static void mountpoint_change_cb(void *data, xine_cfg_entry_t *cfg)
{
  bluray_input_class_t *this = (bluray_input_class_t *) data;

  this->mountpoint = cfg->str_value;
}

static void device_change_cb(void *data, xine_cfg_entry_t *cfg)
{
  bluray_input_class_t *this = (bluray_input_class_t *) data;

  this->device = cfg->str_value;
}

static void language_change_cb(void *data, xine_cfg_entry_t *cfg)
{
  bluray_input_class_t *this = (bluray_input_class_t *) data;

  this->language = cfg->str_value;
}

static void country_change_cb(void *data, xine_cfg_entry_t *cfg)
{
  bluray_input_class_t *this = (bluray_input_class_t *) data;

  this->country = cfg->str_value;
}

static void region_change_cb(void *data, xine_cfg_entry_t *cfg)
{
  bluray_input_class_t *this = (bluray_input_class_t *) data;

  this->region = cfg->num_value;
}

static void parental_change_cb(void *data, xine_cfg_entry_t *cfg)
{
  bluray_input_class_t *this = (bluray_input_class_t *) data;

  this->parental = cfg->num_value;
}

static void free_xine_playlist(bluray_input_class_t *this)
{
  if (this->xine_playlist) {
    int i;
    for (i = 0; i < this->xine_playlist_size; i++) {
      MRL_ZERO(this->xine_playlist[i]);
      free(this->xine_playlist[i]);
    }
    free(this->xine_playlist);
    this->xine_playlist = NULL;
  }

  this->xine_playlist_size = 0;
}

#if INPUT_PLUGIN_IFACE_VERSION < 18
static const char *bluray_class_get_description (input_class_t *this_gen)
{
  (void)this_gen;

  return _("BluRay input plugin");
}
#endif

#if INPUT_PLUGIN_IFACE_VERSION < 18
static const char *bluray_class_get_identifier (input_class_t *this_gen)
{
  (void)this_gen;

  return "bluray";
}
#endif

static char **bluray_class_get_autoplay_list (input_class_t *this_gen, int *num_files)
{
  (void)this_gen;

  static char *autoplay_list[] = { "bluray:/", NULL };

  *num_files = 1;

  return autoplay_list;
}

xine_mrl_t **bluray_class_get_dir(input_class_t *this_gen, const char *filename, int *nFiles)
{
  bluray_input_class_t *this = (bluray_input_class_t*) this_gen;
  char *path = NULL;
  int title = -1, chapter = -1, i, num_pl;

  lprintf("bluray_class_get_dir(%s)\n", filename);

  free_xine_playlist(this);

  if (filename)
    parse_mrl(filename, &path, &title, &chapter);

  BLURAY *bdh    = bd_open(path?:this->mountpoint, NULL);

  if (bdh) {
    num_pl = bd_get_titles(bdh, TITLES_RELEVANT);

    if (num_pl > 0) {

      this->xine_playlist_size = num_pl;
      this->xine_playlist      = calloc(this->xine_playlist_size + 1, sizeof(xine_mrl_t*));

      for (i = 0; i < num_pl; i++) {
        //BLURAY_TITLE_INFO *info = bd_get_title_info(bd, title);

        this->xine_playlist[i] = calloc(1, sizeof(xine_mrl_t));

        if (asprintf(&this->xine_playlist[i]->origin, "bluray:/%s", path?:"") < 0)
          this->xine_playlist[i]->origin = NULL;
        if (asprintf(&this->xine_playlist[i]->mrl,    "bluray:/%s/%d", path?:"", i) < 0)
          this->xine_playlist[i]->mrl = NULL;
        this->xine_playlist[i]->type = mrl_dvd;
        //this->xine_playlist[i]->size = info->duration;

        //bd_free_title_info(info);
      }
    }
  }

  bd_close(bdh);

  free(path);

  if (nFiles)
    *nFiles = this->xine_playlist_size;

  return this->xine_playlist;
}

static int bluray_class_eject_media (input_class_t *this_gen)
{
  (void)this_gen;
#if 0
  bluray_input_class_t *this = (bluray_input_class_t*) this_gen;

  return media_eject_media (this->xine, this->device);
#endif
  return 1;
}

static void bluray_class_dispose (input_class_t *this_gen)
{
  bluray_input_class_t *this   = (bluray_input_class_t *) this_gen;
  config_values_t      *config = this->xine->config;

  free_xine_playlist(this);

  config->unregister_callback(config, "media.bluray.mountpoint");
  config->unregister_callback(config, "media.bluray.device");
  config->unregister_callback(config, "media.bluray.region");
  config->unregister_callback(config, "media.bluray.language");
  config->unregister_callback(config, "media.bluray.country");
  config->unregister_callback(config, "media.bluray.parental");

  free (this);
}

static void *bluray_init_plugin (xine_t *xine, void *data)
{
  (void)data;

  config_values_t      *config = xine->config;
  bluray_input_class_t *this   = (bluray_input_class_t *) calloc(1, sizeof (bluray_input_class_t));

  this->xine = xine;

  this->input_class.get_instance       = bluray_class_get_instance;
#if INPUT_PLUGIN_IFACE_VERSION < 18
  this->input_class.get_identifier     = bluray_class_get_identifier;
  this->input_class.get_description    = bluray_class_get_description;
#else
  this->input_class.identifier         = "bluray";
  this->input_class.description        = _("BluRay input plugin");
#endif
  this->input_class.get_dir            = bluray_class_get_dir;
  this->input_class.get_autoplay_list  = bluray_class_get_autoplay_list;
  this->input_class.dispose            = bluray_class_dispose;
  this->input_class.eject_media        = bluray_class_eject_media;

  this->mountpoint = config->register_filename(config, "media.bluray.mountpoint",
                                               "/mnt/bluray", XINE_CONFIG_STRING_IS_DIRECTORY_NAME,
                                               _("BluRay mount point"),
                                               _("Default mount location for BluRay discs."),
                                               0, mountpoint_change_cb, (void *) this);
  this->device = config->register_filename(config, "media.bluray.device",
                                           "/dev/dvd", XINE_CONFIG_STRING_IS_DIRECTORY_NAME,
                                           _("device used for BluRay playback"),
                                           _("The path to the device "
                                             "which you intend to use for playing BluRy discs."),
                                           0, device_change_cb, (void *) this);

  /* Player settings */
  this->language =
    config->register_string(config, "media.bluray.language",
                            "eng",
                            _("default language for BluRay playback"),
                            _("xine tries to use this language as a default for BluRay playback. "
                              "As far as the BluRay supports it, menus and audio tracks will be presented "
                              "in this language.\nThe value must be a three character"
                              "ISO639-2 language code."),
                            0, language_change_cb, this);
  this->country =
    config->register_string(config, "media.bluray.country",
                            "en",
                            _("BluRay player country code"),
                            _("The value must be a two character ISO3166-1 country code."),
                            0, country_change_cb, this);
  this->region =
    config->register_num(config, "media.bluray.region",
                         7,
                         _("BluRay player region code (1=A, 2=B, 4=C)"),
                         _("This only needs to be changed if your BluRay jumps to a screen "
                           "complaining about a wrong region code. It has nothing to do with "
                           "the region code set in BluRay drives, this is purely software."),
                         0, region_change_cb, this);
  this->parental =
    config->register_num(config, "media.bluray.parental",
                         99,
                         _("parental control age limit (1-99)"),
                         _("Prevents playback of BluRay titles where parental "
                           "control age limit is higher than this limit"),
                         0, parental_change_cb, this);

  return this;
}

/*
 * exported plugin catalog entry
 */

const plugin_info_t xine_plugin_info[] EXPORTED = {
  /* type, API, "name", version, special_info, init_function */
#if INPUT_PLUGIN_IFACE_VERSION <= 17
  { PLUGIN_INPUT | PLUGIN_MUST_PRELOAD, 17, "BLURAY", XINE_VERSION_CODE, NULL, bluray_init_plugin },
  { PLUGIN_INPUT | PLUGIN_MUST_PRELOAD, 17, "BD",     XINE_VERSION_CODE, NULL, bluray_init_plugin },
#elif INPUT_PLUGIN_IFACE_VERSION >= 18
  { PLUGIN_INPUT | PLUGIN_MUST_PRELOAD, 18, "BLURAY", XINE_VERSION_CODE, NULL, bluray_init_plugin },
  { PLUGIN_INPUT | PLUGIN_MUST_PRELOAD, 18, "BD",     XINE_VERSION_CODE, NULL, bluray_init_plugin },
#endif
  { PLUGIN_NONE, 0, "", 0, NULL, NULL }
};