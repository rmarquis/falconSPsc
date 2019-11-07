#include "falclib.h"
#include "chandler.h"
#include "userids.h"
#include "PlayerOp.h"
#include <mmsystem.h>
#include "sim\include\stdhdr.h"
#include "sim\include\simio.h"
#include "ui_setup.h"
#include <tchar.h>
#include "sim\include\inpFunc.h"
#include "sim\include\commands.h"
#include "f4find.h"
#include "sim\include\sinput.h"

//temporary until logbook is working
#include "uicomms.h"

#pragma warning (disable : 4706)	// assignment within conditional expression

extern C_Handler *gMainHandler;
extern C_Parser *gMainParser;
extern char **KeyDescrips;  
extern long Cluster;
extern int g_nThrottleID;		// OW
extern bool g_bEmptyFilenameFix; // 2002-04-18 MN

#define SECOND_KEY_MASK 0xFFFF00
#define MOD2_MASK 0x0000FF
#define KEY1_MASK 0x00FF00
#define MOD1_MASK 0xFF0000

typedef struct
{
	InputFunctionType	func;
	int					buttonId;
	int					mouseSide;
	int					key2;
	int					mod2;
	int					key1;
	int					mod1;
	int					editable;
	char				descrip[_MAX_PATH];
}KeyMap;

typedef struct
{
	int		X;
	int		Y;
	int		W;
	int		H;
}HotSpotStruct;

enum{
	KEY2,
	FLAGS,
	BUTTON_ID,
	MOUSE_SIDE,
	EDITABLE,
	FUNCTION_PTR,
};

KeyVars KeyVar = {FALSE,0,0,0,0,FALSE,FALSE};
KeyMap	UndisplayedKeys[300] = {NULL,0,0,0,0,0,0,0};
int	NumUndispKeys = 0;
int	NumDispKeys = 0;
extern int NoRudder;
extern long mHelmetIsUR; // hack for Italian 1.06 version - Only gets set to true IF UR Helmet detected
extern long mHelmetID;

extern int setABdetent;

extern int NumHats;
//CalibrateStruct Calibration = {FALSE,1,0,TRUE};

//defined in this file
int CreateKeyMapList( C_Window *win);
//SIM_INT Calibrate ( void );


//defined in another file
void InitKeyDescrips(void);
void CleanupKeys(void);
void SetDeleteCallback(void (*cb)(long,short,C_Base*));
void SaveAFile(long TitleID,_TCHAR *filespec,_TCHAR *excludelist[],void (*YesCB)(long,short,C_Base*),void (*NoCB)(long,short,C_Base*), _TCHAR *filename);
void LoadAFile(long TitleID,_TCHAR *filespec,_TCHAR *excludelist[],void (*YesCB)(long,short,C_Base*),void (*NoCB)(long,short,C_Base*));
void CloseWindowCB(long ID,short hittype,C_Base *control);
void AreYouSure(long TitleID,long MessageID,void (*OkCB)(long,short,C_Base*),void (*CancelCB)(long,short,C_Base*));
void AreYouSure(long TitleID,_TCHAR *text,void (*OkCB)(long,short,C_Base*),void (*CancelCB)(long,short,C_Base*));
void SetJoystickCenter(void);
void DelSTRFileCB(long ID,short hittype,C_Base *control);
void DelDFSFileCB(long ID,short hittype,C_Base *control);
void DelLSTFileCB(long ID,short hittype,C_Base *control);
void DelCamFileCB(long ID,short hittype,C_Base *control);
void DelTacFileCB(long ID,short hittype,C_Base *control);
void DelTGAFileCB(long ID,short hittype,C_Base *control);
void DelVHSFileCB(long ID,short hittype,C_Base *control);
void DelKeyFileCB(long ID,short hittype,C_Base *control);

///////////////
////ControlsTab
///////////////

void HideKeyStatusLines(C_Window *win)
{
	if(!win)
		return;

	C_Line *line;
	C_Button *button;
	int count = 1;

	line = (C_Line *)win->FindControl(KEYCODES - count);
	while(line)
	{
		button = (C_Button *)win->FindControl(KEYCODES + count);
		if(!button || button->GetUserNumber(EDITABLE) != -1 )
		{
			line->SetFlagBitOn(C_BIT_INVISIBLE);
			line->Refresh();
		}

		line = (C_Line *)win->FindControl(KEYCODES - ++count);
	}
}

void SetButtonColor(C_Button *button)
{
	if(button->GetUserNumber(EDITABLE) < 1)
		button->SetFgColor(0,RGB(0,255,0)); //green
	else
	{
		button->SetFgColor(0,RGB(230,230,230)); //white
	}
	button->Refresh();
}

void RecenterJoystickCB(long,short hittype,C_Base *)
{
	if((hittype != C_TYPE_LMOUSEUP))
		return;

	SetJoystickCenter();
}

void SetABDetentCB(long,short hittype,C_Base *)
{
	if((hittype != C_TYPE_LMOUSEUP))
		return;

	setABdetent = TRUE;
}

