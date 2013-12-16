#include "Utils.h"

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////MATH FUNCTIONS////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Convert a little endian WORD into a big endian one
//-----------------------------------------------------------------------------
WORD CUtils::MakeBigEndian2b(WORD w)
{
	/*WORD Res = 0;
	BYTE *pBytes = (BYTE*)&Res;
	pBytes[1] = (BYTE)(w & 0x00FF);
	pBytes[0] = (BYTE)((w & 0xFF00) >> 8);*/

	WORD Res = 0;
	_asm {
		push eax;      // Save eax
		xor eax, eax;  // Clear eax

		mov ax,  w;    // Move the given word into ax
		ror ax,  8;    // Exchange the 2 bytes of ax
		mov Res, ax    // Move the content of ax into Res

		pop eax;       // Restore eax
	}
	
	return Res;
}

//-----------------------------------------------------------------------------
// Convert a little endian DWORD into a big endian one
//-----------------------------------------------------------------------------
DWORD CUtils::MakeBigEndian4b(DWORD dw)
{
	/*DWORD Res = 0;
	BYTE *pBytes = (BYTE*)&Res;
	pBytes[3] = (BYTE)(dw & 0x000000FF);
	pBytes[2] = (BYTE)((dw & 0x0000FF00) >> 8);
	pBytes[1] = (BYTE)((dw & 0x00FF0000) >> 16);
	pBytes[0] = (BYTE)((dw & 0xFF000000) >> 24);*/
	
	DWORD Res = 0;
	_asm {
		push eax;       // Save eax
		xor eax, eax;   // Clear eax

		mov eax, dw;    // Move the given dword into eax
		ror eax, 8      // Roll eax 1 byte right
		ror ax,  8      // Exchange the 2 bytes of ax
		ror eax, 16     // Roll eax 2 bytes right
		ror ax,  8      // Exchange the 2 bytes of ax
		ror eax, 8      // Roll eax 1 bytes right
		mov Res, eax    // Move the content of eax into Res

		pop eax;        // Restore eax
	}

	return Res;
}

//-----------------------------------------------------------------------------
// Tell if a number is odd or not
//-----------------------------------------------------------------------------
BOOL CUtils::IsOdd(DWORD x)
{
	return (x & 0x00000001) > 0;
}

//-----------------------------------------------------------------------------
// Return x+1 if x is odd
//-----------------------------------------------------------------------------
DWORD CUtils::MakeEven(DWORD x)
{
	DWORD Res = x;
	if(IsOdd(Res))
		Res++;

	return Res;
}

//-----------------------------------------------------------------------------
// Round the number up to the next 2048 bytes boundary
//-----------------------------------------------------------------------------
DWORD CUtils::MakeSectorEven(DWORD Size)
{
	switch(Size % SECTOR_SIZE)
	{
	case 0:  return Size;                                 
	default: return Size + (SECTOR_SIZE - (Size % SECTOR_SIZE));
	}
}

//-----------------------------------------------------------------------------
// Tell us how many sector a given number of bytes equal to...
//-----------------------------------------------------------------------------
DWORD CUtils::GetSectorTaken(DWORD Size)
{
	// Avoid a useless division by zero
	switch(Size)
	{
	case 0:  return 0;                                 
	default: return MakeSectorEven(Size) / SECTOR_SIZE;
	}
}

//-----------------------------------------------------------------------------
// Increment a pointer address by SECTOR_SIZE (2048)
//-----------------------------------------------------------------------------
void* CUtils::IncSectorPtr(void *_ptr)
{
	_asm{
		push eax;
		mov eax, _ptr;
		add eax, SECTOR_SIZE;
		mov _ptr, eax;	
		pop eax;
	}

	return _ptr;
}

//-----------------------------------------------------------------------------
// Get the size required for a Directory record or a path table entry
//-----------------------------------------------------------------------------
void CUtils::GetStructSizeByNodeName(DWORD *pSize, DWORD BaseSize, DWORD ShortNameLen, DWORD LongNameLen, BOOL IsSpecialDir, BOOL UseUnicode)
{
	if(IsSpecialDir){
		*pSize += MakeEven(BaseSize + 1);
	} else {
		*pSize += MakeEven(BaseSize + (!UseUnicode ? ShortNameLen : LongNameLen));	
	}
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////STRINGS FUNCTIONS///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Compare 2 buffers and return true if they match...
//-----------------------------------------------------------------------------
bool CUtils::CompareBuffers(BYTE *pBuf1, BYTE *pBuf2, DWORD BuffersSize)
{
	return memcmp(&pBuf1[0], &pBuf2[0], BuffersSize) == 0;
}

//-----------------------------------------------------------------------------
// Convert a normal string into an unicode one, using 2 buffers...
//-----------------------------------------------------------------------------
void CUtils::ConvertToUnicode(BYTE *szDest, BYTE *szOriginal, DWORD BufferSize)
{
	DWORD BufSizeDiv2 = BufferSize / 2;

	DWORD DestIndx = 0;
	for(DWORD Cpt = 0; Cpt < BufSizeDiv2; Cpt++){
		szDest[DestIndx] = 0;
		szDest[DestIndx+1] = szOriginal[Cpt];
		DestIndx += 2;
	}

	if(IsOdd(BufferSize))
		szDest[BufferSize-1] = 0;
}

//-----------------------------------------------------------------------------
// Convert a normal string into an unicode one, using the same buffer...
//-----------------------------------------------------------------------------
void CUtils::ConvertToUnicode(BYTE *szBuf, DWORD BufferSize)
{
	BYTE *pTmpBuf = new BYTE [BufferSize];

	DWORD BufSizeDiv2 = BufferSize / 2;

	DWORD DestIndx = 0;
	for(DWORD Cpt = 0; Cpt < BufSizeDiv2; Cpt++){
		pTmpBuf[DestIndx] = 0;
		pTmpBuf[DestIndx+1] = szBuf[Cpt];
		DestIndx += 2;
	}

	if(IsOdd(BufferSize))
		pTmpBuf[BufferSize-1] = 0;

	memcpy(szBuf, pTmpBuf, BufferSize);

	SAFE_DELETE_ARRAY(pTmpBuf);
}

//-----------------------------------------------------------------------------
// Convert all string characters to uppercase...
//-----------------------------------------------------------------------------
void CUtils::ConvertToUppercase(BYTE *szBuf, DWORD BufferSize)
{
	for(DWORD Cpt = 0; Cpt < BufferSize; Cpt++){
		char c = szBuf[Cpt];
		if(c >= 'a' && c <= 'z')
			szBuf[Cpt] -= 32;
	}
}

//-----------------------------------------------------------------------------
// Replace invalid characters by '_'
//-----------------------------------------------------------------------------
void CUtils::TranslateIllegalChars(BYTE *szBuf, DWORD BufferSize)
{
	for(DWORD Cpt = 0; Cpt < BufferSize; Cpt++){
		char c = szBuf[Cpt];

		if(!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == 0))
			szBuf[Cpt] = '_';

		if(c < 0x20)
			szBuf[Cpt] = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////WINDOWS FUNCTIONS//////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Make the given windows process it's pending messages, and remain responsive ...
//-----------------------------------------------------------------------------
void CUtils::ProcessMessages(HWND hWnd)
{
	MSG msg;
	while(PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE)){
	   TranslateMessage(&msg);
	   DispatchMessage(&msg);
	}
}

