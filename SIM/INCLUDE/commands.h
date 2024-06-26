#ifndef _COMMANDS_H
#define _COMMANDS_H

#define SHIFT_KEY  0x1
#define CTRL_KEY   0x2
#define ALT_KEY    0x4
#define MODS_MASK  (CTRL_KEY | ALT_KEY | SHIFT_KEY)
#define KEY_DOWN   0x8
#define SECOND_KEY_SHIFT        8
#define SECOND_KEY_MOD_SHIFT    16


void SimSetBubbleSize (unsigned long val, int state, void*); // JB 000509
void SimHookToggle(unsigned long val, int state, void *); // JB carrier

// JPO
void SimThrottleIdleDetent(unsigned long, int state, void*); // JPO
void SimJfsStart(unsigned long, int state, void*); // JPO
void SimEpuToggle (unsigned long, int state, void*); // JPO
void AFRudderTrimLeft(unsigned long, int state, void*); // JPO
void AFRudderTrimRight(unsigned long, int state, void*); // JPO
void AFAileronTrimLeft(unsigned long, int state, void*); // JPO
void AFAileronTrimRight(unsigned long, int state, void*); // JPO
void AFElevatorTrimUp(unsigned long, int state, void*); // JPO
void AFElevatorTrimDown(unsigned long, int state, void*); // JPO
void AFResetTrim(unsigned long, int state, void*); // JPO
void AFAlternateGear(unsigned long, int state, void*); // JPO
void AFAlternateGearReset(unsigned long, int state, void*); // JPO
void SimFLIRToggle(unsigned long, int state, void*); // JPO
void SimToggleRealisticAvionics(unsigned long, int state, void*); // JPO
void SimIncFuelSwitch(unsigned long, int state, void*); // JPO
void SimDecFuelSwitch(unsigned long, int state, void*); // JPO
void SimIncFuelPump(unsigned long, int state, void*); // JPO
void SimDecFuelPump(unsigned long, int state, void*); // JPO
void SimToggleMasterFuel(unsigned long, int state, void*); // JPO
void SimIncAirSource(unsigned long, int state, void*); // JPO
void SimDecAirSource(unsigned long, int state, void*); // JPO
void SimDecLeftAuxComDigit(unsigned long, int state, void*); // JPO
void SimDecCenterAuxComDigit(unsigned long, int state, void*); //JPO
void SimDecRightAuxComDigit(unsigned long, int state, void*); //JPO
void SimInteriorLight(unsigned long, int state, void*); //JPO
void SimInstrumentLight(unsigned long, int state, void*); //JPO
void SimSpotLight(unsigned long, int state, void*); //JPO
void SimToggleTFR(unsigned long, int state, void*); //JPO
void SimMainPowerDec(unsigned long, int state, void*); //JPO
void SimMainPowerInc(unsigned long, int state, void*); //JPO
void AFFullFlap(unsigned long, int state, void*); // JPO
void AFNoFlap(unsigned long, int state, void*); // JPO
void AFIncFlap(unsigned long, int state, void*); // JPO
void AFDecFlap(unsigned long, int state, void*); // JPO
void AFFullLEF(unsigned long, int state, void*); // JPO
void AFNoLEF(unsigned long, int state, void*); // JPO
void AFIncLEF(unsigned long, int state, void*); // JPO
void AFDecLEF(unsigned long, int state, void*); // JPO
void AFDragChute(unsigned long, int state, void*); // JPO
void AFCanopyToggle(unsigned long, int state, void*); // JPO

