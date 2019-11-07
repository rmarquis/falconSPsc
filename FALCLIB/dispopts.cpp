#include <stdio.h>
#include <stdlib.h>
#include "dispopts.h"
#include "f4find.h"
#include "graphics\include\devmgr.h"
#include "dispcfg.h"
#include "Debuggr.h"

DisplayOptionsClass DisplayOptions;

DisplayOptionsClass::DisplayOptionsClass(void)
{
	Initialize();
}

void DisplayOptionsClass::Initialize(void)
{
	//DispWidth = 640;
	//DispHeight = 480;
	DispWidth = 1024; // JB 011124
	DispHeight = 768; // JB 011124
	DispDepth = 16;	// OW
	DispVideoCard = 0;
	DispVideoDriver = 0;

	FalconDisplay.SetSimMode(DispWidth, DispHeight, DispDepth);	// OW
}

int DisplayOptionsClass::LoadOptions(char *filename)
{
	DWORD size;
	FILE *fp;
	size_t		success = 0;
	char		path[_MAX_PATH];

	sprintf(path,"%s\\config\\%s.dsp",FalconDataDirectory,filename);
	fp = fopen(path,"rb");
	if(!fp)
	{
		MonoPrint("Couldn't open display options\n");
		return FALSE;
	}

	fseek(fp,0,SEEK_END);
	size = ftell(fp);
	fseek(fp,0,SEEK_SET);

	if(size != sizeof(class DisplayOptionsClass))
	{
		MonoPrint("Display options are in old file format\n",filename);
		return FALSE;
	}

	success = fread(this, sizeof(class DisplayOptionsClass), 1, fp);
	fclose(fp);
	if(success != 1)
	{
		MonoPrint("Failed to read display options\n",filename);
		Initialize();
		return FALSE;
	}

	const char	*buf;
	int		i = 0;

	// Make sure the chosen sim video driver is still legal
	buf = FalconDisplay.devmgr.GetDriverName(i);
	while(buf)
	{
		i++;
		buf = FalconDisplay.devmgr.GetDriverName(i);
	}

	if(i<=DispVideoDriver)
	{
		DispVideoDriver = 0;
		DispVideoCard = 0;
	}

	// Make sure the chosen sim video device is still legal
	buf = FalconDisplay.devmgr.GetDeviceName(DispVideoDriver, i);
	while(buf)
	{
		i++;
		buf = FalconDisplay.devmgr.GetDeviceName(DispVideoDriver, i);
	}

	if(i<=DispVideoCard)
	{
		DispVideoDriver = 0;
		DispVideoCard = 0;
	}

	FalconDisplay.SetSimMode(DispWidth, DispHeight, DispDepth);

	return TRUE;
}

int DisplayOptionsClass::SaveOptions(void)
{
	FILE *fp;
	size_t		success = 0;
	char		path[_MAX_PATH];

	sprintf(path,"%s\\config\\display.dsp",FalconDataDirectory);
		
	if((fp = fopen(path,"wb")) == NULL)
	{
		MonoPrint("Couldn't save display options");
		return FALSE;
	}
	success = fwrite(this, sizeof(class DisplayOptionsClass), 1, fp);
	fclose(fp);
	if(success == 1)
		return TRUE;

	return FALSE;
}