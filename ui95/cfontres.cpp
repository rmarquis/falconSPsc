#include "stdafx.h"

#include <windows.h>
#include "chandler.h"

C_Fontmgr::C_Fontmgr()
{
	ID_=0;

	name_[0]=0;
	height_=0;
	first_=0;
	last_=0;

	bytesperline_=0;

	fNumChars_=0;
	fontTable_=NULL;

	dSize_=0;
	fontData_=NULL;

	kNumKerns_=0;
	kernList_=NULL;
}

C_Fontmgr::~C_Fontmgr()
{
	if(fontTable_ || fontData_ || kernList_)
		Cleanup();
}

void C_Fontmgr::Setup(long ID,char *fontfile)
{
	FILE *fp;

	ID_=ID;

	fp=fopen(fontfile,"rb");
	if(!fp)
	{
		MonoPrint("FONT error: %s not opened\n",fontfile);
		return;
	}
	fread(&name_,32,1,fp);
	fread(&height_,sizeof(long),1,fp);
	fread(&first_,sizeof(short),1,fp);
	fread(&last_, sizeof(short),1,fp);
	fread(&bytesperline_,sizeof(long),1,fp);
	fread(&fNumChars_,sizeof(long),1,fp);
	fread(&kNumKerns_,sizeof(long),1,fp);
	fread(&dSize_,sizeof(long),1,fp);
	if(fNumChars_)
	{
		fontTable_=new CharStr[fNumChars_];
		fread(fontTable_,sizeof(CharStr),fNumChars_,fp);
	}
	if(kNumKerns_)
	{
		kernList_=new KerningStr[kNumKerns_];
		fread(kernList_,sizeof(KerningStr),kNumKerns_,fp);
	}
	if(dSize_)
	{
#ifdef USE_SH_POOLS
		fontData_=(char*)MemAllocPtr(UI_Pools[UI_GENERAL_POOL],sizeof(char)*(dSize_),FALSE);
#else
		fontData_=new char[dSize_];
#endif
		fread(fontData_,dSize_,1,fp);
	}
	fclose(fp);
}

void C_Fontmgr::Save(char *filename)
{
	FILE *fp;

	fp=fopen(filename,"wb");
	if(!fp)
	{
		MonoPrint("FONT error: can't create %s\n",filename);
		return;
	}
	fwrite(&name_,32,1,fp);
	fwrite(&height_,sizeof(long),1,fp);
	fwrite(&first_,sizeof(short),1,fp);
	fwrite(&last_, sizeof(short),1,fp);
	fwrite(&bytesperline_,sizeof(long),1,fp);
	fwrite(&fNumChars_,sizeof(long),1,fp);
	fwrite(&kNumKerns_,sizeof(long),1,fp);
	fwrite(&dSize_,sizeof(long),1,fp);
	if(fNumChars_)
	{
		fwrite(fontTable_,sizeof(CharStr),fNumChars_,fp);
	}
	if(kNumKerns_)
	{
		fwrite(kernList_,sizeof(KerningStr),kNumKerns_,fp);
	}
	if(dSize_)
	{
		fwrite(fontData_,dSize_,1,fp);
	}
	fclose(fp);
}

void C_Fontmgr::Cleanup()
{
	ID_=0;
	first_=0;
	last_=0;
	bytesperline_=0;

	fNumChars_=0;
	if(fontTable_)
	{
		delete fontTable_;
		fontTable_=NULL;
	}

	kNumKerns_=0;
	if(kernList_)
	{
		delete kernList_;
		kernList_=NULL;
	}

	dSize_=0;
	if(fontData_)
	{
#ifdef USE_SH_POOLS
		MemFreePtr(fontData_);
#else
		delete fontData_;
#endif
		fontData_=NULL;
	}
}

long C_Fontmgr::Width(_TCHAR *str)
{
	long i;
	long size;
	long thechar;

	if(!str)
		return(0);
	size=0;
	i=0;

	while(str[i])
	//while(!F4IsBadReadPtr(&(str[i]), sizeof(_TCHAR)) && str[i]) // JB 010401 CTD (too much CPU)
	{
		thechar=str[i] & 0xff;
		if(thechar >= first_ && thechar <= last_)
		{
			thechar-=first_;
			size+=fontTable_[thechar].lead + fontTable_[thechar].w + fontTable_[thechar].trail;
		}
		i++;
	}
	return(size+1);
}

long C_Fontmgr::Width(_TCHAR *str,long len)
{
	long i;
	long size;
	long thechar;

	if(!str)
		return(0);
	size=0;
	i=0;
	while(str[i] && i < len)
	{
		thechar=str[i] & 0xff;
		if(thechar >= first_ && thechar <= last_)
		{
			thechar-=first_;
			size+=fontTable_[thechar].lead + fontTable_[thechar].w + fontTable_[thechar].trail;
		}
		i++;
	}
	return(size+1);
}

long C_Fontmgr::Height()
{
	return(height_);
}

