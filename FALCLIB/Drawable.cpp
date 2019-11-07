#include "Renderer\Render2d.h"
#include "sim\include\stdhdr.h"
#include "drawable.h"
#include "sim\include\otwdrive.h"
#include "sim\include\cppanel.h"

void DrawableClass::DisplayExit (void)
{
   if (privateDisplay)
   {
      privateDisplay->Cleanup();
      delete privateDisplay;
      privateDisplay = NULL;
   }
}


void DrawableClass::LabelButton (int idx, char* str1, char* str2, int inverse)
{
// NOTE - Sms buttons are labeled starting at the top left and
//        proceeding clockwise around the MFD.
static float textLoc[20][2] = {
   -0.6F,  0.95F,
   -0.3F,  0.95F,
    0.0F,  0.95F,
    0.3F,  0.95F,
    0.6F,  0.95F,
    0.95F,  0.6F,
    0.95F,  0.3F,
    0.95F,  0.0F,
    0.95F, -0.3F,
    0.95F, -0.6F,
    0.6F, -0.85F,
    0.3F, -0.85F,
    0.0F, -0.85F,
   -0.3F, -0.85F,
   -0.6F, -0.85F,
   -0.95F, -0.6F,
   -0.95F, -0.3F,
   -0.95F,  0.0F,
   -0.95F,  0.3F,
   -0.95F,  0.6F};

float multiLineOffset;
float xPos, yPos;
CPPanel* curPanel = OTWDriver.pCockpitManager->GetActivePanel();

   if (curPanel)
   {
      if ((MFDOn % 2) == 1)
      {
         xPos = curPanel->osbLocation[1][idx][0];
         yPos = curPanel->osbLocation[1][idx][1];
      }
      else
      {
         xPos = curPanel->osbLocation[0][idx][0];
         yPos = curPanel->osbLocation[0][idx][1];
      }
   }
   else
   {
      xPos = textLoc[idx][0];
      yPos = textLoc[idx][1];
   }


   multiLineOffset = display->TextHeight();

	if (inverse)
	{
		inverse = 2;
	}

   if (str2 == NULL)
   {
      if (idx > 4 && idx < 10)
      {
         display->TextRight (xPos, yPos, str1, inverse);
      }
      else if (idx > 14)
      {
         display->TextLeft (xPos, yPos, str1, inverse);
      }
      else
         display->TextCenter (xPos, yPos, str1, inverse);
   }
  else
   {
      if (idx <= 4)
      {
         display->TextCenter (xPos, yPos, str1, inverse);
         display->TextCenter (xPos, yPos - multiLineOffset, str2, inverse);
      }
      else if (idx > 4 && idx < 10)
      {
         display->TextRight (xPos, yPos + multiLineOffset*0.5F, str1, inverse);
         display->TextRight (xPos, yPos - multiLineOffset*0.5F, str2, inverse);
      }
      else if (idx > 14)
      {
         display->TextLeft (xPos, yPos + multiLineOffset*0.5F, str1, inverse);
         display->TextLeft (xPos, yPos - multiLineOffset*0.5F, str2, inverse);
      }
      else
      {
         display->TextCenter (xPos, yPos, str1, inverse);
         display->TextCenter (xPos, yPos + multiLineOffset, str2, inverse);
      }
   }
}
