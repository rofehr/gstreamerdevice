/*
 * cGstreamerdevice.s:
 *
 *
 *
 * $Id$
 */

#include "cGstreamerDevice.h"

class cGstreamerOsdProvider : public cOsdProvider {
private:
	static cOsd *Osd;
protected:
	/*
	 * Construktor
	 */
	cOsd *CreateOsd(int Left, int Top, uint Level)
	{

		return Osd = new cGstreamerOsd(Left, Top, Level);

	};// end of method

	/*
	 * ProvidesTrueColor
	 */
	bool ProvidesTrueColor(void)
	{
		return true;
	};// end of method

	/*
	 * StoreImageData
	 */
	int StoreImageData(const cImage &Image)
	{
		return 0;

	};// end of method

	/*
	 * DropImageData
	 */
	void DropImageData(int ImageHandle)
	{

	};// end of method

public:
	/*
	 * Construktor
	 */
	cGstreamerOsdProvider() : cOsdProvider()
{

};// end of method

	/*
	 * Deconstruktor
	 */
	~cGstreamerOsdProvider()
	{

	};// end of method
};

/*
 * OSD Class
 */
cOsd *cGstreamerOsdProvider::Osd;


/*
 * Construktor
 */
cGstreamerDevice::cGstreamerDevice() : cDevice()
{
	remove(TEMP_PATH);
	g_printerr("GstreamerDevice() : cDevice() \n");
	g_printerr("gstreamer Version %s \n" ,gst_version_string());
};// end of method

/*
 * Deconstruktor
 */
cGstreamerDevice::~cGstreamerDevice()
{
	g_printerr("~cGstreamerDevice() \n");
};// end of method

/*
 * HasDecoder
 */
bool cGstreamerDevice::HasDecoder(void) const
{
	return true;
};// end of method

/*
 * SetPlayMode
 */
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

};// end of method

/*
 * PlayVideo
 */
int cGstreamerDevice::PlayVideo(const uchar *Data, int Length)
{
	g_printerr("PlayVideo  \n");
	return Length;
};// end of method

/*
 * PlayAudio
 */
int cGstreamerDevice::PlayAudio(const uchar *Data, int Length, uchar Id)
{
	g_printerr("PlayAudio \n");
	return Length;

};// end of method

/*
 * PlayTsVideo
 */
int cGstreamerDevice::PlayTsVideo(const uchar *Data, int Length)
{
	g_printerr("PlayTsVideo \n");
	return Length;
};// end of method

/*
 * PlayTsAudio
 */
int cGstreamerDevice::PlayTsAudio(const uchar *Data, int Length)
{
	g_printerr("PlayTsAudio \n");
	return Length;
};// end of method

/*
 * PlayTsSubtitle
 */
int cGstreamerDevice::PlayTsSubtitle(const uchar *Data, int Length)
{
	g_printerr("PlayTsSubtitle \n");
	return Length;
};// end of method

/*
 * PlayPes
 */
int cGstreamerDevice::PlayPes(const uchar *Data, int Length, bool VideoOnly)
{
	g_printerr("PlayPes \n");
	return Length;
};// end of method

/*
 * push_to_buffer
 */
int cGstreamerDevice::push_to_buffer(const uchar *Data, int Length)
{
	return Length;

};// end of method

/*
 * PlayTs
 */
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
};// end of method

/*
 * Poll
 */
bool cGstreamerDevice::Poll(cPoller &Poller, int TimeoutMs)
{
	g_printerr("Poll\n");
	return true;
};// end of method

/*
 * Flush
 */
bool cGstreamerDevice::Flush(int TimeoutMs)
{
	return true;
};// end of method

/*
 * Start
 */
bool cGstreamerDevice::Start(void)
{
	return true;
};// end of method

/*
 * MakePrimaryDevice
 */
void cGstreamerDevice::MakePrimaryDevice(bool On)
{
	if (On) new cGstreamerOsdProvider();
	cDevice::MakePrimaryDevice(On);
};// end of method

/*
 * StartReplayBuffer
 */
void cGstreamerDevice::StartReplayBuffer()
{
	return;
};// end of method

/*
 * ShowOverlay
 */
void cGstreamerDevice::ShowOverlay()
{

};// end of method

/*
 * StartReplay
 */
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


