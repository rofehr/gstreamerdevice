/*
 * Gstreamerdevice.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/config.h>
#include <vdr/device.h>
#include <vdr/osd.h>
#include <vdr/plugin.h>
#include <vdr/args.h>

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

#include "osdgst.h"


static XVisualInfo vinfo;
static Visual *visual;
static Display *dpy = NULL;
static GstElement *appsrc = NULL;

static GstBus *bus = NULL;
static GError *error = NULL;
static gchar *uri = NULL;
static  gchar *local_uri =NULL;
static int blackColor;
static int whiteColor;

static Window win = 0;
static Window glX_win = 0;
//static Window overlay_win = 0;
static GC gc;
//static GC overlay_gc;

// Global Defines
static bool live_stream_is_runnig = FALSE;
static int ilive_stream_count = 0;


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


#define CHUNK_SIZE 4096

#define LOCAL_DEBUG 1

#ifdef LOCAL_DEBUG
// #define TEMP_PATH "/home/roland/data/dummy.ts"
#define TEMP_PATH "/var/cache/dummy.ts"
#else
#define TEMP_PATH "/var/cache/dummy.ts"
#endif


/* playbin flags */
typedef enum {
	GST_PLAY_FLAG_VIDEO = (1 << 0), /* We want to play vidoe output */
	GST_PLAY_FLAG_AUDIO = (1 << 1), /* We want to play audio ourput */
	GST_PLAY_FLAG_TEXT = (1 << 2) /* We want subtitle output */
} GstPlayFlags;

static gboolean handle_message(GstBus *bus, GstMessage *msg)
{
	GError *err;
	gchar *debug_info;

	switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_ERROR:
		gst_message_parse_error(msg, &err, &debug_info);
		g_printerr("Error received fro element %s: %s\n",
				GST_OBJECT_NAME(msg->src), err->message);
		break;
	case GST_MESSAGE_STATE_CHANGED:
		g_printerr("GST_MESSAGE_STATE_CHANGED \n");
		break;
	default:
		break;
	};
	return 0;
}
;
//end of function

static GstBusSyncReply create_window(GstBus *bus, GstMessage *message, GstPipeline *pipeline)
{

	if (!gst_is_video_overlay_prepare_window_handle_message(message))
		return GST_BUS_PASS;

	XSync(dpy, false);

	gst_video_overlay_set_window_handle(
			GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(message)), win);

	gst_message_unref(message);


	return GST_BUS_DROP;
}
;
// end of function

/*
 *
 * open the X11 Windows
 *
 */
static void open_display(const char *display_name = NULL)
{
	if ((!display_name || !*display_name)
			&& !(display_name = getenv("DISPLAY"))) {
		display_name = ":0.0";
	}

	if (!(dpy = XOpenDisplay(display_name))) {
		g_printerr("open_display: faild to connect to X Server (%s) \n",
				display_name);
	}
}
;
//end of function



class cGstreamerOsd : public cOsd {

   	
public:
	
    cOsdgst *Osdgst;
    
	void SetActive(bool on)
	{
		cOsd::SetActive(on);
	}// end of method
	cGstreamerOsd(int Left, int Top, uint Level) : cOsd(Left, Top, Level)
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
                

/*                
				attr.background_pixel = 0x80ffffff;
				attr.colormap = XCreateColormap(dpy, XDefaultRootWindow(dpy), visual, AllocNone);
				attr.border_pixel = 0;

				overlay_win = XCreateWindow(dpy, win, 0 ,0, 1280, 720, 0, 24, InputOutput, visual, CWBackPixel | CWColormap | CWBorderPixel, &attr);

				overlay_gc = XCreateGC(dpy, overlay_win, 0, 0);

				if(gc <0)
				{
					g_printerr("cGstreamerOsd) XCreateGC \n");
				}


				double alpha = 0.8;
				unsigned long opacity = (unsigned long)(0xFFFFFFFFul * alpha);
				Atom XA_NET_WM_WINDOW_OPACITY = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);

				XSetBackground(dpy, overlay_gc, 0x80808080);


				XChangeProperty( dpy, overlay_win,
						XA_NET_WM_WINDOW_OPACITY,
						XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&opacity,1L) ;
				XFlush(dpy);

				XMapWindow(dpy, overlay_win);
				XMapWindow(dpy, win);

				XSync(dpy, false);

				XFlush(dpy);




				// Make Window fullscreen

				XSetForeground(dpy, gc, whiteColor);

				XCompositeRedirectSubwindows(dpy, root, CompositeRedirectAutomatic);

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
				//xev.xclient.data.l[1] = wm_fullscreen;
				xev.xclient.data.l[2] = 0;
				XSendEvent (dpy, DefaultRootWindow(dpy)
						, False,
						SubstructureRedirectMask | SubstructureNotifyMask, &xev);
				xev.xclient.window = overlay_win;
				XFlush(dpy);


				XSetWindowBackgroundPixmap(dpy, win, None);

				XMapRaised(dpy, win);

				XMapWindow(dpy, win);
*/

			}
		}
        
        Osdgst->CreateWindow(dpy);

		g_printerr("cGstreamerOsd(int Left, int Top, uint Level) : cOsd(Left, Top, Level) \n");
	} // end of method

	~cGstreamerOsd()
	{
		SetActive(false);
        
        delete(Osdgst); 
        Osdgst = NULL;
        
		XFlush(dpy);

		g_printerr("~cGstreamerOsd() \n");
	} // end of method


	eOsdError SetAreas(const tArea *Areas, int NumAreas)
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

	} // end of method


	void Flush(void)
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

	cPixmap *CreatePixmap(int Layer, const cRect &ViewPort, const cRect &DrawPort)
	{
		g_printerr("CreatePixmap() \n");
		return cOsd::CreatePixmap(Layer, ViewPort, DrawPort);
	};// end of method

	
	void *CreateMainWindow()
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
    void describe_fbconfig(GLXFBConfig fbconfig)
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
    }; // end of method
    
    static Bool WaitForMapNotify(Display *d, XEvent *e, char *arg)
    {
        return d && e && arg && (e->type == MapNotify) && (e->xmap.window == *(Window*)arg);
    }; // end of method


    
};

