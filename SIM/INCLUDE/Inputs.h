class PilotInputs
{
   public:
      enum TwoWaySwitch {Off, On};
      enum FourWaySwitch {Center, Up, Down, Left, Right};
      TwoWaySwitch pickleButton;
      TwoWaySwitch pitchOverride;
      TwoWaySwitch missileStep;
      TwoWaySwitch missileUncage;
      FourWaySwitch speedBrakeCmd;
      FourWaySwitch missileOverride;
      FourWaySwitch trigger;
      FourWaySwitch tmsPos;
      FourWaySwitch trimPos;
      FourWaySwitch dmsPos;
      FourWaySwitch cmmsPos;
      FourWaySwitch cursorControl;
      FourWaySwitch micPos;
      float manRange;
      float antennaEl;
      float pstick;
      float rstick;
      float throttle;
      float rudder;

      PilotInputs (void);
      ~PilotInputs (void);
      void Update (void);
	  void Reset(void);
};

extern PilotInputs UserStickInputs;

