#include <windows.h>
#include "f4vu.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "cphoneb.h"
#include "uicomms.h"
#include "queue.h"
#include "userids.h"
#include "textids.h"

extern C_Handler *gMainHandler;

PhoneBook *gPlayerBook=NULL;

static long localID=0;
static _TCHAR localDescription[21]="Default";
static ComDataClass localData=
{
	FCT_LAN,
	F4_COMMS_HOST,
	F4_COMMS_COM1,
	F4_COMMS_CBR_256000,
	F4_COMMS_NOPARITY,
	F4_COMMS_ONESTOPBIT,
	F4_COMMS_DPCPA_DTRFLOW,
	0,
	0
};

void SetSingle_Comms_Ctrls();
void LeaveDogfight();
void CheckFlyButton();
void DeleteGroupList(long ID);
void CloseWindowCB(long ID,short hittype,C_Base *control);

// copies stuff from params into window's controls
void CopyDataToWindow(_TCHAR *desc,ComDataClass *entry)
{
	C_Window *win;
	C_Button *btn,*activate;
	C_ListBox *lbox;
	C_EditBox *ebox;
	F4CSECTIONHANDLE* Leave;

	if(desc == NULL || entry == NULL)
		return;

	win=gMainHandler->FindWindow(PB_WIN);
	if(win)
	{
		Leave=UI_Enter(win);

		activate=NULL;
		ebox=(C_EditBox *)win->FindControl(SELECTED_CONNECTION);
		if(ebox)
			ebox->SetText(desc);

		btn=(C_Button *)win->FindControl(CON_TYPE_MODEM);
		if(btn)
		{
			if(entry->protocol == FCT_ModemToModem)
			{
				btn->SetState(1);
				activate=btn;

				btn=(C_Button *)win->FindControl(MODEM_CALL);
				if(btn)
				{
					if(entry->connect_type)
						btn->SetState(1);
					else
						btn->SetState(0);
				}

				btn=(C_Button *)win->FindControl(MODEM_ANSWER);
				if(btn)
				{
					if(!entry->connect_type)
						btn->SetState(1);
					else
						btn->SetState(0);
				}
			}
			else
				btn->SetState(0);
		}

		ebox=(C_EditBox *)win->FindControl(PHONE_NUMBER);
		if(ebox)
			ebox->SetText(entry->phone_number);

		btn=(C_Button *)win->FindControl(CON_TYPE_INTERNET);
		if(btn)
		{
			if(entry->protocol == FCT_WAN || entry->protocol == FCT_Server)
			{
				btn->SetState(1);
				activate=btn;
			}
			else
				btn->SetState(0);
		}

		ebox=(C_EditBox *)win->FindControl(IP_ADDRESS_1);
		if(ebox)
			ebox->SetInteger((entry->ip_address >> 24) & 0xff);
//			ebox->SetInteger(entry->ip_address & 0xff);

		ebox=(C_EditBox *)win->FindControl(IP_ADDRESS_2);
		if(ebox)
			ebox->SetInteger((entry->ip_address >> 16) & 0xff);
//			ebox->SetInteger((entry->ip_address >> 8) & 0xff);

		ebox=(C_EditBox *)win->FindControl(IP_ADDRESS_3);
		if(ebox)
			ebox->SetInteger((entry->ip_address >> 8) & 0xff);
//			ebox->SetInteger((entry->ip_address >> 16) & 0xff);

		ebox=(C_EditBox *)win->FindControl(IP_ADDRESS_4);
		if(ebox)
			ebox->SetInteger(entry->ip_address & 0xff);
//			ebox->SetInteger((entry->ip_address >> 24) & 0xff);

		btn=(C_Button *)win->FindControl(INTERNET_SERVER);
		if(btn)
		{
			if(entry->protocol == FCT_Server)
			{
				btn->SetState(1);

				btn=(C_Button *)win->FindControl(SERIAL_MASTER);
				if(btn)
				{
					if(entry->connect_type)
						btn->SetState(1);
					else
						btn->SetState(0);
				}

				btn=(C_Button *)win->FindControl(SERIAL_SLAVE);
				if(btn)
				{
					if(!entry->connect_type)
						btn->SetState(1);
					else
						btn->SetState(0);
				}
			}
			else
				btn->SetState(0);
		}

		btn=(C_Button *)win->FindControl(CON_TYPE_SERIAL);
		if(btn)
		{
			if(entry->protocol == FCT_NullModem)
			{
				btn->SetState(1);
				activate=btn;
			}
			else
				btn->SetState(0);
		}

		btn=(C_Button *)win->FindControl(CON_TYPE_LAN);
		if(btn)
		{
			if(!activate || entry->protocol == FCT_NoConnection)
			{
				entry->protocol=FCT_LAN;
				activate=btn;
			}
			if(entry->protocol == FCT_LAN)
				btn->SetState(1);
			else
				btn->SetState(0);
		}

		btn=(C_Button *)win->FindControl(CON_TYPE_JETNET);
		if(btn)
		{
			if(entry->protocol == FCT_JetNet || entry->protocol == FCT_Server)
			{
				btn->SetState(1);
				activate=btn;
			}
			else
				btn->SetState(0);
		}

		lbox=(C_ListBox *)win->FindControl(SET_MAX_RATE);
		if(lbox)
			lbox->SetValue(entry->baud_rate);

		lbox=(C_ListBox *)win->FindControl(SET_LAN_BANDWIDTH);
		if(lbox)
			lbox->SetValue(entry->lan_rate);

		lbox=(C_ListBox *)win->FindControl(SET_MODEM_BANDWIDTH);
		if(lbox)
			lbox->SetValue(entry->modem_rate);

		lbox=(C_ListBox *)win->FindControl(SET_INTERNET_BANDWIDTH);
		if(lbox)
			lbox->SetValue(entry->internet_rate);

		lbox=(C_ListBox *)win->FindControl(SET_JETNET_BANDWIDTH);
		if(lbox)
			lbox->SetValue(entry->jetnet_rate);

		lbox=(C_ListBox *)win->FindControl(SET_PORT);
		if(lbox)
			lbox->SetValue(entry->com_port);

		if(activate)
		{
			win->UnHideCluster(activate->GetUserNumber(C_STATE_0));
			win->HideCluster(activate->GetUserNumber(C_STATE_1));
			win->HideCluster(activate->GetUserNumber(C_STATE_2));
			win->HideCluster(activate->GetUserNumber(C_STATE_3));
		}
		win->RefreshWindow();
		UI_Leave(Leave);
	}
}

