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

#include <vdr/config.h>
#include <vdr/osd.h>

#include "osdgst.h"

static XVisualInfo vinfo;
static Visual *visual;
static Display *dpy = NULL;
static int blackColor;
static int whiteColor;

static Window win = 0;
static Window glX_win = 0;
static GC gc;


static int VisDataMain[] = {
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_DOUBLEBUFFER, True,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_ALPHA_SIZE, 8,
		GLX_DEPTH_SIZE, 16,
		None
};


class cGstreamerOsd : public cOsd {

public:
	cOsdgst *Osdgst;

	void SetActive(bool on);

	cGstreamerOsd(int Left, int Top, uint Level);

	~cGstreamerOsd();

	eOsdError SetAreas(const tArea *Areas, int NumAreas);

	void Flush(void);

	cPixmap *CreatePixmap(int Layer, const cRect &ViewPort, const cRect &DrawPort);

	void *CreateMainWindow();

	void describe_fbconfig(GLXFBConfig fbconfig);

	static Bool WaitForMapNotify(Display *d, XEvent *e, char *arg);

};// end of class
