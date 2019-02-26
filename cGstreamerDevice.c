#include "cGstreamerDevice.h"


/* Store the information from the caps that we are interested in. */

static void
 prepare_overlay (GstElement * overlay, GstCaps * caps, gpointer user_data)

{

  CairoOverlayState *state = (CairoOverlayState *) user_data;



  state->valid = gst_video_info_from_caps (&state->vinfo, caps);

}// end of function

/* Draw the overlay. 

 * This function draws a cute "beating" heart. */

static void draw_overlay (GstElement * overlay, cairo_t * cr, guint64 timestamp, guint64 duration, gpointer user_data, cPixmapMemory *pm)

{

  CairoOverlayState *s = (CairoOverlayState *) user_data;

  double scale;

  int width, height;



  if (!s->valid)
    return;
  
  //local_cr = cr;
  width = GST_VIDEO_INFO_WIDTH (&s->vinfo);
  height = GST_VIDEO_INFO_HEIGHT (&s->vinfo);

  
  if( pm == NULL)
  {
     g_printerr("draw_overlay with Pixmap \n");
     int depth = 32; // works fine with depth = 24
     int bitmap_pad = 32;// 32 for 24 and 32 bpp, 16, for 15&16
     int bytes_per_line = 0;// number of bytes in the client image between the start of one
     Display *display=XOpenDisplay(0);
     

     unsigned char *image32=(unsigned char *)malloc(width*height*4);

     XImage *img = XCreateImage(display, CopyFromParent, depth, ZPixmap, 0, (char*)image32, width, height, bitmap_pad, bytes_per_line);

     img->data = (char*)pm->Data();
     
     cairo_surface_t *surface_source =
     cairo_image_surface_create_for_data( (unsigned char*)(img->data),
                                          CAIRO_FORMAT_ARGB32, 
                                          img->width, 
                                          img->height, 
                                          img->bytes_per_line);
     
     cairo_set_source_surface(cr, surface_source, 0, 0);
     cairo_paint(cr);
     cairo_surface_destroy(surface_source);
     
     delete(pm); 
    
  }
  else
  {


  scale = 2 * (((timestamp / (int) 1e7) % 70) + 30) / 100.0;

  cairo_translate (cr, width / 2, (height / 2) - 30);


  /* FIXME: this assumes a pixel-aspect-ratio of 1/1 */

  cairo_scale (cr, scale, scale);



  cairo_move_to (cr, 0, 0);

  cairo_curve_to (cr, 0, -30, -50, -30, -50, 0);

  cairo_curve_to (cr, -50, 30, 0, 35, 0, 60);

  cairo_curve_to (cr, 0, 35, 50, 30, 50, 0);

  cairo_curve_to (cr, 50, -30, 0, -30, 0, 0);

  cairo_set_source_rgba (cr, 0.9, 0.0, 0.1, 0.7);

  cairo_fill (cr);
  }

}// end of function


static gboolean handle_message_shm(GstBus *bus, GstMessage *msg)
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


static GstBusSyncReply create_window_shm(GstBus *bus, GstMessage *message, GstPipeline *pipeline)
{

    if (!gst_is_video_overlay_prepare_window_handle_message(message))
        return GST_BUS_PASS;

    XSync(pipedpy, false);

    gst_video_overlay_set_window_handle(
        GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(message)), pipewin);

    gst_message_unref(message);


    return GST_BUS_DROP;
};// end of function


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
    
/*    
    if ((!display_name || !*display_name)
            && !(display_name = getenv("DISPLAY"))) {
        display_name = ":0.0";
    }

    setenv("DISPLAY", display_name, 1);

    if (!(dpy = XOpenDisplay(display_name))) {
        g_printerr("open_display: faild to connect to X Server (%s) \n",
                   display_name);
    }
*/

};// end of function

class cGstreamerOsd : public cOsd {

public:

    cOsdgst *Osdgst;

    void SetActive(bool on)
    {
        cOsd::SetActive(on);
    };// end of method

