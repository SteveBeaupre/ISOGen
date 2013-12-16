//----------------------------------------------------------------------//
//#define WINAPI __stdcall//use the default windows api calling convention
//----------------------------------------------------------------------//
#include "ISOMaker.h"

CISOMaker ISOMaker;

void  WINAPI _ResetAll();
void  WINAPI _SetWindowsHandles(HWND MainWnd, BOOL GaugePresent);
void  WINAPI _SetVolumeIdentifier(char *VolID);
BOOL  WINAPI _AddDirectory(char *dir);
BOOL  WINAPI _AddFile(char *fname);
BOOL  WINAPI _BuildISO(char *fname, char *bootfname = NULL);
void  WINAPI _PrintTree();
DWORD WINAPI _GetNumErrorsMsg();
char* WINAPI _GetErrorsMsg(DWORD Indx);
