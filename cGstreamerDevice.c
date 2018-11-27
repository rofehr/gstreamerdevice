#include "cGstreamerDevice.h"

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



cGstreamerDevice::cGstreamerDevice() : cDevice()
{
	remove(TEMP_PATH);
	g_printerr("GstreamerDevice() : cDevice() \n");
	g_printerr("gstreamer Version %s \n" ,gst_version_string());
};// end of method


cGstreamerDevice::~cGstreamerDevice()
{
	g_printerr("~cGstreamerDevice() \n");
}		// end of method

bool cGstreamerDevice::HasDecoder(void) const
{
	return true;
}		// end of method

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
		gst_element_set_state (appsrc, GST_STATE_NULL);
		g_printerr("SetPlayMode (%d) GST_STATE_NULL\n",PlayMode);
		break;
	}
	default:
		break;
	}

	return true;

}		// end of method

int cGstreamerDevice::PlayVideo(const uchar *Data, int Length)
{
	g_printerr("PlayVideo  \n");
	return Length;
}		// end of method

int cGstreamerDevice::PlayAudio(const uchar *Data, int Length, uchar Id)
{
	g_printerr("PlayAudio \n");
	return Length;

}		// end of method

int cGstreamerDevice::PlayTsVideo(const uchar *Data, int Length)
{
	g_printerr("PlayTsVideo \n");
	return Length;
}		// end of method

int cGstreamerDevice::PlayTsAudio(const uchar *Data, int Length)
{
	g_printerr("PlayTsAudio \n");
	return Length;
}
int cGstreamerDevice::PlayTsSubtitle(const uchar *Data, int Length)
{
	g_printerr("PlayTsSubtitle \n");
	return Length;
}

int cGstreamerDevice::PlayPes(const uchar *Data, int Length, bool VideoOnly)
{
	g_printerr("PlayPes \n");
	return Length;
}		// end of method

int cGstreamerDevice::push_to_buffer(const uchar *Data, int Length)
{
	return Length;

}

// PlayTs
int cGstreamerDevice::PlayTs(const uchar *Data, int Length, bool VideoOnly)
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

bool cGstreamerDevice::Poll(cPoller &Poller, int TimeoutMs)
{
	g_printerr("Poll\n");
	return true;
}		// end of method

bool cGstreamerDevice::Flush(int TimeoutMs)
{
	return true;
}

bool cGstreamerDevice::Start(void)
{
	return true;
}		// end of method


void cGstreamerDevice::MakePrimaryDevice(bool On)
{
	if (On) new cGstreamerOsdProvider();
	cDevice::MakePrimaryDevice(On);
}; // end of method

void cGstreamerDevice::StartReplayBuffer()
{
	return;
} // end of method

void cGstreamerDevice::ShowOverlay()
{

} // end of method


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


} // end of method


