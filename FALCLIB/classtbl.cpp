// Use objects in directory ObjectSet0708 with this file.
/*
 * Machine Generated Class Table Constants loader file.
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 11-August-1999 at 16:28:04
 * Generated from file Access Class Table
 */

#include <windows.h>
#include <stdio.h>
#include "F4vu.h"
#include "ClassTbl.h"
#include "F4Find.h"
#include "entity.h"
 

// Which langauge should we use?
int gLangIDNum = 1;

/*
* Class Table Constant Init Function
*/

void InitClassTableAndData(char *name,char *objset)
{
FILE* filePtr;
char  fileName[MAX_PATH];

if (stricmp(objset,"objects") != 0)
  {
  //Check for correct object data version
  ShiAssert( stricmp("ObjectSet0708",objset) == 0);
  }

sprintf(fileName, "%s\\%s.ini", FalconObjectDataDir, name);

gLangIDNum = GetPrivateProfileInt("Lang", "Id", 0, fileName);

filePtr = OpenCampFile(name,"ct","rb");
if (filePtr)
    {
     fread (&NumEntities, sizeof (short), 1, filePtr);
     Falcon4ClassTable = new Falcon4EntityClassType[NumEntities];
     fread (Falcon4ClassTable, sizeof (Falcon4EntityClassType), NumEntities,filePtr);
     fclose(filePtr);
    }
else
    {
    Falcon4ClassTable = NULL;
    }
}
