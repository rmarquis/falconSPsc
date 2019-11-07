#include "stdafx.h"
#include <windows.h>
#include "cpcb.h"
//Note: include all callback files below here
#include "cbackproto.h"	

//Exec, Event, Display
// JPO let the array grow by itself
CPCallbackStruct	CPCallbackArray[] = {
			{CBEECMPwrLight,			NULL,					NULL},							//0
			{CBEECMFailLight,			NULL,					NULL},
			{CBEAuxWarnSearchL,		NULL,					NULL},
			{CBEAuxWarnActL,			NULL,					NULL},
			{CBEAuxWarnAltL,			NULL,					NULL},
			{CBEAuxWarnPwrL,			NULL,					NULL},							//5
			{CBEAOAIndLight,			NULL,					NULL},
			{CBERefuelLight,			NULL,					NULL},
			{CBEDiscLight,				NULL,					NULL},
			{CBEAOA,						NULL,					CBDAOA},
			{CBEVVI,						NULL,					CBDVVI},							//10
			{CBEFuelFlow,				NULL,					NULL},
			{CBEOilPressure,			NULL,					NULL},
			{CBENozPos,					NULL,					NULL},
			{CBERPM,						NULL,					NULL},	
			{CBEAOAFastLight,			NULL,					NULL},							//15
			{CBEUHFDigit,				NULL,					NULL},	
			{CheckLandingGearHandle, NULL,				NULL},
			{CBECheckMasterCaution,	NULL,					NULL},
			{CBExSpeedAlt,				NULL,					NULL},
			{CBELaunchL,				NULL,					NULL},							//20
			{CBEHandoffL,				NULL,					NULL},
			{CBEPriModeL,				NULL,					NULL},
			{CBEUnknownL,				NULL,					NULL},
			{CBENavalL,					NULL,					NULL},
			{CBETgtSepL,				NULL,					NULL},							//25
			{CBEJfsRun,				NULL,					NULL},
			{CBEEpuRun,				NULL,					NULL},
			{CBEConfigLight,			NULL,					NULL},							//28
			{CBEInteriorLight,			NULL,					NULL},
			{CBEFlcsPMG,					NULL,					NULL},							//30
			{CBEEpuGen,						NULL,					NULL},
			{CBEAltInd,					NULL,					NULL},
			{CBEAltDial,				NULL,					NULL},
			{CBEMachAsi,				NULL,					NULL},
			{CBEMach,	    		NULL,					NULL},							//35
			{CBEEpuPmg,				NULL,					NULL},
			{CBEToFlcs,				NULL,					NULL},
			{CBEFlcsRly,				NULL,					NULL},
			{CBEBatteryFail,			NULL,					NULL},
			{CBEVRPM,					NULL,					NULL},							//40
			{CBEVNozPos,				NULL,					NULL},
			{CBEVInletTemperature,	NULL,					NULL},	
			{CBEVOilPressure,			NULL,					NULL},	
			{CBEVAltDial,				NULL,					NULL},	
			{CBEEpuAir,				NULL,					NULL},							//45
			{CBEEpuHydrazine,			NULL,					NULL},
			{CBECautionElectric,			NULL,					NULL},	
			{CBEInstrumentLight,			NULL,					NULL},
			{CBESpotLight,			NULL,					NULL},									
			{CBETFRLight,				NULL,					NULL},							//50
			{CBEGearHandleLight,		NULL,					NULL},	
			{CBECkptWingLight,			NULL,					NULL},					//MI 52
			{CBECkptStrobeLight,						NULL,					NULL},					//MI 53
			{NULL,						NULL,					NULL},	
			{CBEChaffCount,			NULL,					NULL},							//55
			{CBEFlareCount,			NULL,					NULL},
			{NULL,						NULL,					NULL},	
			{CBEThreatWarn7,			NULL,					NULL},	
			{CBEThreatWarn8,			NULL,					NULL},
			{CBEThreatWarn9,			NULL,					NULL},							//60	
			{CBEThreatWarn10,   		NULL,					NULL},	
			{CBECaution1,				NULL,					NULL},	
			{CBECaution2,				NULL,					NULL},	
			{CBECaution3,				NULL,					NULL},	
			{CBECaution4,				NULL,					NULL},							//65
			{CBECaution5,				NULL,					NULL},	
			{CBECaution6,				NULL,					NULL},	
			{CBECaution7,				NULL,					NULL},	
			{CBECaution8,				NULL,					NULL},	
			{CBECaution9,				NULL,					NULL},							//70
			{CBECaution10,				NULL,					NULL},	
			{CBECaution11,				NULL,					NULL},	
			{CBECaution12,				NULL,					NULL},	
			{NULL,						NULL,					NULL},	
			{NULL,						NULL,					NULL},							//75
			{NULL,						NULL,					NULL},	
			{CBEHSIRange,				NULL,					NULL},	
			{CBEHSISelectedCourse,	NULL,					NULL},	
			{NULL,						NULL,					NULL},
			{CBELEFPos,				NULL,					NULL},							//80
			{CBEFlapPos,				NULL,					NULL},	
			{CBESpeedBreaks,			NULL,					NULL},	
			{CBECaution13,				NULL,					NULL},	
			{CBECaution14,				NULL,					NULL},								
			{CBECaution15,				NULL,					NULL},							//85
			{CBECaution16,				NULL,					NULL},	
			{CBECaution17,				NULL,					NULL},	
			{CBECautionFwdFuel,			NULL,					NULL},	
			{CBECautionAftFuel,			NULL,					NULL},								
			{CBECautionSec,				NULL,					NULL},							//90
			{CBECautionOxyLow,			NULL,					NULL},	
			{CBECautionProbeHeat,			NULL,					NULL},	
			{CBECautionSeatNotArmed,		NULL,					NULL},	
			{CBECautionBUC,				NULL,					NULL},						
			{CBECautionFuelOilHot,			NULL,					NULL},							//95
			{CBECautionAntiSkid,						NULL,					NULL},											
			{NULL,						NULL,					NULL},											
			{CBEInletTemperature,	NULL,					NULL},											
			{CBECautionMainGen,				NULL,					NULL},											
			{CBECautionStbyGen,				NULL,					NULL},							//100
			{CBETFFail,						NULL,					NULL},											
			{CBEEwsPanelPower,			NULL,					NULL},											
			{CBECanopyLight,			NULL,					NULL},											
			{CBEADIOff,					NULL,					NULL},	//MI 104 ADI Off Flag
			{CBEADIAux,					NULL,					NULL},	//MI 105 ADI Aux Flag
			{CBEHSIOff,					NULL,					NULL},	//MI 106 HSI Off Flag
			{CBELEFLight,				NULL,					NULL},	//MI 107 LEF Lock caution light
			{CBECanopyDamage,			NULL,					NULL},	//MI 108 Canopy damage
			{CBEBUPADIFlag,				NULL,					NULL},	//MI 109 BUP ADI Off Flag
			{CBEAVTRRunLight,			NULL,					NULL},	//MI 110 AVTR Run Light
			{CBEGSFlag,					NULL,					NULL},	//MI 111 ADI GS Flag
			{CBELOCFlag,				NULL,					NULL},	//MI 112 ADI LOC Flag
			{CBEVVIOFF,					NULL,					NULL},	//MI 113 VVI Off Flag
			{CBECockpitFeatures,		NULL,					NULL},	//MI 114 permanent cockpit parts										
			{NULL,						NULL,					NULL},							//115
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL},											
			{CBEInternalFuel,			NULL,					NULL},											
			{CBEExternalFuel,			NULL,					NULL},							//120
			{CBETotalFuel,				NULL,					NULL},											
			{CBEHydPressA,				NULL,					NULL},											
			{CBEEPUFuel,				NULL,					NULL},											
			{CBEClockHours,			NULL,					NULL},											
			{CBEClockMinutes,			NULL,					NULL},							// 125								
			{CBEClockSeconds,			NULL,					NULL},											
			{CBEMagneticCompass,		NULL,					NULL},											
			{CBEHydPressB,				NULL,					NULL},											
			{CBEFrontLandGearLight,	NULL,					NULL},											
			{CBELeftLandGearLight,	NULL,					NULL},								//130										
			{CBERightLandGearLight,	NULL,					NULL},											
			{CBETrimNose,			NULL,					NULL},	//MI 132 Nose trim needle
			{CBETrimWing,			NULL,					NULL},	//MI 133 Wing trim needle
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL},								//135			
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL},								//140			
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL},								//145			
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL},											
			{NULL,						NULL,					NULL}
};
// JPO - count them up
const int TOTAL_CPCALLBACK_SLOTS = sizeof(CPCallbackArray) / sizeof(CPCallbackArray[0]);

			//mTransAeroToState		mTransStateToAero