class cGstreamerOsdProvider : public cOsdProvider {
private:
	static cOsd *Osd;
protected:
	cOsd *CreateOsd(int Left, int Top, uint Level)
	{

		return Osd = new cGstreamerOsd(Left, Top, Level);

	}		// end of method

	bool ProvidesTrueColor(void)
	{
		return true;
	}		// end of method

	int StoreImageData(const cImage &Image)
	{
		return 0;

	}		// end of method

	void DropImageData(int ImageHandle) {}

public:
	cGstreamerOsdProvider() : cOsdProvider() {}
	~cGstreamerOsdProvider() {}
};

cOsd *cGstreamerOsdProvider::Osd;


class cGstreamerDevice : cDevice {
public:

	cGstreamerDevice() : cDevice()
    {
        remove(TEMP_PATH);
		g_printerr("GstreamerDevice() : cDevice() \n");
		g_printerr("gstreamer Version %s \n" ,gst_version_string());
    };// end of method

	virtual ~cGstreamerDevice()
	{
		g_printerr("~cGstreamerDevice() \n");
	}		// end of method

	bool HasDecoder(void) const
	{
		return true;
	}		// end of method

	bool SetPlayMode(ePlayMode PlayMode)
	{

		switch (PlayMode)
		{
		case 0:
		{
			if( live_stream_is_runnig)
			{
				gst_element_set_state (appsrc, GST_STATE_NULL);
				ilive_stream_count = 0;
				live_stream_is_runnig = FALSE;
				remove(TEMP_PATH);
				g_printerr("SetPlayMode (%d) live_stream_is_runnig, ilive_stream_count %d\n",PlayMode, ilive_stream_count);

			}
			break;
		}

		case 1:
		{
			gst_element_set_state (appsrc, GST_STATE_NULL);
			g_printerr("SetPlayMode (%d) GST_STATE_NULL\n",PlayMode);
			break;
		}
		default:
			break;
		}

		return true;

	}		// end of method

	int PlayVideo(const uchar *Data, int Length)
	{
		g_printerr("PlayVideo  \n");
		return Length;
	}		// end of method

	int PlayAudio(const uchar *Data, int Length, uchar Id)
	{
		g_printerr("PlayAudio \n");
		return Length;

	}		// end of method

	int PlayTsVideo(const uchar *Data, int Length)
	{
		g_printerr("PlayTsVideo \n");
		return Length;
	}		// end of method

	int PlayTsAudio(const uchar *Data, int Length)
	{
		g_printerr("PlayTsAudio \n");
		return Length;
	}
	int PlayTsSubtitle(const uchar *Data, int Length)
	{
		g_printerr("PlayTsSubtitle \n");
		return Length;
	}

	int PlayPes(const uchar *Data, int Length, bool VideoOnly = false)
	{
		g_printerr("PlayPes \n");
		return Length;
	}		// end of method

	int push_to_buffer(const uchar *Data, int Length)
	{
		return Length;

	}

