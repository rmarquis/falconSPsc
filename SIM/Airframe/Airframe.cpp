/******************************************************************************/
/*                                                                            */
/*  Unit Name : six_dof.cpp                                                   */
/*                                                                            */
/*  Abstract  : Single point interface for AirframeClass.                    */
/*                                                                            */
/*  Dependencies : Auto-Generated                                             */
/*                                                                            */
/*  Operating System : MS-DOS 6.2,                                            */
/*                                                                            */
/*  Compiler : WATCOM C/C++ V10                                               */
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
#include "stdhdr.h"
#include "airframe.h"
#include "helimm.h"
#include "otwdrive.h"
#include "Graphics\Include\tmap.h"
#include "ui\include\logbook.h"
#include "simbase.h"
#include "soundfx.h"
#include "fsound.h"
#include "simdrive.h"
#include "aircrft.h"
#include "arfrmdat.h"
#include "find.h"
#include "objectiv.h"
#include "atcbrain.h"
#include "Graphics\Include\terrtex.h"
#include "Graphics\Include\rviewpnt.h"  // to get ground type
#include "digi.h"
#include "sms.h"
#include "sim\include\dofsnswitches.h"
#include "ptdata.h"

#include "acmi\src\include\acmirec.h"
extern ACMIRecorder gACMIRec;
extern ACMISwitchRecord acmiSwitch;

extern AeroDataSet *aeroDataset;
//MI for CATIII Default stuff
//	extern bool g_bEnableCATIIIExtension;	//MI replaced with g_bRealisticAvionics
extern bool g_bCATIIIDefault;
//MI RALT stuff
extern bool g_bRealisticAvionics;
extern float g_fRecoveryAOA;
extern unsigned long gFuelState;

#include "flightdata.h"

#ifdef USE_SH_POOLS
MEM_POOL	AirframeClass::pool;
#endif

extern float get_air_speed (float, int);

#ifdef CHECK_PROC_TIMES
ulong gAtmosTime = 0;
ulong gFLCSTime = 0;
ulong gAeroTime = 0;
ulong gEngineTime = 0;
ulong gEOMTime = 0;
ulong gAirframe = 0;
#endif

AirframeClass::AirframeClass(AircraftClass* self)
{
	memset(this, 0 , sizeof(AirframeClass));
   // Initialze interpolaters
   nzcgb = 0.0F;
   curMachBreak			= curAlphaBreak		= 0;
   curRollAlphaBreak	= curRollQbarBreak	= 0;
   curEngMachBreak		= curEngAltBreak	= 0;
   jp01 = jp02 = 0;
   jy01 = jy02 = 0;
   platform = self;
   vehicleIndex = 4; //default to f-16

   pstab	= rstab		= 0.0F;
   plsdamp	= rlsdamp	= ylsdamp	= 1.0f;
   zpdamp	= 0.0F;
   dragIndex	= 0.0F;
   loadingFraction = 1.0F;
   assymetry	= 0.0F;
   tefFactor	= lefFactor = 0.0F;
   curMaxGs		= 9.0F;
   stallMode	= None;
   oscillationTimer		= 0.0F;
   oscillationSlope		= 0.0F;
   p	= q		= r		= 0.0F;
   xdot = ydot	= zdot	= 0.0F;
   alpdot = betdot =0.0F;
   initialFuel = 0.0F;
   tp04 = zp02 = kp04 = kp06 = wp02 = 0.0F;
   kr03 = kr04 = wr01 = 0.0F;
   ty03 = zy02 = ky04 = ky06 = wy02 = 0.0F;
   
   ptrmcmd = 0.0F;
   rtrmcmd = 0.0F;
   ytrmcmd = 0.0F;
   //rshape2 = 0.0f;
   //pshape2 = 0.0f;
   //yshape2 = 0.0f;
   rshape1	= 0.0f;
   pshape1	= 0.0f;
   yshape1	= 0.0f;
   slice	= 0.0F;
   pitch	= 0.0F;
   avgPdelta = avgRdelta = avgYdelta = 0;
   //limiterAssault = 0.0f;
   stallMagnitude = 15.0f;
   desiredMagnitude = 15.0f;
   throtl	= 0.0F;
   speedBrake = 0.0F;
   dbrake	= 0.0F;
   anozl	= 0.0F;
   psi		= theta = phi	= 0.0F;
   mu		= sigma = gmma	= 0.0F;
   alpha	= beta	= 0.0F;
   gearPos	= 0.0F;
   gearHandle = -1.0F;
	 hookHandle = -1.0F; // JB carrier
   altGearDeployed = false; // JPO - marker
   gear		= NULL; // JPO - need to free this ... but where...
   grndphi = grndthe = 0.0F;
    lefPos = tefPos = 0;

   flags	= 0;
   ethrst	= 1.0F;
   xaero	= yaero		= zaero		= 0.0F;
   xwaero	= ywaero	= zwaero	= 0.0F;
   xsaero	= ysaero	= zsaero	= 0.0F;
   xprop	= yprop		= zprop		= 0.0F;
   xwprop	= ywprop	= zwprop	= 0.0F;
   xsprop	= ysprop	= zsprop	= 0.0F;
   epuFuel	= 100.0F;
   epuState	= AUTO;
   hydrAB	= 0; // JPO - start with none.
   jfsaccumulator = 100.0f;
   externalFuel = 0.0F;
   SetFlag (Trimming);
   SetFlag (InAir);
   SetFlag(EngineStopped); // JPO - start off stopped - preflight will start it
   pstick	= rstick	= ypedal	= 0.0F;
   // assume not simple model
   simpleMode = SIMPLE_MODE_OFF;
   hf = NULL;

   gsAvail = 0.0F;
   maxRollDelta = 0.0F;
   startRoll = 0.0F;
   thrust = 0.0F;
   forcedHeading = 0.0F;
   forcedSpeed = 0.0F;
	
#if 1
   // Set our anchor so that when we're moving slowly we can accumulate our position in high precision
   groundAnchorX = 0.0f;
   groundAnchorY = 0.0f;
   groundDeltaX = 0.0f;
   groundDeltaY = 0.0f;
#endif

   memset(oldp01, 0, sizeof(oldp01));
   memset(oldp02, 0, sizeof(oldp02));
   memset(oldp03, 0, sizeof(oldp03));
   memset(oldp04, 0, sizeof(oldp04));
   memset(oldp05, 0, sizeof(oldp05));

   memset(oldr01, 0, sizeof(oldr01));

   memset(oldy01, 0, sizeof(oldy01));
   //memset(oldy02, 0, sizeof(oldy02));
   memset(oldy03, 0, sizeof(oldy03));
   //memset(oldy04, 0, sizeof(oldy04));

   memset(olda01, 0, sizeof(olda01));

   memset(oldRpm, 0, sizeof(oldRpm));
	fuelFlow = 0.0F;
   oldnzcgs = 1.0F;
   vRot = 0.0F;
   vtDot = 0.0F;
   //betdot = 0.0F;
   zr01 = 0.0F;
   ky05 = 0.0F;

   engineFlags = MasterFuelOff; // JPO - set up the engine and fuel systems
    fuelSwitch = FS_NORM;
		fuelPump = FP_OFF; // JB 010414
		airSource = AS_OFF; // JB 010414
    ClearFuel();
    memset(m_tankcap, 0, sizeof m_tankcap);
    memset(m_trate, 0, sizeof m_trate);

	//MI init Landinglights and Parkingbrake
	LLON = FALSE;
	PBON = FALSE;
	BrakesToggle = FALSE;
	epuBurnState = EpuNone;
	generators = GenNone;

	//MI Home Fuel stuff
	HomeFuel = 0;
	//MI JFS spin time
	JFSSpinTime = 240;	//4 minutes available
	dragChute = DRAGC_STOWED;
	canopyState = false;
	nozzlePos = 0;
}