//MI
void SimICPIFF (unsigned long val, int state, void*);
void SimICPLIST (unsigned long val, int state, void*);
void SimICPTHREE (unsigned long val, int state, void*);
void SimICPSIX (unsigned long val, int state, void*);
void SimICPEIGHT (unsigned long val, int state, void*);
void SimICPNINE (unsigned long val, int state, void*);
void SimICPZERO (unsigned long val, int state, void*);
void SimICPNav1(unsigned long val, int state, void *);
void SimICPAA1(unsigned long val, int state, void *);
void SimICPAG1(unsigned long val, int state, void *);
void SimICPResetDED(unsigned long val, int state, void *);
void SimICPDEDUP(unsigned long val, int state, void *);
void SimICPDEDDOWN(unsigned long val, int state, void *);
void SimICPCLEAR(unsigned long val, int state, void *);
void SimICPDEDSEQ(unsigned long val, int state, void *);
void SimRALTSTDBY(unsigned long val, int state, void *);
void SimRALTON(unsigned long val, int state, void *);
void SimRALTOFF(unsigned long val, int state, void *);
void SimLandingLightToggle(unsigned long val, int state, void *);
void SimParkingBrakeToggle(unsigned long val, int state, void *);
void SimLaserArmToggle(unsigned long val, int state, void *);
void SimFuelDoorToggle(unsigned long val, int state, void *);
void SimRightAPSwitch(unsigned long val, int state, void *);
void SimLeftAPSwitch(unsigned long val, int state, void *);
void SimAPOverride(unsigned long val, int state, void *);
void SimWarnReset(unsigned long val, int state, void *);
void SimReticleSwitch(unsigned long val, int state, void *);
void SimTMSUp(unsigned long val, int state, void *);
void SimTMSLeft(unsigned long val, int state, void *);
void SimTMSDown(unsigned long val, int state, void *);
void SimTMSRight(unsigned long val, int state, void *);
void SimSeatArm(unsigned long val, int state, void *);
void SimEWSRWRPower(unsigned long val, int state, void *);
void SimEWSJammerPower(unsigned long val, int state, void *);
void SimEWSChaffPower(unsigned long val, int state, void *);
void SimEWSFlarePower(unsigned long val, int state, void *);
void SimEWSPGMDec(unsigned long, int state, void*);
void SimEWSPGMInc(unsigned long, int state, void*);
void SimEWSProgDec(unsigned long, int state, void*);
void SimEWSProgInc(unsigned long, int state, void*);
void SimInhibitVMS(unsigned long val, int state, void *);
void SimRFSwitch(unsigned long val, int state, void *);
void SimDropProgrammed(unsigned long val, int state, void *);
void SimPinkySwitch(unsigned long val, int state, void *);
void SimGndJettEnable(unsigned long val, int state, void *);
void SimExtlPower(unsigned long val, int state, void *);
void SimExtlAntiColl(unsigned long val, int state, void *);
void SimExtlSteady(unsigned long val, int state, void *);
void SimExtlWing(unsigned long val, int state, void *);
void SimDMSUp(unsigned long val, int state, void *);
void SimDMSLeft(unsigned long val, int state, void *);
void SimDMSDown(unsigned long val, int state, void *);
void SimDMSRight(unsigned long val, int state, void *);
void SimAVTRSwitch(unsigned long val, int state, void *);
void SimAutoAVTR(unsigned long val, int state, void *);
void SimIFFPower(unsigned long val, int state, void *);
void SimIFFIn(unsigned long val, int state, void *);
void SimINSInc(unsigned long val, int state, void *);
void SimINSDec(unsigned long val, int state, void *);
void SimLEFLockSwitch(unsigned long val, int state, void *);
void SimDigitalBUP(unsigned long val, int state, void *);
void SimAltFlaps(unsigned long val, int state, void *);
void SimManualFlyup(unsigned long val, int state, void *);
void SimFLCSReset(unsigned long val, int state, void *);
void SimFLTBIT(unsigned long val, int state, void *);
void SimOBOGSBit(unsigned long val, int state, void *);
void SimMalIndLights(unsigned long val, int state, void *);
void SimProbeHeat(unsigned long val, int state, void *);
void SimEPUGEN(unsigned long val, int state, void *);
void SimTestSwitch(unsigned long val, int state, void *);
void SimOverHeat(unsigned long val, int state, void *);
void SimTrimAPDisc(unsigned long val, int state, void *);
void SimMaxPower(unsigned long val, int state, void *);
void SimABReset(unsigned long val, int state, void *);
void SimTrimNoseUp(unsigned long val, int state, void *);
void SimTrimNoseDown(unsigned long val, int state, void *);
void SimTrimYawLeft(unsigned long val, int state, void *);
void SimTrimYawRight(unsigned long val, int state, void *);
void SimTrimRollLeft(unsigned long val, int state, void *);
void SimTrimRollRight(unsigned long val, int state, void *);
void SimStepMissileVolumeDown(unsigned long val, int state, void *);
void SimStepMissileVolumeUp(unsigned long val, int state, void *);
void SimStepThreatVolumeDown(unsigned long val, int state, void *);
void SimStepThreatVolumeUp(unsigned long val, int state, void *);
void SimTriggerFirstDetent(unsigned long val, int state, void *);
void SimTriggerSecondDetent(unsigned long val, int state, void *);
void SimRetUp(unsigned long val, int state, void *);
void SimRetDn(unsigned long val, int state, void *);
void SimCursorEnable(unsigned long, int state, void*);
void SimStepComm1VolumeUp(unsigned long val, int state, void *);
void SimStepComm1VolumeDown(unsigned long val, int state, void *);
void SimStepComm2VolumeUp(unsigned long val, int state, void *);
void SimStepComm2VolumeDown(unsigned long val, int state, void *);
#ifdef DEBUG
void SimSwitchTextureOnOff(unsigned long val, int state, void *);
#endif
void SimSymWheelUp(unsigned long val, int state, void *);
void SimSymWheelDn(unsigned long val, int state, void *);
void SimToggleCockpit(unsigned long val, int state, void *);
void SimToggleGhostMFDs(unsigned long val, int state, void *);
void SimRangeKnobDown(unsigned long val, int state, void *);
void SimRangeKnobUp(unsigned long val, int state, void *);
//MI

// avionics power panel (JPO)
void SimSMSPower(unsigned long val, int state, void *);
void SimFCCPower(unsigned long val, int state, void *);
void SimMFDPower(unsigned long val, int state, void *);
void SimUFCPower(unsigned long val, int state, void *);
void SimGPSPower(unsigned long val, int state, void *);
void SimDLPower(unsigned long val, int state, void *);
void SimMAPPower(unsigned long val, int state, void *);
void SimTISLPower(unsigned long val, int state, void *);
void SimRightHptPower(unsigned long val, int state, void *);
void SimLeftHptPower(unsigned long val, int state, void *);
void SimFCRPower(unsigned long val, int state, void *);
void SimHUDPower(unsigned long val, int state, void *);

void SimRwrPower(unsigned long val, int state, void *);

void SendRadioMenuMsg (int, int, int);	// not an actual command, but is called by RadioMessageSend

void BreakToggle (unsigned long val, int state, void *);

