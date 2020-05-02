#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>


#include <vdr/config.h>
#include <vdr/device.h>
#include <vdr/osd.h>
#include <vdr/plugin.h>
#include <vdr/args.h>

#include <gst/gst.h>
#include <gio/gio.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/video.h>
#include <gst/video/videooverlay.h>
#include <gst/video/video-overlay-composition.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrender.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>

//#include <GL/gl.h>
//#include <GL/glx.h>

#include "osdgst.h"


#include <string>         // std::string

//#include "mcheck.h"


/* Datastructure to share the state we are interested in between
 * prepare and render function. */

typedef struct
{
  gboolean valid;
  GstVideoInfo vinfo;
} CairoOverlayState;




static XVisualInfo vinfo;
static Visual *visual;
static Display *dpy = NULL;
static GstElement *appsrc = NULL;
static GstElement *overlay = NULL;

static GstBus *pipebus = NULL;
static Display *pipedpy = NULL;
static Window pipewin = 0;
static GdkPixbuf *logo_pixbuf;

    
    

static GstBus *bus = NULL;
static GError *error = NULL;
static gchar *uri = NULL;
static  gchar *local_uri =NULL;
static int blackColor;
static int whiteColor;

static Window win = 0;
static Window root = 0;
static GC gc;

// Global Defines
static bool live_stream_is_runnig = FALSE;
static int ilive_stream_count = 0;

#define CHUNK_SIZE 4096

#define LOCAL_DEBUG 1



/* playbin flags */
typedef enum {
    GST_PLAY_FLAG_VIDEO = (1 << 0), /* We want to play vidoe output */
    GST_PLAY_FLAG_AUDIO = (1 << 1), /* We want to play audio ourput */
    GST_PLAY_FLAG_TEXT = (1 << 2) /* We want subtitle output */
} GstPlayFlags;

#ifdef __cplusplus

extern "C"

{

#endif


 /// C callback feed key press

    extern void FeedKeyPress(const char *, const char *, int, int,

	const char *);


#ifdef __cplusplus

}

#endif



class cGstreamerDevice : cDevice {
public:

    cGstreamerDevice();

    ~cGstreamerDevice();

    void Init();

    bool HasDecoder(void) const;

    bool CanReplay(void) const;

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
    
    void ReplayPlayFile(char* Filename);

    void SetVolumeDevice(int Volume);

protected:

    void MakePrimaryDevice(bool On);

private:    

    void ShowOverlay();

    Window window_handle;
    
    GstElement *mVdrSrc;

    GstElement *pipeline; 

     GstElement *video_converter;

     GstElement *video_sink;

     GstElement *audio_sink;

     GstElement *video_decodebin;

     GstElement *audio_decodebin;

     GstElement *audioqueue;

     GstElement *videoqueue;

     GstElement *audio_convertor;

     Display *keyboard_dpy;

     void Action(void);

     void open_display(const char *display_name );

};

