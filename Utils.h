#ifndef _CUTILS_H_
#define _CUTILS_H_
#ifdef __cplusplus

#define WIN32_LEAN_AND_MEAN // Trim libraies.
#define VC_LEANMEAN         // Trim even further.

#include "Windows.h"
#include "Stdio.h"

#include "SectorType.h"
#include "SafeKill.h"

//----------------------------------------------------------------------//
// The main class...
//----------------------------------------------------------------------//
class CUtils {
public:
	WORD  MakeBigEndian2b(WORD w);
	DWORD MakeBigEndian4b(DWORD dw);
	BOOL  IsOdd(DWORD x);
	DWORD MakeEven(DWORD x);
	DWORD MakeSectorEven(DWORD Size);
	DWORD GetSectorTaken(DWORD Size);
	
	void* IncSectorPtr(void *_ptr);
	void  GetStructSizeByNodeName(DWORD *pSize, DWORD BaseSize, DWORD ShortNameLen, DWORD LongNameLen, BOOL IsSpecialDir, BOOL UseUnicode);

	bool  CompareBuffers(BYTE *pBuf1, BYTE *pBuf2, DWORD BuffersSize);
	void  ConvertToUnicode(BYTE *szDest, BYTE *szOriginal, DWORD BufferSize);
	void  ConvertToUnicode(BYTE *szBuf, DWORD BufferSize);
	void  ConvertToUppercase(BYTE *szBuf, DWORD BufferSize);
	void  TranslateIllegalChars(BYTE *szBuf, DWORD BufferSize);

	void  ProcessMessages(HWND hWnd);
};

#endif
#endif //_CUTILS_H_
