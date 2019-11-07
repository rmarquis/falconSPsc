#include "stdhdr.h"
#include "missile.h"

void MissileClass::Atmosphere(void)
{
float ttheta;     /* Temp Ratio */
float rsigma;     /* Density Ration */
float pdelta;     /* Pressure Delta */
float sound;      /* Speed of sound at altitude */

   /*-----------------------------------------------*/
   /* calculate temperature ratio and density ratio */
   /*-----------------------------------------------*/
   if (alt <= 36089.0F)
   {   
      ttheta = 1.0F - 0.000006875F*alt;
      rsigma = (float)pow (ttheta,4.256F);
   }
   else
   {
      ttheta = 0.7519F;
      rsigma = 0.2971F*(float)exp((36089.0F - alt)/20807.32F);
   }

   /*------------------------------*/
   /* Density, Pressure, Temp, SOS */
   /*------------------------------*/
   pdelta = ttheta*rsigma;
   sound  = AASL*(float)sqrt(ttheta);
   rho    = RHOASL*rsigma;
   ps     = PASL*pdelta;
           
   /*-----------------------------*/
   /* calculate dynamic pressure  */
   /*-----------------------------*/
   mach   = vt/sound;
	 if (ifd) // JB 010718 CTD
	   ifd->qbar   = 0.5F*rho*vt*vt;

   /*------------------------------------------------------------------*/
   /* calculate equivalent and calibrated airspeed and impact pressure */
   /*------------------------------------------------------------------*/
   vcas = FTPSEC_TO_KNOTS*vt*(float)sqrt(rsigma);

	 if (!ifd) // JB 010718 CTD
		return;

   /*------------------------*/
   /* normalizing parameters */
   /*------------------------*/
	 if (mass && !F4IsBadReadPtr(inputData, sizeof(MissileInputData))) // JB 010317 CTD
		ifd->qsom = ifd->qbar*inputData->area/mass;
   ifd->qovt = ifd->qbar/vt;
} 
      