void RefreshJoystickCB(long,short,C_Base *)
{
	
	static SIM_FLOAT JoyXPrev,JoyYPrev,RudderPrev,ThrottlePrev,ABDetentPrev;
	static DWORD ButtonPrev[SIMLIB_MAX_DIGITAL],POVPrev;
	static int Initialize = 1, state = 1;
	C_Bitmap	*bmap;
	C_Window	*win;
	C_Line		*line;
	C_Button	*button;
	
	
	GetJoystickInput();
	
	
		win=gMainHandler->FindWindow(SETUP_WIN);
		if(win != NULL)
		{
			//test to see if joystick moved, if so update the control
			if((IO.analog[0].engrValue != JoyXPrev)||(IO.analog[1].engrValue != JoyYPrev)||Initialize)
			{				
				bmap=(C_Bitmap *)win->FindControl(JOY_INDICATOR);
				if(bmap != NULL)
				{
					bmap->Refresh();
					bmap->SetX((int)(JoyScale + IO.analog[0].engrValue * JoyScale));
					bmap->SetY((int)(JoyScale + IO.analog[1].engrValue * JoyScale));
					bmap->Refresh();					
					win->RefreshClient(1);
				}
			}
			

			if(IO.AnalogIsUsed(3))
			{
				//test to see if rudder moved, if so update the control
				if(((IO.analog[3].engrValue != RudderPrev)||Initialize) && state )
				{
					line=(C_Line *)win->FindControl(RUDDER);
					if(line != NULL)
					{	
						line->Refresh();
						line->SetY((int)(Rudder.top + RudderScale - IO.analog[3].engrValue * RudderScale + .5));
						if(line->GetY() < Rudder.top)
							line->SetY(Rudder.top);
						if(line->GetY() > Rudder.bottom)
							line->SetY(Rudder.bottom);
						line->SetH(Rudder.bottom - line->GetY());
						line->Refresh();					
					}
				}
			}
			
			if(IO.AnalogIsUsed(2))
			{
				//test to see if throttle moved, if so update the control
				if(((IO.analog[2].engrValue != ThrottlePrev)||Initialize) && state )
				{
					line=(C_Line *)win->FindControl(THROTTLE);
					if(line != NULL)
					{
						line->Refresh();
						line->SetY(FloatToInt32(static_cast<float>(Throttle.top + IO.analog[2].ioVal/15000.0F * ThrottleScale + .5)));
						if(line->GetY() < Throttle.top)
							line->SetY(Throttle.top);
						if(line->GetY() > Throttle.bottom)
							line->SetY(Throttle.bottom);
						line->SetH(Throttle.bottom - line->GetY());
						line->Refresh();
					} 
				}

				if(ABDetentPrev != IO.analog[2].center||Initialize)
				{
					line=(C_Line *)win->FindControl(AB_DETENT);
					if(line != NULL)
					{
						line->Refresh();
						line->SetY(FloatToInt32(static_cast<float>(Throttle.top + IO.analog[2].center/15000.0F * ThrottleScale + .5)));
						if(line->GetY() <= Throttle.top - 1)
							line->SetY(Throttle.top);
						if(line->GetY() >= Throttle.bottom)
							line->SetY(Throttle.bottom + 1);
						line->Refresh();
					}
				}
			}
			
			
			unsigned long i;
			for(i=0;i<gCurJoyCaps.dwButtons;i++)
			{
				//only have 8 controls on screen, but joystick
				//may have more than 8 buttons
				int j = i;
				while(j > 7)
					j -= 8;

				button=(C_Button *)win->FindControl(J1 + j);
				if(button != NULL)
				{
					if(IO.digital[i])
						button->SetState(C_STATE_1);
					else
						button->SetState(C_STATE_0);
					button->Refresh();
				}

				if(IO.digital[i])
				{
					C_Text *text = (C_Text *)win->FindControl(CONTROL_KEYS); 
					if(text)
					{
						char string[_MAX_PATH];
						text->Refresh();
						sprintf(string,"%s %d",gStringMgr->GetString(TXT_BUTTON),i+1);							
						text->SetText(string);
						text->Refresh();
					}
					
					if(KeyVar.EditKey)
					{
						button = (C_Button *)win->FindControl(KeyVar.CurrControl);
						UserFunctionTable.SetButtonFunction(i,(InputFunctionType)button->GetUserPtr(FUNCTION_PTR),button->GetUserNumber(BUTTON_ID));
						KeyVar.EditKey = FALSE;
						KeyVar.Modified = TRUE;
						SetButtonColor(button);
					}

					text = (C_Text *)win->FindControl(FUNCTION_LIST);
					if(text)
					{
						InputFunctionType func;
						char *descrip;

						text->Refresh();
						if(func = UserFunctionTable.GetButtonFunction(i,NULL))
						{
							int i = 0;

							C_Button *tButton = (C_Button *)win->FindControl(KEYCODES);
							while(tButton)
							{								
								if(func == (InputFunctionType)tButton->GetUserPtr(5))
								{
									C_Text *temp = (C_Text *)win->FindControl(tButton->GetID()- KEYCODES + MAPPING);
									
									if(temp)
									{
										descrip = temp->GetText();
										if(descrip)
										{
											text->SetText(descrip);
											break;
										}
									}
									text->SetText("");
									break;
								}
								else
								{
									tButton = (C_Button *)win->FindControl(KEYCODES + i++);
								}
							}
						}
						else
						{
							if(i == 0)
							{
								text->SetText(TXT_FIRE_GUN);
							}
							else if(i == 1)
							{
								text->SetText(TXT_FIRE_WEAPON);
							}
							else
							{
								text->SetText(TXT_NO_FUNCTION);
							}
						}
						text->Refresh();
					}							
				}
			}			
			
			int Direction;
			int flags = 0;

			for(i= 0;i<gCurJoyCaps.dwPOVs;i++)
			{
				Direction = 0;
				if((IO.povHatAngle[i] < 2250 || IO.povHatAngle[i] > 33750) && IO.povHatAngle[i] != -1)
				{
					flags |= 0x01;
					Direction = 0;
				}
				else if(IO.povHatAngle[i] < 6750 )
				{
					flags |= 0x03;
					Direction = 1;
				}
				else if(IO.povHatAngle[i] < 11250)
				{
					flags |= 0x02;
					Direction = 2;
				}
				else if(IO.povHatAngle[i] < 15750)
				{
					flags |= 0x06;
					Direction = 3;
				}
				else if(IO.povHatAngle[i] < 20250)
				{
					flags |= 0x04;
					Direction = 4;
				}
				else if(IO.povHatAngle[i] < 24750)
				{
					flags |= 0x0C;
					Direction = 5;
				}
				else if(IO.povHatAngle[i] < 29250)
				{
					flags |= 0x08;
					Direction = 6;
				}
				else if(IO.povHatAngle[i] < 33750)
				{
					flags |= 0x09;
					Direction = 7;
				}

				if(KeyVar.EditKey && IO.povHatAngle[i] != -1)
				{
					C_Button *button;

					button = (C_Button *)win->FindControl(KeyVar.CurrControl);
					UserFunctionTable.SetPOVFunction(i,Direction,(InputFunctionType)button->GetUserPtr(FUNCTION_PTR),button->GetUserNumber(BUTTON_ID));
					KeyVar.EditKey = FALSE;
					KeyVar.Modified = TRUE;
					SetButtonColor(button);
				}

				C_Text *text = (C_Text *)win->FindControl(FUNCTION_LIST);
				if(text && IO.povHatAngle[i] != -1)
				{
					C_Text *text2 = (C_Text *)win->FindControl(CONTROL_KEYS); 
					if(text2)
					{
						char button[_MAX_PATH];
						text2->Refresh();
						sprintf(button,"%s %d : %s", gStringMgr->GetString(TXT_POV), i+1, gStringMgr->GetString(TXT_UP + Direction));																		
						text2->SetText(button);
						text2->Refresh();
					}

					InputFunctionType func;
					char *descrip;

					text->Refresh();
					if(func = UserFunctionTable.GetPOVFunction(i,Direction,NULL))
					{
						int i = 0;

						C_Button *tButton = (C_Button *)win->FindControl(KEYCODES);
						while(tButton)
						{								
							if(func == (InputFunctionType)tButton->GetUserPtr(5))
							{
								C_Text *temp = (C_Text *)win->FindControl(tButton->GetID()- KEYCODES + MAPPING);
								
								if(temp)
								{
									descrip = temp->GetText();
									if(descrip)
									{
										text->SetText(descrip);
										break;
									}
								}
								text->SetText("");
								break;
							}
							else
							{
								tButton = (C_Button *)win->FindControl(KEYCODES + i++);
							}
						}
					}
					else
					{
						text->SetText(TXT_NO_FUNCTION);
					}
					text->Refresh();
				}		
			}

			button=(C_Button *)win->FindControl(UP_HAT);
			if(button != NULL && button->GetState() != C_STATE_DISABLED)
			{
				if(flags & 0x01)
					button->SetState(C_STATE_1);
				else
					button->SetState(C_STATE_0);
				button->Refresh();
			}
			
			button=(C_Button *)win->FindControl(RIGHT_HAT);
			if(button != NULL && button->GetState() != C_STATE_DISABLED)
			{
				if(flags & 0x02)
					button->SetState(C_STATE_1);
				else
					button->SetState(C_STATE_0);
				button->Refresh();
			}
			
			button=(C_Button *)win->FindControl(DOWN_HAT);
			if(button != NULL && button->GetState() != C_STATE_DISABLED)
			{
				if(flags & 0x04)
					button->SetState(C_STATE_1);
				else
					button->SetState(C_STATE_0);
				button->Refresh();
			}
			
			button=(C_Button *)win->FindControl(LEFT_HAT);
			if(button != NULL && button->GetState() != C_STATE_DISABLED)
			{
				if(flags & 0x08)
					button->SetState(C_STATE_1);
				else
					button->SetState(C_STATE_0);
				button->Refresh();
			}
			
			Initialize = 0;
			JoyXPrev = IO.analog[0].engrValue;
			JoyYPrev = IO.analog[1].engrValue; 
			ThrottlePrev = IO.analog[2].engrValue;
			RudderPrev = IO.analog[3].engrValue;
			ABDetentPrev = static_cast<float>(IO.analog[2].center);
			POVPrev = IO.povHatAngle[0];
		}
	//if(Calibration.calibrating)
	//	Calibrate();
	
}//RefreshJoystickCB

SIM_INT CalibrateFile (void)
{
	int i, numAxis;
	FILE* filePtr;
	
	char fileName[_MAX_PATH];
	sprintf(fileName,"%s\\config\\joystick.dat",FalconDataDirectory);

	filePtr = fopen (fileName, "rb");
	if(filePtr != NULL)
	{
		fread (&numAxis, sizeof(int), 1, filePtr);
		
		for (i=0; i<numAxis; i++)
		{
			fread (&(IO.analog[i]), sizeof(SIMLIB_ANALOG_TYPE), 1, filePtr);
		}
		fclose (filePtr);
		
		return (TRUE);
	}
	return FALSE;
}
/*
void StopCalibrating(C_Base *control)
{
	C_Text *text;
	C_Button *button;

	Calibration.calibrating = FALSE;
	Calibration.step = 0;
	Calibration.disp_text = TRUE;
	Calibration.state = 1;

	Calibration.calibrated = CalibrateFile();

	text=(C_Text *)control->Parent_->FindControl(CAL_TEXT);
	text->Refresh();
	text->SetFlagBitOn(C_BIT_INVISIBLE);
	text->Refresh();

	text=(C_Text *)control->Parent_->FindControl(CAL_TEXT2);
	text->Refresh();
	text->SetFlagBitOn(C_BIT_INVISIBLE);
	text->Refresh();

	button = (C_Button *)control->Parent_->FindControl(CALIBRATE);
	button->SetState(C_STATE_0);
	button->Refresh();
}*/