ButtonCallbackStruct ButtonCallbackArray[] = {
			{CBExChaffDispense,		CBEChaffDispense},				//	0
			{CBExCourseSelect,		CBECourseSelect},
			{CBExHeadingSelect,		CBEHeadingSelect},
			{CBExFlareDispense,		CBEFlareDispense},
			{CBExStoresJettison,		CBEStoresJettison},
			{CBExAVTRControl,			CBEAVTRControl},					// 5
			{CBExModeSelect,			CBEModeSelect},
			{CBExMPO,					CBEMPO},
			{CBExHornSilencer,		CBEHornSilencer},
			{CBExMFDButton,			CBEOSB_1L},
			{CBExMFDButton,			CBEOSB_2L},							// 10
			{CBExMFDButton,			CBEOSB_3L},
			{CBExMFDButton,			CBEOSB_4L},
			{CBExMFDButton,			CBEOSB_5L},
			{CBExMFDButton,			CBEOSB_6L},
			{CBExMFDButton,			CBEOSB_7L},							//	15
			{CBExMFDButton,			CBEOSB_8L},
			{CBExMFDButton,			CBEOSB_9L},
			{CBExMFDButton,			CBEOSB_10L},
			{CBExMFDButton,			CBEOSB_11L},
			{CBExMFDButton,			CBEOSB_12L},						//	20
			{CBExMFDButton,			CBEOSB_13L},
			{CBExMFDButton,			CBEOSB_14L},
			{CBExMFDButton,			CBEOSB_15L},
			{CBExMFDButton,			CBEOSB_16L},
			{CBExMFDButton,			CBEOSB_17L},						// 25
			{CBExMFDButton,			CBEOSB_18L},
			{CBExMFDButton,			CBEOSB_19L},
			{CBExMFDButton,			CBEOSB_20L},
			{CBExMFDButton,			CBEOSB_1R},
			{CBExMFDButton,			CBEOSB_2R},							// 30
			{CBExMFDButton,			CBEOSB_3R},
			{CBExMFDButton,			CBEOSB_4R},
			{CBExMFDButton,			CBEOSB_5R},
			{CBExMFDButton,			CBEOSB_6R},
			{CBExMFDButton,			CBEOSB_7R},							// 35
			{CBExMFDButton,			CBEOSB_8R},
			{CBExMFDButton,			CBEOSB_9R},
			{CBExMFDButton,			CBEOSB_10R},
			{CBExMFDButton,			CBEOSB_11R},
			{CBExMFDButton,			CBEOSB_12R},						// 40
			{CBExMFDButton,			CBEOSB_13R},
			{CBExMFDButton,			CBEOSB_14R},
			{CBExMFDButton,			CBEOSB_15R},
			{CBExMFDButton,			CBEOSB_16R},
			{CBExMFDButton,			CBEOSB_17R},						// 45
			{CBExMFDButton,			CBEOSB_18R},
			{CBExMFDButton,			CBEOSB_19R},
			{CBExMFDButton,			CBEOSB_20R},
			{CBExICPSecondaryExclusive,	CBEICPTILS},
			{CBExICPSecondaryExclusive,	CBEICPALOW},						// 50
			{CBExICPSecondaryExclusive,	CBEICPFAck},
			{CBExICPPrevious,			CBEICPPrevious},
			{CBExICPNext,				CBEICPNext},
			{CBExICPSecondaryExclusive,	CBEICPLink},
			{CBExICPSecondaryExclusive,	CBEICPCrus},						// 55
			{CBExICPSecondaryExclusive,	CBEICPStpt},
			{CBExICPSecondaryExclusive,	CBEICPMark},
			{CBExICPEnter,				CBEICPEnter},
			{CBExICPTertiaryExclusive,	CBEICPCom1},
			{CBExICPPrimaryExclusive,	CBEICPNav},							// 60
			{CBExICPPrimaryExclusive,	CBEICPAA},
			{CBExICPPrimaryExclusive,	CBEICPAG},
			{CBExHUDScales,			CBEHUDScales},
			{CBExHUDFPM,				CBEHUDFPM},				
			{CBExHUDDED,				CBEHUDDED},							// 65
			{CBExHUDVelocity,			CBEHUDVelocity},				
			{CBExHUDRadar,				CBEHUDRadar},
			{CBExHUDBrightness,		CBEHUDBrightness},				
			{CBExECMSwitch,			CBEECMSwitch},
			{CBExUHFSwitch,			CBEUHFSwitch},						// 70
			{CBExEject,					CBEEject},
			{NULL,						CBELandGearSelect},				
			{CBExAutoPilot,			CBEAutoPilot},				
			{NULL,						CBEMasterCaution},	
			{CBExAuxCommLeft,			CBEAuxCommLeft},					// 75	
			{CBExAuxCommCenter,		CBEAuxCommCenter},	
			{CBExAuxCommRight,		CBEAuxCommRight},	
			{CBExAuxCommBand,			CBEAuxCommBand},	
			{CBExAuxCommMaster,		CBEAuxCommMaster},	
			{CBExAuxCommAATR,			CBEAuxCommAATR},					// 80
			{CBExUHFMaster,			CBEUHFMaster},	
			{CBExExteriorLite,		CBEExteriorLite},	
			{NULL,						CBEHandoffB},	
			{NULL,						CBEPriModeB},	
			{NULL,						CBEUnknownB},					// 85
			{NULL,						CBENavalB},
			{NULL,						CBETgtSepB},
			{CBExCatIII,				CBECatIII},// OW CAT III cockpit switch extension
			{CBExAMChaffFlare,		CBEAMChaffFlare},
			{CBExICPTertiaryExclusive,		CBEICPCom2},				// 90
			{NULL,						CBEKneeboardMap},								
			{NULL,						CBEKneeboardBrief},								
			{NULL,						CBEAuxWarnSearchB},								
			/// SG's callback goes next
// 2000-11-10 MODIFIED BY S.G. FOR THE Drift C/O switch
//			{NULL,						NULL},								
			{CBExHUDDriftCO,			CBEHUDDriftCO}, // 94
// END OF MODIFIED SECTION
			{NULL,						CBEAuxWarnAltB},					//95		
			{NULL,						CBEAuxWarnPwrB},								
			{CBExICPTertiaryExclusive,	CBEICPLIST},	//MI ICP Stuff 97							
			{CBExICPTertiaryExclusive,	CBEICPIFF},		//MI ICP Stuff 98								
			{CBExICPSecondaryExclusive,	CBETHREEButton},//MI ICP Stuff 99								
			{CBExICPSecondaryExclusive,	CBESIXButton},	//MI ICP Stuff 100
			{CBExICPSecondaryExclusive,	CBEEIGHTButton},//MI ICP Stuff 101								
			{CBExAltLGear,					CBEAltLGear},	//JPO	102
			{CBExJfs,					CBEJfs}, //JPO,	103							
			{CBExEpu,					CBEEpu}, //JPO 104
			{CBExMasterArm,         CBEMasterArm},					   // 105
			{CBExMFDButton,         CBERightGainUp},
			{CBExMFDButton,         CBERightGainDown},
			{CBExMFDButton,         CBELeftGainUp},
			{CBExMFDButton,         CBELeftGainDown},
			{CBExHUDColor,          CBEHUDColor},                 // 110
			{CBExICPSecondaryExclusive,	CBEZEROButton},	//MI ICP Stuff 111
			{CBExICPTertiaryExclusive,	CBEResetDEDPage},	//MI ICP Stuff 112
			{CBExICPTertiaryExclusive,	CBEICPDEDUP},	//MI ICP Stuff 113
			{CBExICPTertiaryExclusive,	CBEICPDEDDOWN},	//MI ICP Stuff 114
			{CBExICPTertiaryExclusive,	CBEICPCLEAR},	//MI ICP Stuff 115
			{CBExRALTSwitch,	CBERALTSwitch},	//RALT - fwd/back RALT switch116
			{CBExRALTSwitch,	CBERALTSTDBY},	//MI RALT Stuff 117
			{CBExRALTSwitch,	CBERALTON	},	//MI RALT Stuff 118
			{CBExRALTSwitch,	CBERALTOFF	},	//MI RALT Stuff 119
			{CBExICPTertiaryExclusive,	CBEICPDEDSEQ},	//MI ICP Stuff 120
			{CBExSmsPower,		CBESmsPower}, // 121 JPO Avionics SMS power
			{CBExFCCPower,		CBEFCCPower}, // 122 JPO Avionics FCC power
			{CBExMFDPower,		CBEMFDPower}, // 123 JPO Avionics MFD power
			{CBExUFCPower,		CBEUFCPower}, // 124 JPO Avionics UFC power
			{CBExGPSPower,		CBEGPSPower}, // 125 JPO Avionics GPS power
			{CBExDLPower,		CBEDLPower}, // 126 JPO Avionics DL power
			{CBExMAPPower,		CBEMAPPower}, // 127 JPO Avionics MAP power
			{CBExTISLPower,		CBETISLPower}, // 128 JPO Avionics TISL power
			{CBExLEFTHPTPower,		CBELEFTHPTPower}, // 129 JPO Avionics Left Hpt power
			{CBExRIGHTHPTPower,		CBERIGHTHPTPower}, // 130 JPO Avionics Right Hpt power
			{CBExHUDPower,		CBEHUDPower}, // 131 JPO Avionics HUD power
			{CBExFCRPower,		CBEFCRPower}, // 132 JPO Avionics FCR power
			{CBExICPSecondaryExclusive,	CBENINEButton},	//MI ICP Stuff 133 - moved from drift c/o
			{CBExFuelDisplay,	CBEFuelSwitch}, // 134 Fuel Switch Step
			{CBExFuelPump,		CBEFuelPump},	// 135 Fuel Pump step
			{CBExFuelCock,		CBEFuelCock},	// 136 Maste fuel cock valve
			{CBExFuelExtTrans,	CBEFuelExtTrans},// 137 Fuel external transfer switch
			{CBExAirSource,		CBEAirSource},	// 138 Air Source Switch
			{CBExAltLGear,		CBEAltLGearReset},//JPO	139 Alternate Gear Reset
			{CBExLandingLightToggle,	CBELandingLightToggle}, //MI 140 LandingLight switch
			{CBExParkingBrakeToggle,	CBEParkingBrakeToggle}, //MI 141 Parkingbrake switch
			{CBExLaserArmToggle,	CBELaserArmToggle}, //MI 142 Laser arm switch
			{CBExFuelDoorToggle,	CBEFuelDoorToggle}, //MI 143 FuelDoor
			{NULL,			CBEKneeboardStpt}, // JPO 144 Kneeboard STPT switch
			{NULL,					NULL},	//145
			{CBExLeftAPSwitch,		CBELeftAPSwitch},	//MI 146 Heading select AP
			{NULL,					NULL},				//147
			{CBExRightAPSwitch,		CBERightAPSwitch},		//MI 148 Alt hold AP
			{NULL,					CBEAPOverride},		//MI 149 AP override
			{CBExWarnReset,			CBEWarnReset},		//MI 150 Warn reset switch
			{CBExReticleSwitch,		CBEReticleSwitch},	//MI 151 MAN Reticle
			{CBExInteriorLightSwitch,	CBEInteriorLightSwitch}, // JPO 152 Interior Lights
			{CBExSeatArmSwitch,		CBESeatArmSwitch},	//MI 153 Seat arm switch
			{CBExEWSRWRPowerSwitch,	CBEEWSRWRPower},	//MI 154 EWS RWR power
			{CBExEWSJammerPowerSwitch,	CBEEWSJammerPower},	//MI 155 EWS Jammer Power
			{CBExEWSChaffPowerSwitch,	CBEEWSChaffPower},	//MI 156 EWS Chaff Power
			{CBExEWSFlarePowerSwitch,	CBEEWSFlarePower},	//MI 157 EWS Flare Power
			{CBExMainPower,			CBEMainPower}, // JPO 158 Main Power switch
			{CBExEWSPGMButton,		CBEEWSPGMButton},		//MI 159 EWS Program select
			{CBExHookToggle,	CBEHookToggle}, // JB carrier 160 hook switch
			{CBExNwsToggle,		CBENwsToggle}, // JPO - NWS option 161
			{CBExIdleDetent,	CBEIdleDetent}, // JPO Idle Detent switch	162
			{CBExInhibitVMS,	CBEInhibitVMS},	//MI 163 Inhibit VMS Switch
			{CBExRFSwitch,		CBERFSwitch},	//MI 164 RF Switch
			{CBExInstrumentLightSwitch,	CBEInstrumentLightSwitch}, // JPO 165 Interior Lights
			{CBExSpotLightSwitch,	CBESpotLightSwitch}, // MI 166 Interior Lights
			{CBExEWSProgButton,		CBEEWSProgButton}, //MI 167 EWS Program select
			{CBExGearHandle,		CBEGearHandle},	//MI 168 Gear handle
			{NULL,				CBEPinkySwitch},	//MI 169 Pinky button on Stick
			{CBExGndJettEnable, CBEGndJettEnable},	//MI 170 Ground Jett enable
			{CBExExtlPower,		CBEExtlPower},	//MI 171 Exterior lighting power
			{CBExExtlAntiColl,	CBEExtlAntiColl},	//MI 172 Exterior lighting anti coll switch
			{CBExExtlSteady,	CBEExtlSteady},	//MI 173 Exterior lighting steady/flash
			{CBExExtlWing,		CBEExtlWing},	//MI 174 Wing light (from inside the cockpit)
			{CBExAVTRSwitch,	CBEAVTRSwitch},	//MI 175 AVTR Switch with 3 positions
			{CBExIFFPower,		CBEIFFPower},	//MI 176 IFF Power
			{CBExINSSwitch,		CBEINSSwitch},	//MI 177 INS
			{CBExLEFLockSwitch, CBELEFLockSwitch},	//MI 178 LEF Lock Switch
			{CBExDigitalBUP,	CBEDigitalBUP},	//MI 179 Digital Backup Switch
			{CBExAltFlaps,		CBEAltFlaps},	//MI 180 Alt Flaps switch
			{CBExManualFlyup,	CBEManualFlyup},	//MI 181 Manual Flyup switch
			{CBExFLCSReset,		CBEFLCSReset},	//MI 182 FLCS Reset Switch
			{CBExFLTBIT,		CBEFLTBIT},		//MI 183 BIT switch on FLCS panel
			{CBExOBOGSBit,		CBEOBOGSBit},	//MI 184 OBOGS Bit Switch Testpanel
			{CBExMalIndLights,	CBEMalIndLights},	//MI 185 Mal + Ind lights Switch Testpanel
			{CBExProbeHeat,		CBEProbeHeat},	//MI 186 Probeheat Switch Testpanel
			{CBExEPUGEN,		CBEEPUGEN},	//MI 187 EPU GEN Switch Testpanel
			{CBExTestSwitch,	CBETestSwitch},	//MI 188 Test Switch Testpanel
			{CBExOverHeat,		CBEOverHeat},	//MI 189 Overheat Switch Testpanel
			{CBExTrimAPDisc,	CBETrimAPDisc},	//MI 190 Trim/AP Disc Switch
			{CBExMaxPower,		CBEMaxPower},	//MI 191 Max Power Switch
			{CBExABReset,		CBEABReset},	//MI 192 AB Reset Switch
			{NULL,				CBETrimNose},	//MI 193 Trim wheel nose
			{CBExTrimYaw,		CBETrimYaw},	//MI 194 Trim wheel Yaw Left
			{NULL,				CBETrimRoll},	//MI 195 Trim wheel Roll Left
			{CBExMissileVol,	CBEMissileVol},	//MI 196 Missile volume knob
			{CBExThreatVol,		CBEThreatVol},	//MI 197 Threat volume knob
			{CBExDeprRet,		CBEDeprRet},	//MI 198 DEPR RET Wheel on the ICP
			{NULL,				CBETFRButton},	//MI 199 TFR button
			{NULL,		CBEFlap}, // JPO 200 Flap adjust
			{NULL,		CBELef}, // JPO 201 LEF adjust
			{CBExDragChute, CBEDragChute}, // JPO 202 Drag chute
			{CBExCanopy,    CBECanopy}, // JPO 203 Canopy state
			{CBExComm1Vol,	CBEComm1Vol},		//MI 204 Comm1 volume
			{CBExComm2Vol,	CBEComm2Vol},		//MI 205 Comm1 volume
			{CBExSymWheel,	CBESymWheel},		//MI 206 Comm1 volume
			{CBExSetNightPanel,	CBESetNightPanel},	//MI 207 Nightlighting for Jags
			{NULL,	NULL}	//
};
// JPO count them up
const int TOTAL_BUTTONCALLBACK_SLOTS = sizeof(ButtonCallbackArray) / sizeof(ButtonCallbackArray[0]);