void KneeboardTogglePage (unsigned long val, int state, void *);
void ToggleNVGMode (unsigned long val, int state, void *);
void ToggleSmoke (unsigned long val, int state, void *);
void TimeAccelerate (unsigned long val, int state, void *);
void TimeAccelerateMaxToggle (unsigned long val, int state, void *);
void TimeAccelerateInc (unsigned long val, int state, void *); // JB 010109
void TimeAccelerateDec (unsigned long val, int state, void *); // JB 010109
void SimFuelDump(unsigned long val, int state, void *); // JB 020313
void SimCycleDebugLabels(unsigned long val, int state, void *); // JB 020316
void AFABFull (unsigned long val, int state, void *);
void BombRippleIncrement (unsigned long val, int state, void *);
void BombIntervalIncrement (unsigned long val, int state, void *);
void BombRippleDecrement (unsigned long val, int state, void *);
void BombIntervalDecrement (unsigned long val, int state, void *);
void BombPairRelease (unsigned long val, int state, void *);
void BombSGLRelease (unsigned long val, int state, void *);
void BombBurstIncrement (unsigned long val, int state, void *);
void BombBurstDecrement (unsigned long val, int state, void *);

void OTWToggleScoreDisplay(unsigned long val, int state, void *);
void OTWToggleSidebar (unsigned long val, int state, void *);
void OTWTrackExternal(unsigned long val, int state, void *); 
void OTWTrackTargetToWeapon(unsigned long val, int state, void *);
void SimRadarAAModeStep (unsigned long val, int state, void *);
void SimRadarAGModeStep (unsigned long val, int state, void *);
void SimRadarGainUp (unsigned long val, int state, void *);
void SimRadarGainDown (unsigned long val, int state, void *);
void SimRadarStandby (unsigned long val, int state, void *);
void SimRadarRangeStepUp (unsigned long val, int state, void *);
void SimRadarRangeStepDown (unsigned long val, int state, void *);
void SimRadarNextTarget (unsigned long val, int state, void *);
void SimRadarPrevTarget (unsigned long val, int state, void *);
void SimRadarBarScanChange (unsigned long val, int state, void *);
void SimRadarAzimuthScanChange (unsigned long val, int state, void *);
void SimRadarFOVStep (unsigned long val, int state, void *);
void SimMaverickFOVStep(unsigned long val, int state, void * pButton);
void SimSOIFOVStep(unsigned long val, int state, void * pButton);
void SimRadarFreeze (unsigned long val, int state, void *);
void SimRadarSnowplow (unsigned long val, int state, void *);
void SimRadarCursorZero (unsigned long val, int state, void *);
void SimACMBoresight (unsigned long val, int state, void *);
void SimDesignate (unsigned long val, int state, void *);
void SimACMVertical (unsigned long val, int state, void *);
void SimDropTrack (unsigned long val, int state, void *);
void SimACMSlew (unsigned long val, int state, void *);
void SimACM30x20 (unsigned long val, int state, void *);
void SimRadarElevationDown (unsigned long val, int state, void *);
void SimRadarElevationUp (unsigned long val, int state, void *);
void SimRadarElevationCenter(unsigned long val, int state, void * pButton);
void SimRWRSetPriority (unsigned long val, int state, void *);
//void SimRWRSetSound (unsigned long val, int state, void *);
void SimRWRSetTargetSep (unsigned long val, int state, void *);
void SimRWRSetUnknowns (unsigned long val, int state, void *);
void SimRWRSetNaval (unsigned long val, int state, void *);
void SimRWRSetGroundPriority (unsigned long val, int state, void *);
void SimRWRSetSearch (unsigned long val, int state, void *);
void SimRWRHandoff (unsigned long val, int state, void *);
void SimNextWaypoint (unsigned long val, int state, void *);
void SimPrevWaypoint (unsigned long val, int state, void *);
void SimTogglePaused (unsigned long val, int state, void *);
void SimPickle (unsigned long val, int state, void *);
void SimTrigger (unsigned long val, int state, void *);
void SimMissileStep (unsigned long val, int state, void *);
void SimCursorUp (unsigned long val, int state, void *);
void SimCursorDown (unsigned long val, int state, void *);
void SimCursorLeft (unsigned long val, int state, void *);
void SimCursorRight (unsigned long val, int state, void *);
void SimToggleAutopilot (unsigned long val, int state, void *);
void SimStepSMSLeft (unsigned long val, int state, void *);
void SimStepSMSRight (unsigned long val, int state, void *);
void SimSelectSRMOverride (unsigned long val, int state, void *);
void SimSelectMRMOverride (unsigned long val, int state, void *);
void SimDeselectOverride (unsigned long val, int state, void *);
void SimToggleMissileCage (unsigned long val, int state, void *);
// Marco Edit - for AIM9 Spot/Scan
void SimToggleMissileSpotScan (unsigned long val, int state, void *);
// Marco Edit - for Bore/Slave
void SimToggleMissileBoreSlave (unsigned long val, int state, void *);
// Marco Edit - For Auto Uncage
void SimToggleMissileTDBPUncage(unsigned long val, int state, void *);

