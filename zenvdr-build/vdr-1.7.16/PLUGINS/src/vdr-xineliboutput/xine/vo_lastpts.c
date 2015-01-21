/*
 * vo_lastpts.c:
 *
 * See the main source file 'xineliboutput.c' for copyright information and
 * how to reach the author.
 *
 * $Id: vo_lastpts.c,v 1.3 2011/02/13 14:39:41 phintuka Exp $
 *
 */

#include <stdlib.h>

#include <xine/xine_internal.h>
#include <xine/video_out.h>
#include <xine/metronom.h>

#include "xvdr_metronom.h"

#include "vo_hook.h"

/*
 *  lastpts_hook_t
 */
typedef struct {
  vo_driver_hook_t  h;
  xine_stream_t    *prev_stream;
  metronom_t       *xvdr_metronom;
} lastpts_hook_t;

/*
 * interface
 */

/*
 * override display_frame()
 */

static void lastpts_display_frame(vo_driver_t *self, vo_frame_t *vo_img)
{
  lastpts_hook_t *this = (lastpts_hook_t*)self;

#if 0
  if (vo_img->stream) {
    vo_img->stream->metronom->set_option(vo_img->stream->metronom, XVDR_METRONOM_LAST_VO_PTS, vo_img->pts);
  }
#else

  /* detect intercepted metronom with XVDR_* option support.
   * This prevents flooding log with "unknown option in set_option" messages
   */
  if (vo_img->stream != this->prev_stream && vo_img->stream) {
    this->prev_stream = vo_img->stream;
    if (vo_img->stream->metronom->get_option(vo_img->stream->metronom, XVDR_METRONOM_ID) == XVDR_METRONOM_ID) {
      this->xvdr_metronom = vo_img->stream->metronom;
    }
  }

  if (this->xvdr_metronom) {
    this->xvdr_metronom->set_option(this->xvdr_metronom, XVDR_METRONOM_LAST_VO_PTS, vo_img->pts);
  }

#endif

  this->h.orig_driver->display_frame(this->h.orig_driver, vo_img);
}


/*
 * init()
 */
vo_driver_t *vo_lastpts_init(void)
{
  lastpts_hook_t *this = calloc(1, sizeof(lastpts_hook_t));

  this->h.vo.display_frame = lastpts_display_frame;

  return &this->h.vo;
}

