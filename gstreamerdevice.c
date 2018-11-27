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

#define LOCAL_DEBUG 1

#include "cGstreamerDevice.h"


#define CHUNK_SIZE 4096


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
};// end of function

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


#include <X11/Xlib.h>

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
