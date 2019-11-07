/******************************************************************************/
/*                                                                            */
/*  Unit Name : io.cpp                                                        */
/*                                                                            */
/*  Abstract  : Source file for functions implementing the SIMLIB_IO_CLASS.   */
/*                                                                            */
/*  Dependencies : Auto-Generated                                             */
/*                                                                            */
/*  Operating System : MS-DOS 6.2, Windows 3.1                                */
/*                                                                            */
/*  Compiler : MSVC V1.5                                                      */
/*                                                                            */
/*  Naming Conventions :                                                      */
/*                                                                            */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Private Functions   : Mixed case with no underscores                  */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Global Variables    : Mixed case with no underscores                  */
/*      Classless Functions : Mixed case with no underscores                  */
/*      Classes             : All upper case seperated by an underscore       */
/*      Defined Constants   : All upper case seperated by an underscore       */
/*      Macros              : All upper case seperated by an underscore       */
/*      Structs/Types       : All upper case seperated by an underscore       */
/*      Private Variables   : All lower case seperated by an underscore       */
/*      Public Variables    : All lower case seperated by an underscore       */
/*      Local Variables     : All lower case seperated by an underscore       */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                               */
/*                                                                            */
/******************************************************************************/
#include <windows.h>
#include <mmsystem.h>
#include <time.h>
#include "stdhdr.h"
#include "simio.h"
#include "error.h"
#include "simfile.h"
#include "f4find.h"

/*----------------------------------------------------*/
/* Memory Allocation for externals declared elsewhere */
/*----------------------------------------------------*/
SIMLIB_IO_CLASS   IO;
UINT CurrJoystick = JOYSTICKID1;


/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT SIMLIB_IO_CLASS::Init (SIM_FILE_NAME)           */
/*                                                                  */
/* Description:                                                     */
/*    Initialize the I/O subsystem and read in any old calibration  */
/*    data.                                                         */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FILE_NAME fname - pathname of the file to be opened.      */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK for success, SIMLIB_ERR for failure.                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
//SIM_INT SIMLIB_IO_CLASS::Init (char* fname)
SIM_INT SIMLIB_IO_CLASS::Init (char*)
{
SIM_INT retval = SIMLIB_ERR;
MMRESULT joyret;
JOYCAPS joycaps;

   // Calibrate the Joystick
   joyret = joyGetDevCaps (CurrJoystick, &joycaps, sizeof (JOYCAPS));

   if (joyret == JOYERR_NOERROR)
   {
      retval = SIMLIB_OK;
//      Calibrate ("config\\joystick.dat");
   }
   else if (joyret == MMSYSERR_NODRIVER)
   {
      retval = SIMLIB_OK;
      MonoPrint ("No Joystick Driver\n");
//      SimLibPrintError ("MMSYSERR No Driver");
   }
   else if (joyret == MMSYSERR_INVALPARAM)
   {
      retval = SIMLIB_OK;
      MonoPrint ("Bad Joystick Parameter\n");
      SimLibPrintError ("MMSYSERR Invalid Parameter");
   }
   return (retval);
}

SIM_INT SIMLIB_IO_CLASS::Calibrate (char* fileName)
{
int i, numAxis;
SimlibFileClass* calFile;

   calFile = SimlibFileClass::Open(fileName, SIMLIB_READ | SIMLIB_BINARY);
   if (calFile)
   {
      calFile->Read (&numAxis, sizeof(int));

	   for (i=0; i<numAxis; i++)
	   {
		   calFile->Read (&analog[i], sizeof(SIMLIB_ANALOG_TYPE));
      }
      calFile->Close ();
      delete calFile;
	   return (SIMLIB_OK);
   }
   else
   {
   JOYCAPS joycaps;

      MonoPrint ("No calibration, using default.\n");

      // Calibrate the Joystick
      joyGetDevCaps (CurrJoystick, &joycaps, sizeof (JOYCAPS));
      numAxis = joycaps.wNumAxes;
      analog[0].min = joycaps.wXmin;
      analog[0].max = joycaps.wXmax;
      analog[0].center = (analog[0].min + analog[0].max) / 2;

      analog[1].min = joycaps.wYmin;
      analog[1].max = joycaps.wYmax;
      analog[1].center = (analog[1].min + analog[1].max) / 2;

      analog[2].min = joycaps.wZmin;
      analog[2].max = joycaps.wZmax;
      analog[2].center = (analog[2].min + analog[2].max) / 2;

      analog[3].min = joycaps.wRmin;
      analog[3].max = joycaps.wRmax;
      analog[3].center = (analog[3].min + analog[3].max) / 2;
			
		analog[0].isUsed = analog[1].isUsed = TRUE;

		if (!(joycaps.wCaps & JOYCAPS_HASZ))
      {
         analog[2].isUsed = FALSE;
      }
      else
      {
         analog[2].isUsed = TRUE;
      }

      if (!(joycaps.wCaps & JOYCAPS_HASR))
      {
         analog[3].isUsed = FALSE;
      }
      else
      {
         analog[3].isUsed = TRUE;
      }

      for (i=0; i<4; i++)
      {
         if (analog[i].isUsed)
         {
            analog[i].mUp = 1.0F /(analog[i].max - analog[i].center);
            analog[i].bUp = -analog[i].mUp * analog[i].center;
            analog[i].mDown = 1.0F / (analog[i].center - analog[i].min);
            analog[i].bDown = -analog[i].mDown * analog[i].center;
         }
         else
         {
            analog[i].mUp = analog[i].mDown = 0.0F;
            analog[i].bUp = analog[i].bDown = 0.0F;
         }
      }
   }

   return SIMLIB_ERR;
}

