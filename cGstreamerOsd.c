#include "cGstreamerOsd.h"

/*
 * WaitForNotify
 */
static Bool WaitForMapNotify(Display *d, XEvent *e, char *arg)
{
    return d && e && arg && (e->type == MapNotify) && (e->xmap.window == *(Window*)arg);
};// end of method


/*
 * SetActive
 */
void cGstreamerOsd::SetActive(bool on)
{
	cOsd::SetActive(on);
};// end of method

/*
 * Construktor
 */
cGstreamerOsd::cGstreamerOsd(int Left, int Top, uint Level) : cOsd(Left, Top, Level)
{
	Osdgst = new cOsdgst(Left,  Top,  Level);

	if( dpy != NULL)
	{
		blackColor = BlackPixel(dpy, DefaultScreen(dpy));whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

		if ( win == 0)
		{

			XSetWindowAttributes attr;
			XVisualInfo *visual_list;
			XVisualInfo visual_template;
			int nxvisuals;
			int i;


			nxvisuals = 0;
			visual_template.screen = DefaultScreen(dpy);
			visual_list = XGetVisualInfo (dpy, VisualScreenMask, &visual_template, &nxvisuals);

			if (!XMatchVisualInfo(dpy, XDefaultScreen(dpy), 24, TrueColor, &vinfo))
			{
				fprintf(stderr, "no such visual\n");
			}

			visual = vinfo.visual;

			attr.background_pixel = 0;
			attr.colormap = XCreateColormap(dpy, XDefaultRootWindow(dpy), visual, AllocNone);
			attr.border_pixel = 0;

			win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0 ,0, 1280, 720, 0, 24, InputOutput, visual, CWBackPixel | CWColormap | CWBorderPixel, &attr);
			gc = XCreateGC(dpy, win, 0, NULL);

			XMapWindow(dpy, win);
			XSync(dpy, false);
			XFlush(dpy);


			Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", true);
			Atom wm_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", true);
			XEvent xev;
			memset(&xev, 0, sizeof(xev));
			xev.type = ClientMessage;
			xev.xclient.window = win;
			xev.xclient.message_type = wm_state;
			xev.xclient.format = 32;
			xev.xclient.data.l[0] = 1;
			xev.xclient.data.l[1] = wm_fullscreen;
			xev.xclient.data.l[2] = 0;
			XSendEvent (dpy, DefaultRootWindow(dpy)
					, False,
					SubstructureRedirectMask | SubstructureNotifyMask, &xev);
			xev.xclient.window = win;
			XFlush(dpy);
		}
	}

	Osdgst->CreateWindow(dpy);

	g_printerr("cGstreamerOsd(int Left, int Top, uint Level) : cOsd(Left, Top, Level) \n");
};// end of method

/*
 * Deconstruktor
 */
cGstreamerOsd::~cGstreamerOsd()
{
	SetActive(false);

	delete(Osdgst);
	Osdgst = NULL;

	//XFlush(dpy);

	g_printerr("~cGstreamerOsd() \n");
};// end of method

/*
 * SetAreas
 */
eOsdError cGstreamerOsd::SetAreas(const tArea *Areas, int NumAreas)
{
	if(!IsTrueColor())
	{
		cBitmap *bitmap;
		int i;

		for (i = 0; (bitmap = GetBitmap(i)); i++)
		{
			bitmap->Clean();
		}
	}


	g_printerr("SetAreas(const tArea *Areas, int NumAreas) \n");
	//return oeOk;
	return cOsd::SetAreas(Areas, NumAreas);

};// end of method

/*
 * Flush
 */
void cGstreamerOsd::Flush(void)
{
	cPixmapMemory *pm;

	if(!Active())
	{
		return;
	}

	if(!IsTrueColor())
	{
		return;
	}


	LOCK_PIXMAPS;
	while ((pm = (dynamic_cast < cPixmapMemory * >(RenderPixmaps()))))
	{

        Osdgst->FlushOsd(pm);
	}

};// end of method

/*
 * CreatPixmap
 */
cPixmap *cGstreamerOsd::CreatePixmap(int Layer, const cRect &ViewPort, const cRect &DrawPort)
{
	g_printerr("CreatePixmap() \n");
	return cOsd::CreatePixmap(Layer, ViewPort, DrawPort);
};// end of method

/*
 * CreateMainWindow
 */
