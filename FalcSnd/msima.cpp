/*************************************************************************
	$Header: /usr2/cvs/Falcon/FalcSnd/msima.cpp,v 1.2 2001/03/15 23:01:02 codec Exp $
	
	convert IMA ADPCM into a MS PCM wave file
	
*************************************************************************/

#include <stdio.h>
#include <string.h>

#include "mssw.h"
#include "msima.h"
static char				ssBuff[5120];		//source stereo adpcm buffer
static char				smBuff[5120];		//source mono adpcm buffer
//-----------------------------------------------------------------------
long ImaAdpcmDecodeStereo(FILE *sptr, char *dBuff, long sBuffSize, long *sRead)
{
	long	bytesRead;
	long	decodedSize;

	if (sBuffSize == 0)
		return 0;

	bytesRead = fread(ssBuff, sizeof(char), sBuffSize, sptr);
	decodedSize = ImaDecodeS16(ssBuff, dBuff, bytesRead);

	*sRead = bytesRead;

	return decodedSize;
}
//-----------------------------------------------------------------------
long ImaAdpcmDecodeMono(FILE *sptr, char *dBuff, long sBuffSize, long *sRead)
{
	long	bytesRead;
	long	decodedSize;

	if (sBuffSize == 0)
		return 0;

	bytesRead = fread(smBuff, sizeof(char), sBuffSize, sptr);
	decodedSize = ImaDecodeM16((char*)smBuff, (char *)dBuff, bytesRead);

	*sRead = bytesRead;

	return decodedSize;
}
//-----------------------------------------------------------------------