int SIMLIB_IO_CLASS::ReadFile (void)
{
	size_t		success = 0;
	char		path[_MAX_PATH];
	long		size;
	SIMLIB_ANALOG_TYPE temp[SIMLIB_MAX_ANALOG];
	FILE *fp;

	sprintf(path,"%s\\config\\joystick.cal",FalconDataDirectory);
	
	fp = fopen(path,"rb");
	if(!fp)
		return FALSE;

	fseek(fp,0,SEEK_END);
	size = ftell(fp);
	fseek(fp,0,SEEK_SET);

	if(size != sizeof(SIMLIB_ANALOG_TYPE)*SIMLIB_MAX_ANALOG)
		return FALSE;

	success = fread(temp, sizeof(SIMLIB_ANALOG_TYPE), SIMLIB_MAX_ANALOG, fp);
	fclose(fp);
	if(success != SIMLIB_MAX_ANALOG)
		return FALSE;

	analog[0].center = temp[0].center;
	analog[1].center = temp[1].center;
	analog[2].center = temp[2].center;
	analog[3].center = temp[3].center;
	return TRUE;
}

int SIMLIB_IO_CLASS::SaveFile (void)
{
	size_t		success = 0;
	char		path[_MAX_PATH];
	FILE *fp;

	sprintf(path,"%s\\config\\joystick.cal",FalconDataDirectory);
	
	fp = fopen(path,"wb");
	if(!fp)
		return FALSE;

	success = fwrite(analog, sizeof(SIMLIB_ANALOG_TYPE), SIMLIB_MAX_ANALOG, fp);
	fclose(fp);
	if(success != SIMLIB_MAX_ANALOG)
		return FALSE;

	return TRUE;
}

