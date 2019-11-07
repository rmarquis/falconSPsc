#ifndef _HARMPOD_H
#define _HARMPOD_H

#include "rwr.h"

class FalconPrivateList;
class ALR56Class;
class GroundListElement;

#define HTS_DISPLAY_RADIUS	0.65F
#define HTS_CURSOR_RATE		0.25F
#define HTS_Y_OFFSET		-0.3F

class HarmTargetingPod : public RwrClass
{
  public:
	HarmTargetingPod(int idx, SimMoverClass* newPlatform);
	virtual ~HarmTargetingPod(void);
	virtual void  GetAGCenter (float* x, float* y);	// Center of rwr ground search
	
#if 0
	class ListElement
	{
	  public:
		ListElement(FalconEntity *newEntity);
		~ListElement();
		
		FalconEntity*	BaseObject(void) {return baseObject;};
		void			HandoffBaseObject(void);
		
		ListElement*	next;
		int				flags;
		int				symbol;
		VU_TIME			lastHit;

	  private:
		FalconEntity*	baseObject;
	};
	
	enum DisplayFlags {
		Radiate    = 0x1,
		Track      = 0x2,
		Launch     = 0x4,
      UnChecked  = 0x8,
	};
#endif
		
	float	cursorX, cursorY;

	virtual SimObjectType*	Exec (SimObjectType* targetList);
	virtual void			Display (VirtualDisplay *newDisplay);

	virtual int	ObjectDetected (FalconEntity*, int trackType, int dummy = 0);

	int				GetRange (void)				{ return displayRange; };
	void			SetRange (int newRange)		{ displayRange = newRange; };

	void			LockTargetUnderCursor(void);
	void			BoresightTarget(void);
	void			NextTarget(void);
	void			PrevTarget(void);
	virtual void	SetDesiredTarget( SimObjectType* newTarget );
   VU_ID       FindIDUnderCursor(void);
	GroundListElement*	FindTargetUnderCursor( void );


  protected:
	static int		flash;			// Do we draw flashing things this frame?
	int				displayRange;
	//ListElement*	emmitterList;

	void			BuildPreplannedTargetList( void );
	GroundListElement*	FindEmmitter( FalconEntity *entity );
	void			LockListElement( GroundListElement* );
	void			DrawWEZ( class MissileClass *theMissile );
};

#endif