void SimDropChaff (unsigned long val, int state, void *);
void SimDropFlare (unsigned long val, int state, void *);
void SimToggleDropPattern (unsigned long val, int state, void *);
void SimHSDRangeStepUp (unsigned long val, int state, void *);
void SimHSDRangeStepDown (unsigned long val, int state, void *);
void SimToggleInvincible (unsigned long val, int state, void *);
void SimFCCSubModeStep (unsigned long val, int state, void *);
void SimEndFlight (unsigned long val, int state, void *);
void SimNextAAWeapon (unsigned long val, int state, void *);
void SimNextAGWeapon (unsigned long val, int state, void *);
void SimNextNavMode (unsigned long val, int state, void *);
void SimEject(unsigned long val, int state, void *);
void AFBrakesOut (unsigned long val, int state, void *);
void AFBrakesIn (unsigned long val, int state, void *);
void AFBrakesToggle (unsigned long val, int state, void *);
void AFGearToggle (unsigned long val, int state, void *);
void AFCoarseThrottleUp (unsigned long val, int state, void *);
void AFCoarseThrottleDown (unsigned long val, int state, void *);
void AFElevatorUp (unsigned long val, int state, void *);
void AFElevatorDown (unsigned long val, int state, void *);
void AFAileronLeft (unsigned long val, int state, void *);
void AFAileronRight (unsigned long val, int state, void *);
void AFThrottleUp (unsigned long val, int state, void *);
void AFThrottleDown (unsigned long val, int state, void *);
void AFRudderLeft (unsigned long val, int state, void *);
void AFRudderRight (unsigned long val, int state, void *);
void AFABOn (unsigned long val, int state, void *);
void AFIdle (unsigned long val, int state, void *);
void OTWTimeOfDayStep (unsigned long val, int state, void *);
void OTWStepNextAC (unsigned long val, int state, void *);
void OTWStepPrevAC (unsigned long val, int state, void *);
void OTWStepNextPadlock (unsigned long val, int state, void *);
void OTWStepNextPadlockAA (unsigned long val, int state, void *);		// 2002-03-12 S.G.
void OTWStepNextPadlockAG (unsigned long val, int state, void *);		// 2002-03-12 S.G.
void OTWStepPrevPadlock (unsigned long val, int state, void *);
void OTWStepPrevPadlockAA (unsigned long val, int state, void *);		// 2002-03-12 S.G.
void OTWStepPrevPadlockAG (unsigned long val, int state, void *);		// 2002-03-12 S.G.
void OTWToggleNames (unsigned long val, int state, void *);
void OTWToggleCampNames (unsigned long val, int state, void *);
void OTWSelectF3PadlockMode (unsigned long val, int state, void *);
void OTWSelectF3PadlockModeAA (unsigned long val, int state, void *);	// 2002-03-12 S.G.
void OTWSelectF3PadlockModeAG (unsigned long val, int state, void *);	// 2002-03-12 S.G.
void OTWSelectEFOVPadlockMode (unsigned long val, int state, void *);
void OTWSelectEFOVPadlockModeAA (unsigned long val, int state, void *);	// 2002-03-12 S.G.
void OTWSelectEFOVPadlockModeAG (unsigned long val, int state, void *);	// 2002-03-12 S.G.
void OTWRadioMenuStep (unsigned long val, int state, void *);
void OTWRadioMenuStepBack (unsigned long val, int state, void *);
void OTWStepMFD1 (unsigned long val, int state, void *);
void OTWStepMFD2 (unsigned long val, int state, void *);
void OTWStepMFD3 (unsigned long val, int state, void *);
void OTWStepMFD4 (unsigned long val, int state, void *);
void OTWToggleScales (unsigned long val, int state, void *);
void OTWTogglePitchLadder (unsigned long val, int state, void *);
void OTWStepHeadingScale (unsigned long val, int state, void *);
void OTWSelectHUDMode (unsigned long val, int state, void *);
void OTWToggleGLOC (unsigned long val, int state, void *);
void OTWSelectChaseMode (unsigned long val, int state, void *);
void OTWSelectOrbitMode (unsigned long val, int state, void *);
void OTWSelectAirFriendlyMode (unsigned long val, int state, void *);
void OTWSelectIncomingMode (unsigned long val, int state, void *);
void OTWSelectGroundFriendlyMode (unsigned long val, int state, void *);
void OTWSelectAirEnemyMode (unsigned long val, int state, void *);
void OTWSelectGroundEnemyMode (unsigned long val, int state, void *);
void OTWSelectTargetMode (unsigned long val, int state, void *);
void OTWSelectWeaponMode (unsigned long val, int state, void *);
void OTWSelectFlybyMode (unsigned long val, int state, void *);
void OTWSelectSatelliteMode (unsigned long val, int state, void *);
void OTWShowTestVersion (unsigned long val, int state, void *);
void OTWShowVersion (unsigned long val, int state, void *);
void OTWSelect2DCockpitMode (unsigned long val, int state, void *);
void OTWSelect3DCockpitMode (unsigned long val, int state, void *);
void OTWToggleBilinearFilter (unsigned long val, int state, void *);
void OTWToggleShading (unsigned long val, int state, void *);
void OTWToggleHaze (unsigned long val, int state, void *);
void OTWToggleLocationDisplay (unsigned long val, int state, void *);
void OTWToggleAeroDisplay (unsigned long val, int state, void *); // JPO
void OTWToggleRoof (unsigned long val, int state, void *);
void OTWToggleActionCamera (unsigned long val, int state, void *);
void OTWScaleDown (unsigned long val, int state, void *);
void OTWScaleUp (unsigned long val, int state, void *);
void OTWSetObjDetail (unsigned long val, int state, void * pButton);
void OTWObjDetailDown (unsigned long val, int state, void * pButton);
void OTWObjDetailUp (unsigned long val, int state, void * pButton);
void OTWTextureIncrease (unsigned long val, int state, void *);
void OTWTextureDecrease (unsigned long val, int state, void *);
void OTWToggleClouds (unsigned long val, int state, void *);
void OTWStepHudColor (unsigned long val, int state, void *);
void OTWToggleEyeFly (unsigned long val, int state, void *);
void OTWEnterPosition (unsigned long val, int state, void *);
void OTWToggleFrameRate (unsigned long val, int state, void *);
void OTWToggleAutoScale (unsigned long val, int state, void *);
void OTWSetScale (unsigned long val, int state, void *);
void OTWViewLeft (unsigned long val, int state, void *);
void OTWViewRight (unsigned long val, int state, void *);
void OTWViewUp (unsigned long val, int state, void *);
void OTWViewDown (unsigned long val, int state, void *);
void OTWViewDownLeft (unsigned long val, int state, void * pButton);
void OTWViewDownRight (unsigned long val, int state, void * pButton);
void OTWViewUpLeft (unsigned long val, int state, void * pButton);
void OTWViewUpRight (unsigned long val, int state, void * pButton);
void OTWViewReset (unsigned long val, int state, void *);
void OTWViewZoomIn (unsigned long val, int state, void *);
void OTWViewZoomOut (unsigned long val, int state, void *);
void OTWSwapMFDS (unsigned long val, int state, void *);
void OTWGlanceForward (unsigned long val, int state, void *);
void OTWCheckSix (unsigned long val, int state, void *);
void OTWStateStep (unsigned long val, int state, void *);
void CommandsSetKeyCombo (unsigned long val, int state, void *);
void KevinsFistOfGod (unsigned long val, int state, void *);
void SuperCruise (unsigned long val, int state, void *);
// 2000-11-10 FUNCTIONS ADDED BY S.G. FOR THE Drift C/O switch
void SimDriftCO (unsigned long val, int state, void *);
// END OF ADDED SECTION
// 2000-11-17 FUNCTIONS ADDED BY S.G. FOR THE Cat I/III switch
void SimCATSwitch (unsigned long val, int state, void *);
// END OF ADDED SECTION
void SimRegen(unsigned long val, int state, void *); // 2002-03-22 ADDED BT S.G.
void OTW1200View (unsigned long val, int state, void *);
void OTW1200DView (unsigned long val, int state, void *);
void OTW1200HUDView (unsigned long val, int state, void *);
void OTW1200LView (unsigned long val, int state, void *);
void OTW1200RView (unsigned long val, int state, void *);
void OTW1000View (unsigned long val, int state, void *);
void OTW200View (unsigned long val, int state, void *);
void OTW300View (unsigned long val, int state, void *);
void OTW400View (unsigned long val, int state, void *);
void OTW800View (unsigned long val, int state, void *);
void OTW900View (unsigned long val, int state, void *);
void RadioMessageSend (unsigned long val, int state, void *);
void SimToggleChatMode (unsigned long val, int state, void *);
void SimMotionFreeze (unsigned long val, int state, void *);
void ScreenShot (unsigned long val, int state, void *);
void FOVToggle (unsigned long val, int state, void *);
void OTWToggleAlpha (unsigned long val, int state, void *);
void ACMIToggleRecording (unsigned long val, int state, void *);
void SimSelectiveJettison (unsigned long val, int state, void *);
void SimEmergencyJettison (unsigned long val, int state, void *);
void SimWheelBrakes (unsigned long val, int state, void *);
void SimECMOn (unsigned long val, int state, void *);
void SoundOff(unsigned long val, int state, void * pButton);
void SimStepMasterArm(unsigned long val, int state, void * pButton);
void SimArmMasterArm(unsigned long val, int state, void * pButton);
void SimSafeMasterArm(unsigned long val, int state, void * pButton);
void SimSimMasterArm(unsigned long val, int state, void * pButton);


