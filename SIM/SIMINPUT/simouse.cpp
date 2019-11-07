#include "F4Thread.h"
#include "sinput.h"
#include "cpmanager.h"
#include "dispcfg.h"
#include "simdrive.h"
#include "dispopts.h"

int		gxFuzz;
int		gyFuzz;
int		gxPos;
int		gyPos;
int		gxLast;
int		gyLast;
int		gMouseSensitivity;
int		gSelectedCursor;
VU_TIME	gTimeLastMouseMove; 
static int didSpin = 0;
static int didTilt = 0;

extern bool g_bRealisticAvionics;

//***********************************
//	void OnSimMouseInput()
//***********************************

//void OnSimMouseInput(HWND hWnd)
void OnSimMouseInput(HWND)
{
	DIDEVICEOBJECTDATA	ObjData[DMOUSE_BUFFERSIZE];
	DWORD				dwElements=0;
	HRESULT				hResult;
	UINT				i=0;
	int					dx = 0, dy = 0;
	int					action=0;
	BOOL				passThru = TRUE;						
   static BOOL          oneDown = FALSE;

	dwElements = DMOUSE_BUFFERSIZE;
	hResult = gpDIDevice[SIM_MOUSE]->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), ObjData, &dwElements, 0);

	if(hResult == DIERR_INPUTLOST){
      gpDeviceAcquired[SIM_MOUSE] = FALSE;
      return;
	}

	if(SUCCEEDED(hResult)){

		gTimeLastMouseMove = vuxRealTime;

		for(i = 0; i < dwElements; i++){

			if(ObjData[i].dwOfs == DIMOFS_X) {
			   dx			+= ObjData[i].dwData;
				action	= CP_MOUSE_MOVE;
			}
			else if(ObjData[i].dwOfs == DIMOFS_Y) {
				dy			+= ObjData[i].dwData; 
				action	= CP_MOUSE_MOVE;
			}
			else if(ObjData[i].dwOfs == DIMOFS_BUTTON0 && !(ObjData[i].dwData & 0x80)) {
				action	= CP_MOUSE_BUTTON0;
			}
			else if(ObjData[i].dwOfs == DIMOFS_BUTTON1 && !(ObjData[i].dwData & 0x80)) {
				action	= CP_MOUSE_BUTTON1;
				oneDown = FALSE;
			}
			else if(ObjData[i].dwOfs == DIMOFS_BUTTON1 && (ObjData[i].dwData & 0x80)) {
				action	= static_cast<unsigned long>(-1);
				oneDown = TRUE;
			}
			else {
				continue;
			}

         if (action == CP_MOUSE_BUTTON0 || action == CP_MOUSE_BUTTON1) {
            passThru = OTWDriver.HandleMouseClick (gxPos, gyPos);
			}

			if (passThru) {
			   gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(action, gxPos, gyPos);
         }
      }

		//MI don't move the head with the right mousebutton down
		if(g_bRealisticAvionics)
			oneDown = FALSE;
	// DO the mouse move if one happened
		if (dx || dy)
		{
			if (oneDown)
			{
				// Head pan in virtual cockpit
				if (dx > 3)
				{
					didSpin = TRUE;
					OTWDriver.ViewSpinRight();
				}
				else if (dx < -3)
				{
					didSpin = TRUE;
					OTWDriver.ViewSpinLeft();
				}
				else
				{
					didSpin = FALSE;
					OTWDriver.ViewSpinHold();
				}

				if (dy < -3)
				{
					didTilt = TRUE;
					OTWDriver.ViewTiltUp();
				}
				else if (dy > 3)
				{
					didTilt = TRUE;
					OTWDriver.ViewTiltDown();
				}
				else
				{
					didTilt = FALSE;
					OTWDriver.ViewTiltHold();
				}
			}
			else
			{
				if (didTilt)
				{
					didTilt = FALSE;
					OTWDriver.ViewTiltHold();
				}

				if (didSpin)
				{
					didSpin = FALSE;
					OTWDriver.ViewSpinHold();
				}

				// Update cursor position otherwise
				UpdateCursorPosition(dx, dy);
			}
		}
		else
		{
			if (didTilt)
			{
				didTilt = FALSE;
				OTWDriver.ViewTiltHold();
			}

			if (didSpin)
			{
				didSpin = FALSE;
				OTWDriver.ViewSpinHold();
			}
		}
	}
}

//*******************************************************************
//	void UpdateCursorPosition()
//
//	Move our private cursor in the requested direction, subject to
//	clipping, scaling and all that other stuff.
//
//	This does not redraw the cursor.  That is done in the OTW routines
//*******************************************************************

void UpdateCursorPosition(DWORD xOffset, DWORD yOffset)
{

	// Pick up any leftover fuzz from last time.  This is importatnt
	// when scaling dow mouse motions.  Otherwise, the user can drag to
	// the right extremely slow tfor the length of the table and not
	// get anywhere.

	xOffset += gxFuzz;	gxFuzz = 0;
	yOffset += gyFuzz;	gyFuzz = 0;

	switch(gMouseSensitivity){

		case LO_SENSITIVITY:
			gxFuzz = xOffset % 2;	// Remember the fuzz for next time
			gyFuzz = yOffset % 2;
			
			xOffset /= 2;
			yOffset /= 2;
		break;

		case NORM_SENSITIVITY:		// No Adjustments needed
		default:
		break;
		
		case HI_SENSITIVITY:
			xOffset *= 2;				// Magnify
			yOffset *= 2;
		break;
	}

	gxLast = gxPos;
	gyLast = gyPos;

	gxPos += xOffset;
	gyPos += yOffset;

	// Constrain the cursor to the client area
	if(gxPos < 0){
		gxPos = 0;
	}
	if(gxPos >= DisplayOptions.DispWidth){
		gxPos = DisplayOptions.DispWidth - 1;
	}
	if(gyPos < 0){
		gyPos = 0;
	}
	if(gyPos >= DisplayOptions.DispHeight){
		gyPos = DisplayOptions.DispHeight - 1;
	}
}
