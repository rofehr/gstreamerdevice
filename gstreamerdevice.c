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


#include "cGstreamerDevice.h"
#include "filebrowser.h"
#include "gst_service.h"

#include <X11/Xlib.h>

static const char *VERSION        = "0.0.2";
static const char *DESCRIPTION    = "gstreamer Output device";
static const char *MAINMENUENTRY  = "gstreamerdevice";

using std::string;




class cPluginGstreamerdevice : public cPlugin {
private:

    cGstreamerDevice *player;
    
    bool IsIsoImage(char* Filename);

    void PlayFileHandleType(char* Filename, bool Shuffle=false);


public:
    cPluginGstreamerdevice(void) {};
    
    virtual ~cPluginGstreamerdevice() {};
    
    virtual const char *Version(void) {
        return VERSION;
    };
    
    virtual const char *Description(void) {
        return DESCRIPTION;
    };
    
    virtual const char *CommandLineHelp(void) {
        return NULL;
    };
    
    bool ProcessArgs(int argc, char *argv[])
    {
        return true;
    };// end of method

    virtual bool Initialize(void);

    bool Start(void)
    {
        return true;

    };// end of method

    virtual void Housekeeping(void) {};
    
    virtual const char *MainMenuEntry(void) {
        return "GST-Player";
    };

    virtual cOsdObject *MainMenuAction(void);

    
    virtual cMenuSetupPage *SetupMenu(void) {
        return NULL;
    };
    
    virtual bool SetupParse(const char *Name, const char *Value) {
        return false;
    };
    
    virtual bool Service(const char *Id, void *Data = NULL);
};

cOsdObject *cPluginGstreamerdevice::MainMenuAction(void)
{
    return new cFilebrowser("/", "");
}// end of method

bool cPluginGstreamerdevice::Initialize(void)
{
    g_printerr("cPluginGstreamerdevice:Initialize(): gstreamerdevice \n");

    player = new cGstreamerDevice();

    return true;
}// end of method

bool cPluginGstreamerdevice::Service(const char *id, void *data)

{

  if (strcmp(id, "Gst_Seek") == 0)

  {

/*
    Mpv_Seek *seekInfo = (Mpv_Seek *)data;



    cMpvControl* control = dynamic_cast<cMpvControl*>(cControl::Control(true));

		if(control) {

      if(seekInfo->SeekRelative != 0)

      {

          control->SeekRelative(seekInfo->SeekRelative);

      }

      else if(seekInfo->SeekAbsolute >= 0)

      {

        control->SeekTo(seekInfo->SeekAbsolute);

      }

		}

*/
    return true;

  }

  if (strcmp(id, "Gst_PlayFile") == 0)

  {

    Gst_PlayFile *playFile = (Gst_PlayFile *)data;

    PlayFileHandleType(playFile->Filename);

    return true;

  }

  if (strcmp(id, "Gst_PlaylistShuffle") == 0)

  {

      
    Gst_PlaylistShuffle *shuffleFile = (Gst_PlaylistShuffle *)data;

    PlayFileHandleType(shuffleFile->Filename, true);

    
    return true;

  }

  if (strcmp(id, "Gst_SetTitle") == 0)

  {

/*      
    Mpv_SetTitle *setTitle = (Mpv_SetTitle *) data;

    MpvPluginConfig->TitleOverride = setTitle->Title;

*/    
    return true;

  }

  
  if (strcmp(id, GST_START_PLAY_SERVICE) == 0)

  {

    Gst_StartPlayService_v1_0_t *r = (Gst_StartPlayService_v1_0_t *) data;

    PlayFileHandleType(r->Filename);

    return true;

  }

/*  
  if (strcmp(id, GST_SET_TITLE_SERVICE) == 0)

  {

    Gst_SetTitleService_v1_0_t *r = (Gst_SetTitleService_v1_0_t *) data;

    MpvPluginConfig->TitleOverride = r->Title;

    return true;

  }

*/  
  return false;

}
// end of method

bool cPluginGstreamerdevice::IsIsoImage(char* Filename)

{

/*
  vector<string> IsoExtensions;

  IsoExtensions.push_back("bin");

  IsoExtensions.push_back("dvd");

  IsoExtensions.push_back("img");

  IsoExtensions.push_back("iso");

  IsoExtensions.push_back("mdf");

  IsoExtensions.push_back("nrg");

  for (unsigned int i=0;i<IsoExtensions.size();i++)

  {

    if (Filename.substr(Filename.find_last_of(".") + 1) == IsoExtensions[i])

      return true;

  }


*/
  return false;

}
// end of method


void cPluginGstreamerdevice::PlayFileHandleType(char* Filename, bool Shuffle)

{
/*
  if (IsIsoImage(Filename)) // handle dvd iso images

  {

    Filename = "dvd://menu/" + Filename;

  }

*/  
  player->ReplayPlayFile(Filename );

}
// end of method


VDRPLUGINCREATOR(cPluginGstreamerdevice); // Don't touch this!