/*
SIM_INT Calibrate ( void )
{
	int retval;
		
	if (S_joyret == JOYERR_NOERROR)
	{
		retval = SIMLIB_OK;
		int size;
		FILE* filePtr;
		C_Base	*control;
		C_Window	*win;
		C_Text		*text,*text2;
		C_Button	*button;
		RECT		client;

		if(Calibration.state)
		{
			Calibration.state = 0;
			Calibration.disp_text = TRUE;
			//waiting for user to let go of all buttons
			for(int i =0;i < S_joycaps.wNumButtons;i++)
			{
				if(IO.digital[i]) //button pressed
				{
					Calibration.state = 1; 
					break;
				}
			}
		}
		else
		{
			win = gMainHandler->FindWindow(SETUP_WIN);
			control = win->FindControl(JOY_INDICATOR);
			
			text=(C_Text *)win->FindControl(CAL_TEXT);
			text2=(C_Text *)win->FindControl(CAL_TEXT2);

			switch(Calibration.step)
			{
			int i;

			case 0:
			
				if(Calibration.disp_text)
				{
					MonoPrint ("Center the joystick, throttle, and rudder and push a button.\n");
					if(text != NULL)
					{
						text->Refresh();
						text->SetFlagBitOff(C_BIT_INVISIBLE);
						text->SetText(TXT_CTR_JOY);
						text->Refresh();
					}

					if(text2)
					{
						text2->Refresh();
						text2->SetFlagBitOff(C_BIT_INVISIBLE);
						text2->Refresh();
					}

					if(!Calibration.calibrated)
					{
						IO.analog[0].mUp = IO.analog[0].mDown = IO.analog[0].bUp = IO.analog[0].bDown = 0.0F;
						IO.analog[1].mUp = IO.analog[1].mDown = IO.analog[1].bUp = IO.analog[1].bDown = 1.1F;
						IO.analog[2].mUp = IO.analog[2].mDown = IO.analog[2].bUp = IO.analog[2].bDown = 2.2F;
						IO.analog[3].mUp = IO.analog[3].mDown = IO.analog[3].bUp = IO.analog[3].bDown = 3.3F;
					}

					IO.analog[0].min = IO.analog[1].min = IO.analog[2].min = IO.analog[3].min = 65536.0F;
					IO.analog[0].max = IO.analog[1].max = IO.analog[2].max = IO.analog[3].max = 0.0F;

					IO.analog[0].isUsed = IO.analog[1].isUsed = TRUE;

					if (!(S_joycaps.wCaps & JOYCAPS_HASZ))
					{
						IO.analog[2].isUsed = FALSE;
						IO.analog[2].max = 0;
						IO.analog[2].min = -1;
						IO.analog[2].engrValue = 1.0F;
					}
					else
					{
						IO.analog[2].isUsed = TRUE;
					}
					
					if (!(S_joycaps.wCaps & JOYCAPS_HASR))
					{
						IO.analog[3].isUsed= FALSE;
						IO.analog[3].max = 1;
						IO.analog[3].min = -1;
						IO.analog[3].engrValue = 0.0F;
					}
					else
					{
						IO.analog[3].isUsed = TRUE;
					}

					IO.analog[2].center = 32768;

					Calibration.disp_text = FALSE;
				}

				IO.analog[0].center = IO.analog[0].ioVal;
				IO.analog[1].center = IO.analog[1].ioVal;
				IO.analog[3].center = IO.analog[3].ioVal;
				
				for(i =0;i < S_joycaps.wNumButtons;i++)
				{
					if(IO.digital[i])
						Calibration.state = 1;		//button pressed
				}
				
				if(Calibration.state)
					Calibration.step++;

				break;
			
			case 1:

				if(Calibration.disp_text)
				{
					MonoPrint ("Move the joystick to the corners and push a button.\n");
					if(text != NULL)
					{
						text->Refresh();
						text->SetText(TXT_MV_JOY);
						text->Refresh();
					}
					Calibration.disp_text = FALSE;
				}
				
				IO.analog[0].max = max(IO.analog[0].max, IO.analog[0].ioVal);
				IO.analog[1].max = max(IO.analog[1].max, IO.analog[1].ioVal);
				IO.analog[0].min = min(IO.analog[0].min, IO.analog[0].ioVal);
				IO.analog[1].min = min(IO.analog[1].min, IO.analog[1].ioVal);
				
				for(i =0;i < S_joycaps.wNumButtons;i++)
				{
					if(IO.digital[i])
						Calibration.state = 1;		//button pressed
				}

				if(Calibration.state)
					Calibration.step++;

				break;

			case 2:

				if (S_joycaps.wCaps & JOYCAPS_HASZ)
				{
					if(Calibration.disp_text)
					{
						MonoPrint ("Move the throttle to the ends and push a button\n");
						if(text != NULL && IO.analog[2].isUsed)
						{
							text->Refresh();
							text->SetText(TXT_MV_THR);
							text->Refresh();
						}
						Calibration.disp_text = FALSE;
					}
					
					IO.analog[2].max = max(IO.analog[2].max, IO.analog[2].ioVal);
					IO.analog[2].min = min(IO.analog[2].min, IO.analog[2].ioVal);
					
					for(i =0;i < S_joycaps.wNumButtons;i++)
					{
						if(IO.digital[i])
							Calibration.state = 1;		//button pressed
					}

					if(Calibration.state)
						Calibration.step++;
				}
				else
				{
					Calibration.step++;
					Calibration.state = 1;
				}
				break;

			case 3:
				if (S_joycaps.wCaps & JOYCAPS_HASR)
				{
					if(Calibration.disp_text)
					{
						MonoPrint ("Move the rudder to the ends and push a button\n");
						if(text != NULL && IO.analog[3].isUsed)
						{
							text->Refresh();
							text->SetText(TXT_MV_RUD);
							text->Refresh();
						}
						Calibration.disp_text = FALSE;
					}
					
					IO.analog[3].max = max(IO.analog[3].max, IO.analog[3].ioVal);
					IO.analog[3].min = min(IO.analog[3].min, IO.analog[3].ioVal);

					for(int i =0;i < S_joycaps.wNumButtons;i++)
					{
						if(IO.digital[i])
							Calibration.state = 1;		//button pressed
					}

					if(Calibration.state)
						Calibration.step++;
					
				}
				else
				{
					Calibration.step++;
					Calibration.state = 1;
				}
				break;

			case 4:
				for (i=0; i<S_joycaps.wNumAxes; i++)
				{
					if (IO.analog[i].isUsed)
					{
						IO.analog[i].mUp = 1.0F /(IO.analog[i].max - IO.analog[i].center);
						IO.analog[i].bUp = -IO.analog[i].mUp * IO.analog[i].center;
						IO.analog[i].mDown = 1.0F / (IO.analog[i].center - IO.analog[i].min);
						IO.analog[i].bDown = -IO.analog[i].mDown * IO.analog[i].center;
					}
					else
					{
						IO.analog[i].mUp = IO.analog[i].mDown = 0.0F;
						IO.analog[i].bUp = IO.analog[i].bDown = 0.0F;
					}
				}
				
				char filename[_MAX_PATH];
				sprintf(filename,"%s\\config\\joystick.dat",FalconDataDirectory);
				filePtr = fopen (filename, "wb");
				if(filePtr)
				{
					fwrite (&S_joycaps.wNumAxes, sizeof(int), 1, filePtr);
					
					for (i=0; i<S_joycaps.wNumAxes; i++)
					{
						fwrite (&(IO.analog[i]), sizeof(SIMLIB_ANALOG_TYPE), 1, filePtr);
					}
					fclose (filePtr);
				}
				else
				{
					ShiAssert(filePtr == NULL);
				}
				
				button = (C_Button *)win->FindControl(CALIBRATE);
				button->SetState(C_STATE_0);
				button->Refresh();
				
				
				
				if(text != NULL)
				{
					text->Refresh();
					text->SetFlagBitOn(C_BIT_INVISIBLE);
					text->Refresh();
				}

				if(text2)
				{
					text2->Refresh();
					text2->SetFlagBitOn(C_BIT_INVISIBLE);
					text2->Refresh();
				}
					
				client = win->GetClientArea(1);
				
				if(control)
				{
					size = ((C_Bitmap *)control)->GetH();
				}
				
				JoyScale = (float)(client.right - client.left - size)/2.0F;
				RudderScale = (Rudder.bottom - Rudder.top )/2.0F;
				ThrottleScale = (Throttle.bottom - Throttle.top )/2.0F;
				
				Calibration.calibrated = TRUE;
				Calibration.calibrating = FALSE;
				Calibration.step = 0;
				Calibration.state = 1;
			}
		}
		
	}
	else if (S_joyret == MMSYSERR_NODRIVER)
	{
		SimLibPrintError ("MMSYSERR No Driver");
		return SIMLIB_ERR;
	}
	else if (S_joyret == MMSYSERR_INVALPARAM)
	{
		SimLibPrintError ("MMSYSERR Invalid Parameter");
		return SIMLIB_ERR;
	}

	return retval;
}




void CalibrateCB(long ID,short hittype,C_Base *control)
{
	if((hittype != C_TYPE_LMOUSEUP))
		return;

	Calibration.calibrating = 1;
	//Calibrate();
	
}//CalibrateCB

*/