    cGstreamerOsd(int Left, int Top, uint Level) : cOsd(Left, Top, Level)
    {
        Osdgst = new cOsdgst(Left,  Top,  Level);


        Osdgst->CreateWindow(dpy);

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

        g_printerr("SetAreas(const tArea *Areas, int NumAreas) %d NumAres\n", NumAreas);

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
            
            //draw_overlay(cairo_overlay, local_cr, 1,1, overlay_state, pm);
            //g_printerr("Flush with Pixmap \n");
           
            
        }

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


bool cGstreamerDevice::CreateShmSrc()
{
    //shmsrc socket-path=/tmp/tmpsock is-live=1 

  if (!pipesrc)
  {
    pipesrc = gst_element_factory_make ("shmsrc", NULL);
  }
    
    if (!pipesrc)
    { 
      g_printerr("cGstreamerDevice::CreateShmSrc() Could not make shmsrc \n");
    }
    
    g_object_set (pipesrc, "socket-path", "/tmp/tmpsock", "do-timestamp", TRUE, "is-live", TRUE, NULL);
    
/*
    pipebus = gst_element_get_bus(pipesrc);
    gst_bus_set_sync_handler(pipebus, (GstBusSyncHandler) create_window, pipesrc, NULL);
    gst_bus_add_watch(pipebus, (GstBusFunc)handle_message, NULL);
*/
    
}; // end of method



void cGstreamerDevice::Init()
{
    pipesrc = NULL;
    spipe = NULL;
    
    g_printerr("void cGstreamerDevice::Init() \n");

    setenv("GST_VAAPI_ALL_DRIVERS", "1", 1);
    setenv("GST_DEBUG", "2", 2);

    open_display();
    gst_init (NULL, NULL);
/*
    FILE *fd = fopen(TEMP_PATH,"a+");

    uri = g_strdup_printf ("playbin uri=file://%s", TEMP_PATH);

    appsrc = gst_element_factory_make("playbin", "playbin");

    local_uri = g_strdup_printf ("file://%s", TEMP_PATH);
    g_object_set(appsrc, "uri", local_uri, NULL);
    g_printerr("cGstreamerDevice::Init() g_object_set uri %s \n", local_uri);
*/


/*    
    bus = gst_element_get_bus(appsrc);
    gst_bus_set_sync_handler(bus, (GstBusSyncHandler) create_window, appsrc, NULL);
    gst_bus_add_watch(bus, (GstBusFunc)handle_message, NULL);

    gint flags;
    g_object_get(appsrc, "flags", &flags, NULL);
    flags |= GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO;
    flags &= ~GST_PLAY_FLAG_TEXT;
    g_object_set(appsrc, "flags", flags, NULL);


    

    GstRegistry *registry = NULL;
    GstElementFactory *factory = NULL;
    registry = gst_registry_get();
    if (!registry)
        g_printerr("cPluginGstreamerdevice:ProcessArgs(): registry is null \n");
    factory = gst_element_factory_find("vaapidecodebin");
    if(!factory)
    {
        g_printerr("cPluginGstreamerdevice:ProcessArgs(): factory is null \n");
    }
    else
    {
      gst_plugin_feature_set_rank(GST_PLUGIN_FEATURE(factory)
                                , GST_RANK_PRIMARY - 1);
    }

    g_printerr("gstreamer Version %s \n" ,gst_version_string());
  */  
    
    
    // Test
    
    //Create Overlay

    FILE *fd = fopen(TEMP_PATH,"a+");


    appsrc = gst_element_factory_make("playbin", "playbin");
    local_uri = g_strdup_printf ("file://%s", TEMP_PATH);
    g_object_set(appsrc, "uri", local_uri, NULL);

    
    //uri = g_strdup_printf ("%s", TEMP_PATH);
    //appsrc = gst_element_factory_make ("filesrc", "source");
    //appsrc = gst_element_factory_make ("videotestsrc", "source");;
    //g_object_set(appsrc, "location", uri, NULL);

    adaptor1 = gst_element_factory_make ("videoconvert", "adaptor1");

    cairo_overlay = gst_element_factory_make ("cairooverlay", "overlay");

    adaptor2 = gst_element_factory_make ("videoconvert", "adaptor2");

    decoder  = gst_element_factory_make ("decodebin", "decodebin");

    sink = gst_element_factory_make ("xvimagesink", "sink");
    if(sink == NULL)
    {
      sink = gst_element_factory_make ("autovideosink", "sink");
    }
    
   
    /* If failing, the element could not be created */

    g_assert (cairo_overlay);



    /* allocate on heap for pedagogical reasons, makes code easier to transfer */

    overlay_state = g_new0 (CairoOverlayState, 1);
    
    
    /* Hook up the neccesary signals for cairooverlay */

    g_signal_connect (cairo_overlay, "draw", G_CALLBACK (draw_overlay), overlay_state);

    g_signal_connect (cairo_overlay, "caps-changed", G_CALLBACK (prepare_overlay), overlay_state);
    
/*    
    pipeline = gst_pipeline_new ("cairo-overlay-example");

    gst_bin_add_many (GST_BIN (pipeline), appsrc, adaptor1, cairo_overlay, adaptor2, sink, NULL);
    
    if (gst_element_link_many (appsrc, adaptor1,cairo_overlay, adaptor2, sink, NULL) != TRUE) 
    {
         g_printerr("Failed to link elements!");
    }
*/
    //bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    //gst_bus_add_watch(bus, (GstBusFunc)handle_message, NULL);
    //gst_bus_add_signal_watch (bus);
    //g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (handle_message), NULL);

};//end if method

cGstreamerDevice::cGstreamerDevice() : cDevice()
{
    remove(TEMP_PATH);
    remove("/tmp/tmpsock");
    g_printerr("GstreamerDevice() : cDevice() \n");

    Init();

    if( dpy != NULL)
    {
        blackColor = BlackPixel(dpy, DefaultScreen(dpy));
        whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

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

            XSelectInput(dpy, win, 
                 StructureNotifyMask |

                 ExposureMask |

                 KeyPressMask |

                 ButtonPressMask |

                 FocusChangeMask);
            
            
            
            XMapWindow(dpy, win);
            XSync(dpy, false);
            XFlush(dpy);
  /*         
             while (dpy > 0) 
             {

                XEvent event;



                XLockDisplay (dpy);

                XNextEvent (dpy, &event);

                XUnlockDisplay (dpy);



                switch (event.type) 
                {


                    case ButtonPress:

                    break;



                    case KeyPress:

                    case KeyRelease:

                    break;



                    default:; // ignore other events.



                };
             };
            
   */         
            
            
            
/*            

            XSelectInput(dpy, win, KeyPressMask | KeyReleaseMask );

            XEvent event;
            while (1)

            {

                XNextEvent(dpy, &event);

 


                if (event.type == KeyPress)

                {

                    printf( "KeyPress: %x\n", event.xkey.keycode );



                    if ( event.xkey.keycode == 0x09 )

                          break;

                }

            else if (event.type == KeyRelease)

            {

                printf( "KeyRelease: %x\n", event.xkey.keycode );

            }

    }

*/

/*
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
*/
            pipedpy = dpy;
            pipewin = win;

        }
    }


};// end of method