// copies window's controls into params
void CopyDataFromWindow(_TCHAR *desc,ComDataClass *entry)
{
	C_Window *win;
	C_Button *btn;
	C_ListBox *lbox;
	C_EditBox *ebox;

	if(desc == NULL || entry == NULL)
		return;

	entry->protocol = FCT_NoConnection;

	win=gMainHandler->FindWindow(PB_WIN);
	if(win)
	{
		ebox=(C_EditBox *)win->FindControl(SELECTED_CONNECTION);
		if(ebox)
			_tcscpy(desc,ebox->GetText());

		btn=(C_Button *)win->FindControl(CON_TYPE_MODEM);
		if(btn)
		{
			if(btn->GetState())
			{
				entry->protocol = FCT_ModemToModem;

				btn=(C_Button *)win->FindControl(MODEM_CALL);
				if(btn)
				{
					if(btn->GetState())
						entry->connect_type=F4_COMMS_DIAL;
				}

				btn=(C_Button *)win->FindControl(MODEM_ANSWER);
				if(btn)
				{
					if(btn->GetState())
						entry->connect_type=F4_COMMS_ANSWER;
				}
			}
		}

		ebox=(C_EditBox *)win->FindControl(PHONE_NUMBER);
		if(ebox)
			_tcscpy(entry->phone_number,ebox->GetText());

		btn=(C_Button *)win->FindControl(CON_TYPE_INTERNET);
		if(btn)
		{
			if(btn->GetState())
				entry->protocol = FCT_WAN;
		}

		entry->ip_address=0;
		ebox=(C_EditBox *)win->FindControl(IP_ADDRESS_1);
		if(ebox)
			entry->ip_address |= (ebox->GetInteger() & 0xff) << 24;
//			entry->ip_address |= (ebox->GetInteger() & 0xff);

		ebox=(C_EditBox *)win->FindControl(IP_ADDRESS_2);
		if(ebox)
			entry->ip_address |= (ebox->GetInteger() & 0xff) << 16;
//			entry->ip_address |= (ebox->GetInteger() & 0xff) << 8;

		ebox=(C_EditBox *)win->FindControl(IP_ADDRESS_3);
		if(ebox)
			entry->ip_address |= (ebox->GetInteger() & 0xff) << 8;
//			entry->ip_address |= (ebox->GetInteger() & 0xff) << 16;

		ebox=(C_EditBox *)win->FindControl(IP_ADDRESS_4);
		if(ebox)
			entry->ip_address |= (ebox->GetInteger() & 0xff);
//			entry->ip_address |= (ebox->GetInteger() & 0xff) << 24;

		btn=(C_Button *)win->FindControl(INTERNET_SERVER);
		if(btn)
		{
			if(btn->GetState() && entry->protocol == FCT_WAN)
				entry->protocol = FCT_Server;
		}

		btn=(C_Button *)win->FindControl(CON_TYPE_LAN);
		if(btn)
		{
			if(btn->GetState())
				entry->protocol = FCT_LAN;
		}

		btn=(C_Button *)win->FindControl(CON_TYPE_SERIAL);
		if(btn)
		{
			if(btn->GetState())
			{
				entry->protocol = FCT_NullModem;

				btn=(C_Button *)win->FindControl(SERIAL_MASTER);
				if(btn)
				{
					if(btn->GetState())
						entry->connect_type=1;
				}

				btn=(C_Button *)win->FindControl(SERIAL_SLAVE);
				if(btn)
				{
					if(btn->GetState())
						entry->connect_type=0;
				}
			}
		}

		btn=(C_Button *)win->FindControl(CON_TYPE_JETNET);
		if(btn)
		{
			if(btn->GetState())
				entry->protocol = FCT_JetNet;
		}

		lbox=(C_ListBox *)win->FindControl(SET_MAX_RATE);
		if(lbox)
			entry->baud_rate=static_cast<uchar>(lbox->GetTextID());

		lbox=(C_ListBox *)win->FindControl(SET_LAN_BANDWIDTH);
		if(lbox)
			entry->lan_rate=static_cast<uchar>(lbox->GetTextID());

		lbox=(C_ListBox *)win->FindControl(SET_MODEM_BANDWIDTH);
		if(lbox)
			entry->modem_rate=static_cast<uchar>(lbox->GetTextID());

		lbox=(C_ListBox *)win->FindControl(SET_INTERNET_BANDWIDTH);
		if(lbox)
			entry->internet_rate=static_cast<uchar>(lbox->GetTextID());

		lbox=(C_ListBox *)win->FindControl(SET_JETNET_BANDWIDTH);
		if(lbox)
			entry->jetnet_rate=static_cast<uchar>(lbox->GetTextID());

		lbox=(C_ListBox *)win->FindControl(SET_PORT);
		if(lbox)
			entry->com_port=static_cast<uchar>(lbox->GetTextID());
	}
}

