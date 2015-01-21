/*
 * media_player.h: Media and image players
 *
 * See the main source file 'xineliboutput.c' for copyright information and
 * how to reach the author.
 *
 * $Id: media_player.h,v 1.23 2010/12/19 14:46:24 phintuka Exp $
 *
 */

#ifndef __XINELIB_PLAYER_H
#define __XINELIB_PLAYER_H

class cPlaylist;

class cPlayerFactory
{
  public:

    // interact with current player

    static bool IsOpen(void);
    static void Queue (const char *Mrl);

    // launch new media player

    static bool Launch(const char *Mrl, const char *SubFile = NULL) { return Launch(pmNone, Mrl, SubFile); };

    static bool Launch(ePlayMode PlayMode, const char *Mrl, const char *SubFile = NULL, bool BackToMenu = false);
    static bool Launch(ePlayMode PlayMode, cPlaylist *Playlist, bool BackToMenu = false);
};

#endif // __XINELIB_PLAYER_H
