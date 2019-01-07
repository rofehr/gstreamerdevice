/*
 * osdgst.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
    #include "osdgst.h"

    static Bool WaitForMapNotify(Display *d, XEvent *e, char *arg)
    {
        return d && e && arg && (e->type == MapNotify) && (e->xmap.window == *(Window*)arg);
    }; // end of method


    /*
    * Constructor
    */
    cOsdgst::cOsdgst(int Left, int Top, uint Level) : cOsd(Left, Top, Level)
    {

    }; // end of method

    /*
    * Deconstruktor
    */
    cOsdgst::~cOsdgst()
    {
        XCloseDisplay(Xdisplay);
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
            if( (pict_format->direct.alphaMask > 0) && (pict_format->depth == 32) ) {
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
                CWColormap|
                CWBorderPixel|
                CWEventMask;


        window_handle = XCreateWindow( Xdisplay, Xroot, 0, 0, 1920, 1080, 0, osd_visual->depth, InputOutput, osd_visual->visual, attr_mask, &attr);
       

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
        hints.width = osd_width;
        hints.height = osd_height;
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
        
        osd_gc = XCreateGC(Xdisplay, window_handle, 0, 0);

        
        Atom wm_state = XInternAtom(Xdisplay, "_NET_WM_STATE", true);
		Atom wm_fullscreen = XInternAtom(Xdisplay, "_NET_WM_STATE_FULLSCREEN", true);
		XEvent xev;
		memset(&xev, 0, sizeof(xev));
		xev.type = ClientMessage;
		xev.xclient.window = window_handle;
		xev.xclient.message_type = wm_state;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = 1;
		xev.xclient.data.l[1] = wm_fullscreen;
		xev.xclient.data.l[2] = 0;
		XSendEvent (Xdisplay, DefaultRootWindow(Xdisplay), 
                    False,
					SubstructureRedirectMask | SubstructureNotifyMask, &xev);
        xev.xclient.window = window_handle;
		XFlush(Xdisplay);

        
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

    /*
    * fatalError
    */
    void cOsdgst::Debug(const char *why)
    {
        fprintf(stderr, "%s",why);
    }; // end of method

    /*
    * describe_fbconfig
    */
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

    /*
    * Flush
    */
    void cOsdgst::FlushOsd(cPixmapMemory *pm)
    {
            int depth = 32; // works fine with depth = 24
            int bitmap_pad = 32;// 32 for 24 and 32 bpp, 16, for 15&16
            int bytes_per_line = 0;// number of bytes in the client image between the start of one

            unsigned char *image32=(unsigned char *)malloc(pm->ViewPort().Width()*pm->ViewPort().Height()*4);

            XImage *img = XCreateImage(Xdisplay, osd_visual->visual, depth, ZPixmap, 0, (char*)image32, pm->ViewPort().Width(), pm->ViewPort().Height(), bitmap_pad, bytes_per_line);

            img->data = (char*)pm->Data();

            Pixmap pixmap = XCreatePixmap(Xdisplay, window_handle, pm->ViewPort().Width(), pm->ViewPort().Height(), depth);


            int w = pm->ViewPort().Width();
            int h = pm->ViewPort().Height();
            int X = pm->ViewPort().X();
            int Y = pm->ViewPort().Y();
            int T = Top();
            int L = Left();


            XPutImage(Xdisplay, pixmap, osd_gc, img, 0, 0, 0, 0, pm->ViewPort().Width(), pm->ViewPort().Height());


            XCopyArea(Xdisplay, pixmap, window_handle,osd_gc,
                        0 ,0,
                        pm->ViewPort().Width(), pm->ViewPort().Height(),
                        L+X, T+Y);

            XFlush(Xdisplay);
            XSync(Xdisplay, true);
            XFreePixmap(Xdisplay, pixmap);

            DestroyPixmap(pm);
            
        Debug("Flush(void) \n");
    };// end of method

    /*
    * Flush
    */
    cPixmap *cOsdgst::CreatePixmap(int Layer, const cRect &ViewPort, const cRect &DrawPort)
	{
		Debug("CreatePixmap() \n");
		return cOsd::CreatePixmap(Layer, ViewPort, DrawPort);
	};// end of method

