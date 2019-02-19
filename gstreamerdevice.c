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



#include <X11/Xlib.h>

static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "gstreamer Output device";
static const char *MAINMENUENTRY  = "gstreamerdevice";

class cPluginGstreamerdevice : public cPlugin {
private:
public:
    cPluginGstreamerdevice(void) {}
    virtual ~cPluginGstreamerdevice() {}
    virtual const char *Version(void) {
        return VERSION;
    }
    virtual const char *Description(void) {
        return DESCRIPTION;
    }
    virtual const char *CommandLineHelp(void) {
        return NULL;
    }
    bool ProcessArgs(int argc, char *argv[])
    {
        return true;
    };// end of method

    virtual bool Initialize(void);

    bool Start(void)
    {
        return true;

    };// end of method

    virtual void Housekeeping(void) {}
    virtual const char *MainMenuEntry(void) {
        return NULL;
    }
    virtual cOsdObject *MainMenuAction(void) {
        return NULL;
    }
    virtual cMenuSetupPage *SetupMenu(void) {
        return NULL;
    }
    virtual bool SetupParse(const char *Name, const char *Value) {
        return false;
    };
};

bool cPluginGstreamerdevice::Initialize(void)
{
    g_printerr("cPluginGstreamerdevice:Initialize(): gstreamerdevice \n");

    new cGstreamerDevice();

    return true;
}



VDRPLUGINCREATOR(cPluginGstreamerdevice); // Don't touch this!
