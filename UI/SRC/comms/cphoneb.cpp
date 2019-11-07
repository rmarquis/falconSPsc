#include <windows.h>
#include "stdio.h"
#include "f4vu.h"
#include "cphoneb.h"

PhoneBook::PhoneBook()
{
	Root_=NULL;
	Current_=NULL;
}

PhoneBook::~PhoneBook()
{
	if(Root_)
		Cleanup();
}

void PhoneBook::Setup()
{
}

void PhoneBook::Cleanup()
{
	RemoveAll();
}

PHONEBOOK *PhoneBook::FindID(long ID)
{
	PHONEBOOK *cur;

	cur=Root_;
	while(cur)
	{
		if(cur->ID == ID)
			return(cur);
		cur=cur->Next;
	}
	return(NULL);
}

void PhoneBook::Load(char *filename)
{
	long count,i;
	FILE *ifp;
	_TCHAR desc[22];
	ComDataClass tmpentry;

	ifp=fopen(filename,"rb");
	if(!ifp)
		return;

	fread(&count,sizeof(long),1,ifp);

	for(i=0;i<count;i++)
	{
		fread(desc,sizeof(_TCHAR)*20,1,ifp);
		desc[20]=0;
		fread(&tmpentry,sizeof(ComDataClass),1,ifp);
		Add(desc,&tmpentry);
	}
	fclose(ifp);
}

void PhoneBook::Save(char *filename)
{
	PHONEBOOK *cur;
	long count;
	FILE *ofp;

	count=0;
	cur=Root_;
	while(cur)
	{
		count++;
		cur=cur->Next;
	}

	ofp=fopen(filename,"wb");
	if(!ofp)
		return;

	fwrite(&count,sizeof(long),1,ofp);

	cur=Root_;
	while(cur)
	{
		fwrite(cur->description,sizeof(_TCHAR)*20,1,ofp);
		fwrite(&cur->entry,sizeof(ComDataClass),1,ofp);
		cur=cur->Next;
	}

	fclose(ofp);
}

void PhoneBook::Add(_TCHAR *desc,ComDataClass *newentry)
{
	PHONEBOOK *newph,*cur;

	long tmpID=1;

	while(FindID(tmpID))
		tmpID++;

	newph=new PHONEBOOK;
	newph->ID=tmpID;
	_tcsncpy(newph->description,desc,20);
	newph->description[20]=0;
	memcpy(&newph->entry,newentry,sizeof(ComDataClass));
	newph->Next=NULL;

	if(!Root_)
		Root_=newph;
	else
	{
		cur=Root_;
		while(cur->Next)
			cur=cur->Next;

		cur->Next=newph;
	}
}

void PhoneBook::Remove(long ID)
{
	PHONEBOOK *cur,*prev;

	if(!Root_) return;

	if(Root_->ID == ID)
	{
		cur=Root_;
		Root_=Root_->Next;
		if(Current_ == cur)
			Current_=Root_;
		delete cur;
	}
	else
	{
		prev=Root_;
		cur=prev->Next;
		while(cur)
		{
			if(cur->ID == ID)
			{
				prev->Next=cur->Next;
				if(Current_ == cur)
					Current_=Current_->Next;
				delete cur;
				return;
			}
			prev=cur;
			cur=cur->Next;
		}
	}
}

void PhoneBook::RemoveAll()
{
	PHONEBOOK *cur,*prev;

	cur=Root_;
	while(cur)
	{
		prev=cur;
		cur=cur->Next;
		delete prev;
	}
	Root_=NULL;
	Current_=NULL;
}