void SimHsiCourseInc (unsigned long val, int state, void *);
void SimHsiCourseDec (unsigned long val, int state, void *);
void SimHsiHeadingInc (unsigned long val, int state, void *);
void SimHsiHeadingDec (unsigned long val, int state, void *);
void SimAVTRToggle (unsigned long val, int state, void *);
void SimMPOToggle (unsigned long val, int state, void *);
void SimSilenceHorn (unsigned long val, int state, void *);
void SimStepHSIMode(unsigned long val, int state, void *);
void SimCATLimiterToggle(unsigned long val, int state, void * pButton);
void DecreaseAlow(unsigned long val, int state, void * pButton);
void IncreaseAlow(unsigned long val, int state, void * pButton);


void SimCBEOSB_1L(unsigned long val, int state, void *);
void SimCBEOSB_2L(unsigned long val, int state, void *);
void SimCBEOSB_3L(unsigned long val, int state, void *);
void SimCBEOSB_4L(unsigned long val, int state, void *);
void SimCBEOSB_5L(unsigned long val, int state, void *);
void SimCBEOSB_6L(unsigned long val, int state, void *);
void SimCBEOSB_7L(unsigned long val, int state, void *);
void SimCBEOSB_8L(unsigned long val, int state, void *);
void SimCBEOSB_9L(unsigned long val, int state, void *);
void SimCBEOSB_10L(unsigned long val, int state, void *);
void SimCBEOSB_11L(unsigned long val, int state, void *);
void SimCBEOSB_12L(unsigned long val, int state, void *);
void SimCBEOSB_13L(unsigned long val, int state, void *);
void SimCBEOSB_14L(unsigned long val, int state, void *);
void SimCBEOSB_15L(unsigned long val, int state, void *);
void SimCBEOSB_16L(unsigned long val, int state, void *);
void SimCBEOSB_17L(unsigned long val, int state, void *);
void SimCBEOSB_18L(unsigned long val, int state, void *);
void SimCBEOSB_19L(unsigned long val, int state, void *);
void SimCBEOSB_20L(unsigned long val, int state, void *);

