/******************************************************************************/
/*                                                                            */
/*  Unit Name : simio.h                                                       */
/*                                                                            */
/*  Abstract  : Header file with class definition for SIMLIB_IO_CLASS and     */
/*              defines used in its implementation.                           */
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
#ifndef _SIMIO_H
#define _SIMIO_H

#define SIMLIB_MAX_ANALOG              4
#define SIMLIB_MAX_DIGITAL             32
#define SIMLIB_MAX_POV				   4
#define SIMLIB_ANALOG_IN               0
#define SIMLIB_ANALOG_OUT              1
#define SIMLIB_ANALOG_BIPOLAR          0
#define SIMLIB_ANALOG_UNIPOLAR         1
#define SIMLIB_NUM_CALIBRATION_STEPS   5

typedef struct
{
   SIM_LONG  min;
   SIM_LONG  max;
   SIM_LONG  center;
   SIM_LONG  ioVal;
   SIM_INT   direction;
   SIM_INT   type;
   SIM_INT   isUsed;
   SIM_FLOAT mUp, mDown;
   SIM_FLOAT bUp, bDown;
   SIM_FLOAT engrValue;
} SIMLIB_ANALOG_TYPE;

class SIMLIB_IO_CLASS
{
   private:
   public:
      SIMLIB_ANALOG_TYPE analog[SIMLIB_MAX_ANALOG];
      SIM_SHORT digital[SIMLIB_MAX_DIGITAL];
      SIM_INT analog_mask;
	  DWORD povHatAngle[SIMLIB_MAX_POV];	// VWF POV Kludge 11/24/97

   public:
      SIM_INT Init (char *fname);
      SIM_INT Calibrate (char *fname);
      SIM_INT Calibrate (int stepNum);
      SIM_FLOAT ReadAnalog  (SIM_INT id);
      SIM_INT ReadDigital (SIM_INT id);
      SIM_INT WriteAnalog  (SIM_INT id, SIM_FLOAT val);
      SIM_INT WriteDigital (SIM_INT id, SIM_INT val);
      SIM_INT ReadAllAnalogs  (void);
      SIM_INT ReadAllDigitals (void);
      SIM_INT WriteAllAnalogs  (void);
      SIM_INT WriteAllDigitals (void);
      SIM_INT AnalogIsUsed(SIM_INT id) {return (analog[id].isUsed);};
      void    SetAnalogIsUsed(SIM_INT id, int flag) {analog[id].isUsed = flag;};

	  int ReadFile (void);
	  int SaveFile (void);
};

extern SIMLIB_IO_CLASS   IO;
#endif
