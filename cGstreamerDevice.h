#include <vdr/config.h>
#include <vdr/osd.h>
#include <vdr/device.h>


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <time.h>

#include <gst/gst.h>
#include <gio/gio.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/videooverlay.h>
#include <gst/video/video-overlay-composition.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrender.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include "cGstreamerOsd.h"

static GstElement *appsrc = NULL;
static GstBus *bus = NULL;
static GError *error = NULL;
static gchar *uri = NULL;
static gchar *local_uri =NULL;

static bool live_stream_is_runnig = FALSE;
static int ilive_stream_count = 0;


#ifdef LOCAL_DEBUG
// #define TEMP_PATH "/home/roland/data/dummy.ts"
#define TEMP_PATH "/var/cache/dummy.ts"
#else
#define TEMP_PATH "/var/cache/dummy.ts"
#endif


class cGstreamerDevice : cDevice {
public:

	cGstreamerDevice();

	~cGstreamerDevice();

	bool HasDecoder(void) const;

	bool SetPlayMode(ePlayMode PlayMode);

	int PlayVideo(const uchar *Data, int Length);

	int PlayAudio(const uchar *Data, int Length, uchar Id);

	int PlayTsVideo(const uchar *Data, int Length);

	int PlayTsAudio(const uchar *Data, int Length);

	int PlayTsSubtitle(const uchar *Data, int Length);

	int PlayPes(const uchar *Data, int Length, bool VideoOnly = false);

	int push_to_buffer(const uchar *Data, int Length);

	int PlayTs(const uchar *Data, int Length, bool VideoOnly = false);

	bool Poll(cPoller &Poller, int TimeoutMs = 0);

	bool Flush(int TimeoutMs = 0);

	bool Start(void);

protected:

	void MakePrimaryDevice(bool On);

	void StartReplayBuffer();

	void ShowOverlay();

	void StartReplay();
};