void SimCBEOSB_1R(unsigned long val, int state, void *);
void SimCBEOSB_2R(unsigned long val, int state, void *);
void SimCBEOSB_3R(unsigned long val, int state, void *);
void SimCBEOSB_4R(unsigned long val, int state, void *);
void SimCBEOSB_5R(unsigned long val, int state, void *);
void SimCBEOSB_6R(unsigned long val, int state, void *);
void SimCBEOSB_7R(unsigned long val, int state, void *);
void SimCBEOSB_8R(unsigned long val, int state, void *);
void SimCBEOSB_9R(unsigned long val, int state, void *);
void SimCBEOSB_10R(unsigned long val, int state, void *);
void SimCBEOSB_11R(unsigned long val, int state, void *);
void SimCBEOSB_12R(unsigned long val, int state, void *);
void SimCBEOSB_13R(unsigned long val, int state, void *);
void SimCBEOSB_14R(unsigned long val, int state, void *);
void SimCBEOSB_15R(unsigned long val, int state, void *);
void SimCBEOSB_16R(unsigned long val, int state, void *);
void SimCBEOSB_17R(unsigned long val, int state, void *);
void SimCBEOSB_18R(unsigned long val, int state, void *);
void SimCBEOSB_19R(unsigned long val, int state, void *);
void SimCBEOSB_20R(unsigned long val, int state, void *);
void SimCBEOSB_GAINUP_R(unsigned long val, int state, void *);
void SimCBEOSB_GAINUP_L(unsigned long val, int state, void *);
void SimCBEOSB_GAINDOWN_R(unsigned long val, int state, void *);
void SimCBEOSB_GAINDOWN_L(unsigned long val, int state, void *);

void SimICPTILS(unsigned long val, int state, void *);
void SimICPALOW(unsigned long val, int state, void *);
void SimICPFAck(unsigned long val, int state, void *);
void SimICPPrevious(unsigned long val, int state, void *);
void SimICPNext(unsigned long val, int state, void *);
void SimICPLink(unsigned long val, int state, void *);
void SimICPCrus(unsigned long val, int state, void *);
void SimICPStpt(unsigned long val, int state, void *);
void SimICPMark(unsigned long val, int state, void *);
void SimICPEnter(unsigned long val, int state, void *);
void SimICPCom1(unsigned long val, int state, void *);
void SimICPCom2(unsigned long val, int state, void *);
void SimICPNav(unsigned long val, int state, void *);
void SimICPAA(unsigned long val, int state, void *);
void SimICPAG(unsigned long val, int state, void *);

void SimHUDScales(unsigned long val, int state, void *);
void SimHUDFPM(unsigned long val, int state, void *);
void SimHUDDED(unsigned long val, int state, void *);
void SimHUDVelocity(unsigned long val, int state, void *);
void SimHUDRadar(unsigned long val, int state, void *);
void SimHUDBrightness(unsigned long val, int state, void *);
//MI
void SimHUDBrightnessUp(unsigned long val, int state, void *);
void SimHUDBrightnessDown(unsigned long val, int state, void *);

void SimCycleRadioChannel(unsigned long val, int state, void *);
void SimToggleRadioVolume(unsigned long val, int state, void *);
void ExtinguishMasterCaution(unsigned long val, int state, void *);

void SimCycleLeftAuxComDigit(unsigned long val, int state, void *);
void SimCycleCenterAuxComDigit(unsigned long val, int state, void *);
void SimCycleRightAuxComDigit(unsigned long val, int state, void *);
void SimCycleBandAuxComDigit(unsigned long val, int state, void *);
void SimToggleAuxComMaster(unsigned long val, int state, void *);
void SimToggleAuxComAATR(unsigned long val, int state, void *);
void SimToggleUHFMaster(unsigned long val, int state, void *);
void SimTransmitCom1(unsigned long val, int state, void *);//me123
void SimTransmitCom2(unsigned long val, int state, void *);//me123
void SimToggleExtLights(unsigned long val, int state, void *);
void SimOpenChatBox(unsigned long val, int state, void *);
void SaveCockpitDefaults(unsigned long val, int state, void *);
void LoadCockpitDefaults(unsigned long val, int state, void *);

void SimCBEOSB_1(int);
void SimCBEOSB_2(int);
void SimCBEOSB_3(int);
void SimCBEOSB_4(int);
void SimCBEOSB_5(int);
void SimCBEOSB_6(int);
void SimCBEOSB_7(int);
void SimCBEOSB_8(int);
void SimCBEOSB_9(int);
void SimCBEOSB_10(int);
void SimCBEOSB_11(int);
void SimCBEOSB_12(int);
void SimCBEOSB_13(int);
void SimCBEOSB_14(int);
void SimCBEOSB_15(int);
void SimCBEOSB_16(int);
void SimCBEOSB_17(int);
void SimCBEOSB_18(int);
void SimCBEOSB_19(int);
void SimCBEOSB_20(int);

void GotoSMSMenu(int);
void GotoFCRMenu(int);
void GotoWpnPage(int);

void RadioTankerCommand (unsigned long val, int state, void * pButton);
void RadioTowerCommand (unsigned long val, int state, void * pButton);
void RadioAWACSCommand (unsigned long val, int state, void * pButton);


// ---------------------------------------------------------------------------
// Flight Menu Commands
// ---------------------------------------------------------------------------

void RadioWingCommand (unsigned long val, int state, void * pButton);
void RadioElementCommand (unsigned long val, int state, void * pButton);
void RadioFlightCommand (unsigned long val, int state, void * pButton);
void OTWRadioPageStep (unsigned long val, int state, void *);
void OTWRadioPageStepBack (unsigned long val, int state, void *);

// ---------------------------------------------------------------------------
// Flight Commands
// ---------------------------------------------------------------------------


//
// Commands that modify Action and Search States
//

void WingmanClearSix (unsigned long val, int state, void *);
void ElementClearSix (unsigned long val, int state, void *);
void FlightClearSix (unsigned long val, int state, void *);

void WingmanCheckSix (unsigned long val, int state, void *);
void ElementCheckSix (unsigned long val, int state, void *);
void FlightCheckSix (unsigned long val, int state, void *);

