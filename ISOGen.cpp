#include "ISOGen.h"

/**************************************************
main procedure.....................................
**************************************************/
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD fdwreason, LPVOID lpReserved)
{
    switch(fdwreason)
	{
    case DLL_PROCESS_ATTACH: DisableThreadLibraryCalls(hInst); break;
    case DLL_PROCESS_DETACH: break;
    case DLL_THREAD_ATTACH:  break;
    case DLL_THREAD_DETACH:  break;
    }
    return TRUE;
}

///////////////////////////////////////////////////
///////////////////////////////////////////////////

void WINAPI _ResetAll()
{
	ISOMaker.Reset();
}

///////////////////////////////////////////////////

void WINAPI _SetWindowsHandles(HWND MainWnd, BOOL GaugePresent)
{
	ISOMaker.SetWindowsHandles(MainWnd, GaugePresent);
}

///////////////////////////////////////////////////

void WINAPI _SetVolumeIdentifier(char *VolID)
{
	ISOMaker.SetVolumeIdentifier(VolID);
}

///////////////////////////////////////////////////

BOOL WINAPI _AddDirectory(char *dir)
{
	return ISOMaker.AddDirectory(dir);
}

///////////////////////////////////////////////////

BOOL WINAPI _AddFile(char *fname)
{
	return ISOMaker.AddFile(fname);
}

///////////////////////////////////////////////////

BOOL WINAPI _BuildISO(char *fname, char *bootfname)
{
	return ISOMaker.BuildISO(fname, bootfname);
}

///////////////////////////////////////////////////

void WINAPI _PrintTree()
{
	ISOMaker.PrintTree();
}

///////////////////////////////////////////////////

DWORD WINAPI _GetNumErrorsMsg()
{
	return ISOMaker.GetNumErrorsMsg();
}

///////////////////////////////////////////////////

char* WINAPI _GetErrorsMsg(DWORD Indx)
{
	return ISOMaker.GetErrorsMsg(Indx);
}

///////////////////////////////////////////////////