//function assumes you have passed a char * that has enough memory allocated
void DoShiftStates(char *mods, int ShiftStates)
{
	int plus = 0;

	if(ShiftStates & _SHIFT_DOWN_)
	{
		strcat(mods,gStringMgr->GetString(TXT_SHIFT_KEY));
		plus++;
	}

	if(ShiftStates & _CTRL_DOWN_)
	{
		if(plus)
		{
			strcat(mods,"+");
			plus--;
		}
		strcat(mods,gStringMgr->GetString(TXT_CONTROL_KEY));
		plus++;
	}
	
	if(ShiftStates & _ALT_DOWN_)
	{
		if(plus)
		{
			strcat(mods,"+");
			plus--;
		}

		strcat(mods,gStringMgr->GetString(TXT_ALTERNATE_KEY));
		plus++;
	}
	
	if(plus)
		strcat(mods," ");
}


BOOL KeystrokeCB(unsigned char DKScanCode,unsigned char,unsigned char ShiftStates,long)
{
	if(DKScanCode == DIK_ESCAPE)
		return FALSE; 

	if(Cluster == 8004)
	{
		if(	DKScanCode == DIK_LSHIFT	|| DKScanCode == DIK_RSHIFT		|| \
			DKScanCode == DIK_LCONTROL	|| DKScanCode == DIK_RCONTROL	|| \
			DKScanCode == DIK_LMENU		|| DKScanCode == DIK_RMENU		|| \
			DKScanCode == 0x45)
				return TRUE;

		if( GetAsyncKeyState(VK_SHIFT) & 0x8001 )
			ShiftStates |= _SHIFT_DOWN_;
		else
			ShiftStates &= ~_SHIFT_DOWN_;		
		
		//int flags = ShiftStates;
		int flags = ShiftStates + (KeyVar.CommandsKeyCombo << SECOND_KEY_SHIFT) + (KeyVar.CommandsKeyComboMod << SECOND_KEY_MOD_SHIFT);

		C_Window *win;
		int CommandCombo = 0;

		win = gMainHandler->FindWindow(SETUP_WIN);		

		if(KeyVar.EditKey)
		{
			C_Button *button;
			long ID;

			button = (C_Button *)win->FindControl(KeyVar.CurrControl);
			flags = ShiftStates + (button->GetUserNumber(FLAGS) & SECOND_KEY_MASK);
			KeyVar.CommandsKeyCombo = (button->GetUserNumber(FLAGS) & KEY1_MASK) >> SECOND_KEY_SHIFT;
			KeyVar.CommandsKeyComboMod = (button->GetUserNumber(FLAGS) & MOD1_MASK) >> SECOND_KEY_MOD_SHIFT;

			//here is where we need to change the key combo for the function
			if(DKScanCode != button->GetUserNumber(KEY2) || flags != button->GetUserNumber(FLAGS))
			{
				char keydescrip[_MAX_PATH];
				keydescrip[0] = 0;
				int pmouse, pbutton;
				InputFunctionType theFunc;
				InputFunctionType oldFunc;
				theFunc = (InputFunctionType)button->GetUserPtr(FUNCTION_PTR);
				//is the key combo already used?
				if(oldFunc = UserFunctionTable.GetFunction(DKScanCode,flags,&pmouse,&pbutton))
				{
					C_Button *temp;

					ID = UserFunctionTable.GetControl(DKScanCode,flags);
					KeyVar.OldControl = ID;
					

					//there is a function mapped but it's not visible
					//don't allow user to remap this key combo
					if(!ID && oldFunc)
						return TRUE;

					
					temp = (C_Button *)win->FindControl(ID);
					if( temp && (temp->GetUserNumber(EDITABLE) < 1) )
					{
						//this keycombo is not remappable
						return TRUE;
					}
					//remove old function from place user wants to use
					UserFunctionTable.RemoveFunction(DKScanCode,flags);
				}
					
				//remove function that's being remapped from it's old place
				UserFunctionTable.RemoveFunction(button->GetUserNumber(KEY2),button->GetUserNumber(FLAGS));
				
				//add function into it's new place
				UserFunctionTable.AddFunction(DKScanCode,flags,button->GetUserNumber(BUTTON_ID),button->GetUserNumber(MOUSE_SIDE), theFunc);
				UserFunctionTable.SetControl(DKScanCode,flags,KeyVar.CurrControl);

				//mark that the keymapping needs to be saved
				KeyVar.Modified = TRUE;

				//setup button with new values
				button->SetUserNumber(KEY2,DKScanCode);
				button->SetUserNumber(FLAGS,flags);
				
				char mods[40] = {0};
				_TCHAR firstKey[MAX_PATH] = {0};
				_TCHAR totalDescrip[MAX_PATH] = {0};

				// JPO crash log detection.
				ShiAssert (DKScanCode >= 0 && DKScanCode < 256);
				ShiAssert (FALSE == IsBadStringPtr(KeyDescrips[DKScanCode], MAX_PATH));
				if(KeyVar.CommandsKeyCombo > 0)
				{
					DoShiftStates(firstKey,KeyVar.CommandsKeyComboMod);
					DoShiftStates(mods,ShiftStates);
					
					if (KeyDescrips[DKScanCode])
						_stprintf(totalDescrip,"%s%s : %s%s",firstKey,KeyDescrips[KeyVar.CommandsKeyCombo],mods,KeyDescrips[DKScanCode]);
				}
				else
				{
					DoShiftStates(totalDescrip,ShiftStates);

					if (KeyDescrips[DKScanCode])
						strcat(totalDescrip,KeyDescrips[DKScanCode]);
				}

				//DoShiftStates(keydescrip,ShiftStates);
				//strcat(keydescrip,KeyDescrips[DKScanCode]);
				
				button->Refresh();
				button->SetText(0,totalDescrip);
				button->Refresh();

				if(KeyVar.OldControl)
				{
					//if we unmapped another function to map this one we 
					//need to update the first functions buttton
					C_Button *temp;

					temp = (C_Button *)win->FindControl(KeyVar.OldControl);
					//strcpy(keydescrip,"No Key Assigned");
					if(temp)
					{
						SetButtonColor(temp);
						temp->SetUserNumber(KEY2,-1);
						temp->SetUserNumber(FLAGS,temp->GetUserNumber(FLAGS) & SECOND_KEY_MASK);
						temp->Refresh();
						temp->SetText(0,TXT_NO_KEY);
						temp->Refresh();
					}
				}
			}
			SetButtonColor(button);
		}

		if(KeyVar.OldControl)
		{
			// if we stole another functions mapping, move to the function
			// and leave ourselves in edit mode
			C_Button *temp;
			UI95_RECT Client;

			temp = (C_Button *)win->FindControl(KeyVar.OldControl);
			if(temp)
			{
				temp->SetFgColor(0,RGB(0,255,255));

				Client = win->GetClientArea(temp->GetClient());

				win->SetVirtualY(temp->GetY()-Client.top,temp->GetClient());
				win->AdjustScrollbar(temp->GetClient()); 
				win->RefreshClient(temp->GetClient());
			}

			KeyVar.CurrControl = KeyVar.OldControl;
			KeyVar.OldControl = 0;
		}
		else
		{
			//key changed leave edit mode
			KeyVar.EditKey = FALSE;
		}

		C_Text	*text;
		
		if(DKScanCode == 0xC5)
			DKScanCode = 0x45;

		//build description for display at bottom of window
		if(KeyDescrips[DKScanCode])
		{
			
			//if(DKScanCode == 0x44)

			char mods[40] = {0};
			char *descrip = NULL;
			int pmouse,pbutton;
			InputFunctionType function;
			
			text = (C_Text *)win->FindControl(CONTROL_KEYS); //CONTROL_KEYS
			if(text)
			{
				_TCHAR firstKey[MAX_PATH] = {0};
				_TCHAR totalDescrip[MAX_PATH] = {0};
				if(KeyVar.CommandsKeyCombo > 0)
				{
					DoShiftStates(firstKey,KeyVar.CommandsKeyComboMod);
					DoShiftStates(mods,ShiftStates);
					_stprintf(totalDescrip,"%s%s : %s%s",firstKey,KeyDescrips[KeyVar.CommandsKeyCombo],mods,KeyDescrips[DKScanCode]);
				}
				else
				{
					DoShiftStates(totalDescrip,ShiftStates);
					strcat(totalDescrip,KeyDescrips[DKScanCode]);
				}
				//DoShiftStates(mods,ShiftStates);
				//strcat(mods,KeyDescrips[DKScanCode]);
				text->Refresh();
				text->SetText(totalDescrip);
				text->Refresh();
			}
			
			//flags = flags + (KeyVar.CommandsKeyCombo << SECOND_KEY_SHIFT) + (KeyVar.CommandsKeyComboMod << SECOND_KEY_MOD_SHIFT);
			function = UserFunctionTable.GetFunction(DKScanCode,flags,&pmouse,&pbutton);			

			text = (C_Text *)win->FindControl(FUNCTION_LIST);
			if(text)
			{
				text->Refresh();
				if(function)
				{
					C_Text *temp;
					C_Button *btn;
					long ID;

					ID = UserFunctionTable.GetControl(DKScanCode,flags) ;

					CommandCombo = 0;

					btn = (C_Button *)win->FindControl(ID);
					if(btn)
						CommandCombo = btn->GetUserNumber(EDITABLE);

					if(CommandCombo == -1)
					{
						KeyVar.CommandsKeyCombo = DKScanCode;
						KeyVar.CommandsKeyComboMod = ShiftStates;
					}
					else
					{
						KeyVar.CommandsKeyCombo = 0;
						KeyVar.CommandsKeyComboMod = 0;
					}

					ID = ID - KEYCODES + MAPPING;
					temp = (C_Text *)win->FindControl(ID);
					if(temp)
						descrip = temp->GetText();

					if(descrip)
						text->SetText(descrip);
					else
						text->SetText(TXT_NO_FUNCTION);

					if(!KeyVar.EditKey && temp)
					{
						UI95_RECT Client;
						Client = win->GetClientArea(temp->GetClient());

						win->SetVirtualY(temp->GetY() - Client.top,temp->GetClient());
						win->AdjustScrollbar(temp->GetClient()); 
						win->RefreshClient(temp->GetClient());
					}
				}
				else
				{
					text->SetText(TXT_NO_FUNCTION);
					KeyVar.CommandsKeyCombo = 0;
					
					KeyVar.CommandsKeyComboMod = 0;
				}				
				text->Refresh();
			}
		}
		return TRUE;
	}
	

	return FALSE;
}

