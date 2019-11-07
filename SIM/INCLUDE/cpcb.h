#ifndef _CPCB_H
#define _CPCB_H

//Note: Remember to update this value when adding new objects
//#define TOTAL_CPCALLBACK_SLOTS	150
//MI Need more callbackslots!!
//#define TOTAL_BUTTONCALLBACK_SLOTS	111
//#define TOTAL_BUTTONCALLBACK_SLOTS	122

// JPO - just don't bother. We'll calculate them at compile time!

extern const int TOTAL_CPCALLBACK_SLOTS, TOTAL_BUTTONCALLBACK_SLOTS;

typedef void (*CPCallback)(void *);
typedef void (*ButtonCallback) (void *, int);


typedef struct {
	CPCallback	ExecCallback;
	CPCallback	EventCallback;
	CPCallback	DisplayCallback;
} CPCallbackStruct;


typedef struct {
	ButtonCallback TransAeroToState;
	ButtonCallback	TransStateToAero;
} ButtonCallbackStruct;



extern CPCallbackStruct	CPCallbackArray[];
extern ButtonCallbackStruct ButtonCallbackArray[];
#endif
