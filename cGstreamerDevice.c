#include "cGstreamerDevice.h"


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
};// end of function

static GstBusSyncReply create_window(GstBus *bus, GstMessage *message, GstPipeline *pipeline)
{

	if (!gst_is_video_overlay_prepare_window_handle_message(message))
		return GST_BUS_PASS;

	XSync(dpy, false);

	gst_video_overlay_set_window_handle(
			GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(message)), win);

	gst_message_unref(message);


	return GST_BUS_DROP;
};// end of function

/*
 * open the X11 Windows
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
};// end of function

class cGstreamerOsd : public cOsd {

	Display *dpy;
public:

	cOsdgst *Osdgst;

	void SetActive(bool on)
	{
		cOsd::SetActive(on);
	};// end of method

	cGstreamerOsd(int Left, int Top, uint Level) : cOsd(Left, Top, Level)
	{
		Osdgst = new cOsdgst(Left,  Top,  Level);


		Osdgst->CreateWindow(win);

		g_printerr("cGstreamerOsd(int Left, int Top, uint Level) : cOsd(Left, Top, Level) \n");
	};// end of method

	~cGstreamerOsd()
	{
		SetActive(false);

		delete(Osdgst);
		Osdgst = NULL;


		g_printerr("~cGstreamerOsd() \n");
	};// end of method


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

	};// end of method


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

};// end of class




class cGstreamerOsdProvider : public cOsdProvider {
private:
	static cOsd *Osd;
protected:
	cOsd *CreateOsd(int Left, int Top, uint Level)
	{

		return Osd = new cGstreamerOsd(Left, Top, Level);

	};// end of method

	bool ProvidesTrueColor(void)
	{
		return true;
	};// end of method

	int StoreImageData(const cImage &Image)
	{
		return 0;

	};// end of method

	void DropImageData(int ImageHandle) {}

public:
	cGstreamerOsdProvider() : cOsdProvider() {}
	~cGstreamerOsdProvider() {}
};// end of class

cOsd *cGstreamerOsdProvider::Osd;




void cGstreamerDevice::Init()
{
	g_printerr("void cGstreamerDevice::Init() \n");
	
	setenv("GST_VAAPI_ALL_DRIVERS", "1", 1);
	setenv("GST_DEBUG", "2", 2);

	open_display();
	gst_init (NULL, NULL);
    
        FILE *fd = fopen(TEMP_PATH,"a+");
    
	uri = g_strdup_printf ("playbin uri=file://%s", TEMP_PATH);

	appsrc = gst_element_factory_make("playbin", "playbin");
	local_uri = g_strdup_printf ("file://%s", TEMP_PATH);
	g_object_set(appsrc, "uri", local_uri, NULL);
	g_printerr("cGstreamerDevice::Init() g_object_set uri %s \n", local_uri);

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
			, GST_RANK_PRIMARY - 1);

	g_printerr("gstreamer Version %s \n" ,gst_version_string());

};//end if method

cGstreamerDevice::cGstreamerDevice() : cDevice()
{
	remove(TEMP_PATH);
	g_printerr("GstreamerDevice() : cDevice() \n");

	Init();

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

			win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0 ,0, 1920, 1080, 0, 24, InputOutput, visual, CWBackPixel | CWColormap | CWBorderPixel, &attr);
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

	//StartReplay();

};// end of method

cGstreamerDevice::~cGstreamerDevice()
{
	g_printerr("~cGstreamerDevice() \n");
};// end of method

bool cGstreamerDevice::HasDecoder(void) const
{
	return true;
};// end of method

bool cGstreamerDevice::SetPlayMode(ePlayMode PlayMode)
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
	      //gst_element_set_state (appsrc, GST_STATE_NULL);
	      //g_printerr("SetPlayMode (%d)  GST_STATE_NULL \n",PlayMode);
	    break;
	}
	default:
		break;
	}

	return true;

};// end of method

int cGstreamerDevice::PlayVideo(const uchar *Data, int Length)
{
	g_printerr("PlayVideo  \n");
	return Length;
};// end of method

int cGstreamerDevice::PlayAudio(const uchar *Data, int Length, uchar Id)
{
	g_printerr("PlayAudio \n");
	return Length;

};// end of method

int cGstreamerDevice::PlayTsVideo(const uchar *Data, int Length)
{
	g_printerr("PlayTsVideo \n");
	return Length;
};// end of method

int cGstreamerDevice::PlayTsAudio(const uchar *Data, int Length)
{
	g_printerr("PlayTsAudio \n");
	return Length;
};//end of method

int cGstreamerDevice::PlayTsSubtitle(const uchar *Data, int Length)
{
	g_printerr("PlayTsSubtitle \n");
	return Length;
};// end of method

int cGstreamerDevice::PlayPes(const uchar *Data, int Length, bool VideoOnly)
{
	g_printerr("PlayPes \n");
	return Length;
};// end of method

int push_to_buffer(const uchar *Data, int Length)
{
	return Length;

};// end of method

// PlayTs
int cGstreamerDevice::PlayTs(const uchar *Data, int Length, bool VideoOnly)
{
	g_printerr("PlayTs Length (%d)\n", Length);
	
	FILE *fd = fopen(TEMP_PATH,"a+");
	if(fd != NULL)
	{
		fwrite(Data, 1, Length, fd);
		fclose(fd);
	}


	if( ilive_stream_count < 30000)
	{
		ilive_stream_count++;
		g_printerr("PlayTs (%d)\n", ilive_stream_count);
		return Length;
	}

	else
	{
		if(!live_stream_is_runnig)
		{
			StartReplay();
			live_stream_is_runnig = TRUE;
			ilive_stream_count++;	
			if(ilive_stream_count > 3000000)
			{
				ilive_stream_count=30000;
				remove(TEMP_PATH);
				g_printerr("PlayTs(remove) (%d)\n", ilive_stream_count);
				FILE *fd = fopen(TEMP_PATH,"a+");
				if(fd != NULL)
				{
					fwrite(Data, 1, Length, fd);
					fclose(fd);
				}
				g_printerr("Clear Temp-File \n");
				ilive_stream_count++;	
			}
			return Length;
		}
	}

	g_printerr("PlayTs ilive_stream_count (%d)\n", ilive_stream_count);
	ilive_stream_count++;	
	
	return Length;
};// end of method

bool cGstreamerDevice::Poll(cPoller &Poller, int TimeoutMs)
{
	g_printerr("Poll\n");
	
	
	return true;
};// end of method

bool cGstreamerDevice::Flush(int TimeoutMs)
{
	g_printerr("Flush\n");
	return true;
};// end of methso

bool cGstreamerDevice::Start(void)
{
	g_printerr("Start\n");
	return true;
};// end of method

void cGstreamerDevice::MakePrimaryDevice(bool On)
{
	if (On) new cGstreamerOsdProvider();
	cDevice::MakePrimaryDevice(On);
};// end of method

void cGstreamerDevice::StartReplayBuffer()
{
	return;
};// end of method

void cGstreamerDevice::ShowOverlay()
{

};// end of method


void cGstreamerDevice::StartReplay()
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


};// end of method