	// PlayTs
	int PlayTs(const uchar *Data, int Length, bool VideoOnly = false)
	{


		FILE *fd = fopen(TEMP_PATH,"a+");
		if(fd != NULL)
		{
			fwrite(Data, 1, Length, fd);
			fclose(fd);
		}


		if( ilive_stream_count < 15000)
		{
			ilive_stream_count++;
			return Length;
		}

		else
		{
			if(!live_stream_is_runnig)
			{
				StartReplay();
				live_stream_is_runnig = TRUE;

				return Length;
			}

		}


		return Length;
	}		// end of method

	bool Poll(cPoller &Poller, int TimeoutMs = 0)
	{
		g_printerr("Poll\n");
		return true;
	}		// end of method

	bool Flush(int TimeoutMs = 0) {return true;}
	bool Start(void)
	{
		return true;
	}		// end of method

protected:

	void MakePrimaryDevice(bool On) {if (On) new cGstreamerOsdProvider(); cDevice::MakePrimaryDevice(On);}

	void StartReplayBuffer()
	{
		return;
	} // end of method

	void ShowOverlay()
	{

	} // end of method


	void StartReplay()
	{

		g_printerr(local_uri);
		g_printerr("\n");

		gst_element_set_state (appsrc, GST_STATE_PLAYING);

		g_printerr("StartReplay() \n");

		if (error)
		{
			g_printerr("No pipeline error(%s) \n", error->message);
		}
		else
		{

		}


	} // end of method

};



#include <X11/Xlib.h>
//#include <gst/gl/x11/gstgldisplay_x11.h>

static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "gstreamer Output device";
//static const char *MAINMENUENTRY  = "gstreamerdevice";

class cPluginGstreamerdevice : public cPlugin {
private:
public:
	cPluginGstreamerdevice(void) {}
	virtual ~cPluginGstreamerdevice() {}
	virtual const char *Version(void) { return VERSION; }
	virtual const char *Description(void) { return DESCRIPTION; }
	virtual const char *CommandLineHelp(void) { return NULL; }
	bool ProcessArgs(int argc, char *argv[])
	{
		g_printerr("cPluginGstreamerdevice:ProcessArgs(): gstreamerdevice \n");

		g_printerr("cPluginGstreamerdevice:ProcessArgs(): check if Xorg is runnig \n");

		dpy = NULL;

		setenv("GST_VAAPI_ALL_DRIVERS", "1", 1);
		setenv("GST_DEBUG", "2", 2);

		open_display();
		gst_init (&argc, &argv);

		uri = g_strdup_printf ("playbin uri=file://%s", TEMP_PATH);

		appsrc = gst_element_factory_make("playbin", "playbin");
		local_uri = g_strdup_printf ("file://%s", TEMP_PATH);
		g_object_set(appsrc, "uri", local_uri, NULL);
		g_printerr("cPluginGstreamerdevice:ProcessArgs(): g_object_set uri %s \n", local_uri);

		bus = gst_element_get_bus(appsrc);
		gst_bus_set_sync_handler(bus, (GstBusSyncHandler) create_window, appsrc, NULL);
		gst_bus_add_watch(bus, (GstBusFunc)handle_message, NULL);


		/* Set flags to show Audio and Video, but ignore Subtitles */
		gint flags;
		g_object_get(appsrc, "flags", &flags, NULL);
		flags |= GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO;
		flags &= ~GST_PLAY_FLAG_TEXT;
		g_object_set(appsrc, "flags", flags, NULL);


		/* enable Hardwaredecoding 'vaapidecode' */

		GstRegistry *registry = NULL;
		GstElementFactory *factory = NULL;
		registry = gst_registry_get();
		if (!registry)
			g_printerr("cPluginGstreamerdevice:ProcessArgs(): registry is null \n");
		factory = gst_element_factory_find("vaapidecode");
		if(!factory)
			g_printerr("cPluginGstreamerdevice:ProcessArgs(): factory is null \n");
		gst_plugin_feature_set_rank(GST_PLUGIN_FEATURE(factory)
				, GST_RANK_PRIMARY + 1);


		XFlush(dpy);

		return true;

	} // end of method

	virtual bool Initialize(void);

	bool Start(void)
	{
		return true;

	} // end of method

	virtual void Housekeeping(void) {}
	virtual const char *MainMenuEntry(void) { return NULL; }
	virtual cOsdObject *MainMenuAction(void) { return NULL; }
	virtual cMenuSetupPage *SetupMenu(void) { return NULL; }
	virtual bool SetupParse(const char *Name, const char *Value) { return false; };
};

bool cPluginGstreamerdevice::Initialize(void)
{
	g_printerr("cPluginGstreamerdevice:Initialize(): gstreamerdevice \n");

	new cGstreamerDevice();

	return true;
}



VDRPLUGINCREATOR(cPluginGstreamerdevice); // Don't touch this!