CharStr *C_Fontmgr::GetChar(short ID)
{
	if(fontTable_ && ID >= first_ && ID <= last_)
		return(&fontTable_[ID-first_]);
	return(NULL);
}


void C_Fontmgr::Draw(SCREEN *surface,_TCHAR *str,long length,WORD color,long x,long y)
{
	long idx,i,j;
	long xoffset,yoffset;
	unsigned long thechar;
	unsigned char *sstart,*sptr,seg=0;
	WORD *dstart,*dptr,*dendh,*dendv;

	if(!fontData_)
		return;

	if(!str)
		return;
	idx=0;
	xoffset=x;
	yoffset=y;
	dendv=surface->mem + surface->width * surface->height; // Make sure we don't go past the end of the surface
	while(str[idx] && idx < length)
	{
		thechar=str[idx]&0xff;
		if(thechar >= (unsigned long)first_ && thechar <= (unsigned long)last_)//! 
		{
			thechar-=first_;
			xoffset+=fontTable_[thechar].lead;

			sstart=(unsigned char *)(fontData_ + (thechar * bytesperline_ * height_));
			dstart=surface->mem + (yoffset * surface->width) + xoffset;
			dendh=surface->mem + (yoffset * surface->width) + surface->width;
			for(i=0;i<height_ && dstart < dendv;i++)
			{
				dptr=dstart;
				sptr=sstart;
				for(j=0;j<fontTable_[thechar].w;j++)
				{
					if(dptr < dendh)
					{
						if(!(j&0x7))
							seg=*sptr++;

						if(seg & 1)
							*dptr++=color;
						else
							dptr++;
						seg >>= 1;
					}
				}
				sstart+=bytesperline_;
				dstart+=surface->width;
				dendh+=surface->width;
			}
			xoffset+=fontTable_[thechar].w + fontTable_[thechar].trail;
		}

		idx++;
	}
}

void C_Fontmgr::DrawSolid(SCREEN *surface,_TCHAR *str,long length,WORD color,WORD bgcolor,long x,long y)
{
	long idx,i,j;
	long xoffset,yoffset;
	unsigned long thechar;
	unsigned char *sstart,*sptr,seg=0;
	WORD *dstart,*dptr,*dendh,*dendv;

	if(!fontData_)
		return;

	if(!str)
		return;
	idx=0;
	xoffset=x;
	yoffset=y;
	dendv=surface->mem + surface->width * surface->height; // Make sure we don't go past the end of the surface
	while(str[idx] && idx < length)
	{
		thechar=str[idx]&0xff;
		if(thechar >= (unsigned long)first_ && thechar <= (unsigned long)last_)//! 
		{
			thechar-=first_;

			sstart=(unsigned char *)(fontData_ + (thechar * bytesperline_ * height_));
			dstart=surface->mem + (yoffset * surface->width) + xoffset;
			dendh=surface->mem + (yoffset * surface->width) + surface->width;
			for(i=0;i<height_ && dstart < dendv;i++)
			{
				dptr=dstart;
				sptr=sstart;
				for(j=0;j<fontTable_[thechar].lead;j++)
					if(dptr < dendh)
						*dptr++=bgcolor;
				for(j=0;j<fontTable_[thechar].w;j++)
				{
					if(!(j&0x7))
						seg=*sptr++;
					if(dptr < dendh)
					{
						if(seg & 1)
							*dptr++=color;
						else
							*dptr++=bgcolor;
						seg >>= 1;
					}
				}
				for(j=0;j<fontTable_[thechar].trail;j++)
					if(dptr < dendh)
						*dptr++=bgcolor;

				sstart+=bytesperline_;
				dstart+=surface->width;
				dendh+=surface->width;
			}
			xoffset+=fontTable_[thechar].lead + fontTable_[thechar].w + fontTable_[thechar].trail;
		}

		idx++;
	}
}

void C_Fontmgr::Draw(SCREEN *surface,_TCHAR *str,WORD color,long x,long y)
{
	if(str)	Draw(surface,str,_tcsclen(str),color,x,y);
}

void C_Fontmgr::DrawSolid(SCREEN *surface,_TCHAR *str,WORD color,WORD bgcolor,long x,long y)
{
	if(str)	DrawSolid(surface,str,_tcsclen(str),color,bgcolor,x,y);
}

