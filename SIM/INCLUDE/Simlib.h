/******************************************************************************/
/*                                                                            */
/*  Unit Name : simlib.h                                                      */
/*                                                                            */
/*  Abstract  : Header file with prototypes and defines used as part of the   */
/*              simulation infrastructure.                                    */
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
#ifndef _SIMTYPE_H
#define _SIMTYPE_H

/*-----------------------------------*/
/* Defined constants used throughout */
/*-----------------------------------*/
#define SIMLIB_OK                      0
#define SIMLIB_ERR                     -1
#define SIMLIB_NULL                    0

/*---------------------------------------------------------------------*/
/* These types are defined for use in all interfaces external to       */
/* a module. Inside a given module these types do not need to be used. */
/*---------------------------------------------------------------------*/
typedef int    SIM_INT;
typedef unsigned int    SIM_UINT;
typedef unsigned long	SIM_ULONG;
typedef long   SIM_LONG;
typedef void   SIM_VOID;
typedef float  SIM_FLOAT;
typedef short  SIM_SHORT;
typedef double SIM_DOUBLE;

/*-----------------------------------*/
/* Debug Utility Function Prototypes */
/*-----------------------------------*/
SIM_INT SimLibPrintError (char *fmt, ...);
SIM_INT SimLibPrintMessage (char *fmt, ...);

/*---------*/
/* Globals */
/*---------*/
extern float  SimLibMinorFrameTime;
extern float  SimLibMinorFrameRate;
extern float  SimLibMajorFrameTime;
extern float  SimLibMajorFrameRate;
extern float  SimLibTimeOfDay;
extern VU_TIME  SimLibElapsedTime;
extern unsigned int   SimLibFrameCount;
extern int    SimLibMinorPerMajor;
extern char debstr[10][120];

#endif