SIM_INT SIMLIB_IO_CLASS::Calibrate (int stepNum)
{
int i, numAxis = 4;
SimlibFileClass* calFile = NULL;
MMRESULT joyret;
JOYCAPS joycaps;
int retval = SIMLIB_ERR;

   // Calibrate the Joystick
   joyret = joyGetDevCaps (CurrJoystick, &joycaps, sizeof (JOYCAPS));

   if (joyret == JOYERR_NOERROR)
   {
      retval = SIMLIB_OK;

      switch (stepNum)
      {
         case 0:
            MonoPrint ("Center the joystick, throttle, and rudder and push a button\n");
            digital[0] = digital[1] = FALSE;
            analog[0].mUp = analog[0].mDown = analog[0].bUp = analog[0].bDown = 0.0F;
            analog[1].mUp = analog[1].mDown = analog[1].bUp = analog[1].bDown = 1.1F;
            analog[2].mUp = analog[2].mDown = analog[2].bUp = analog[2].bDown = 2.2F;
            analog[3].mUp = analog[3].mDown = analog[3].bUp = analog[3].bDown = 3.3F;

            while (digital[0] || digital[1])
               ReadAllAnalogs();

            while (!(digital[0] || digital[1]))
            {
               ReadAllAnalogs();
               analog[0].center = analog[0].ioVal;
               analog[1].center = analog[1].ioVal;
               analog[2].center = analog[2].ioVal;
               analog[3].center = analog[3].ioVal;
            }
			
			analog[0].isUsed = analog[1].isUsed = TRUE;

			if (!(joycaps.wCaps & JOYCAPS_HASZ))
            {
               analog[2].isUsed = FALSE;
               analog[2].max = 0;
               analog[2].min = -1;
               analog[2].engrValue = 1.0F;
            }
            else
            {
               analog[2].isUsed = TRUE;
            }

            if (!(joycaps.wCaps & JOYCAPS_HASR))
            {
               analog[3].isUsed= FALSE;
               analog[3].max = 1;
               analog[3].min = -1;
               analog[3].engrValue = 0.0F;
            }
            else
            {
               analog[3].isUsed = TRUE;
            }
         break;

         case 1:
            MonoPrint ("Move the joystick to the corners and push a button\n");

            while (digital[0] || digital[1])
               ReadAllAnalogs();

            while (!(digital[0] || digital[1]))
            {
               ReadAllAnalogs();
               analog[0].max = max(analog[0].max, analog[0].ioVal);
               analog[1].max = max(analog[1].max, analog[1].ioVal);
               analog[0].min = min(analog[0].min, analog[0].ioVal);
               analog[1].min = min(analog[1].min, analog[1].ioVal);
            }
         break;

         case 2:
            MonoPrint ("Move the throttle to the ends and push a button\n");

            while (digital[0] || digital[1])
               ReadAllAnalogs();

            while (!(digital[0] || digital[1]))
            {
               ReadAllAnalogs();
               analog[2].max = max(analog[2].max, analog[2].ioVal);
               analog[2].min = min(analog[2].min, analog[2].ioVal);
            }
         break;

         case 3:
            MonoPrint ("Move the rudder to the ends and push a button\n");

            while (digital[0] || digital[1])
               ReadAllAnalogs();

            while (!(digital[0] || digital[1]))
            {
               ReadAllAnalogs();
               analog[3].max = max(analog[3].max, analog[3].ioVal);
               analog[3].min = min(analog[3].min, analog[3].ioVal);
            }
			break;

		 case 4:

            for (i=0; i<4; i++)
            {
               if (analog[i].isUsed)
               {
                  analog[i].mUp = 1.0F /(analog[i].max - analog[i].center);
                  analog[i].bUp = -analog[i].mUp * analog[i].center;
                  analog[i].mDown = 1.0F / (analog[i].center - analog[i].min);
                  analog[i].bDown = -analog[i].mDown * analog[i].center;
               }
               else
               {
                  analog[i].mUp = analog[i].mDown = 0.0F;
                  analog[i].bUp = analog[i].bDown = 0.0F;
               }
            }

            calFile = SimlibFileClass::Open("config\\joystick.dat", SIMLIB_WRITE | SIMLIB_BINARY);
            if (calFile)
            {
               calFile->Write (&numAxis, sizeof(int));

	            for (i=0; i<numAxis; i++)
	            {
		            calFile->Write (&analog[i], sizeof(SIMLIB_ANALOG_TYPE));
               }
               calFile->Close ();
               delete calFile;
            }
         break;
      }
   }
   else if (joyret == MMSYSERR_NODRIVER)
   {
      SimLibPrintError ("MMSYSERR No Driver");
	  return SIMLIB_ERR;
   }
   else if (joyret == MMSYSERR_INVALPARAM)
   {
      SimLibPrintError ("MMSYSERR Invalid Parameter");
	  return SIMLIB_ERR;
   }

   return retval;
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_IO_CLASS::ReadAnalog  (SIM_INT)        */
/*                                                                  */
/* Description:                                                     */
/*    Get the value of a particular analog input channel            */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_INT id - I/O Channel number                               */
/*                                                                  */
/* Outputs:                                                         */
/*    Scaled output value for the desired channel                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_IO_CLASS::ReadAnalog  (SIM_INT id)
{
        return (analog[id].engrValue);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT SIMLIB_IO_CLASS::ReadDigital (SIM_INT)          */
/*                                                                  */
/* Description:                                                     */
/*    Get the value of a particular digital input                   */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_INT id - channel to read                                  */
/*                                                                  */
/* Outputs:                                                         */
/*    TRUE if pushed, FALSE it not.                                 */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SIMLIB_IO_CLASS::ReadDigital (SIM_INT id)
{
        return (digital[id]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT SIMLIB_IO_CLASS::WriteAnalog  (SIM_INT,         */
/*            SIM_FLOAT)                                            */
/*                                                                  */
/* Description:                                                     */
/*    Set an analog ouput value.                                    */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_INT id    - Analog output channel to write to             */
/*    SIM_FLOAT val - Value to output                               */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK for success, SIMLIB_ERR for failure.                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SIMLIB_IO_CLASS::WriteAnalog  (SIM_INT id, SIM_FLOAT val)
{
    if (val)
        return (id);
    else
        return (SIMLIB_ERR);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT SIMLIB_IO_CLASS::WriteAnalog  (SIM_INT,         */
/*            SIM_INT)                                              */
/*                                                                  */
/* Description:                                                     */
/*    Set an analog ouput value.                                    */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_INT id  - Digital output channel to write to              */
/*    SIM_INT val - Value to output                                 */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK for success, SIMLIB_ERR for failure.                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SIMLIB_IO_CLASS::WriteDigital (SIM_INT id, SIM_INT val)
{
    if (val)
        return (id);
    else
        return (SIMLIB_ERR);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT SIMLIB_IO_CLASS::ReadAllAnalogs (void)          */
/*                                                                  */
/* Description:                                                     */
/*    Read all the configured analog input devices.                 */
/*    NOTE:  Beacause windows is weird Digital I/O is done here too */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK for success, SIMLIB_ERR for failure.                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SIMLIB_IO_CLASS::ReadAllAnalogs (void)
{
int i;
SIM_INT retval = SIMLIB_ERR;
JOYINFOEX joyinfoex;
UINT joyret;

   joyinfoex.dwFlags = JOY_RETURNALL;
   joyinfoex.dwSize = sizeof (JOYINFOEX);
   joyret = joyGetPosEx(CurrJoystick, &joyinfoex);

   switch (joyret)
   {
      case JOYERR_NOERROR:
         retval = SIMLIB_OK;
         analog[0].ioVal = (SIM_LONG)joyinfoex.dwXpos;
         analog[1].ioVal = (SIM_LONG)joyinfoex.dwYpos;
         analog[2].ioVal = (SIM_LONG)joyinfoex.dwZpos;
         analog[3].ioVal = (SIM_LONG)joyinfoex.dwRpos;

		povHatAngle[0] = joyinfoex.dwPOV; // VWF POV Kludge 11/24/97

		//read ALL the joystick buttons DSP 2/26/98
		for(i=0;i<SIMLIB_MAX_DIGITAL;i++)
			digital[i] = (joyinfoex.dwButtons & (JOY_BUTTON1<<i)) && TRUE;
			

         // Do buttons here because this is screwed up;
         //digital[0] = (joyinfoex.dwButtons & JOY_BUTTON1) && TRUE;
         //digital[1] = (joyinfoex.dwButtons & JOY_BUTTON2) && TRUE;

         for (i=0; i<SIMLIB_MAX_ANALOG; i++)
         {
            if (analog[i].ioVal > analog[i].center)
               analog[i].engrValue = analog[i].ioVal * analog[i].mUp + analog[i].bUp;
            else
               analog[i].engrValue = analog[i].ioVal * analog[i].mDown + analog[i].bDown;
         }
      break;

      case MMSYSERR_NODRIVER:
      case JOYERR_PARMS:
         analog[0].engrValue = 0.0F;
         analog[1].engrValue = 0.0F;
         analog[2].engrValue = 0.0F;
         analog[3].engrValue = 0.0F;

			povHatAngle[0] = (unsigned long)-1;

         SimLibErrno = EACCESS;
      break;

      case JOYERR_UNPLUGGED:
         SimLibErrno = ENOTFOUND;
         analog[0].engrValue = 0.0F;
         analog[1].engrValue = 0.0F;
         analog[2].engrValue = 0.0F;
         analog[3].engrValue = 0.0F;

			povHatAngle[0] = (unsigned long)-1;
      break;
   }

   return (retval);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT SIMLIB_IO_CLASS::ReadAllDigitals (void)         */
/*                                                                  */
/* Description:                                                     */
/*    Read all the configured digital input devices.                */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK for success, SIMLIB_ERR for failure.                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SIMLIB_IO_CLASS::ReadAllDigitals (void)
{
#ifndef _WINDOWS_
int i, value;

#ifdef OLD_ACM
        OUT_BYTE(GAMEPORT,0);
        value = ~IN_BYTE(GAMEPORT);
#else
	OUT_BYTE(GAMEPORT,4);
	value = ~IN_BYTE(GAMEPORT);
#endif

        for (i=0; i<SIMLIB_MAX_DIGITAL; i++)
                if (value & (0x10 << i))
                        digital[i] = TRUE;
                else
                        digital[i] = FALSE;
#endif

        return (SIMLIB_OK);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT SIMLIB_IO_CLASS::WriteAllAnalogs (void)         */
/*                                                                  */
/* Description:                                                     */
/*    Write all analog data out to the appropriate channel.  At     */
/*    this time no hardware exists to support this, but it might.   */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK for success, SIMLIB_ERR for failure.                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SIMLIB_IO_CLASS::WriteAllAnalogs (void)
{
        return (SIMLIB_OK);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT SIMLIB_IO_CLASS::WriteAllDigitals (void)        */
/*                                                                  */
/* Description:                                                     */
/*    Write all digital data out to the appropriate channel.  At    */
/*    this time no hardware exists to support this, but it might.   */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK for success, SIMLIB_ERR for failure.                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SIMLIB_IO_CLASS::WriteAllDigitals (void)
{
        return (SIMLIB_OK);
}