void C_Fontmgr::Draw(SCREEN *surface,_TCHAR *str,long length,WORD color,long x,long y,UI95_RECT *cliprect)
//!void C_Fontmgr::Draw(SCREEN *surface,_TCHAR *str,short length,WORD color,long x,long y,UI95_RECT *cliprect)
{
	long idx,i,j;
	long xoffset,yoffset;
	unsigned long thechar;
	unsigned char *sstart,*sptr,seg=0;
	WORD *dstart,*dptr;
	WORD *dendh,*dendv;
	WORD *dclipx,*dclipy;

	if(!fontData_)
		return;

	if(!str)
		return;
	idx=0;
	xoffset=x;
	yoffset=y;
	dclipy=surface->mem + (cliprect->top * surface->width);
	dendv=surface->mem + (cliprect->bottom * surface->width); // Make sure we don't go past the end of the surface
	while(str[idx] && idx < length)
	{
		thechar=str[idx]&0xff;
		if(thechar >= (unsigned long)first_ && thechar <= (unsigned long)last_)
		{
			thechar-=first_;
			xoffset+=fontTable_[thechar].lead;

			sstart=(unsigned char *)(fontData_ + (thechar * bytesperline_ * height_));
			dstart=surface->mem + (yoffset * surface->width) + xoffset;
			dclipx=surface->mem + (yoffset * surface->width) + cliprect->left;
			dendh=dclipx + (cliprect->right-cliprect->left);

			for(i=0;i<height_ && dstart < dendv;i++)
			{
				if(dstart >= dclipy)
				{
					dptr=dstart;
					sptr=sstart;
					for(j=0;j<fontTable_[thechar].w;j++)
					{
						if(!(j&0x7))
						{
							seg=*sptr++;
							if(!seg)
							{
								j+=7;
								dptr+=8;
								continue;
							}
						}
						if(dptr < dendh)
						{
							if(dptr >= dclipx)
							{
								if(seg & 1)
									*dptr++=color;
								else
									dptr++;
							}
							else
								dptr++;
							seg >>= 1;
						}
					}
				}
				sstart+=bytesperline_;
				dclipx+=surface->width;
				dstart+=surface->width;
				dendh+=surface->width;
			}
			xoffset+=fontTable_[thechar].w + fontTable_[thechar].trail;
		}

		idx++;
	}
}

void C_Fontmgr::DrawSolid(SCREEN *surface,_TCHAR *str,long length,WORD color,WORD bgcolor,long x,long y,UI95_RECT *cliprect)
//!void C_Fontmgr::DrawSolid(SCREEN *surface,_TCHAR *str,short length,WORD color,WORD bgcolor,long x,long y,UI95_RECT *cliprect)
{
	long idx,i,j;
	long xoffset,yoffset;
	unsigned long thechar;
	unsigned char *sstart,*sptr,seg=0;
	WORD *dstart,*dptr;
	WORD *dendh,*dendv;
	WORD *dclipx,*dclipy;

	if(!fontData_)
		return;

	if(!str)
		return;
	idx=0;
	xoffset=x;
	yoffset=y;
	dclipy=surface->mem + (cliprect->top * surface->width);
	dendv=surface->mem + (cliprect->bottom * surface->width); // Make sure we don't go past the end of the surface
	while(str[idx] && idx < length)
	{
		thechar=str[idx]&0xff;
		if(thechar >= (unsigned long)first_ && thechar <= (unsigned long)last_)
		{
			thechar-=first_;

			sstart=(unsigned char *)(fontData_ + (thechar * bytesperline_ * height_));
			dstart=surface->mem + (yoffset * surface->width) + xoffset;
			dclipx=surface->mem + (yoffset * surface->width) + cliprect->left;
			dendh=dclipx + (cliprect->right-cliprect->left);

			for(i=0;i<height_ && dstart < dendv;i++)
			{
				if(dstart >= dclipy)
				{
					dptr=dstart;
					sptr=sstart;
					for(j=0;j<fontTable_[thechar].lead;j++)
					{
						if(dptr < dendh)
						{
							if(dptr >= dclipx)
								*dptr++=bgcolor;
							else
								dptr++;
						}
					}

					for(j=0;j<fontTable_[thechar].w;j++)
					{
						if(dptr < dendh)
						{
							if(!(j&0x7))
								seg=*sptr++;
							if(dptr >= dclipx)
							{
								if(seg & 1)
									*dptr++=color;
								else
									*dptr++=bgcolor;
							}
							else
								dptr++;
							seg >>= 1;
						}
					}
					for(j=0;j<fontTable_[thechar].trail;j++)
					{
						if(dptr < dendh)
						{
							if(dptr >= dclipx)
								*dptr++=bgcolor;
							else
								dptr++;
						}
					}
				}
				sstart+=bytesperline_;
				dclipx+=surface->width;
				dstart+=surface->width;
				dendh+=surface->width;
			}
			xoffset+=fontTable_[thechar].lead + fontTable_[thechar].w + fontTable_[thechar].trail;
		}

		idx++;
	}
}

void C_Fontmgr::Draw(SCREEN *surface,_TCHAR *str,WORD color,long x,long y,UI95_RECT *cliprect)
{
	if(str)
		Draw(surface,str,_tcsclen(str),color,x,y,cliprect);
}

void C_Fontmgr::DrawSolid(SCREEN *surface,_TCHAR *str,WORD color,WORD bgcolor,long x,long y,UI95_RECT *cliprect)
{
	if(str)
		DrawSolid(surface,str,_tcsclen(str),color,bgcolor,x,y,cliprect);
}
