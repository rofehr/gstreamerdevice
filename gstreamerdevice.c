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
static Window root = 0;
static Window overlay_win = 0;
static GC gc;
static GC overlay_gc;

// Global Defines
static bool live_stream_is_runnig = FALSE;
static int ilive_stream_count = 0;


#define CHUNK_SIZE 4096

#define LOCAL_DEBUG 1

#ifdef LOCAL_DEBUG
// #define TEMP_PATH "/home/roland/data/dummy.ts"
#define TEMP_PATH "/var/cache/dummy.ts"
#else
#define TEMP_PATH "/var/cache/dummy.ts"
#endif

/*
struct Data
{
  GstElement *pipe;
  GstElement *src;
  GstElement *id;
  GstElement *sink;
  GstElement *gdkpixbufoverlay;
};

Data s_data;
 */



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
	
	cOsdgst Osdgst;
	
	void SetActive(bool on)
	{
		cOsd::SetActive(on);
	}// end of method
	cGstreamerOsd(int Left, int Top, uint Level) : cOsd(Left, Top, Level)
	{
		Osdgst.CreateWindow();
		
		if( dpy != NULL)
		{
			blackColor = BlackPixel(dpy, DefaultScreen(dpy));whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

			if ( win == 0)
			{

				root = RootWindow(dpy, DefaultScreen(dpy));

				XSetWindowAttributes attr;
				XVisualInfo *visual_list;
				XVisualInfo visual_template;
				int nxvisuals;
				int i;


				nxvisuals = 0;
				visual_template.screen = DefaultScreen(dpy);visual_list = XGetVisualInfo (dpy, VisualScreenMask, &visual_template, &nxvisuals);

				for (i = 0; i < nxvisuals; ++i)
				{
					printf("  %3d: visual 0x%lx class %d (%s) depth %d\n",
							i,
							visual_list[i].visualid,
							visual_list[i].c_class,
							visual_list[i].c_class == TrueColor ? "TrueColor" : "unknown",
									visual_list[i].depth);

				}

				if (!XMatchVisualInfo(dpy, XDefaultScreen(dpy), 24, TrueColor, &vinfo))
				{
					fprintf(stderr, "no such visual\n");
				}

				visual = vinfo.visual;

				//attr.background_pixel = BlackPixel(dpy,DefaultScreen(dpy));
				attr.background_pixel = 0;
				attr.colormap = XCreateColormap(dpy, XDefaultRootWindow(dpy), visual, AllocNone);
				attr.border_pixel = 0;

				win = XCreateWindow(dpy, DefaultRootWindow(dpy), Left ,Top, 1280, 720, 0, 24, InputOutput, visual, CWBackPixel | CWColormap | CWBorderPixel, &attr);
				gc = XCreateGC(dpy, win, 0, NULL);

				attr.background_pixel = 0x80ffffff;
				attr.colormap = XCreateColormap(dpy, XDefaultRootWindow(dpy), visual, AllocNone);
				attr.border_pixel = 0;


				//overlay_win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0 ,0, 1280, 720, 0, 24, InputOutput, visual, CWBackPixel | CWColormap | CWBorderPixel, &attr);
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


			}
		}

		g_printerr("cGstreamerOsd(int Left, int Top, uint Level) : cOsd(Left, Top, Level) \n");
	} // end of method

	~cGstreamerOsd()
	{
		SetActive(false);
		XUnmapWindow(dpy, overlay_win);
		XClearWindow(dpy, overlay_win);
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

		XMapWindow(dpy, overlay_win);

		LOCK_PIXMAPS;
		while ((pm = (dynamic_cast < cPixmapMemory * >(RenderPixmaps()))))
		{

			int depth = 24; // works fine with depth = 24
			int bitmap_pad = 32;// 32 for 24 and 32 bpp, 16, for 15&16
			int bytes_per_line = 0;// number of bytes in the client image between the start of one

			unsigned char *image32=(unsigned char *)malloc(pm->ViewPort().Width()*pm->ViewPort().Height()*4);

			XImage *img = XCreateImage(dpy, vinfo.visual, depth, ZPixmap, 0, (char*)image32, pm->ViewPort().Width(), pm->ViewPort().Height(), bitmap_pad, bytes_per_line);

			img->data = (char*)pm->Data();

			Pixmap pixmap = XCreatePixmap(dpy, overlay_win, pm->ViewPort().Width(), pm->ViewPort().Height(), depth);


			int w = pm->ViewPort().Width();
			int h = pm->ViewPort().Height();
			int X = pm->ViewPort().X();
			int Y = pm->ViewPort().Y();
			int T = Top();
			int L = Left();

			g_printerr("XPutImage (with %d), (height %d), (X %d), (Y %d), (T %d), (L %d) \n", w, h, X, Y, T, L );

			XPutImage(dpy, pixmap, overlay_gc, img, 0, 0, 0, 0, pm->ViewPort().Width(), pm->ViewPort().Height());


			XCopyArea(dpy, pixmap, overlay_win,overlay_gc,
					0 ,0,
					pm->ViewPort().Width(), pm->ViewPort().Height(),
					L+X, T+Y);

			XFlush(dpy);


			/*
        Picture picture = XRenderCreatePicture(
                        dpy , pixmap,
                        XRenderFindStandardFormat(dpy, PictStandardRGB24), 0, 0);

        Picture win_picture = XRenderCreatePicture(
                        dpy , pixmap,
                        XRenderFindStandardFormat(dpy, PictStandardRGB24), 0, 0);
			 */
			/*
        XRenderComposite( dpy, PictOpSrc, picture, None,
                          pixmap, 0, 0, 0, 0, 0, 0, pm->ViewPort().Width(), pm->ViewPort().Height() );
			 */

			/*
        double alpha = 0.4;
        unsigned long opacity = (0xFFFFFFFF / 0xff) * alpha;
        Atom XA_NET_WM_WINDOW_OPACITY = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);

        //XSetBackground(dpy, overlay_gc, 0x80808080);

        XChangeProperty( dpy, overlay_win,
                         XA_NET_WM_WINDOW_OPACITY,
                         XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&opacity,1) ;

        XFlush(dpy);
			 */
			/*
        XCopyArea(dpy, pixmap, overlay_win,overlay_gc,
                  0 ,0,
                  pm->ViewPort().Width(), pm->ViewPort().Height(),
                  pm->DrawPort().X(), pm->DrawPort().Y());
			 */

			int shape_event_base, shape_error_base;
			bool ret = XShapeQueryExtension (dpy, &shape_event_base, &shape_error_base);
			if(ret)
			{
				//XFillRectangle(dpy, pixmap, gc, 0, 0, pm->ViewPort().Width(),  pm->ViewPort().Height());

				//XShapeCombineMask(dpy, overlay_win, ShapeBounding, 0, 0, pixmap, ShapeSet);
			}


			XSync(dpy, true);
			XFreePixmap(dpy, pixmap);
			DestroyPixmap(pm);

		}

		//XSync(dpy, true);
		//XFlush(dpy);

		g_printerr("Flush(void) \n");
	}		// end of method

	cPixmap *CreatePixmap(int Layer, const cRect &ViewPort, const cRect &DrawPort)
	{
		g_printerr("CreatePixmap() \n");
		return cOsd::CreatePixmap(Layer, ViewPort, DrawPort);
	}		// end of method

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
	/*
    bool SignalStats( int &Valid, double *Strength = NULL, double *Cnr = NULL, double *BerPre = NULL, double *BerPost = NULL, double *Per = NULL, int *Status = NULL) const
    {
	return false;
    }// end of method
	 */

	//Data data;

	cGstreamerDevice() : cDevice()
{


		remove(TEMP_PATH);
		g_printerr("GstreamerDevice() : cDevice() \n");
		g_printerr("gstreamer Version %s \n" ,gst_version_string());
}		// end of method

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
				//gst_element_set_state (pipeline, GST_STATE_NULL);
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
			//gst_element_set_state (pipeline, GST_STATE_NULL);
			g_printerr("SetPlayMode (%d) GST_STATE_NULL\n",PlayMode);
			break;
		}
		default:
			break;
		}

		//g_printerr("SetPlayMode (%d)\n",PlayMode);

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
		//gst_element_set_state (pipeline, GST_STATE_PLAYING);

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

		//const gchar *attrib = " ! tsparse ! video/x-h264 ! h264parse !avdec_h264 ! x264enc ! vaapidecode ! vaapisink";

		//uri = g_strdup_printf ("playbin uri=file://%s %s", TEMP_PATH, attrib);
		uri = g_strdup_printf ("playbin uri=file://%s", TEMP_PATH);

		appsrc = gst_element_factory_make("playbin", "playbin");
		//g_object_set(appsrc, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_cropped_multilingual.webm", NULL);
		local_uri = g_strdup_printf ("file://%s", TEMP_PATH);
		g_object_set(appsrc, "uri", local_uri, NULL);
		g_printerr("cPluginGstreamerdevice:ProcessArgs(): g_object_set uri %s \n", local_uri);

		bus = gst_element_get_bus(appsrc);
		gst_bus_set_sync_handler(bus, (GstBusSyncHandler) create_window, appsrc, NULL);
		gst_bus_add_watch(bus, (GstBusFunc)handle_message, NULL);



		/*
    pipeline = gst_pipeline_new ("my_pipeline");
    local_uri = g_strdup_printf ("%s", TEMP_PATH);
    GstElement *filesrc  = gst_element_factory_make ("filesrc", "my_filesource");
    //GstElement *sink     = gst_element_factory_make ("autovideosink", "autovideosink");
    GstElement *sink     = gst_element_factory_make ("autovideosink", "sink");

    GstElement *decoder  = gst_element_factory_make ("decodebin", "my_decoder");

    g_object_set (G_OBJECT (filesrc), "location", local_uri, NULL);
    gst_bin_add_many (GST_BIN (pipeline), filesrc, decoder, sink, NULL);
    if (!gst_element_link_many (filesrc, decoder, sink, NULL)) {
      g_print ("Failed to link one or more elements!\n");
      return -1;
    }
		 */
		/*
    bus = gst_element_get_bus(pipeline);
    gst_bus_set_sync_handler(bus, (GstBusSyncHandler) create_window, pipeline, NULL);
    gst_bus_add_watch(bus, (GstBusFunc)handle_message, NULL);
		 */


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