void KeycodeCB(long ID,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(KeyVar.EditKey)
	{
		C_Button *button;
		
		button = (C_Button *)control->Parent_->FindControl(KeyVar.CurrControl);
		
		SetButtonColor(button);
	}

	if(KeyVar.CurrControl == ID && KeyVar.EditKey)
	{
		KeyVar.EditKey = FALSE;
		return;
	}

	if( control->GetUserNumber(EDITABLE) < 1 )
	{
		KeyVar.CurrControl = ID;
		KeyVar.EditKey = FALSE;
	}
	else 
	{
		KeyVar.CurrControl = ID;
		((C_Button *)control)->SetFgColor(0,RGB(0,255,255));
		((C_Button *)control)->Refresh();
		KeyVar.EditKey = TRUE;
	}
	
	return;
}

int AddUndisplayedKey(KeyMap &Map)	
{
	if(NumUndispKeys < 300)
	{
		UndisplayedKeys[NumUndispKeys].func = Map.func;
		UndisplayedKeys[NumUndispKeys].buttonId = Map.buttonId;
		UndisplayedKeys[NumUndispKeys].mouseSide = Map.mouseSide;
		UndisplayedKeys[NumUndispKeys].key2 = Map.key2;
		UndisplayedKeys[NumUndispKeys].mod2 = Map.mod2;
		UndisplayedKeys[NumUndispKeys].key1= Map.key1;
		UndisplayedKeys[NumUndispKeys].mod1= Map.mod1;
		UndisplayedKeys[NumUndispKeys].editable = Map.editable;
		strcpy(UndisplayedKeys[NumUndispKeys].descrip, Map.descrip);
		NumUndispKeys++;
		return TRUE;
	}
	return FALSE;
}

int AddKeyMapLines(C_Window *win, C_Line *Hline, C_Line *Vline, int count)
{
	int retval = TRUE;

	if(!win)
		return FALSE;

	C_Line *line;
	line = (C_Line *)win->FindControl(HLINE + count);
	if(!line)
	{
		line = new C_Line;
		if(line)
		{
			line->Setup(HLINE + count,Hline->GetType());
			line->SetColor(RGB(191,191,191));
			line->SetXYWH(Hline->GetX(),Hline->GetY() + Vline->GetH()*count,Hline->GetW(),Hline->GetH());
			line->SetFlags(Hline->GetFlags());
			line->SetClient(Hline->GetClient());
			line->SetGroup(Hline->GetGroup());
			line->SetCluster(Hline->GetCluster());

			win->AddControl(line);
			line->Refresh();
		}
		else
			retval = FALSE;
	}

	line = (C_Line *)win->FindControl(VLINE + count);
	if(!line)
	{
		line = new C_Line;
		if(line)
		{
			line->Setup(VLINE + count,Vline->GetType());
			line->SetColor(RGB(191,191,191));
			line->SetXYWH(Vline->GetX(),Vline->GetY() + Vline->GetH()*count,Vline->GetW(),Vline->GetH());
			line->SetFlags(Vline->GetFlags());
			line->SetClient(Vline->GetClient());
			line->SetGroup(Vline->GetGroup());
			line->SetCluster(Vline->GetCluster());

			win->AddControl(line);
			line->Refresh();
		}
		else 
			retval = FALSE;
	}
	

	return retval;
}

void UpdateKeyMapButton(C_Button *button, KeyMap &Map, int count)
{
	int flags = Map.mod2 + (Map.key1 << SECOND_KEY_SHIFT) + (Map.mod1 << SECOND_KEY_MOD_SHIFT);

	button->SetUserNumber(KEY2,Map.key2);
	button->SetUserNumber(FLAGS,flags);
	button->SetUserNumber(BUTTON_ID,Map.buttonId);
	button->SetUserNumber(MOUSE_SIDE,Map.mouseSide);
	button->SetUserNumber(EDITABLE,Map.editable);
	button->SetUserPtr(FUNCTION_PTR,(void*)Map.func);

	if(Map.key2 == -1)
	{
		button->SetText(0,TXT_NO_KEY);
	}
	else
	{
		
		_TCHAR totalDescrip[MAX_PATH] = {0};
		if(Map.key1 > 0)
		{
			_TCHAR firstMod[MAX_PATH] = {0};
			_TCHAR secondMod[MAX_PATH] = {0};
			DoShiftStates(firstMod,Map.mod1);
			DoShiftStates(secondMod,Map.mod2);
			_stprintf(totalDescrip,"%s%s : %s%s",firstMod,KeyDescrips[Map.key1],secondMod,KeyDescrips[Map.key2]);
		}
		else
		{
			DoShiftStates(totalDescrip,Map.mod2);
			strcat(totalDescrip,KeyDescrips[Map.key2]);
		}
		UserFunctionTable.SetControl(Map.key2,flags,KEYCODES + count); //define this as KEYCODES
		button->SetText(0,totalDescrip);
	}			

	SetButtonColor(button);
	
}

int UpdateKeyMap(C_Window *win, C_Button *Keycodes, int height, KeyMap &Map, HotSpotStruct HotSpot, int count )
{
	C_Button *button;
	long flags;

	flags = Map.mod2 + (Map.key1 << SECOND_KEY_SHIFT) + (Map.mod1 << SECOND_KEY_MOD_SHIFT);

	button = (C_Button *)win->FindControl(KEYCODES + count);
	if(button)
	{
		button->Refresh();
		UpdateKeyMapButton(button, Map, count);
		button->Refresh();
		return TRUE;
	}
	else
	{
		button = new C_Button;
		if(button)
		{
			button->Setup(KEYCODES + count, Keycodes->GetType(), Keycodes->GetX(),
							Keycodes->GetY() + height*count);
			
			button->SetClient(Keycodes->GetClient());
			button->SetGroup(Keycodes->GetGroup());
			button->SetCluster(Keycodes->GetCluster());
			button->SetFont(Keycodes->GetFont());
			button->SetFlags(Keycodes->GetFlags());
			button->SetCallback(KeycodeCB);
			button->SetHotSpot(HotSpot.X, HotSpot.Y, HotSpot.W, HotSpot.H);
			
			if(Keycodes->GetSound(1))
				button->SetSound((Keycodes->GetSound(1))->ID, 1);
			
			UpdateKeyMapButton(button, Map, count);

			win->AddControl(button);
			button->Refresh();
			return TRUE;
		}
	}
	return FALSE;
}

