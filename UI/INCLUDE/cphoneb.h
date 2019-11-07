#ifndef _PHONE_BOOK_H_
#define _PHONE_BOOK_H_

// Just incase they are not already included :D
#include <tchar.h>
#include "apitypes.h"
#include "f4comms.h"
#include "comdata.h"

typedef struct PhoneListStr PHONEBOOK;

struct PhoneListStr
{
	long ID;
	_TCHAR description[21];
	ComDataClass entry;
	PHONEBOOK *Next;
};

class PhoneBook
{
	private:
		PHONEBOOK *Root_;
		PHONEBOOK *Current_;


	public:

		PhoneBook();
		~PhoneBook();

		void Setup();
		void Cleanup();
		void Load(char *filename);
		void Save(char *filename);
		void Add(_TCHAR *desc,ComDataClass *newentry);
		void Remove(long ID);
		void RemoveAll();
		PHONEBOOK *FindID(long ID);
		ComDataClass *GetFirst()   { Current_=Root_; return(GetCurrent()); }
		ComDataClass *GetNext()    { if(Current_) { Current_=Current_->Next; return(GetCurrent()); } return(NULL); }
		ComDataClass *GetCurrent() { if(Current_) return(&Current_->entry); return(NULL); }
		PHONEBOOK *GetCurrentPtr() { return(Current_); }
};

#endif