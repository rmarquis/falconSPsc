#ifndef _INPFUNC_H
#define _INPFUNC_H

#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>
#include <tchar.h>
#include "playerop.h"


#define STARTDAT_MOUSE_LEFT 0
#define STARTDAT_MOUSE_RIGHT 1

#define MAX_POV_DIR 8

typedef void (*InputFunctionType)(unsigned long, int state, void *);

typedef struct
{
	InputFunctionType func;		  //this is the pointer to the appropriate function
	int				  cpButtonID; //this is the ID of the associated cockpit button
}joyButton;

typedef struct
{
	InputFunctionType func[MAX_POV_DIR];	  //this is the pointer to the appropriate function
	int		 cpButtonID[MAX_POV_DIR]; //this is the ID of the associated cockpit button
}POVfunc;

struct FunctionPtrListEntry
{
	InputFunctionType theFunc;
	int buttonId;
	int mouseSide;
	int flags;
	long controlID;
	struct FunctionPtrListEntry* next;
};

class InputFunctionHashTable
{
   public:
      enum {NumHashEntries = 256 /*DIK_APPS +1*/,  NumButtons = 32, NumPOVs = 4};

   private:
      struct FunctionPtrListEntry* functionTable[NumHashEntries];
	  joyButton  buttonTable[NumButtons];
	  POVfunc	 POVTable[NumPOVs];

   public:
      InputFunctionHashTable(void);
      ~InputFunctionHashTable(void);
      void AddFunction (int key, int flags, int buttonId, int mouseSide, InputFunctionType funcPtr);
      void RemoveFunction (int key, int flags);
      void ClearTable(void);
      InputFunctionType GetFunction (int key, int flags, int* pbuttonId, int* pmouseSide);
	  long GetControl(int key, int flags);
	  BOOL SetControl(int key, int flags, long control);
	  BOOL SetButtonFunction(int buttonId, InputFunctionType theFunc, int cpButtonID );
	  InputFunctionType GetButtonFunction(int buttonID, int* cpButtonID);
	  BOOL SetPOVFunction(int POV, int dir, InputFunctionType theFunc, int cpButtonID );
	  InputFunctionType GetPOVFunction(int POV, int dir, int* cpButtonID);
	
};

extern InputFunctionHashTable UserFunctionTable;

extern int CommandsKeyCombo;
extern int CommandsKeyComboMod;

void SetupInputFunctions (void);
void CleanupInputFunctions (void);
void CallInputFunction (unsigned long val, int state, void *);
void LoadFunctionTables(_TCHAR *fname = PlayerOptions.GetKeyfile());
InputFunctionType FindFunctionFromString (char* str);
char * FindStringFromFunction(InputFunctionType func);

#endif
