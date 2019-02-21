
//////////////////////////////////////////////////////////////////////////////
///                                                                        ///
/// This file is part of the VDR mpv plugin and licenced under AGPLv3      ///
///                                                                        ///
/// See the README file for copyright information                          ///
///                                                                        ///
//////////////////////////////////////////////////////////////////////////////

#ifndef __GST_SERVICE_H
#define __GST_SERVICE_H

#define GST_START_PLAY_SERVICE "Gst-StartPlayService_v1_0"
#define GST_SET_TITLE_SERVICE "Gst-SetTitleService_v1_0"

// Deprecated will be removed in a future release, please use Mpv_Playfile instead
typedef struct
{
  char* Filename;
  char* Title;
} Gst_StartPlayService_v1_0_t;

// Deprecated will be removed in a future release, please use Mpv_SetTitle instead
typedef struct
{
  char* Title;
} Gst_SetTitleService_v1_0_t;

// play the given Filename, this can be a media file or a playlist
typedef struct
{
  char *Filename;
} Gst_PlayFile;

// start the given playlist in shuffle mode
typedef struct
{
  char *Filename;
} Gst_PlaylistShuffle;

// Overrides the displayed title in replay info
typedef struct
{
  char *Title;
}Gst_SetTitle;

typedef struct
{
  int SeekAbsolute;
  int SeekRelative;
} Gst_Seek;

#endif
