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
void *cOsdgst::CreateWindow(Display *dpy)
{
    XEvent event;
    int x,y, attr_mask;
    XSizeHints hints;
    XWMHints *startup_state;
    XTextProperty textprop;
    XSetWindowAttributes attr = {0,};
    static char *title = "FTB's little OpenGL example - ARGB extension by WXD";

    const char *display_name = NULL;
    if ((!display_name || !*display_name)
            && !(display_name = getenv("DISPLAY"))) {
        display_name = ":0.0";
    }

    setenv("DISPLAY", display_name, 1);

    if (!(Xdisplay = XOpenDisplay(display_name))) {
        g_printerr("open_display: faild to connect to X Server (%s) \n",
                   display_name);
    }


    if (!Xdisplay) {
        fatalError("Couldn't connect to X server\n");
    }
    else
    {
       // Xdisplay = dpy;
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

                if( (pict_format->direct.alphaMask > 0) && (pict_format->depth = 32) ) {
                    fbconfig = fbconfigs[i];
                     g_printerr("FB config found %d", i);
                    break;
                }
            }

            if(!fbconfig) {
                g_printerr("No matching FB config found");
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

    window_handle = XCreateWindow( Xdisplay, Xroot, 0, 0, 1920, 1080, 0, 32, InputOutput, osd_visual->visual, attr_mask, &attr);



    if( !window_handle ) {
        fatalError("Couldn't create the window\n");
    }


    textprop.value = (unsigned char*)title;
    textprop.encoding = XA_STRING;
    textprop.format = 8;
    textprop.nitems = strlen(title);

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
    Atom cmAtom = XInternAtom(Xdisplay, "_NET_WM_CM_S0", 0);
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
    XSendEvent( Xdisplay, DefaultRootWindow(Xdisplay),
                False,
                SubstructureRedirectMask | SubstructureNotifyMask, &xev);

    xev.xclient.window = window_handle;

    double alpha = 0.8;
    unsigned long opacity = (unsigned long)(0xFFFFFFFFul * alpha);
    Atom XA_NET_WM_WINDOW_OPACITY = XInternAtom(Xdisplay, "_NET_WM_WINDOW_OPACITY", False);

    XSetBackground(Xdisplay, osd_gc, 0x80808080);


    XChangeProperty( Xdisplay, window_handle,
                     XA_NET_WM_WINDOW_OPACITY,
                     XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&opacity,1L) ;

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
    g_printerr("%s",why);
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

    g_printerr("FBConfig selected:\n"
               "Doublebuffer: %s\n"
               "Red Bits: %d, Green Bits: %d, Blue Bits: %d, Alpha Bits: %d, Depth Bits: %d\n",
               doublebuffer == True ? "Yes" : "No",
               red_bits, green_bits, blue_bits, alpha_bits, depth_bits);
}; // end of method

/*
* write_png_for_image
*/
void cOsdgst::write_png_for_image(XImage *image, int width, int height, char *filename)
{
    int code = 0;
    FILE *fp;
    png_structp png_ptr;
    png_infop png_info_ptr;
    png_bytep png_row;

    char buffer[50];
    int n;

    n = sprintf(buffer, filename);

// Open file
    fp = fopen(buffer, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Could not open file for writing\n");
        code = 1;
    }

// Initialize write structure
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fprintf(stderr, "Could not allocate write struct\n");
        code = 1;
    }

// Initialize info structure
    png_info_ptr = png_create_info_struct(png_ptr);
    if (png_info_ptr == NULL) {
        fprintf(stderr, "Could not allocate info struct\n");
        code = 1;
    }

// Setup Exception handling
    if (setjmp(png_jmpbuf (png_ptr))) {
        fprintf(stderr, "Error during png creation\n");
        code = 1;
    }

    png_init_io(png_ptr, fp);

// Write header (8 bit colour depth)
    png_set_IHDR(png_ptr, png_info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

// Set title
    char *title = "Screenshot";
    if (title != NULL) {
        png_text title_text;
        title_text.compression = PNG_TEXT_COMPRESSION_NONE;
        title_text.key = "Title";
        title_text.text = title;
        png_set_text(png_ptr, png_info_ptr, &title_text, 1);
    }

    png_write_info(png_ptr, png_info_ptr);

// Allocate memory for one row (3 bytes per pixel - RGB)
    png_row = (png_bytep) malloc(3 * width * sizeof(png_byte));

    unsigned long red_mask = image->red_mask;
    unsigned long green_mask = image->green_mask;
    unsigned long blue_mask = image->blue_mask;

// Write image data
//int xxx, yyy;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned long pixel = XGetPixel(image, x, y);
            unsigned char blue = pixel & blue_mask;
            unsigned char green = (pixel & green_mask) >> 8;
            unsigned char red = (pixel & red_mask) >> 16;
            png_byte *ptr = &(png_row[x * 3]);
            ptr[0] = red;
            ptr[1] = green;
            ptr[2] = blue;
        }
        png_write_row(png_ptr, png_row);
    }

// End write
    png_write_end(png_ptr, NULL);

// Free
    fclose(fp);
    if (png_info_ptr != NULL)
        png_free_data(png_ptr, png_info_ptr, PNG_FREE_ALL, -1);
    if (png_ptr != NULL)
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
    if (png_row != NULL)
        free(png_row);

}; // end of method


/*
* Flush
*/
void cOsdgst::FlushOsd(cPixmapMemory *pm)
{
    int depth = 32; // works fine with depth = 24
    int bitmap_pad = 32;// 32 for 24 and 32 bpp, 16, for 15&16
    int bytes_per_line = 0;// number of bytes in the client image between the start of one
    unsigned int uiWidth = pm->ViewPort().Width();
    unsigned int uiHeight = pm->ViewPort().Height();

    unsigned char *image32=(unsigned char *)malloc(uiWidth*uiHeight*4);

    XImage *img = XCreateImage(Xdisplay, osd_visual->visual, depth, ZPixmap, 0, (char*)image32, uiWidth, uiHeight, bitmap_pad, bytes_per_line);

    img->data = (char*)pm->Data();

    Pixmap pixmap = XCreatePixmap(Xdisplay, window_handle, uiWidth, uiHeight, depth);

    // XWriteBitmapFile(Xdisplay, "/var/cache/osd.png", pixmap, uiWidth, uiHeight, -1, -1);
    //write_png_for_image(img, uiWidth, uiHeight, "/var/cache/osd.png");

    int w = uiWidth;
    int h = uiHeight;
    int X = pm->ViewPort().X();
    int Y = pm->ViewPort().Y();
    int T = Top();
    int L = Left();


    XPutImage(Xdisplay, pixmap, osd_gc, img, 0, 0, 0, 0, uiWidth, uiHeight);


    XCopyArea(Xdisplay, pixmap, window_handle,osd_gc,
              0 ,0,
              uiWidth, uiHeight,
              L+X, T+Y);

    XFlush(Xdisplay);
    XSync(Xdisplay, true);
    XFreePixmap(Xdisplay, pixmap);

    DestroyPixmap(pm);

    //Debug("Flush(void) \n");
    g_printerr("cOsdgst::FlushOsd(cPixmapMemory *pm) %d ViewPort().Width %d ViewPort().Height\n",pm->ViewPort().Width() ,pm->ViewPort().Height());
};// end of method

/*
* Flush
*/
cPixmap *cOsdgst::CreatePixmap(int Layer, const cRect &ViewPort, const cRect &DrawPort)
{
    Debug("CreatePixmap() \n");
    return cOsd::CreatePixmap(Layer, ViewPort, DrawPort);
};// end of method