int UpdateMappingDescrip(C_Window *win, C_Text *Mapping, int height, _TCHAR *descrip, int count)
{
	C_Text *text;

	text = (C_Text *)win->FindControl(MAPPING+count);
	if(text)
	{
		text->Refresh();
		text->SetText(descrip);
		text->Refresh();
		return TRUE;
	}
	else
	{
		text = new C_Text;
		if(text)
		{
			text->Setup(MAPPING+count, Mapping->GetType());
			text->SetFGColor(Mapping->GetFGColor());
			text->SetBGColor(Mapping->GetBGColor());
			text->SetClient(Mapping->GetClient());
			text->SetGroup(Mapping->GetGroup());
			text->SetCluster(Mapping->GetCluster());
			text->SetFont(Mapping->GetFont());
			text->SetFlags(Mapping->GetFlags());
			text->SetXY(Mapping->GetX(),Mapping->GetY() + height*count);
			text->SetText(descrip);

			win->AddControl(text);
			text->Refresh();
			return TRUE;
		}
	}
	return FALSE;
}

int SetHdrStatusLine(C_Window *win, C_Button *Keycodes, C_Line *Vline, KeyMap &Map, HotSpotStruct HotSpot, int count)
{
	C_Line *line;

	line = (C_Line *)win->FindControl(KEYCODES - count);
	if(line)
	{
		if(Map.editable != -1)
		{
			line->SetFlagBitOn(C_BIT_INVISIBLE);
		}
		else
		{
			line->SetFlagBitOff(C_BIT_INVISIBLE);
		}
		line->Refresh();
		return TRUE;
	}
	else
	{
		line = new C_Line;
		if(line)
		{
			UI95_RECT	client;
			client = win->GetClientArea(3);

			line->Setup(KEYCODES - count,0);
			//line->SetXYWH(	Keycodes->GetX() + HotX,
			//				Keycodes->GetY() + Vline->GetH()*count + HotY,
			//				HotW,HotH);
			line->SetXYWH(	Keycodes->GetX() + HotSpot.X,
							Keycodes->GetY() + Vline->GetH()*count + HotSpot.Y,
							client.right - client.left,HotSpot.H);
			
			line->SetColor(RGB(65,128,173));//lt blue
			line->SetFlags(Vline->GetFlags());
			line->SetClient(Vline->GetClient());
			line->SetGroup(Vline->GetGroup());
			line->SetCluster(Vline->GetCluster());
			win->AddControl(line);
			line->Refresh();
			//this is a header, so we put a lt blue line behind it
			if(Map.editable != -1)
			{
				line->SetFlagBitOn(C_BIT_INVISIBLE);
			}
			line->Refresh();
			return TRUE;
		}
	}
	return FALSE;
}


BOOL SaveKeyMapList( char *filename )
{
	if(!KeyVar.Modified)
		return TRUE;

	KeyVar.Modified = FALSE;

	FILE *fp;
	C_Window *win;
	C_Button *button;
	InputFunctionType theFunc;
	char *funcDescrip;
	int i,key1,mod1,mod2,flags, count = 0;
	
	C_Text *text;
	char descrip[_MAX_PATH];
	
	win = gMainHandler->FindWindow(SETUP_WIN);
	if(!win)
		return FALSE;

	char path[_MAX_PATH];
	sprintf(path,"%s\\config\\%s.key",FalconDataDirectory,filename);

	fp = fopen(path,"wt");
	if(!fp)
		return FALSE;

	button = (C_Button *)win->FindControl(KEYCODES); 

	while(button)
	{
		//int pmouse,pbutton;

		flags = button->GetUserNumber(FLAGS);
		mod2 = flags & MOD2_MASK;
		key1 = (flags & KEY1_MASK) >> SECOND_KEY_SHIFT;
		mod1 = (flags & MOD1_MASK) >> SECOND_KEY_MOD_SHIFT;

		theFunc = (InputFunctionType)button->GetUserPtr(FUNCTION_PTR);
		funcDescrip = FindStringFromFunction(theFunc);

		text = (C_Text *)win->FindControl(MAPPING + count);
		if(text)
			sprintf(descrip, "%c%s%c", '"', text->GetText(), '"');
		else
			strcpy(descrip,"");

		if(key1 == 0xff)
		{
			key1 = -1;
			mod1 = 0;
		}

		fprintf(fp,"%s %d %d %#X %X %#X %X %d %s\n",funcDescrip,button->GetUserNumber(BUTTON_ID),button->GetUserNumber(MOUSE_SIDE),button->GetUserNumber(KEY2),mod2,key1,mod1,button->GetUserNumber(EDITABLE),descrip);

		count++;
		button = (C_Button *)win->FindControl(KEYCODES + count);
	}

	for(i = 0; i < NumUndispKeys;i++)
	{
		funcDescrip = FindStringFromFunction(UndisplayedKeys[i].func);

		fprintf(fp,"%s %d %d %#X %d %#X %d %d %s\n",
					funcDescrip,
					UndisplayedKeys[i].buttonId ,
					UndisplayedKeys[i].mouseSide,
					UndisplayedKeys[i].key2,
					UndisplayedKeys[i].mod2,
					UndisplayedKeys[i].key1,
					UndisplayedKeys[i].mod1,
					UndisplayedKeys[i].editable,
					UndisplayedKeys[i].descrip
					);
	}

	for(i =0;i<UserFunctionTable.NumButtons;i++)
	{
		int cpButtonID;
		theFunc = UserFunctionTable.GetButtonFunction(i, &cpButtonID);
		
		if(theFunc)
		{
			funcDescrip = FindStringFromFunction(theFunc);

			fprintf(fp,"%s %d %d -2 0 0x0 0\n",funcDescrip,i,cpButtonID);
		}
	}

	for(i =0;i<UserFunctionTable.NumPOVs;i++)
	{
		int cpButtonID;
		for(int j=0; j < 8; j++)
		{
			theFunc = UserFunctionTable.GetPOVFunction(i,j, &cpButtonID);
			
			if(theFunc)
			{
				funcDescrip = FindStringFromFunction(theFunc);

				fprintf(fp,"%s %d %d -3 %d 0x0 0\n",funcDescrip,i,cpButtonID,j);
			}
		}
	}

	fclose(fp);

	return TRUE;
}

void SaveKeyCB(long,short hittype,C_Base *control)
{
	C_Window *win;
	C_EditBox * ebox;
	_TCHAR fname[MAX_PATH];

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	win=gMainHandler->FindWindow(SAVE_WIN);
	if(!win)
		return;

	gMainHandler->HideWindow(win);
	gMainHandler->HideWindow(control->Parent_);

	ebox=(C_EditBox*)win->FindControl(FILE_NAME);
	if(ebox)
	{
		_tcscpy(fname,ebox->GetText());

		if(fname[0] == 0)
			return;
		
		KeyVar.Modified = TRUE;
		if(SaveKeyMapList(fname))
			_tcscpy(PlayerOptions.keyfile,ebox->GetText());
		
	}
}

void VerifySaveKeyCB(long ID,short hittype,C_Base *control)
{
	C_EditBox * ebox;
	_TCHAR fname[MAX_PATH];
	FILE *fp;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	ebox=(C_EditBox*)control->Parent_->FindControl(FILE_NAME);
	if(ebox)
	{
		//dpc EmptyFilenameSaveFix, modified by MN - added a warning to enter a filename
		if (g_bEmptyFilenameFix)
		{
			if (_tcslen(ebox->GetText()) == 0)
			{
				AreYouSure(TXT_WARNING, TXT_ENTER_FILENAME,CloseWindowCB,CloseWindowCB);
				return;
			}
		}
		//end EmptyFilenameSaveFix
		_stprintf(fname,"config\\%s.key",ebox->GetText ());
		fp=fopen(fname,"r");
		if(fp)
		{
			fclose(fp);
			AreYouSure(TXT_WARNING,TXT_FILE_EXISTS,SaveKeyCB,CloseWindowCB);
		}
		else
			SaveKeyCB(ID,hittype,control);
	}
}