void WingmanBreakLeft (unsigned long val, int state, void *);
void ElementBreakLeft (unsigned long val, int state, void *);
void FlightBreakLeft (unsigned long val, int state, void *);

void WingmanBreakRight (unsigned long val, int state, void *);
void ElementBreakRight (unsigned long val, int state, void *);
void FlightBreakRight (unsigned long val, int state, void *);

void WingmanPince (unsigned long val, int state, void *);
void ElementPince (unsigned long val, int state, void *);
void FlightPince (unsigned long val, int state, void *);

void WingmanPosthole (unsigned long val, int state, void *);
void ElementPosthole (unsigned long val, int state, void *);
void FlightPosthole (unsigned long val, int state, void *);

void WingmanChainsaw (unsigned long val, int state, void *);
void ElementChainsaw (unsigned long val, int state, void *);
void FlightChainsaw (unsigned long val, int state, void *);

void WingmanFlex (unsigned long val, int state, void *);
void ElementFlex (unsigned long val, int state, void *);
void FlightFlex (unsigned long val, int state, void *);

void WingmanGoShooterMode (unsigned long val, int state, void *);
void ElementGoShooterMode (unsigned long val, int state, void *);
void FlightGoShooterMode (unsigned long val, int state, void *);

void WingmanGoCoverMode (unsigned long val, int state, void *);
void ElementGoCoverMode (unsigned long val, int state, void *);
void FlightGoCoverMode (unsigned long val, int state, void *);

void WingmanSearchGround (unsigned long val, int state, void *);
void ElementSearchGround (unsigned long val, int state, void *);
void FlightSearchGround (unsigned long val, int state, void *);

void WingmanSearchAir (unsigned long val, int state, void *);
void ElementSearchAir (unsigned long val, int state, void *);
void FlightSearchAir (unsigned long val, int state, void *);

void WingmanResumeNormal (unsigned long val, int state, void *);
void ElementResumeNormal (unsigned long val, int state, void *);
void FlightResumeNormal (unsigned long val, int state, void *);

void WingmanRejoin (unsigned long val, int state, void *);
void ElementRejoin (unsigned long val, int state, void *);
void FlightRejoin (unsigned long val, int state, void *);

// 
// Commands that modify the state basis
//

void WingmanDesignateTarget (unsigned long val, int state, void *);
void ElementDesignateTarget (unsigned long val, int state, void *);
void FlightDesignateTarget (unsigned long val, int state, void *);

void WingmanDesignateGroup (unsigned long val, int state, void *);
void ElementDesignateGroup (unsigned long val, int state, void *);
void FlightDesignateGroup (unsigned long val, int state, void *);

void WingmanWeaponsHold (unsigned long val, int state, void *);
void ElementWeaponsHold (unsigned long val, int state, void *);
void FlightWeaponsHold (unsigned long val, int state, void *);

void WingmanWeaponsFree (unsigned long val, int state, void *);
void ElementWeaponsFree (unsigned long val, int state, void *);
void FlightWeaponsFree (unsigned long val, int state, void *);


//
// Commands that modify formation
//

void WingmanSpread (unsigned long val, int state, void * pButton);
void ElementSpread (unsigned long val, int state, void * pButton);
void FlightSpread (unsigned long val, int state, void * pButton);

void WingmanStack (unsigned long val, int state, void * pButton);
void ElementStack (unsigned long val, int state, void * pButton);
void FlightStack (unsigned long val, int state, void * pButton);

void WingmanLadder (unsigned long val, int state, void * pButton);
void ElementLadder (unsigned long val, int state, void * pButton);
void FlightLadder (unsigned long val, int state, void * pButton);

void WingmanFluid (unsigned long val, int state, void * pButton);
void ElementFluid (unsigned long val, int state, void * pButton);
void FlightFluid (unsigned long val, int state, void * pButton);

void WingmanWedge (unsigned long val, int state, void * pButton);
void ElementWedge (unsigned long val, int state, void * pButton);
void FlightWedge (unsigned long val, int state, void * pButton);

void WingmanTrail (unsigned long val, int state, void * pButton);
void ElementTrail (unsigned long val, int state, void * pButton);
void FlightTrail (unsigned long val, int state, void * pButton);

void WingmanResCell (unsigned long val, int state, void * pButton);
void ElementResCell (unsigned long val, int state, void * pButton);
void FlightResCell (unsigned long val, int state, void * pButton);

void WingmanBox (unsigned long val, int state, void * pButton);
void ElementBox (unsigned long val, int state, void * pButton);
void FlightBox (unsigned long val, int state, void * pButton);

void WingmanArrow (unsigned long val, int state, void * pButton);
void ElementArrow (unsigned long val, int state, void * pButton);
void FlightArrow (unsigned long val, int state, void * pButton);

void WingmanKickout (unsigned long val, int state, void *);
void ElementKickout (unsigned long val, int state, void *);
void FlightKickout (unsigned long val, int state, void *);

void WingmanCloseup (unsigned long val, int state, void *);
void ElementCloseup (unsigned long val, int state, void *);
void FlightCloseup (unsigned long val, int state, void *);

void WingmanToggleSide (unsigned long val, int state, void *);
void ElementToggleSide (unsigned long val, int state, void *);
void FlightToggleSide (unsigned long val, int state, void *);

void WingmanIncreaseRelAlt (unsigned long val, int state, void *);
void ElementIncreaseRelAlt (unsigned long val, int state, void *);
void FlightIncreaseRelAlt (unsigned long val, int state, void *);

void WingmanDecreaseRelAlt (unsigned long val, int state, void *);
void ElementDecreaseRelAlt (unsigned long val, int state, void *);
void FlightDecreaseRelAlt (unsigned long val, int state, void *);