void Phone_Select_CB(long ID,short hittype,C_Base *)
{
	PHONEBOOK *data;
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	data=gPlayerBook->FindID(ID);
	if(data)
	{
		localID=ID;
		_tcscpy(localDescription,data->description);
		memcpy(&localData,&data->entry,sizeof(ComDataClass));

		CopyDataToWindow(localDescription,&localData);
	}
}

void CopyPBToWindow(long ID,long Client)
{
	C_Window *win;
	C_Button *btn=NULL;
	PHONEBOOK *entry;
	F4CSECTIONHANDLE* Leave;
	int y=4;

	win=gMainHandler->FindWindow(ID);
	if(win)
	{
		Leave=UI_Enter(win);
		DeleteGroupList(ID);

		gPlayerBook->GetFirst();
		entry=gPlayerBook->GetCurrentPtr();
		while(entry)
		{
			btn=new C_Button;
			btn->Setup(entry->ID,C_TYPE_RADIO,0,0);
			btn->SetXY(5,y);
			btn->SetHotSpot(0,0,win->ClientArea_[Client].right-win->ClientArea_[Client].left-10,gFontList->GetHeight(win->Font_));
			btn->SetText(C_STATE_0,entry->description);
			btn->SetText(C_STATE_1,entry->description);
			btn->SetColor(C_STATE_0,0x00dddddd);
			btn->SetColor(C_STATE_1,0x0000ff00);
			btn->SetFont(win->Font_);
			btn->SetGroup(100);
			btn->SetClient(static_cast<short>(Client));
			btn->SetCallback(Phone_Select_CB);
			btn->SetUserNumber(_UI95_DELGROUP_SLOT_,_UI95_DELGROUP_ID_);
			win->AddControl(btn);
			y+=gFontList->GetHeight(win->Font_)+2;

			gPlayerBook->GetNext();
			entry=gPlayerBook->GetCurrentPtr();
		}
		win->RefreshClient(Client);
		win->ScanClientAreas();
		UI_Leave(Leave);
	}
	if(!localID && !btn)
	{
		_tcscpy(localDescription,gStringMgr->GetString(TXT_DEFAULT));
		localID=1;
		CopyDataToWindow(localDescription,&localData);
	}
	else if(btn)
	{
		btn->SetState(1);
		Phone_Select_CB(btn->GetID(),C_TYPE_LMOUSEUP,btn);
	}
}