void SaveKeyButtonCB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	SetDeleteCallback(DelKeyFileCB);
	SaveAFile(TXT_SAVE_KEYBOARD,"config\\*.KEY",NULL,VerifySaveKeyCB,CloseWindowCB,"");
}


int RemoveExcessControls(C_Window *win, int count)
{
	C_Button *button;
	C_Text *text;
	C_Line *line;
	int retval = FALSE;

	button = (C_Button *)win->FindControl(KEYCODES + count);
	if(button)
	{
		win->RemoveControl(KEYCODES + count);
		retval = TRUE;
	}

	text = (C_Text *)win->FindControl(MAPPING + count);
	if(text)
	{
		win->RemoveControl(MAPPING + count);
		retval = TRUE;
	}

	line = (C_Line *)win->FindControl(KEYCODES - count);
	if(line)
	{
		win->RemoveControl(KEYCODES - count);
		retval = TRUE;
	}

	line = (C_Line *)win->FindControl(HLINE + count);
	if(line)
	{
		win->RemoveControl(HLINE + count);
		retval = TRUE;
	}

	line = (C_Line *)win->FindControl(VLINE + count);
	if(line)
	{
		win->RemoveControl(VLINE + count);
		retval = TRUE;
	}
	
	//need to remove them if they exist
	return retval;
}


int UpdateKeyMapList( char *fname,int flag )
{
	FILE *fp;
	
	char filename[_MAX_PATH];
	C_Window *win;


	win = gMainHandler->FindWindow(SETUP_WIN);
	if(!win)
	{
		KeyVar.NeedUpdate = TRUE;
		return FALSE;
	}

	if(flag)
		sprintf(filename,"%s\\config\\%s.key",FalconDataDirectory,fname);
	else
		sprintf(filename,"%s\\config\\keystrokes.key",FalconDataDirectory);

	fp = fopen(filename,"rt");
	if(!fp)
		return FALSE;
	
	UserFunctionTable.ClearTable();
	LoadFunctionTables(fname);
	
	char keydescrip[_MAX_PATH];
	C_Button	*button;
	C_Text		*text;
	
	keydescrip[0] = 0;
		
	int count = 0;
	int key1, mod1;
	int key2, mod2, editable;
//	int flags =0, 
	int buttonId, mouseSide;
	InputFunctionType theFunc;
	char buff[_MAX_PATH];
	char funcName[_MAX_PATH];
	char descrip[_MAX_PATH];
	char *parsed;
	HotSpotStruct HotSpot;
	
	C_Line *Vline;
	C_Line *Hline;
	C_Button *Keycodes;
	C_Text *Mapping;

	Keycodes = button = (C_Button *)win->FindControl(KEYCODES); //define this as KEYCODES
	Mapping = text = (C_Text *)win->FindControl(MAPPING);     //define this as MAPPING
	Hline = (C_Line *)win->FindControl(HLINE);     //define this as HLINE
	Vline = (C_Line *)win->FindControl(VLINE);     //define this as VLINE

	HotSpot.X = -3;
	HotSpot.Y = -1;
	if(Vline)
	{
		HotSpot.W = Vline->GetX() -1;
		HotSpot.H = Vline->GetH() - 1;
	}
	else
	{
		HotSpot.W = 30;
		HotSpot.H = 12;
	}

	KeyVar.Modified = FALSE;

	NumUndispKeys = 0;
	
	SetCursor(gCursors[CRSR_WAIT]);
	while(fgets(buff,_MAX_PATH,fp) )
	{
		
		if (buff[0] == ';' || buff[0] == '\n' || buff[0] == '#')
               continue;
		
		if(sscanf (buff, "%s %d %d %x %x %x %x %d %[^\n]s", funcName, &buttonId, &mouseSide, &key2, &mod2, &key1, &mod1, &editable, &descrip) < 8)
			continue;

		if(key2 < -1)
			continue;

		theFunc = FindFunctionFromString (funcName);
		
		keydescrip[0] = 0;

		if(!theFunc)
			continue;

		KeyMap Map;
		Map.func = theFunc;
		Map.buttonId = buttonId;
		Map.mouseSide = mouseSide;
		Map.editable = editable;
		strcpy(Map.descrip, descrip);
		Map.key1 = key1;
		Map.mod1 = mod1;
		Map.key2 = key2;
		Map.mod2 = mod2;

		if(editable == -2)
		{
			AddUndisplayedKey(Map);
		
			ShiAssert(NumUndispKeys < 300);

			continue;
		}
		
		parsed = descrip +1;
		parsed[strlen(descrip) -2] = 0;
		parsed[37] = 0;

		if(!count)
		{
			//first time through .. special case
			UpdateKeyMapButton(button, Map, count);

			Mapping->Refresh();
			Mapping->SetText(parsed);
			Mapping->Refresh();
		}
		else
		{
			SetHdrStatusLine(win, Keycodes, Vline, Map, HotSpot, count);
			//the rest of the times through
			UpdateKeyMap(win, Keycodes, Vline->GetH(), Map, HotSpot, count );

			UpdateMappingDescrip(win, Mapping, Vline->GetH(),parsed, count);//this will add the control if it doesn't exist

			AddKeyMapLines(win, Hline, Vline, count);
		 }

		count++;
		if(count > 10000)
			break;
	}

	NumDispKeys = count;

	fclose(fp);

	while(RemoveExcessControls(win,count++));

	gMainHandler->WindowToFront(win);

	SetCursor(gCursors[CRSR_F16]);
	return TRUE;
}

void LoadKeyCB(long,short hittype,C_Base *control)
{
	C_EditBox * ebox;
	_TCHAR fname[MAX_PATH];

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	gMainHandler->HideWindow(control->Parent_);

	ebox=(C_EditBox*)control->Parent_->FindControl(FILE_NAME);
	if(ebox)
	{
		_tcscpy(fname,ebox->GetText());
		for(unsigned long i=0;i<_tcslen(fname);i++)
			if(fname[i] == '.')
				fname[i]=0;

		if(fname[0] == 0)
			return;
		
		KeyVar.Modified = TRUE;
		if(UpdateKeyMapList(fname, USE_FILENAME))
			_tcscpy(PlayerOptions.keyfile,ebox->GetText());
		
	}
}


void LoadKeyButtonCB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	SetDeleteCallback(DelKeyFileCB);
	LoadAFile(TXT_LOAD_KEYBOARD,"config\\*.KEY",NULL,LoadKeyCB,CloseWindowCB);
}

int CreateKeyMapList( char *filename )
{
	FILE *fp;
	char path[_MAX_PATH];

	C_Window *win;

	win = gMainHandler->FindWindow(SETUP_WIN);
	if(!win)
		return FALSE;

	sprintf(path,"%s\\config\\%s.key",FalconDataDirectory,filename);
	
	fp = fopen(path,"rt");
	if(!fp)
	{
		sprintf(path,"%s\\config\\keystrokes.key",FalconDataDirectory);
		fp = fopen(path,"rt");
		if(!fp)
			return FALSE;
	}
	
	int count = 0;
	int key1, mod1;
	int key2, mod2;
//	int flags =0,
	int buttonId, mouseSide, editable;
	InputFunctionType theFunc;
	char buff[_MAX_PATH];
	char funcName[_MAX_PATH];
	char keydescrip[_MAX_PATH];
	char descrip[_MAX_PATH];
	char *parsed;
	HotSpotStruct HotSpot;
	//int	 HotX,HotY,HotW,HotH;
	UI95_RECT	client;

	C_Button *Keycodes;
	C_Text *Mapping;
	C_Line *Vline;
	C_Line *Hline;

	keydescrip[0] = 0;

	client = win->GetClientArea(3);
	Keycodes = (C_Button *)win->FindControl(KEYCODES); //define this as KEYCODES
	Mapping = (C_Text *)win->FindControl(MAPPING);     //define this as MAPPING
	Hline = (C_Line *)win->FindControl(HLINE);     //define this as HLINE
	Vline = (C_Line *)win->FindControl(VLINE);     //define this as VLINE
	
	HotSpot.X = -3;
	HotSpot.Y = -1;
	if(Vline)
	{
		HotSpot.W = Vline->GetX() -1;
		HotSpot.H = Vline->GetH() - 1;
	}
	else
	{
		HotSpot.W = 30;
		HotSpot.H = 12;
	}

	NumDispKeys = 0;
	NumUndispKeys = 0;

	while(fgets(buff,_MAX_PATH,fp) )
	{
		if (buff[0] == ';' || buff[0] == '\n' || buff[0] == '#')
               continue;
		
		if(sscanf (buff, "%s %d %d %x %x %x %x %d %[^\n]s", funcName, &buttonId, &mouseSide, &key2, &mod2, &key1, &mod1, &editable, &descrip) < 8)
			continue;

		if(key2 == -2)
			continue;

		theFunc = FindFunctionFromString (funcName);
		
		keydescrip[0] = 0;

		if(!theFunc)
			continue;

		KeyMap Map;
		Map.func = theFunc;
		Map.buttonId = buttonId;
		Map.mouseSide = mouseSide;
		Map.editable = editable;
		strcpy(Map.descrip, descrip);
		Map.key1 = key1;
		Map.mod1 = mod1;
		Map.key2 = key2;
		Map.mod2 = mod2;

		if(editable == -2)
		{
			AddUndisplayedKey(Map);
		
			ShiAssert(NumUndispKeys < 300);

			continue;
		}

		parsed = descrip +1;
		parsed[strlen(descrip) -2] = 0;
		parsed[37] = 0;

		if(!count)
		{
			NumDispKeys++;
			//first time through .. special case
			UpdateKeyMapButton(Keycodes, Map, count);
						
			Keycodes->SetCallback(KeycodeCB);
			Keycodes->SetHotSpot(HotSpot.X, HotSpot.Y, HotSpot.W, HotSpot.H);		
			Keycodes->Refresh();

			Mapping->Refresh();
			Mapping->SetText(parsed);
			Mapping->Refresh();
		}
		else
		{
			NumDispKeys++;
			SetHdrStatusLine(win, Keycodes, Vline, Map, HotSpot, count);
			//the rest of the times through
			UpdateKeyMap(win, Keycodes, Vline->GetH(), Map, HotSpot, count );

			UpdateMappingDescrip(win, Mapping, Vline->GetH(),parsed, count);//this will add the control if it doesn't exist

			AddKeyMapLines(win, Hline, Vline, count);
		 }

	count++;
	if(count > 10000)
		break;
	}
   	
	fclose(fp);
	return FALSE;

}