void *cGstreamerOsd::CreateMainWindow()
{
    XEvent event;
    int x,y, attr_mask;
    XSizeHints hints;
    XWMHints *startup_state;
    XTextProperty textprop;
    XSetWindowAttributes attr = {0,};
    int numfbconfigs;
    static char *title = "Main Window";

    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        g_printerr("Couldn't connect to X server\n");
    }
    int Xscreen = DefaultScreen(dpy);
    Window Xroot = RootWindow(dpy, Xscreen);

    GLXFBConfig *fbconfigs = glXChooseFBConfig(dpy, Xscreen, VisDataMain, &numfbconfigs);
    GLXFBConfig fbconfig = 0;
    for(int i = 0; i<numfbconfigs; i++) {
        osd_visual = (XVisualInfo*) glXGetVisualFromFBConfig(dpy, fbconfigs[i]);
        if(!osd_visual)
            continue;

        pict_format = XRenderFindVisualFormat(dpy, osd_visual->visual);
        if(!pict_format)
            continue;

        fbconfig = fbconfigs[i];
        if(pict_format->direct.alphaMask > 0) {
            break;
        }
    }

    if(!fbconfig) {
        g_printerr("No matching FB config found");
    }

    describe_fbconfig(fbconfig);

    /* Create a colormap - only needed on some X clients, eg. IRIX */
    cmap = XCreateColormap(dpy, Xroot, osd_visual->visual, AllocNone);

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
            CWBackPixmap|
            CWColormap|
            CWBorderPixel|
            CWEventMask|
            CWBackPixel;

    attr.background_pixel = 0;

    osd_width = DisplayWidth(dpy, DefaultScreen(dpy))/2;
    osd_height = DisplayHeight(dpy, DefaultScreen(dpy))/2;
    x=osd_width/2, y=osd_height/2;

    //window_handle = XCreateWindow( Xdisplay, Xroot, x, y, 1280, 720, 0, osd_visual->depth, InputOutput, osd_visual->visual, attr_mask, &attr);
    //win = XCreateWindow( dpy, Xroot, 0, 0, 1280, 720, 0, osd_visual->depth, InputOutput, osd_visual->visual, attr_mask, &attr);
    win = XCreateWindow(dpy, Xroot, x, y, 1280, 720, 0, osd_visual->depth, InputOutput, osd_visual->visual, CWBackPixel | CWColormap | CWBorderPixel, &attr);



    if( !win ) {
        g_printerr("Couldn't create the window\n");
    }

#if USE_GLX_CREATE_WINDOW
    fputs("glXCreateWindow ", stderr);
    int glXattr[] = { None };
    glX_win = glXCreateWindow(dpy, fbconfig, win, glXattr);
    if( !glX_window_handle ) {
        fatalError("Couldn't create the GLX window\n");
    }
#else
    glX_win = win;
#endif

    textprop.value = (unsigned char*)title;
    textprop.encoding = XA_STRING;
    textprop.format = 8;
    textprop.nitems = strlen(title);

    hints.x = x;
    hints.y = y;
    hints.width = osd_width;
    hints.height = osd_height;
    hints.flags = USPosition|USSize;

    startup_state = XAllocWMHints();
    startup_state->initial_state = NormalState;
    startup_state->flags = StateHint;

    XSetWMProperties(dpy, win,&textprop, &textprop,
            NULL, 0,
            &hints,
            startup_state,
            NULL);

    XFree(startup_state);

    XMapWindow(dpy, win);
    //XIfEvent(dpy, &event, WaitForMapNotify, (char*)&win);

    if ((del_atom = XInternAtom(dpy, "WM_DELETE_WINDOW", 0)) != None) {
        XSetWMProtocols(dpy, win, &del_atom, 1);
    }

    gc = XCreateGC(dpy, win, 0, NULL);

    XSetForeground(dpy, gc, whiteColor);

	g_printerr("CreateMainWindow() \n");
};// end of method

/*
* describe_fbconfig
*/
void cGstreamerOsd::describe_fbconfig(GLXFBConfig fbconfig)
{
    int doublebuffer;
    int red_bits, green_bits, blue_bits, alpha_bits, depth_bits;

    glXGetFBConfigAttrib(dpy, fbconfig, GLX_DOUBLEBUFFER, &doublebuffer);
    glXGetFBConfigAttrib(dpy, fbconfig, GLX_RED_SIZE, &red_bits);
    glXGetFBConfigAttrib(dpy, fbconfig, GLX_GREEN_SIZE, &green_bits);
    glXGetFBConfigAttrib(dpy, fbconfig, GLX_BLUE_SIZE, &blue_bits);
    glXGetFBConfigAttrib(dpy, fbconfig, GLX_ALPHA_SIZE, &alpha_bits);
    glXGetFBConfigAttrib(dpy, fbconfig, GLX_DEPTH_SIZE, &depth_bits);

    fprintf(stderr, "FBConfig selected:\n"
            "Doublebuffer: %s\n"
            "Red Bits: %d, Green Bits: %d, Blue Bits: %d, Alpha Bits: %d, Depth Bits: %d\n",
            doublebuffer == True ? "Yes" : "No",
                    red_bits, green_bits, blue_bits, alpha_bits, depth_bits);
};// end of method