cGstreamerDevice::~cGstreamerDevice()
{
    g_printerr("~cGstreamerDevice() \n");
};// end of method

bool cGstreamerDevice::HasDecoder(void) const
{
    return true;
};// end of method

bool cGstreamerDevice::CanReplay(void) const
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
            ilive_stream_count = 0;
            live_stream_is_runnig = FALSE;
            remove(TEMP_PATH);
            g_printerr("SetPlayMode (%d) live_stream_is_runnig, ilive_stream_count %d\n",PlayMode, ilive_stream_count);
            
            gst_element_set_state (appsrc, GST_STATE_NULL);
            gst_element_set_state (pipeline, GST_STATE_NULL);

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

    FILE *fd = fopen(TEMP_PATH,"a+");
    if(fd != NULL)
    {
        fwrite(Data, 1, Length, fd);
        fclose(fd);
    }


    if( ilive_stream_count < 30000)
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
            ilive_stream_count++;
            return Length;
        }
    }

    
        
    
/*    
    if(spipe == NULL)
    {    
      #define FILE_MODE 0644
      spipe = sp_writer_create( "/tmp/tmpsock" , 366, FILE_MODE  ); 
    }  

    // shmsrc erzeugen
    if(pipesrc == NULL)
    {
      CreateShmSrc();
    }

    


    if(spipe != NULL)
    {
        ShmBlock* block = sp_writer_alloc_block (spipe, Length);
        if(block != NULL)
        {    
          char* shmbuf = sp_writer_block_get_buf (block);
          memcpy (shmbuf, Data, Length);
          int ret = sp_writer_send_buf (spipe, shmbuf, Length,NULL);

          //g_printerr("sp_writer_send_buf (%d) Client\n",ret);
        
          sp_writer_free_block (block);
          if(!live_stream_is_runnig)
          {
              gst_element_set_state (pipesrc, GST_STATE_PLAYING);
              live_stream_is_runnig = TRUE;
          }
        }
        
    }

 */   

    
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
    gst_element_set_state (appsrc, GST_STATE_NULL);
    
  //  local_uri = g_strdup_printf ("file://%s", TEMP_PATH);
  //  g_object_set(appsrc, "uri", local_uri, NULL);

    g_printerr(local_uri);
    g_printerr("\n");


    g_printerr("StartReplay() \n");

    //GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(appsrc), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline_graph");

    if (error)
    {
        g_printerr("No pipeline error(%s) \n", error->message);
    }
    else
    {
    }
    
    gst_element_set_state (appsrc, GST_STATE_PLAYING);
    gst_element_set_state (pipeline, GST_STATE_PLAYING);

};// end of method


void cGstreamerDevice::ReplayPlayFile(char* Filename)
{

    g_printerr(Filename);
    g_printerr("\n");
    
    gst_element_set_state (appsrc, GST_STATE_NULL);
    
    std::string str2 ("http");
    std::string temp = Filename;
    std::size_t found = temp.find(str2);
    if (found!=std::string::npos)
    {
        local_uri = Filename;
    }
    else
    {
        local_uri = g_strdup_printf ("file://%s", Filename);
    }
    
    //Test
    //local_uri ="http://nrodl.zdf.de/none/3sat/19/02/190220_teufelskraut_oder_wunderbluete_ard/1/190220_teufelskraut_oder_wunderbluete_ard_2328k_p35v13.mp4";
    g_object_set(appsrc, "uri", local_uri, NULL);
    
    gst_element_set_state (appsrc, GST_STATE_PLAYING);

    g_printerr("ReplayPlayFile() \n");

    //GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(appsrc), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline_graph");

    if (error)
    {
        g_printerr("No pipeline error(%s) \n", error->message);
    }
    else
    {
    }


};// end of method