void WingmanVic (unsigned long val, int state, void *);
void ElementVic (unsigned long val, int state, void *);
void FlightVic (unsigned long val, int state, void *);

void WingmanFinger4 (unsigned long val, int state, void *);
void ElementFinger4 (unsigned long val, int state, void *);
void FlightFinger4 (unsigned long val, int state, void *);

void WingmanEchelon (unsigned long val, int state, void *);
void ElementEchelon (unsigned long val, int state, void *);
void FlightEchelon (unsigned long val, int state, void *);

// placeholder formation commands

void WingmanForm1 (unsigned long val, int state, void *);
void ElementForm1 (unsigned long val, int state, void *);
void FlightForm1 (unsigned long val, int state, void *);

void WingmanForm2 (unsigned long val, int state, void *);
void ElementForm2 (unsigned long val, int state, void *);
void FlightForm2 (unsigned long val, int state, void *);

void WingmanForm3 (unsigned long val, int state, void *);
void ElementForm3 (unsigned long val, int state, void *);
void FlightForm3 (unsigned long val, int state, void *);

void WingmanForm4 (unsigned long val, int state, void *);
void ElementForm4 (unsigned long val, int state, void *);
void FlightForm4 (unsigned long val, int state, void *);


//
// Transient Commands
//

void WingmanGiveBra (unsigned long val, int state, void *);
void ElementGiveBra (unsigned long val, int state, void *);
void FlightGiveBra (unsigned long val, int state, void *);

void WingmanGiveStatus (unsigned long val, int state, void *);
void ElementGiveStatus (unsigned long val, int state, void *);
void FlightGiveStatus (unsigned long val, int state, void *);

void WingmanGiveDamageReport (unsigned long val, int state, void *);
void ElementGiveDamageReport (unsigned long val, int state, void *);
void FlightGiveDamageReport (unsigned long val, int state, void *);

void WingmanGiveFuelState (unsigned long val, int state, void *);
void ElementGiveFuelState (unsigned long val, int state, void *);
void FlightGiveFuelState (unsigned long val, int state, void *);

void WingmanGiveWeaponsCheck (unsigned long val, int state, void *);
void ElementGiveWeaponsCheck (unsigned long val, int state, void *);
void FlightGiveWeaponsCheck (unsigned long val, int state, void *);

// M.N. emergency store drop
void WingmanDropStores (unsigned long val, int state, void *);
void ElementDropStores (unsigned long val, int state, void *);
void FlightDropStores (unsigned long val, int state, void *);


//
// Other Commands
//

void WingmanRTB (unsigned long val, int state, void *);
void ElementRTB (unsigned long val, int state, void *);
void FlightRTB (unsigned long val, int state, void *);

// Direct access to ATC commands
void ATCRequestClearance (unsigned long val, int state, void * pButton);
void ATCRequestEmergencyClearance (unsigned long val, int state, void * pButton);
void ATCRequestTakeoff (unsigned long val, int state, void * pButton);
void ATCRequestTaxi (unsigned long val, int state, void * pButton);
void ATCTaxiing (unsigned long val, int state, void * pButton);
void ATCReadyToGo (unsigned long val, int state, void * pButton);
void ATCRotate (unsigned long val, int state, void * pButton);
void ATCGearUp (unsigned long val, int state, void * pButton);
void ATCGearDown (unsigned long val, int state, void * pButton);
void ATCBrake (unsigned long val, int state, void * pButton);
void ATCAbortApproach (unsigned long val, int state, void * pButton);

// Direct access to FAC commands
void FACCheckIn (unsigned long val, int state, void * pButton);
void FACWilco (unsigned long val, int state, void * pButton);
void FACUnable (unsigned long val, int state, void * pButton);
void FACReady (unsigned long val, int state, void * pButton);
void FACIn (unsigned long val, int state, void * pButton);
void FACOut (unsigned long val, int state, void * pButton);
void FACRequestMark (unsigned long val, int state, void * pButton);
void FACRequestTarget (unsigned long val, int state, void * pButton);
void FACRequestBDA (unsigned long val, int state, void * pButton);
void FACRequestLocation (unsigned long val, int state, void * pButton);
void FACRequestTACAN (unsigned long val, int state, void * pButton);

// Direct access to Tanker commands
void TankerRequestFuel (unsigned long val, int state, void * pButton);
void TankerReadyForGas (unsigned long val, int state, void * pButton);
void TankerDoneRefueling (unsigned long val, int state, void * pButton);
void TankerBreakaway (unsigned long val, int state, void * pButton);

// Direct access to AWACS commands
void AWACSRequestPicture (unsigned long val, int state, void * pButton);
void AWACSRequestTanker (unsigned long val, int state, void * pButton);
// MN
void AWACSRequestCarrier (unsigned long val, int state, void * pButton);
void AWACSWilco (unsigned long val, int state, void * pButton);
void AWACSUnable (unsigned long val, int state, void * pButton);
void AWACSRequestHelp (unsigned long val, int state, void * pButton);
void AWACSRequestRelief (unsigned long val, int state, void * pButton);

// ---------------------------------------------------------------------------
// End Flight Commands!
// ---------------------------------------------------------------------------

void SimSpeedyGonzalesUp (unsigned long val, int state, void *);
void SimSpeedyGonzalesDown (unsigned long val, int state, void *);

///---------------------------------------------------------------------------
/// Testing Only Commands
///---------------------------------------------------------------------------
#define _DO_VTUNE_

#ifdef _DO_VTUNE_
void ToggleVtune (unsigned long val, int state, void *);
#endif

#endif