AirframeClass::~AirframeClass(void)
{
    delete []gear;
    if ( hf )
    {
	delete hf;
    }
}

// assign data - break into two parts else we have a race to initialise.
void AirframeClass::InitData (int idx)
{
	if (idx >= 0)
	{
		ReadData(idx);
		AeroRead(idx);
		AuxAeroRead(idx); // JB 010714
		EngineRead(idx);
		FcsRead(idx);
	}
	else
	{
		MonoPrint ("Unknown airframe index %d, using F16 instead\n", idx);
		ReadData(4);
		AeroRead(4);
		AuxAeroRead(4); // JB 010714
		EngineRead(4);
		FcsRead(4);
	}
}

void AirframeClass::Init (int idx)
{
	// stuff that ought to be read in (JPO)
	if(auxaeroData->fuelFwdRes == 0 && auxaeroData->fuelAftRes == 0) {
// everything else - guess based on F16 proportions.
#if 0
	    platform->IsF16()) {
	    m_tankcap[TANK_FWDRES] = 480.0f;
	    m_tankcap[TANK_AFTRES] = 480.0f;
	    m_tankcap[TANK_A1] = 2810.0f - 480.0f;
	    m_tankcap[TANK_F1] = 3250.0f - 480.0f;
	    m_tankcap[TANK_WINGAL] = 550.0f;
	    m_tankcap[TANK_WINGFR] = 550.0f;
	    m_trate[TANK_A1] = m_trate[TANK_F1] = 20000.0f/3600.0f;
	    m_trate[TANK_WINGAL]  = m_trate[TANK_WINGFR]= 6000.0f/3600.0f;
	    m_trate[TANK_LEXT] = m_trate[TANK_REXT] = 30000.0f/3600.0f;
	    m_trate[TANK_CLINE] = 18000.0f/3600.0f;
	else { 
#endif
	    float intfuel = aeroDataset[idx].inputData[AeroDataSet::InternalFuel];
	    // assume 7.5% in resevoirs
	    m_tankcap[TANK_AFTRES] = m_tankcap[TANK_FWDRES] = intfuel * 0.075f;
	    // 7.7% in each wing 
	    m_tankcap[TANK_WINGAL] = m_tankcap[TANK_WINGFR] = intfuel * 0.077f;
	    intfuel -= m_tankcap[TANK_AFTRES] + m_tankcap[TANK_FWDRES] + 
		m_tankcap[TANK_WINGAL] + m_tankcap[TANK_WINGFR];
	    // rest split between fwd and aft tanks.
	    m_tankcap[TANK_F1] = intfuel / 2;
	    m_tankcap[TANK_A1] = intfuel / 2;
	    m_trate[TANK_A1] = m_trate[TANK_F1] = 4 * 20000.0f/3600.0f;
	    m_trate[TANK_WINGAL]  = m_trate[TANK_WINGFR]= 4 * 6000.0f/3600.0f;
	    m_trate[TANK_LEXT] = m_trate[TANK_REXT] = 4 * 30000.0f/3600.0f;
	    m_trate[TANK_CLINE] = 4 * 18000.0f/3600.0f;
	}
	else {
	    m_tankcap[TANK_FWDRES] = auxaeroData->fuelFwdRes;
	    m_tankcap[TANK_AFTRES] = auxaeroData->fuelAftRes;
	    m_tankcap[TANK_A1] = auxaeroData->fuelAft1;
	    m_tankcap[TANK_F1] = auxaeroData->fuelFwd1;
	    m_tankcap[TANK_WINGAL] = auxaeroData->fuelWingAl;
	    m_tankcap[TANK_WINGFR] = auxaeroData->fuelWingFr;
	    m_trate[TANK_A1] = auxaeroData->fuelAft1Rate;
	    m_trate[TANK_F1] = auxaeroData->fuelFwd1Rate;
	    m_trate[TANK_WINGAL]  = auxaeroData->fuelWingAlRate;
	    m_trate[TANK_WINGFR]= auxaeroData->fuelWingFrRate;
	    m_trate[TANK_LEXT] = m_trate[TANK_REXT] = auxaeroData->fuelWingExtRate;
	    m_trate[TANK_CLINE] = auxaeroData->fuelClineRate;

	    ShiAssert(m_tankcap[TANK_FWDRES] + m_tankcap[TANK_AFTRES] +
		m_tankcap[TANK_A1] + m_tankcap[TANK_F1] +
		m_tankcap[TANK_WINGAL] + m_tankcap[TANK_WINGFR] ==
		aeroDataset[idx].inputData[AeroDataSet::InternalFuel]);
	}
	FindExternalTanks();

	// tank transfer rates.
	m_trate[TANK_FWDRES] = 0.0f;
	m_trate[TANK_AFTRES] = 0.0f;

	// now split the fuel between all tanks.
	AllocateFuel(initialFuel);
	/*--------------*/
	/* Initial Mass */
	/*--------------*/
	mass  = weight / GRAVITY;
    loadingFraction = weight/emptyWeight;

	/*------------------*/
	/* initial velocity */
	/*------------------*/
	Atmosphere();

	if (IsSet(InAir))
	{
		Initialize();
		TrimModel();		
#ifndef ACMI
		//InitializeEOM();
#endif
	}
	else
	{
		vt			= 0.0F;
		thrtab		= 0.0F;
		dbrake		= 0.0F;
		gearPos		= 1.0F;
		gearHandle	= 1.0F;
		// lights on on the ground JPO
		//MI not in realistic, now we have switches for that
		if(!g_bRealisticAvionics)
			platform->SetStatusBit (STATUS_EXT_LIGHTS|STATUS_EXT_NAVLIGHTS|STATUS_EXT_TAILSTROBE);
		else
			platform->ClearStatusBit (STATUS_EXT_LIGHTS|STATUS_EXT_NAVLIGHTS|STATUS_EXT_TAILSTROBE);
		speedBrake	= 0.0F;
		pwrlev		= -1.0F;
		sigma		= initialPsi;
		//Trigenometry();
		ClearFlag(Trimming);
		Initialize();
		Aerodynamics();
		Gains();
	}

	// Get the height of the ground under us
	groundZ = OTWDriver.GetGroundLevel(x, y, &gndNormal);

	// Set our anchor point (only used if we start on the ground, but what the hey...)
	groundAnchorX = x;
	groundAnchorY = y;
	groundDeltaX = 0.0f;
	groundDeltaY = 0.0f;

	//MI CATIII as default
	//if(g_bCATIIIDefault && g_bEnableCATIIIExtension)	MI
	if(g_bCATIIIDefault && g_bRealisticAvionics)
	{
		if(platform->IsPlayer())
			SetFlag(CATLimiterIII);
	}
	//MI RALT stuff
	if(g_bRealisticAvionics)
	{
		if(platform->IsPlayer())
		{
		    platform->RALTStatus = AircraftClass::RaltStatus::ROFF;
		}
	}
	//MI PB and LL stuff
	if(platform->IsPlayer())
	{
		LLON = FALSE;
		PBON = FALSE;
		BrakesToggle = FALSE;
	}
	//MI
	JFSSpinTime = auxaeroData->jfsSpinTime;
}

void AirframeClass::Reinit (void)
{
	if (simpleMode == SIMPLE_MODE_HF )
	{
		if ( hf == NULL )
		{
	    	if ( stricmp( LogBook.Name(), "mrsteed0" ) == 0 )
				hf = new HeliMMClass( platform, A109 );
	    	else if ( stricmp( LogBook.Name(), "mrsteed1" ) == 0 )
				hf = new HeliMMClass( platform, COBRA );
	    	else if ( stricmp( LogBook.Name(), "mrsteed2" ) == 0 )
				hf = new HeliMMClass( platform, MD500 );
			else
				hf = new HeliMMClass( platform, STABLE );
		}
      	hf->SetControls( 0.0f, 0.0f, 0.0f, 0.0f );
		hf->Init( x, y, z );
	}
	else if (IsSet(InAir))
	{
		// LRKLUDGE
		//Clamp vt to some minimum value
		if(!platform->IsPlayer())
		{
			vt		= max (vt, minVcas * 0.6F * KNOTS_TO_FTPSEC);
			vcas	= vt * FTPSEC_TO_KNOTS;

			if(!IsSet(GearBroken))
			{
				gearPos		= 0.0F;
				gearHandle	= -1.0F;
			}
		}		

		ClearFlag(Trimming);
		ClearFlag(EngineOff);
		ReInitialize();
	}
	else
	{
		// Set our anchor point for low speed/high precision maneuvering
		groundAnchorX = x;
		groundAnchorY = y;
		groundDeltaX = 0.0f;
		groundDeltaY = 0.0f;

		if(!IsSet(GearBroken))
		{
			gearPos		= 1.0F;
			gearHandle	= 1.0F;
		}

		ClearFlag(Trimming);
		ReInitialize();
	}

	//MI CATIII as default
	//if(g_bCATIIIDefault && g_bEnableCATIIIExtension)	MI
	if(g_bCATIIIDefault && g_bRealisticAvionics)
	{
		if(platform->IsPlayer())
		{
			SetFlag(CATLimiterIII);
		}
	}
	//MI RALT stuff
	if(g_bRealisticAvionics)
	{
		if(platform->IsPlayer())
		{
		    platform->RALTStatus = AircraftClass::RaltStatus::ROFF;
		}
	}
	//MI PB and LL stuff
	if(platform->IsPlayer())
	{
		LLON = FALSE;
		PBON = FALSE;
	}
	//MI
	JFSSpinTime = auxaeroData->jfsSpinTime;
}

void AirframeClass::Exec(void)
{
	int i;

// 2002-03-28 MN for refuel debugging
	if (gFuelState && this == SimDriver.playerEntity->af)
	{
		AllocateFuel(gFuelState);
		gFuelState = 0;
	}

	// helicopter mode easter egg
	if ( hf && simpleMode == SIMPLE_MODE_HF )
	{
		RunHeliModel();
		return;
	}

	// simple flight model for AI's
	if ( simpleMode == SIMPLE_MODE_AF 
		&& !IsSet(AirframeClass::OnObject)) // JB carrier
	{
		Atmosphere();
		Aerodynamics();
		Axial(SimLibMajorFrameTime);
		EngineModel(SimLibMajorFrameTime);
		Accelerometers();

		SimpleModel();
		return;
	}

#ifdef CHECK_PROC_TIMES
	gAirframe = GetTickCount();
#endif
	for (i=0; i<SimLibMinorPerMajor; i++)
	{
#ifdef CHECK_PROC_TIMES
		gAtmosTime = GetTickCount();
#endif
		Atmosphere();
#ifdef CHECK_PROC_TIMES
		gAtmosTime = GetTickCount() - gAtmosTime;
		gFLCSTime = GetTickCount();
#endif
		FlightControlSystem();
#ifdef CHECK_PROC_TIMES
		gFLCSTime = GetTickCount() - gFLCSTime;
		gAeroTime = GetTickCount();
#endif
		Aerodynamics();
#ifdef CHECK_PROC_TIMES
		gAeroTime = GetTickCount() - gAeroTime;
		gEngineTime = GetTickCount();
#endif
		EngineModel(SimLibMinorFrameTime);
		Accelerometers();
#ifdef CHECK_PROC_TIMES
		gEngineTime = GetTickCount() - gEngineTime;
		gEOMTime = GetTickCount();
#endif
		EquationsOfMotion(SimLibMinorFrameTime);
#ifdef CHECK_PROC_TIMES
		gEOMTime = GetTickCount() - gEOMTime;
#endif
	}
#ifdef CHECK_PROC_TIMES
	gAirframe = GetTickCount() - gAirframe;
	//MonoPrint("Airframe: %3d Atmos: %3d FLCS: %3d Aero %3d Eng: %3d EOM: %3d\n", gAirframe, gAtmosTime, gFLCSTime, gAeroTime, gEngineTime, gEOMTime);
#endif
}

void AirframeClass::RemoteUpdate(void)
{
	ObjectGeometry	*pa = platform->platformAngles;
	mlTrig Trig;
	float newGamma, newSigma;
	float dt;

	dt = (vuxGameTime - platform->LastUpdateTime ()) / 1000.0F;

	platform->SetUpdateTime (vuxGameTime);

	xdot = platform->XDelta();
	ydot = platform->YDelta();
	zdot = platform->ZDelta();

//	MonoPrint ("Remote Update %08x ", platform);
//	MonoPrint ("x,y,z %f %f %f\n", platform->XPos (), platform->YPos (), platform->ZPos ());
//	MonoPrint ("dt %f\n", dt);
//	MonoPrint ("dx,dy,dz %f %f %f\n", xdot, ydot, zdot);

	x = platform->XPos() + xdot * dt;
	y = platform->YPos() + ydot * dt;
	z = platform->ZPos() + zdot * dt;

	groundAnchorX = x;
	groundAnchorY = y;
	groundDeltaX = 0.0f;
	groundDeltaY = 0.0f;
//	MonoPrint ("x,y,z %f %f %f\n", x, y, z);

	theta = platform->Pitch();
	psi = platform->Yaw();

	mlSinCos(&Trig, theta);
	pa->sinthe = Trig.sin;
	pa->costhe = Trig.cos;

	mlSinCos(&Trig, psi);
	pa->sinpsi = Trig.sin;
	pa->cospsi = Trig.cos;

	float xydelta = (float)sqrt(xdot * xdot + ydot * ydot);

	if(xydelta && !platform->OnGround ())
		newSigma = (float)atan2(ydot, xdot);
	else 
		newSigma = psi;

	r = (sigma - newSigma)/SimLibMajorFrameTime;
	sigma = newSigma;

	groundZ = OTWDriver.GetGroundLevel(x, y, &gndNormal);
	float mag = (float)sqrt(gndNormal.x*gndNormal.x + gndNormal.y*gndNormal.y + gndNormal.z*gndNormal.z);
	gndNormal.x /= mag;
	gndNormal.y /= mag;
	gndNormal.z /= mag;

	if(platform->OnGround ())
	{
		CalculateGroundPlane(&gmma, &mu);
		q = 0.0F;
		p = 0.0F;
		phi = mu;
	}
	else
	{
		//not strictly true but close enough
		p = (phi - platform->Roll())/SimLibMajorFrameTime;
		mu = phi = platform->Roll();

		if(xydelta)
			newGamma = (float)atan(zdot/xydelta);
		else if(platform->OnGround ())
			newGamma = gmma;
		else
		{
			newGamma = 0.0F;
			vt = 300.0F*KNOTS_TO_FTPSEC;
		}

		q = (gmma - newGamma)/SimLibMajorFrameTime;
		gmma = newGamma;
	}

	mlSinCos(&Trig, mu);
	pa->sinmu = Trig.sin;
	pa->cosmu = Trig.cos;
	pa->sinphi = Trig.sin;
	pa->cosphi = Trig.cos;

	mlSinCos(&Trig, gmma);
	pa->singam = Trig.sin;
	pa->cosgam = Trig.cos;

	mlSinCos(&Trig, sigma);
	pa->sinsig = Trig.sin;
	pa->cossig = Trig.cos;

	ResetOrientation();	

	platform->dmx[0][0] =  pa->cospsi * pa->costhe;
	platform->dmx[0][1] =  pa->sinpsi * pa->costhe;
	platform->dmx[0][2] = -pa->sinthe;
	
	platform->dmx[1][0] =  pa->cospsi * pa->sinthe * pa->sinphi - pa->sinpsi * pa->cosphi;
	platform->dmx[1][1] =  pa->cospsi * pa->cosphi + pa->sinpsi * pa->sinthe * pa->sinphi;
	platform->dmx[1][2] =  pa->costhe * pa->sinphi;
						
	platform->dmx[2][0] =  pa->sinpsi * pa->sinphi + pa->cospsi * pa->sinthe * pa->cosphi;
	platform->dmx[2][1] = -pa->cospsi * pa->sinphi + pa->sinpsi * pa->sinthe * pa->cosphi;
	platform->dmx[2][2] =  pa->costhe * pa->cosphi;

	vcas = get_air_speed(vt * FTPSEC_TO_KNOTS, -1*FloatToInt32(z));

	pstick = 0.0F; // (q*vt + platform->platformAngles->cosmu * platform->platformAngles->cosgam * 0.5F)/9.0F;
	rstick = 0.0F; // p/(225.0F*DTR);
	ypedal = 0.0F;
	platform->MoveSurfaces();
	platform->DBrain()->CheckLead();
	platform->DBrain()->UpdateTaxipoint();	

	if (platform->OnGround ())
	{
		ClearFlag (InAir);
		if(vt < 80.0F *KNOTS_TO_FTPSEC)
			SetFlag(Planted);
		else
			ClearFlag(Planted);
		
	}
	else
	{
		SetFlag (InAir);
		ClearFlag (Planted);
	}

	if (platform->Status () & STATUS_GEAR_DOWN)
	{
		gearHandle = 1.0F;
		gearPos += 0.3F * SimLibMinorFrameTime;
	}
	else 
	{
		gearHandle = -1.0F;
		gearPos -= 0.3F * SimLibMinorFrameTime;
	}

	gearPos = min ( max ( gearPos, 0.0F), 1.0F);
}

void AirframeClass::SetPosition (float nx, float ny, float nz)
{
	x = nx;
	y = ny;
	z = nz;
}


void AirframeClass::SetStallConditions(void)
{
	mlTrig Trig;

#if 0
	static count = 0;
	//if(!count)
		//MonoPrint("P*(Y+R): %f  P+R+Y/2: %f  Damp: %f  Alpha: %f\n  ",avgPdelta*(avgRdelta + avgYdelta),avgPdelta+avgRdelta+avgYdelta/2.0F,zp01,alpha);

	count++;
	count %= 1;
#endif
	//avgPdelta = (fabs(fabs(pstick) - pshape1)+ fabs(pshape1 - pshape2))/2.0f;
	//avgRdelta = (fabs(fabs(rstick) - rshape1)+ fabs(rshape1 - rshape2))/2.0f;
	//avgYdelta = (fabs(fabs(ypedal) - yshape1)+ fabs(yshape1 - yshape2))/2.0f;

	avgPdelta = (float)fabs(fabs(pstick) - pshape1);
	avgRdelta = (float)fabs(fabs(rstick) - rshape1);
	avgYdelta = (float)fabs(fabs(ypedal) - yshape1);

	//pshape2 = pshape1;
	pshape1 = (float)fabs(pstick);
	
	//rshape2 = rshape1;
	rshape1 = (float)fabs(rstick);
	
	//yshape2 = yshape1;
	yshape1 = (float)fabs(ypedal);
	
   if(platform->IsF16() &&
      platform->AutopilotType() == AircraftClass::APOff)
	{		
		switch(stallMode)
		{
		case None:
			
			zpdamp *= 0.99F;
			if((avgPdelta * (0.05F + avgRdelta + avgYdelta))/vcas > 0.0005F)
			{
				zpdamp = (avgPdelta * (0.05F + avgRdelta + avgYdelta))/vcas * 5.0F;
				zpdamp = min(zpdamp, 0.2F);
			}
						
			if(	!IsSet(Simplified)	&& 
				(	(!platform->OnGround () && vcas < 180.0f) ||
					( fabs(pshape) > 0.85F  && fabs(rshape) > 0.85F )	|| 
					( fabs(alpha)  > 18.0F)	||//&& IsSet(LowSpdHorn) )		||
					( fabs(pshape) > 0.6F	&& fabs(rshape) > 0.6F && IsSet(LowSpdHorn) ) ) )
			{
				loadingFraction = weight/emptyWeight;//me123
				if(	(!platform->OnGround () && vcas < 60.0f ||
					 !platform->OnGround () && fabs(alpha)  > 18.0F &&
					  vcas < 60.0f + 
					  60*(loadingFraction - 1.3F) + 
					  10*fabs(assymetry/weight)*10.0F) ||

					alpha > 31.0F  - 9*(loadingFraction - 1.3F) - //me123 addet 9
								//me123 bulshit IsSet(LowSpdHorn)*3.0F 
								//me123 - IsSet(CATLimiterIII)*5.0F           
								- fabs(assymetry/weight)*10.0F		||// 10 from 5 me123
					alpha < -14.0F + (loadingFraction - 1.3F) +
								IsSet(LowSpdHorn)*3.0F + fabs(assymetry/weight)*5.0F )
				{
					
					stallMode = EnteringDeepStall;
					if(alpha < 0.0F)
						pitch = (float)fabs(platform->platformAngles->singam) * platform->platformAngles->cosphi*qbar/-10.0F*DTR;
					else
						pitch = (float)fabs(platform->platformAngles->singam) * platform->platformAngles->cosphi*qbar/-30.0F*DTR;
					//slice = -r*20.0F + (assymetry/weight)*0.005F;		
					if(platform->platformAngles->sinmu > 0.0F)
						stallMagnitude = 10.0f;
					else
						stallMagnitude = 5.0f;
					stallMagnitude += (1.0f - rand()/(float)RAND_MAX * 2.0f) * 5.0f;
					stallMagnitude *= loadingFraction;
					if(platform->platformAngles->cosgam * platform->platformAngles->sinmu < 0.0F)
						slice -= stallMagnitude *0.02F* DTR;
					else
						slice += stallMagnitude *0.02F* DTR;
					slice += (assymetry/weight)*0.05F;
					desiredMagnitude = stallMagnitude;

					mlSinCos(&Trig, vuxGameTime/(1200.0f/loadingFraction));
					oscillationTimer = Trig.sin;
					oscillationSlope = Trig.cos;

					oldp02[5] = 0.0F;
				}
			}
			break;

		case FlatSpin:
			//you're screwed
			break;

		case Spinning:
			if( fabs(slice) > 1.0F)
			{
				stallMode = FlatSpin;
				SetFlag(EngineOff);
			}
			else if( fabs(slice) < 0.25F)
			{
				stallMode = DeepStall;
				if( alpha > 0.0f)
					oldp02[5] = alpha - 60.0F;
				else
					oldp02[5] = alpha + 40.0F;
			}
			mlSinCos(&Trig, vuxGameTime/(1200.0f/loadingFraction));
			oscillationTimer = Trig.sin;
			oscillationSlope = Trig.cos;
			break;

		case EnteringDeepStall:
			if(platform->platformAngles->sinthe < -0.766044F && fabs(alpha * (rand()/(float)RAND_MAX + platform->platformAngles->cosphi)) < 15.0F)
			{
				stallMode = Recovering;
			}
			else if(platform->platformAngles->sinthe < 0.342F && fabs(platform->platformAngles->cosphi) > 0.82F )
			{
				if( alpha > 0.0f && fabs(alpha - (60.0f + oscillationTimer * stallMagnitude * max(0.0F,(0.3F - fabs(r))*3.3F))) < 10.0F &&
					oscillationSlope*q > 0.0F)
				{
					stallMode = DeepStall;
					
					oldp02[5] = alpha - 60.0F;
				}
				else if(alpha < 0.0f && fabs(alpha - (-40.0f + oscillationTimer * stallMagnitude * max(0.0F,(0.3F - fabs(r))*3.3F))) < 10.0F &&
						oscillationSlope*q > 0.0F)
				{
					stallMode = DeepStall;
					oldp02[5] = alpha + 40.0F;
				}
			}
			mlSinCos(&Trig, vuxGameTime/(1200.0f/loadingFraction));
			oscillationTimer = Trig.sin;
			oscillationSlope = Trig.cos;
			break;

		case DeepStall:
			pitch *= 0.9F;
			//you leave the DeepStall in pitch.cpp
			if(stallMode == DeepStall && fabs(slice) > 0.3F && alpha < -10.0F)
			{
				stallMode = Spinning;
				oldp02[5] = alpha;
			}
			mlSinCos(&Trig, vuxGameTime/(1200.0f/loadingFraction));
			oscillationTimer = Trig.sin;
			oscillationSlope = Trig.cos;
			break;

		case Recovering:
			
			if((alpha > g_fRecoveryAOA || alpha < -11.0f) && platform->platformAngles->sinthe > 0.342F)
			{
				stallMode = EnteringDeepStall;
				stallMagnitude += (1.0f - rand()/(float)RAND_MAX * 2.0f) * 5.0f;
				stallMagnitude *= loadingFraction;
				desiredMagnitude = stallMagnitude;
			}
			
			if(qbar > 30.0F && alpha < 18.0f)
				stallMode = None;

			mlSinCos(&Trig, vuxGameTime/(1200.0f/loadingFraction));
			oscillationTimer = Trig.sin;
			oscillationSlope = Trig.cos;
			break;
		}

		//check for low speed warning tone
		if(platform->platformAngles->sinthe > .707 && 1.8888F * theta * RTD + 45.0F > vcas ||
			(g_bRealisticAvionics && gearPos > 0.9F && cockpitFlightData.alpha >= 15.0F))
		{
			
			if(!IsSet(HornSilenced))
			{
				SetFlag(LowSpdHorn);
				//play tone
				F4SoundFXSetDist(auxaeroData->sndLowSpeed, TRUE, 0.0f, 1.0f);
			}

		}
		else if(IsSet(LowSpdHorn))
		{
			ClearFlag(HornSilenced);
			ClearFlag(LowSpdHorn);
		}
	}
}

void  AirframeClass::AddWeapon(float Weight,float DragIndex, float offset)
{
	ShiAssert(Weight >= 0.0F);
	ShiAssert(dragIndex >= 0.0F);

	weight += Weight;
	mass += Weight/GRAVITY;
	dragIndex += DragIndex*(18238.0F/emptyWeight); //equate for relative size differences

	loadingFraction = weight/emptyWeight;
	assymetry += offset*Weight;
}

void  AirframeClass::RemoveWeapon(float Weight, float DragIndex, float offset)
{
//	ShiAssert(weight - Weight >= emptyWeight + fuel + externalFuel);
	if(weight - Weight < emptyWeight + fuel + externalFuel)
		Weight = weight - (emptyWeight + fuel + externalFuel);

	weight -= Weight;
	mass -= Weight/GRAVITY;
	dragIndex -= DragIndex*(18238.0F/emptyWeight);
	dragIndex = max(dragIndex, 0.0F);

	loadingFraction = weight/emptyWeight;
	assymetry -= offset*Weight;
}

float AirframeClass::CalcThrotlPos(float speed)
{
	float tempMach, tempqsom, Mu_fric;
	float th1,th2, newthrotl, cd, drag, accel = 1.0F;
	int EngAltBreak	= 0;
	int EngMachBreak = 0;
	int	AlphaBreak = 0;
	int MachBreak = 0;
	int groundType;

	if ( vt > speed * 1.1f )
		return 0.0f;

	if(speed <= 0.0F)
		return 0.0F;

	if(vt < speed)
	{
		if(vt < 0.2F * speed)
		{
			if(vt < 10.0F)
				return 1.5F;

			tempMach = speed*5.0F/AASL;
		}
		else
			tempMach = (speed * ( speed/vt) )/AASL;	

		accel = 1.2F;
	}
	else
		tempMach = speed/AASL;

	tempMach = min(tempMach,2.0F);

	th1 = Math.TwodInterp (-z, tempMach, engineData->alt,
      engineData->mach, engineData->thrust[0], engineData->numAlt,
      engineData->numMach, &EngAltBreak, &EngMachBreak);
   th2 = Math.TwodInterp (-z, tempMach, engineData->alt,
      engineData->mach, engineData->thrust[1], engineData->numAlt,
      engineData->numMach, &EngAltBreak, &EngMachBreak);


	 cd = Math.TwodInterp(tempMach, alpha, aeroData->mach, aeroData->alpha,
                    aeroData->cdrag, aeroData->numMach,
                    aeroData->numAlpha, &MachBreak, &AlphaBreak) *
           aeroData->cdFactor;


	 tempqsom = 0.5F * rho * speed * speed * area/mass;

	 drag  = (cd + 0.002F + auxaeroData->CDLDGFactor*gearPos)*tempqsom;
	 
	 groundType = OTWDriver.GetGroundType ( x, y );

	 Mu_fric = CalcMuFric(groundType);

	 newthrotl = min( ((drag + Mu_fric*(1.0F - nzcgs)*GRAVITY)/ethrst*mass - th1)/(th2-th1) * accel, 1.5F);

	 return newthrotl;
}

float AirframeClass::CalcDesAlpha(float desGs)
{
	return oldp03[2] + (desGs - nzcgb)*GRAVITY/(qsom*clalpha);
}

float AirframeClass::CalcDesSpeed(float desAlpha)
{
	//MonoPrint("DesVt: %6.2f   Vt: %6.2f    Alpha: %5.2f\n", sqrt( 2.0F*GRAVITY*mass/( area*rho*(cl + (desAlpha - oldp03[2])*clalpha)) ), vt, alpha);
	if((cl + (desAlpha - oldp03[2])*clalpha) < 0.0F)
		return vt;
	else
		return (float)sqrt( 2.0F*GRAVITY*mass/( area*rho*(cl + (desAlpha - oldp03[2])*clalpha)) );
}

void AirframeClass::RunHeliModel(void)
{
	// continue to model gear
	Axial(SimLibMajorFrameTime);

	// continue to model engine for rpm and stuff?
	EngineModel(SimLibMajorFrameTime);

	// rpm is about 0.75 at idle
	throtl = max( 0.0f, (rpm - 0.75f)/0.25f );

	// set stick values
      hf->SetControls( pstick, rstick, throtl, ypedal );

	// exec the helicopter model
	hf->Exec();

	// copy over heli vars to af vars

	// x,y,z world coords
	x = hf->XE.x;
	y = hf->XE.y;
	z = hf->XE.z;

	// NOTE: 
	//	p	=	roll rate (around X axis)
	//	q	=	pitch rate (around Y axis)
	//	r	=	yaw rate (around Z axis)
	//	phi	=	roll (around X axis)
	//	theta=	pitch (around Y axis)
	//	psi	=	yaw (around Z axis)
	phi = hf->XE.ax;
	theta = hf->XE.ay;
	psi = hf->XE.az;
	p = hf->VE.ax;
	q = hf->VE.ay;
	r = hf->VE.az;

	xdot = hf->VE.x;
	ydot = hf->VE.y;
	zdot = hf->VE.z;

	vcas = hf->kias;
	alpha = hf->alpha;
	beta = hf->beta;
	gmma = hf->gmma;
	mu = hf->mu;
	sigma = hf->sigma;
	vt = hf->vta;

	// Check for ground contact
	if ( (IsSet(InAir)) && (z > -TheMap.GetMEA(x, y)) )
	{
		//Tpoint normal;
		//float groundZ;
		
		//groundZ = OTWDriver.GetGroundLevel(x, y, &normal);
		
		if (z >= groundZ - 5.0F)
		{
			z = groundZ - 5.0F;
			ClearFlag(InAir);
		}
	}
}

int AirframeClass::AddFuel (float lbs)
{
	if(fuel + lbs < 0.0F)
	{
		lbs = -fuel;
	}

	//MI fix for filling up too much. We also need to check if we have the tanks on our plane.
	//float maxfuel = aeroDataset[vehicleIndex].inputData[2] + m_tankcap[TANK_LEXT]+m_tankcap[TANK_REXT]+m_tankcap[TANK_CLINE]; //me123 maeroo for external fuel
	ShiAssert(platform);
	ShiAssert(platform->Sms);
	int center = (platform->Sms->NumHardpoints() - 1) / 2 + 1;
	WeaponClassDataType		*wd = NULL;
	float maxfuel = 0.0F;
	maxfuel += aeroDataset[vehicleIndex].inputData[2];
	
    for (int i=0; i<platform->Sms->NumHardpoints(); i++)
    { 
		// if its a tank - try and guess which one.
		// assume only three possible positions... ?
		if (platform->Sms->hardPoint[i] && 
			platform->Sms->hardPoint[i]->GetWeaponClass() == wcTank) {
			wd = &WeaponDataTable[platform->Sms->hardPoint[i]->weaponId];
			ShiAssert(wd);
			if (i < center) 
				maxfuel += m_tankcap[TANK_LEXT];
			else if (i > center) 
				maxfuel += m_tankcap[TANK_REXT];
			else 
				maxfuel += m_tankcap[TANK_CLINE];
		}
    }

	if(fuel + lbs + externalFuel > maxfuel)
	{
		lbs = maxfuel - (fuel + externalFuel);
		weight += lbs;
		mass += lbs/GRAVITY;
		fuel = maxfuel;
		AllocateFuel(fuel); // JPO - allocate between tanks
		return FALSE;
	}

	fuel += lbs;
	weight += lbs;
	mass += lbs/GRAVITY;
	AllocateFuel(fuel + externalFuel); // JPO allocate between tanks
	return TRUE;
}

void  AirframeClass::AddExternalFuel (float lbs)
{
	if(externalFuel + lbs < 0.0F)
	{
		lbs = -externalFuel;
	}

	externalFuel += lbs;
	weight += lbs;
	mass += lbs/GRAVITY;
}

void AirframeClass::ResetAlpha(void)
{
	oldp02[0] = alpha;
	oldp02[1] = alpha;
	oldp02[2] = alpha;
	oldp02[3] = alpha;
	oldp02[4] = alpha;
	oldp02[5] = alpha;

	oldp03[0] = alpha;
	oldp03[1] = alpha;
	oldp03[2] = alpha;
	oldp03[3] = alpha;
	oldp03[4] = alpha;
	oldp03[5] = alpha;

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////



void AirframeClass::ResetFuel (void)
{
        float lbs = aeroDataset[vehicleIndex].inputData[AeroDataSet::InternalFuel] - fuel; // JPO - use the constant...
	weight += lbs;
	mass += lbs/GRAVITY;

	float fullfuel = 0;
	for (int i = 0; i < MAX_FUEL; i++)
		fullfuel += m_tankcap[i];
	//fuel = aeroDataset[vehicleIndex].inputData[2];
	// JPO - (re)allocate the fuel 
	AllocateFuel(fullfuel);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

GearData::GearData(void)
{
	strength	= 100.0F;	//how many hitpoints it has left
	vel			= 0.0F;		//at what rate is it currently compressing/extending in ft/s
	obstacle	= 0.0F;		//rock height/rut depth
	flags		= 0;		//gear stuck/broken, door stuck/broken,
}
// JPO - attempt to locate external tanks and capacity.
void AirframeClass::FindExternalTanks (void)
{
    int center = (platform->Sms->NumHardpoints() - 1) / 2 + 1;
    WeaponClassDataType		*wd;

    for (int i=0; i<platform->Sms->NumHardpoints(); i++)
    {
      // if its a tank - try and guess which one.
      // assume only three possible positions... ?
	if (platform->Sms->hardPoint[i] && 
	    platform->Sms->hardPoint[i]->GetWeaponClass() == wcTank) {
	    wd = &WeaponDataTable[platform->Sms->hardPoint[i]->weaponId];
	    ShiAssert (wd);
	    if (i < center) 
		m_tankcap[TANK_LEXT] = wd->Strength;
	    else if (i > center) 
		m_tankcap[TANK_REXT] = wd->Strength;
	    else 
		m_tankcap[TANK_CLINE] = wd->Strength;
	}
    }
}
void AirframeClass::ToggleLL(void)
{
	if(LLON)
		LLON = FALSE;
	else
		LLON = TRUE;
}
void AirframeClass::TogglePB(void)
{
	//not if we're rolling
	if(vt > 1.0F * KNOTS_TO_FTPSEC)
	{
		PBON = FALSE;
		ClearFlag(WheelBrakes);
		return;
	}
    if(PBON) {
	PBON = FALSE;
	ClearFlag(WheelBrakes);
    }
    else {
	PBON = TRUE;
	SetFlag(WheelBrakes);		
    }
}

// JB carrier start
void AirframeClass::ToggleHook(void)
{
    int sw = platform->IsComplex() ? COMP_HOOK : SIMP_HOOK;
  if(IsSet(Hook))
	{
    hookHandle = -1.0F;
		ClearFlag(Hook);

		if ( gACMIRec.IsRecording() )
		{
			acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
			acmiSwitch.data.type = platform->Type();
			acmiSwitch.data.uniqueID = platform->Id();
			acmiSwitch.data.switchNum = sw;
			acmiSwitch.data.prevSwitchVal = platform->GetSwitch (sw);
			acmiSwitch.data.switchVal = 0;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}

		platform->SetSwitch(sw, 0);
	}
  else
	{
    hookHandle = 1.0F;
		SetFlag(Hook);		

		if ( gACMIRec.IsRecording() )
		{
			acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
			acmiSwitch.data.type = platform->Type();
			acmiSwitch.data.uniqueID = platform->Id();
			acmiSwitch.data.switchNum = sw;
			acmiSwitch.data.prevSwitchVal = platform->GetSwitch (sw);
			acmiSwitch.data.switchVal = 1;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}

		platform->SetSwitch(sw, 1);
	}
}
// JB carrier end

// JPO TEF/LEF manual stuff

void AirframeClass::TEFClose()
{
    tefPos = 0;
}

void AirframeClass::TEFMax()
{
    tefPos = auxaeroData->tefMaxAngle;
}

void AirframeClass::TEFInc()
{
    tefPos += auxaeroData->tefMaxAngle /auxaeroData->tefNStages;
    if (tefPos > auxaeroData->tefMaxAngle)
	tefPos = auxaeroData->tefMaxAngle;
}
void AirframeClass::TEFDec()
{
    tefPos -= auxaeroData->tefMaxAngle /auxaeroData->tefNStages;
    if (tefPos < 0)
	tefPos = 0;
}

void AirframeClass::TEFTakeoff()
{
    tefPos = auxaeroData->tefTakeOff;
}


void AirframeClass::LEFClose()
{
    lefPos = 0;
}
void AirframeClass::LEFMax() 
{
    lefPos = auxaeroData->lefMaxAngle;
}
void AirframeClass::LEFInc()
{
    lefPos += auxaeroData->lefMaxAngle /auxaeroData->lefNStages;
    if (lefPos > auxaeroData->lefMaxAngle)
	lefPos = auxaeroData->lefMaxAngle;
}
void AirframeClass::LEFDec()
{
    lefPos -= auxaeroData->lefMaxAngle /auxaeroData->lefNStages;
    if (lefPos < 0)
	lefPos = 0;
}
void AirframeClass::LEFTakeoff()
{
    lefPos = auxaeroData->lefGround;
}

void AirframeClass::TEFLEFStage1()
{
    if (auxaeroData->tefNStages > 2)
        tefPos = auxaeroData->tefMaxAngle /auxaeroData->tefNStages;
    if (auxaeroData->lefNStages > 2)
        lefPos = auxaeroData->lefMaxAngle /auxaeroData->lefNStages;
}

void AirframeClass::TEFLEFStage2()
{
    int pos = auxaeroData->tefNStages/2.0f + 0.5;
    if (auxaeroData->tefNStages > 1)
        tefPos = pos * auxaeroData->tefMaxAngle / auxaeroData->tefNStages;
    pos = auxaeroData->lefNStages/2;
    if (auxaeroData->lefNStages > 1)
        lefPos = pos * auxaeroData->lefMaxAngle /auxaeroData->lefNStages;
}

void AirframeClass::TEFLEFStage3()
{
    TEFMax();
    LEFMax();
}

void AirframeClass::SetFlaps(bool islanding)
{
#if 0
    float clmax = Math.TwodInterp(mach, aoamax, aeroData->mach, aeroData->alpha,
                    aeroData->clift, aeroData->numMach,
                    aeroData->numAlpha, &curMachBreak, &curAlphaBreak) *
            aeroData->clFactor;
    float vstall = (float)sqrt(2.0 * weight / (rho * area * clmax));
#endif
    float vflapmin = auxaeroData->maxFlapVcas - auxaeroData->flapVcasRange;
    float vflapmax = auxaeroData->maxFlapVcas;

    if (auxaeroData->hasTef != AUX_LEFTEF_MANUAL)
	return;
    if (islanding) { // we want to get some flaps down
	if (vcas > vflapmax) { // no flaps above vflapmax
	    TEFClose();
	    LEFClose();
	}
	else if (vcas < vflapmin) { // everything below vstall
	    TEFMax();
	    LEFMax();
	}
	else { // interpolate
	    float vdiff = (vcas - vflapmin) / auxaeroData->flapVcasRange;
	    int stage = auxaeroData->tefNStages - vdiff * auxaeroData->tefNStages - 1;
	    stage = max(0, min(stage, auxaeroData->tefNStages));
	    tefPos = stage * auxaeroData->tefMaxAngle / auxaeroData->tefNStages;
	    if (auxaeroData->hasLef == AUX_LEFTEF_MANUAL) {
		stage = auxaeroData->lefNStages - (vdiff * auxaeroData->lefNStages) - 1;
		stage = max(0,min(stage, auxaeroData->lefNStages));
		lefPos = stage * auxaeroData->lefMaxAngle / auxaeroData->lefNStages;
	    }
	}
    }
    else {
	if (vcas > vflapmax - (vflapmax - vflapmin)/2 ) {
	    TEFClose();
	    LEFClose();
	}
	else {
	    TEFTakeoff();
	    LEFTakeoff();
	}
    }
}

void AirframeClass::CanopyToggle()
{
    if (canopyState == true)
	canopyState = false;
    else if (!IsSet(InAir))
	canopyState = true;
}

int AirframeClass::GetParkType() 
{
	return auxaeroData->largePark ? LargeParkPt : SmallParkPt; 
}