void SetKeyDefaultCB(long,short,C_Base *)
{
	if(UpdateKeyMapList( 0,USE_DEFAULT))
		KeyVar.Modified = TRUE;
}

void ControllerSelectCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_SELECT)
		return;

	DIDEVICEOBJECTINSTANCE devobj;
	HRESULT hres;
	C_ListBox *lbox;
	lbox = (C_ListBox *)control;
	int JoystickSetupResult, newcontroller;

	//if( (lbox->GetTextID() < SIM_JOYSTICK1) || (lbox->GetTextID() > gTotalJoy + SIM_JOYSTICK1 - 1) )
	//	return;

	newcontroller = lbox->GetTextID() - 1;

	//Hack until other control modes work
	//newcontroller = SIM_JOYSTICK1;

	if(newcontroller == gCurController)
		return;

	if(gCurController == SIM_KEYBOARD)
	{
		SaveKeyMapList("laptop");
		UpdateKeyMapList( PlayerOptions.GetKeyfile(),TRUE );
	}

	if(newcontroller == SIM_MOUSE)
	{
		
	}
	else if(newcontroller == SIM_KEYBOARD)
	{
		SaveKeyMapList(PlayerOptions.GetKeyfile());
		UpdateKeyMapList("laptop",TRUE );
	}
	else if(newcontroller == SIM_NUMDEVICES)
	{
		//this is used to represent an intellipoint
	}
	else
	{
		C_Line		*line = NULL;
		C_Button	*button = NULL;
		int			hasPOV = FALSE;

		gCurJoyCaps.dwSize = sizeof(DIDEVCAPS);
		gpDIDevice[newcontroller]->GetCapabilities(&gCurJoyCaps);

		if(NumHats != -1)
			gCurJoyCaps.dwPOVs = NumHats;

		if(gCurJoyCaps.dwPOVs > 0)
			hasPOV = TRUE;

		button=(C_Button *)control->Parent_->FindControl(LEFT_HAT);
		if(button != NULL)
		{
			if (!hasPOV)
				button->SetFlagBitOn(C_BIT_INVISIBLE);
			else
				button->SetFlagBitOff(C_BIT_INVISIBLE);
			button->Refresh();
		}
		
		button=(C_Button *)control->Parent_->FindControl(RIGHT_HAT);
		if(button != NULL)
		{
			if (!hasPOV)
				button->SetFlagBitOn(C_BIT_INVISIBLE);
			else
				button->SetFlagBitOff(C_BIT_INVISIBLE);
			button->Refresh();
		}
		
		button=(C_Button *)control->Parent_->FindControl(CENTER_HAT);
		if(button != NULL)
		{
			if (!hasPOV)
				button->SetFlagBitOn(C_BIT_INVISIBLE);
			else
				button->SetFlagBitOff(C_BIT_INVISIBLE);
			button->Refresh();
		}
		
		button=(C_Button *)control->Parent_->FindControl(UP_HAT);
		if(button != NULL)
		{
			if (!hasPOV)
				button->SetFlagBitOn(C_BIT_INVISIBLE);
			else
				button->SetFlagBitOff(C_BIT_INVISIBLE);
			button->Refresh();
		}
		
		button=(C_Button *)control->Parent_->FindControl(DOWN_HAT);
		if(button != NULL)
		{
			if (!hasPOV)
				button->SetFlagBitOn(C_BIT_INVISIBLE);
			else
				button->SetFlagBitOff(C_BIT_INVISIBLE);
			button->Refresh();
		}
		devobj.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);

		hres = gpDIDevice[newcontroller]->GetObjectInfo(&devobj, g_nThrottleID,DIPH_BYOFFSET);

		if(SUCCEEDED(hres))
			IO.SetAnalogIsUsed(2, TRUE);
		else
			IO.SetAnalogIsUsed(2, FALSE);

		line=(C_Line *)control->Parent_->FindControl(THROTTLE);
		if(line != NULL)
		{
			line->Refresh();
			if( !IO.AnalogIsUsed(2) )
			{
				line->SetColor(RGB(130,130,130)); //grey
				line->SetH(Throttle.bottom - Throttle.top);
				line->SetY(Throttle.top);
			}
			else
				line->SetColor(RGB(60,123,168)); //blue
			line->Refresh();
		}

		hres = gpDIDevice[newcontroller]->GetObjectInfo(&devobj,DIJOFS_RZ,DIPH_BYOFFSET);
		if(!NoRudder && SUCCEEDED(hres))
			IO.SetAnalogIsUsed(3, TRUE);
		else
			IO.SetAnalogIsUsed(3, FALSE);

		line=(C_Line *)control->Parent_->FindControl(RUDDER);
		if(line != NULL)
		{
			line->Refresh();
			if( !IO.AnalogIsUsed(3) )
			{
				line->SetColor(RGB(130,130,130)); //grey
				line->SetH(Rudder.bottom - Rudder.top);
				line->SetY(Rudder.top);
			}
			else
				line->SetColor(RGB(60,123,168)); //blue
			line->Refresh();
		}

		JoystickSetupResult = VerifyResult(gpDIDevice[newcontroller]->Acquire());
		gpDeviceAcquired[newcontroller] = JoystickSetupResult;
	}
	
	gCurController = newcontroller;
	//make selected joystick current joystick
}

void BuildControllerList(C_ListBox *lbox)
{	
	lbox->RemoveAllItems();
	
	//lbox->AddItem(SIM_MOUSE + 1,C_TYPE_ITEM,TXT_MOUSE);
	lbox->AddItem(SIM_KEYBOARD + 1,C_TYPE_ITEM,TXT_KEYBOARD);
	//lbox->AddItem(SIM_NUMDEVICES + 1,C_TYPE_ITEM,TXT_INTELLIPOINT);

	if(gTotalJoy)
	{
		for(int i = 0; i < gTotalJoy; i++)
		{
			if(!stricmp(gDIDevNames[SIM_JOYSTICK1 + i],"tm"))
			{
				delete [] gDIDevNames[SIM_JOYSTICK1 + i];
				gDIDevNames[SIM_JOYSTICK1 + i] = new TCHAR[MAX_PATH];
				_tcscpy(gDIDevNames[SIM_JOYSTICK1 + i],"Thrustmaster");
			}

			lbox->AddItem(i + SIM_JOYSTICK1 + 1,C_TYPE_ITEM,gDIDevNames[SIM_JOYSTICK1 + i]);
			if(mHelmetIsUR && i == mHelmetID)
				lbox->SetItemFlags(i + SIM_JOYSTICK1 + 1,0);
		}
	}
	lbox->Refresh();
}
