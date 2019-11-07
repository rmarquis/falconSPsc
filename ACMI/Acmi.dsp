# Microsoft Developer Studio Project File - Name="Acmi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Acmi - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Acmi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Acmi.mak" CFG="Acmi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Acmi - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Acmi - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ".."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Acmi - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\Falcon4___Win32_Release\Acmi"
# PROP BASE Intermediate_Dir "..\Falcon4___Win32_Release\Acmi"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Falcon4___Win32_Release\Acmi"
# PROP Intermediate_Dir "..\Falcon4___Win32_Release\Acmi"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /Zp4 /MT /W3 /GX /Zi /O1 /Op /Ob2 /I "..\Falclib\Include" /I "..\Vu2\Include" /I "..\Acmi\Src\Include" /I "..\Graphics" /I "..\Codelib\Include" /I "..\\" /I "..\Campaign\Include" /D "NDEBUG" /D "_LIB" /D TARGET=m_i486 /D "WIN32" /D "_MBCS" /D "STRICT" /D "WIN32_LEAN_AND_MEAN" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Acmi - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\Falcon4___Win32_Debug\Acmi"
# PROP BASE Intermediate_Dir "..\Falcon4___Win32_Debug\Acmi"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Falcon4___Win32_Debug\Acmi"
# PROP Intermediate_Dir "..\Falcon4___Win32_Debug\Acmi"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MTd /W3 /Gm /GX /ZI /Od /I "..\Falclib\Include" /I "..\Vu2\Include" /I "..\Acmi\Src\Include" /I "..\Graphics" /I "..\Codelib\Include" /I "..\\" /I "..\Campaign\Include" /D "_DEBUG" /D "_LIB" /D TARGET=m_i486 /D "WIN32" /D "_MBCS" /D "STRICT" /D "WIN32_LEAN_AND_MEAN" /FD /GZ /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "Acmi - Win32 Release"
# Name "Acmi - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\AcmiCam.cpp
# End Source File
# Begin Source File

SOURCE=.\src\acmihash.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Acmiloop.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Acmirec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\AcmiTape.cpp
# End Source File
# Begin Source File

SOURCE=.\src\acmiUI.cpp
# End Source File
# Begin Source File

SOURCE=.\src\AcmiView.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\src\include\acmiCam.h
# End Source File
# Begin Source File

SOURCE=.\src\include\acmihash.h
# End Source File
# Begin Source File

SOURCE=.\src\include\acmirec.h
# End Source File
# Begin Source File

SOURCE=.\src\include\acmitape.h
# End Source File
# Begin Source File

SOURCE=.\src\include\AcmiUI.h
# End Source File
# Begin Source File

SOURCE=.\src\include\AcmiView.h
# End Source File
# End Group
# End Target
# End Project
