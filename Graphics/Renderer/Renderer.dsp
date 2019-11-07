# Microsoft Developer Studio Project File - Name="Renderer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Renderer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Renderer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Renderer.mak" CFG="Renderer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Renderer - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Renderer - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "..\.."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Renderer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\Falcon4___Win32_Release\Graphics\Renderer"
# PROP Intermediate_Dir "..\..\Falcon4___Win32_Release\Graphics\Renderer"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /Zp4 /MT /W3 /GX /Zi /O1 /Op /Ob2 /I "..\..\Codelib\Include" /I "..\..\\" /I "..\Include" /I "..\..\vu2\include" /D "NDEBUG" /D "_LIB" /D TARGET=m_i486 /D "WIN32" /D "_MBCS" /D "STRICT" /D "WIN32_LEAN_AND_MEAN" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Renderer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\Falcon4___Win32_Debug\Graphics\Renderer"
# PROP BASE Intermediate_Dir "..\..\Falcon4___Win32_Debug\Graphics\Renderer"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\Falcon4___Win32_Debug\Graphics\Renderer"
# PROP Intermediate_Dir "..\..\Falcon4___Win32_Debug\Graphics\Renderer"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MTd /W3 /Gm /GX /ZI /Od /I "..\..\Codelib\Include" /I "..\..\\" /I "..\Include" /I "..\..\vu2\include" /D "_DEBUG" /D "_LIB" /D TARGET=m_i486 /D "WIN32" /D "_MBCS" /D "STRICT" /D "WIN32_LEAN_AND_MEAN" /FD /GZ /c
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

# Name "Renderer - Win32 Release"
# Name "Renderer - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\canvas3d.cpp
# End Source File
# Begin Source File

SOURCE=.\clip2d.cpp
# End Source File
# Begin Source File

SOURCE=.\clip3d.cpp
# End Source File
# Begin Source File

SOURCE=.\Display.cpp
# End Source File
# Begin Source File

SOURCE=.\gmComposit.cpp
# End Source File
# Begin Source File

SOURCE=.\gmradar.cpp
# End Source File
# Begin Source File

SOURCE=.\Mono2d.cpp
# End Source File
# Begin Source File

SOURCE=.\OTW.cpp
# End Source File
# Begin Source File

SOURCE=.\OTWcull.cpp
# End Source File
# Begin Source File

SOURCE=.\OTWdraw.cpp
# End Source File
# Begin Source File

SOURCE=.\OTWsky.cpp
# End Source File
# Begin Source File

SOURCE=.\Render2d.cpp
# End Source File
# Begin Source File

SOURCE=.\Render3d.cpp
# End Source File
# Begin Source File

SOURCE=.\RenderIR.cpp
# End Source File
# Begin Source File

SOURCE=.\RenderNVG.cpp
# End Source File
# Begin Source File

SOURCE=.\RenderTV.cpp
# End Source File
# Begin Source File

SOURCE=.\RenderWire.cpp
# End Source File
# Begin Source File

SOURCE=.\Rviewpnt.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Include\canvas3d.h
# End Source File
# Begin Source File

SOURCE=..\include\display.h
# End Source File
# Begin Source File

SOURCE=..\Include\gmComposit.h
# End Source File
# Begin Source File

SOURCE=..\Include\gmradar.h
# End Source File
# Begin Source File

SOURCE=..\Include\Mono2d.h
# End Source File
# Begin Source File

SOURCE=..\include\render2d.h
# End Source File
# Begin Source File

SOURCE=..\Include\Render3d.h
# End Source File
# Begin Source File

SOURCE=..\Include\RenderIR.h
# End Source File
# Begin Source File

SOURCE=..\Include\RenderNVG.h
# End Source File
# Begin Source File

SOURCE=..\include\renderow.h
# End Source File
# Begin Source File

SOURCE=..\Include\RenderTV.h
# End Source File
# Begin Source File

SOURCE=..\Include\RenderWire.h
# End Source File
# Begin Source File

SOURCE=..\include\rviewpnt.h
# End Source File
# End Group
# End Target
# End Project
