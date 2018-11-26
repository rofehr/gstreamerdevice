#include "osdgst.h"

static Bool WaitForMapNotify(Display *d, XEvent *e, char *arg)
{
	return d && e && arg && (e->type == MapNotify) && (e->xmap.window == *(Window*)arg);
}; // end of method


/*
 * Constructor
 */
cOsdgst::cOsdgst()
{

}; // end of method

/*
 * Deconstruktor
 */
cOsdgst::~cOsdgst()
{

}; // end of method

/*
 * CreateWindow
 */
void *cOsdgst::CreateWindow()
{
	XEvent event;
	int x,y, attr_mask;
	XSizeHints hints;
	XWMHints *startup_state;
	XTextProperty textprop;
	XSetWindowAttributes attr = {0,};
	static char *title = "FTB's little OpenGL example - ARGB extension by WXD";

	Xdisplay = XOpenDisplay(NULL);
	if (!Xdisplay) {
		fatalError("Couldn't connect to X server\n");
	}
	Xscreen = DefaultScreen(Xdisplay);
	Xroot = RootWindow(Xdisplay, Xscreen);

	fbconfigs = glXChooseFBConfig(Xdisplay, Xscreen, VisData, &numfbconfigs);
	fbconfig = 0;
	for(int i = 0; i<numfbconfigs; i++) {
		osd_visual = (XVisualInfo*) glXGetVisualFromFBConfig(Xdisplay, fbconfigs[i]);
		if(!osd_visual)
			continue;

		pict_format = XRenderFindVisualFormat(Xdisplay, osd_visual->visual);
		if(!pict_format)
			continue;

		fbconfig = fbconfigs[i];
		if(pict_format->direct.alphaMask > 0) {
			break;
		}
	}

	if(!fbconfig) {
		fatalError("No matching FB config found");
	}

	describe_fbconfig(fbconfig);

	/* Create a colormap - only needed on some X clients, eg. IRIX */
	cmap = XCreateColormap(Xdisplay, Xroot, osd_visual->visual, AllocNone);

	attr.colormap = cmap;
	attr.background_pixmap = None;
	attr.border_pixmap = None;
	attr.border_pixel = 0;
	attr.event_mask =
			StructureNotifyMask |
			EnterWindowMask |
			LeaveWindowMask |
			ExposureMask |
			ButtonPressMask |
			ButtonReleaseMask |
			OwnerGrabButtonMask |
			KeyPressMask |
			KeyReleaseMask;

	attr_mask = 
			//	CWBackPixmap|
			CWColormap|
			CWBorderPixel|
			CWEventMask;

	width = DisplayWidth(Xdisplay, DefaultScreen(Xdisplay))/2;
	height = DisplayHeight(Xdisplay, DefaultScreen(Xdisplay))/2;
	x=width/2, y=height/2;

	window_handle = XCreateWindow(	Xdisplay,
			Xroot,
			x, y, width, height,
			0,
			osd_visual->depth,
			InputOutput,
			osd_visual->visual,
			attr_mask, &attr);

	if( !window_handle ) {
		fatalError("Couldn't create the window\n");
	}

#if USE_GLX_CREATE_WINDOW
	fputs("glXCreateWindow ", stderr);
	int glXattr[] = { None };
	glX_window_handle = glXCreateWindow(Xdisplay, fbconfig, window_handle, glXattr);
	if( !glX_window_handle ) {
		fatalError("Couldn't create the GLX window\n");
	}
#else
	glX_window_handle = window_handle;
#endif

	textprop.value = (unsigned char*)title;
	textprop.encoding = XA_STRING;
	textprop.format = 8;
	textprop.nitems = strlen(title);

	hints.x = x;
	hints.y = y;
	hints.width = width;
	hints.height = height;
	hints.flags = USPosition|USSize;

	startup_state = XAllocWMHints();
	startup_state->initial_state = NormalState;
	startup_state->flags = StateHint;

	XSetWMProperties(Xdisplay, window_handle,&textprop, &textprop,
			NULL, 0,
			&hints,
			startup_state,
			NULL);

	XFree(startup_state);

	XMapWindow(Xdisplay, window_handle);
	XIfEvent(Xdisplay, &event, WaitForMapNotify, (char*)&window_handle);

	if ((del_atom = XInternAtom(Xdisplay, "WM_DELETE_WINDOW", 0)) != None) {
		XSetWMProtocols(Xdisplay, window_handle, &del_atom, 1);
	}
}; // end of method

/*
 * isExtensiionSupported
 */
int cOsdgst::isExtensionSupported(const char * extList, const char *extension)
{
	const char *start;
	const char *where, *terminator;

	/* Extension names should not have spaces. */
	where = strchr(extension, ' ');
	if ( where || *extension == '\0' )
		return 0;

	/* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string. Don't be fooled by sub-strings,
     etc. */
	for ( start = extList; ; ) {
		where = strstr( start, extension );

		if ( !where )
			break;

		terminator = where + strlen( extension );

		if ( where == start || *(where - 1) == ' ' )
			if ( *terminator == ' ' || *terminator == '\0' )
				return 1;

		start = terminator;
	}
	return 0;
}; // end of method

/*
 * fatalError
 */
void cOsdgst::fatalError(const char *why)
{
	fprintf(stderr, "%s",why);
	exit(0x666);
}; // end of method


void cOsdgst::describe_fbconfig(GLXFBConfig fbconfig)
{
	int doublebuffer;
	int red_bits, green_bits, blue_bits, alpha_bits, depth_bits;

	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_DOUBLEBUFFER, &doublebuffer);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_RED_SIZE, &red_bits);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_GREEN_SIZE, &green_bits);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_BLUE_SIZE, &blue_bits);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_ALPHA_SIZE, &alpha_bits);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_DEPTH_SIZE, &depth_bits);

	fprintf(stderr, "FBConfig selected:\n"
			"Doublebuffer: %s\n"
			"Red Bits: %d, Green Bits: %d, Blue Bits: %d, Alpha Bits: %d, Depth Bits: %d\n",
			doublebuffer == True ? "Yes" : "No", 
					red_bits, green_bits, blue_bits, alpha_bits, depth_bits);
}; // end of method