void Phone_New_CB(long,short hittype,C_Base *)
{
	long newid;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	newid=1;
	while(gPlayerBook->FindID(newid))
		newid++;

	localID=newid;
	_tcscpy(localDescription,gStringMgr->GetString(TXT_NEW_ENTRY));
	memset(&localData,0,sizeof(ComDataClass));

	CopyDataToWindow(localDescription,&localData);
}

void Phone_Apply_CB(long,short hittype,C_Base *)
{
	PHONEBOOK *apply;
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	CopyDataFromWindow(localDescription,&localData);

	apply=gPlayerBook->FindID(localID);
	if(apply)
	{
		_tcscpy(apply->description,localDescription);
		memcpy(&apply->entry,&localData,sizeof(ComDataClass));
	}
	else
	{
		gPlayerBook->Add(localDescription,&localData);
	}
	CopyPBToWindow(PB_WIN,0);
}

void Phone_Remove_CB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(gPlayerBook->FindID(localID))
	{
		gPlayerBook->Remove(localID);
		CopyPBToWindow(PB_WIN,0);
	}
}

void Phone_Connect_CB(long n,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(gCommsMgr->Online())
		return;

	CopyDataFromWindow(localDescription,&localData);

	if(localData.protocol == FCT_JetNet)
	{
		// The bahaviour depends on the sender (this is a hack I know but so the whole falcon code is actually a hack)
		if(control != gMainHandler->FindWindow(JETNET_WIN)->FindControl(JETNET_BROWSER_PLAY))
		{	
			// Close the phone book window
			C_Base *wndClose = control->GetParent()->FindControl(CLOSE_WINDOW);
			if(wndClose)
				CloseWindowCB(wndClose->GetID(), hittype, wndClose);

			// Open the in-game server browser window
			C_Window *win;

			win=gMainHandler->FindWindow(JETNET_WIN);
			if(!win) return;

			gMainHandler->ShowWindow(win);
			gMainHandler->WindowToFront(win);

			return;
		}

		else
		{
			// Here we ovveride the IP
			localData.ip_address = n;
		}
	}

	if(!gUICommsQ)
	{
		CommsQueue *nq=new CommsQueue;
		if(nq)
		{
			nq->Setup(gCommsMgr->AppWnd_);
			gUICommsQ=nq;
		}
	}
	gCommsMgr->StartComms(&localData);
	SetSingle_Comms_Ctrls();
	LeaveDogfight();
	CheckFlyButton();
	if(gCommsMgr->Online() && control->Parent_)
		gMainHandler->DisableWindowGroup(control->Parent_->GetGroup());
}

void Phone_ConnectType_CB(long,short hittype,C_Base *control)
{
	int i;

	if(hittype != C_TYPE_LMOUSEUP)
		return;


	control->Parent_->UnHideCluster(control->GetUserNumber(0));
	i=1;
	while(control->GetUserNumber(i))
	{
		control->Parent_->HideCluster(control->GetUserNumber(i));
		i++;
	}
	control->Parent_->RefreshWindow();
}
