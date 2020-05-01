#include "cGstreamerDevice.h"

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

        Osdgst->Init(overlay);

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



void cGstreamerDevice::open_display(const char *display_name = NULL)
{

}// end of function

void cGstreamerDevice::Action(void) {

if (!pipeline)
{
//     pipeline = gst_parse_launch("appsrc name=vdrsource !  decodebin  name=demux  demux.  !  queue  !  audioconvert  !  audioresample  !  autoaudiosink demux. !  videoconvert   !  videoscale  !  video/x-raw,width=1920 ,height=1080 ,method=1  ! autovideosink " , NULL);
	//pipeline = gst_parse_launch("appsrc name=vdrsource ! decodebin name=demux demux. ! queue ! audioconvert ! audioresample ! autoaudiosink demux. ! videoconvert ! gdkpixbufoverlay location=logo.png name=overlay !kmssink name=videosink" , NULL);
pipeline = gst_parse_launch("appsrc name=vdrsource ! decodebin name=demux demux. ! queue ! audioconvert ! audioresample ! autoaudiosink demux. ! videoconvert ! kmssink name=videosink" , NULL);

    if (!pipeline) {
     g_printerr("!pipeline /n");
     return;
    }

    mVdrSrc = gst_bin_get_by_name (GST_BIN(pipeline), "vdrsource");

    if (!mVdrSrc) {
      if (pipeline) gst_object_unref (GST_OBJECT (pipeline));
      g_printerr("!mVdrSrc (faild) /n");
      return;
    }
	else
	{
		g_printerr("!mVdrSrc (successful) /n");
	}

	//overlay = gst_element_factory_make ("gdkpixbufoverlay", NULL);

	//g_object_set (overlay, "location", "logo.png", NULL);

	overlay = gst_bin_get_by_name (GST_BIN(pipeline), "overlay");
	if(!overlay)
	{
			g_printerr("!overlay (faild) /n");
	}
	else
	{
			g_printerr("!overlay (successful) /n");
	}


	video_sink = gst_bin_get_by_name (GST_BIN(pipeline), "videosink");
	if(!video_sink)
	{
			g_printerr("!video_sink (faild) /n");
	}
	else
	{
			g_printerr("!video_sink (successful) /n");
	}

	//gst_bin_add_many (GST_BIN (pipeline), overlay, NULL);

	gst_element_link_many (mVdrSrc, overlay, video_sink, NULL);

    gst_element_set_state (pipeline, GST_STATE_PLAYING);

}
}; // end of method

void cGstreamerDevice::SetVolumeDevice(int Volume)
{
     if (!pipeline) {
     g_printerr("!pipeline");
     return;
    }
    else
     {
        //g_object_set(pipeline, "volume", Volume, NULL);
     }

}; // end of method

void cGstreamerDevice::Init()
{
    
    g_printerr("void cGstreamerDevice::Init() \n");

    setenv("GST_VAAPI_ALL_DRIVERS", "1", 1);
    
    // Info Debug
    setenv("GST_DEBUG", "4", 2);

    // Error Debug
   //setenv("GST_DEBUG", "1", 2);
    
   gst_init (NULL, NULL);

    // Init gstreamer pipeline
    //Action();

   // Init keboard Window
   open_display("");

    g_printerr("gstreamer Version %s \n" ,gst_version_string());

};//end if method

cGstreamerDevice::cGstreamerDevice() : cDevice()
{
    //mtrace();

    pipeline = NULL;
    Init();

};// end of method

cGstreamerDevice::~cGstreamerDevice()
{
    //muntrace() ;

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
    GstStateChangeReturn ret;

    
    switch (PlayMode)
    {
    case 0:
    {

            gst_element_set_state (pipeline, GST_STATE_NULL);
            //gst_element_set_state (pipeline, GST_STATE_READY);
  

            if (pipeline) {

                gst_object_unref (GST_OBJECT (pipeline));

                pipeline = NULL;

            }


         //gst_element_set_locked_state(pipeline, TRUE);
         //gst_element_set_state(pipeline, GST_STATE_READY);
         //gst_element_set_state(pipeline, GST_STATE_PAUSED);

         g_printerr("SetPlayMode (0)\n");
        break;
    }

    case 1:
    {
         if(pipeline == NULL)
         {
                Action();
                g_printerr("SetPlayMode (1);Action()\n");
         }
         else
         {

         //gst_element_set_locked_state(pipeline, FALSE);
         //gst_element_set_state (pipeline, GST_STATE_PLAYING);
         }

         g_printerr("SetPlayMode (1)\n");
        break;
    }
    default:
         g_printerr("SetPlayMode (default)\n");
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


    GstFlowReturn ret;

    guint8* bufferData = (guint8*) g_malloc (Length);

    memcpy(bufferData, Data, Length);

    GstBuffer *buf = gst_buffer_new_wrapped(bufferData, Length);

    g_signal_emit_by_name (mVdrSrc, "push-buffer", buf, &ret);

    gst_buffer_unref (buf);
    
     if (ret < GST_FLOW_OK) {

        g_printerr("Error sending buffer: %d", ret);
        gst_buffer_unref (buf);
        return -1;

    }


    return Length;
};// end of method

bool cGstreamerDevice::Poll(cPoller &Poller, int TimeoutMs)
{
    g_printerr("Poll\n");


    return true;
};// end of method

bool cGstreamerDevice::Flush(int TimeoutMs)
{
    //g_printerr("Flush\n");
    return true;
};// end of methso

bool cGstreamerDevice::Start(void)
{
    //g_printerr("Start\n");
    return true;
};// end of method

void cGstreamerDevice::MakePrimaryDevice(bool On)
{
    if (On) new cGstreamerOsdProvider();
    cDevice::MakePrimaryDevice(On);
    g_printerr("cGstreamerDevice::MakePrimaryDevice(bool On)\n");
};// end of method

void cGstreamerDevice::ShowOverlay()
{

};// end of method


void cGstreamerDevice::ReplayPlayFile(char* Filename)
{

    g_printerr(Filename);
    g_printerr("\n");
    
    gst_element_set_state (pipeline, GST_STATE_NULL);
    
    if (pipeline) {

        gst_object_unref (GST_OBJECT (pipeline));

        pipeline = NULL;

    }

    std::string str2 ("http");
    std::string temp = Filename;
    std::size_t found = temp.find(str2);
    if (found!=std::string::npos)
    {
        local_uri = Filename;
    }
    else
    {
        local_uri = g_strdup_printf ("filesrc  location=%s ! decodebin  name=demux  demux.  !  queue  !  audioconvert  !  audioresample  !  autoaudiosink demux. !  videoconvert    ! videoscale ! video/x-raw,width=1920,height=1080  ! xvimagesink ", Filename);
    }
    
     pipeline = gst_parse_launch(local_uri , NULL);

     g_printerr(local_uri);
     g_printerr("\n");
    
     gst_element_set_state (pipeline, GST_STATE_PLAYING);

    g_printerr("ReplayPlayFile() \n");

    if (error)
    {
        g_printerr("No pipeline error(%s) \n", error->message);
    }
    else
    {
    }


};// end of method

