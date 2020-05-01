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

#include <libpng16/png.h>

//#include <GL/gl.h>
//#include <GL/glut.h>
//#include <GL/glx.h>
//#include <GL/glxext.h>

#include <gst/gst.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <X11/Xutil.h>

#include <vdr/config.h>
#include <vdr/osd.h>

static int Xscreen;
static Atom del_atom;
static Display *Xdisplay;
static Colormap cmap;
static XVisualInfo *osd_visual;
static XRenderPictFormat *pict_format;
static int numfbconfigs;
static int osd_width, osd_height;
static GC osd_gc;


class cOsdgst : public cOsd {
private:

    void write_png_for_image(XImage *image, int width, int height, char *filename);

	GstElement *m_overlay;
    
public:

    Window Xroot, window_handle;

    cOsdgst(int Left, int Top, uint Level);

    ~cOsdgst();

    void *Init(GstElement *overlay);

    void *CreateWindow(Display *dpy);

    int isExtensionSupported(const char * extList, const char *extension);

    void fatalError(const char *why);

    void Debug(const char *why);

    //void describe_fbconfig(GLXFBConfig fbconfig);

    void FlushOsd(cPixmapMemory *pm);

    cPixmap *CreatePixmap(int Layer, const cRect &ViewPort, const cRect &DrawPort);

}; // end of class
