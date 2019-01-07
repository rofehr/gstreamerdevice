/*
 * osdgst.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <time.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

#include <gst/gst.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xutil.h>

#include <vdr/config.h>
#include <vdr/osd.h>

static int Xscreen;
static Atom del_atom;
static Display *Xdisplay;
static Colormap cmap;
static XVisualInfo *osd_visual;
static XRenderPictFormat *pict_format;
static GLXFBConfig *fbconfigs, fbconfig;
static int numfbconfigs;
static GLXContext render_context;
static GLXWindow glX_window_handle;
static int osd_width, osd_height;
static GC osd_gc;

static int VisData[] = {
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


class cOsdgst : public cOsd {
private:

public:
    
    Window Xroot, window_handle;
 
	cOsdgst(int Left, int Top, uint Level); // end of method

	~cOsdgst();

	void *CreateWindow();

	int isExtensionSupported(const char * extList, const char *extension);

	void fatalError(const char *why);

 	void Debug(const char *why);

	void describe_fbconfig(GLXFBConfig fbconfig);
    
    void FlushOsd(cPixmapMemory *pm);
    
    cPixmap *CreatePixmap(int Layer, const cRect &ViewPort, const cRect &DrawPort);

}; // end of class
