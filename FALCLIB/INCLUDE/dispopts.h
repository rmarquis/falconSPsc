#ifndef _DISPLAY_OPTIONS_
#define _DISPLAY_OPTIONS_


class DisplayOptionsClass
{
	public:
		unsigned short DispWidth;			//
		unsigned short DispHeight;
		unsigned char DispVideoCard;			//
		unsigned char DispVideoDriver;			//

		// OW
		int DispDepth;		// Display Mode depth

		DisplayOptionsClass(void);
		void Initialize(void);
		int LoadOptions(char *filename ="display");
		int SaveOptions(void);


};

extern DisplayOptionsClass DisplayOptions;

#endif