#include "ISOMaker.h"

//-----------------------------------------------------------------------------
// Contructor...
//-----------------------------------------------------------------------------
CISOMaker::CISOMaker()
{
	OutF  = NULL;
	BootF = NULL;

	Reset();
}

//-----------------------------------------------------------------------------
// Destructor...
//-----------------------------------------------------------------------------
CISOMaker::~CISOMaker()
{
	CloseOutputFile();
	CloseBootSectorFile();

	FoldersTree.ClearNodes(NULL, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////DLL Functions/////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

void CISOMaker::Reset()
{
	CloseOutputFile();
	CloseBootSectorFile();

	hMainWnd      = NULL;
	bGaugePresent = FALSE;

	FoldersTree.ClearNodes(NULL, 0);
	FoldersTree.CreateRootDirectory();

	FoldersNodes.clear();

	BootSectorID       = 0;
	TotalSectorsCount  = 0;
	TotalSectorsWriten = 0;
	ZeroMemory(&PathTableSize[0],              sizeof(DWORD) * 4);
	ZeroMemory(&PathTableFirstSector[0],       sizeof(DWORD) * 4);
	ZeroMemory(&FirstDirectoryRecordSector[0], sizeof(DWORD) * 2);
	FilesFirstDirectoryRecordSector = 0;

	ZeroMemory(&CustomVolumeIdentifier[0], 32);
}

///////////////////////////////////////////////////////////////////////////////////////////

void CISOMaker::SetWindowsHandles(HWND MainWnd, BOOL GaugePresent)
{
	hMainWnd      = MainWnd;
	bGaugePresent = GaugePresent;
}

///////////////////////////////////////////////////////////////////////////////////////////

void CISOMaker::PostProgressBarMessage(DWORD *x, DWORD Total, DWORD IncrementBy)
{
	*x += IncrementBy;
	if(hMainWnd != NULL && bGaugePresent){
		PostMessage(hMainWnd, WM_GAUGE_UPDATE, *x, Total);
		Utils.ProcessMessages(hMainWnd);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////

void CISOMaker::SetVolumeIdentifier(char *VolID)
{
	ZeroMemory(&CustomVolumeIdentifier[0], 32);
	DWORD StrLen = strlen(VolID);
	if(StrLen > 16){StrLen = 16;}
	if(StrLen > 0){memcpy(&CustomVolumeIdentifier[0], VolID, StrLen);}
}

///////////////////////////////////////////////////////////////////////////////////////////

BOOL CISOMaker::AddDirectory(char *pDirName)
{
	return FoldersTree.AddDirectory(NULL, pDirName);
}

///////////////////////////////////////////////////////////////////////////////////////////

BOOL CISOMaker::AddFile(char *fname)
{
	return FoldersTree.AddFile(NULL, fname);
}

///////////////////////////////////////////////////////////////////////////////////////////

void CISOMaker::PrintTree()
{
	FoldersTree.PrintNodes(NULL, 0);
}


///////////////////////////////////////////////////////////////////////////////////////////

DWORD CISOMaker::GetNumErrorsMsg()
{
	return FoldersTree.ErrorsLog.GetNodeCount();
}

///////////////////////////////////////////////////////////////////////////////////////////

char* CISOMaker::GetErrorsMsg(DWORD Indx)
{
	return FoldersTree.ErrorsLog.GetErrorMsg(Indx);
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////DISK WRITE FUNCTIONS////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Write the .iso file
//-----------------------------------------------------------------------------
BOOL CISOMaker::BuildISO(char *fname, char *bootfname)
{
	// Don't go further if we have errors...
	if(FoldersTree.ErrorsLog.GetNumErrors() > 0)
		return FALSE;

	///////////////////////////////////////////////////////////////////////////////////////////
	
	// Open the boot sector file if a file name is given
	if(bootfname != NULL && !LoadBootSectorFile(bootfname)){
		FoldersTree.ErrorsLog.LogError(_ERROR_BOOT_FILE_LOAD_FAIL_, bootfname);
		return FALSE;
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	
	// Create the file
	if(!CreateOutputFile(fname)){
		FoldersTree.ErrorsLog.LogError(_ERROR_FILE_CREATION_FAIL_, fname);
		CloseBootSectorFile();
		return FALSE;
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////
	
	// Pre-Calculate some data before starting generating/writing this shit up...
	PreCalcStuffs();

	///////////////////////////////////////////////////////////////////////////////////////////
	
	// Write the boot sectors
	WriteFirst32k();

	///////////////////////////////////////////////////////////////////////////////////////////
	
	// Write the Volume Descriptors
	WriteVolumeDescriptorsSectors(IsBootSectorFileLoaded());

	///////////////////////////////////////////////////////////////////////////////////////////

	// Write the boot catalog sector
	if(IsBootSectorFileLoaded())
		WriteBootCatSector();
	
	///////////////////////////////////////////////////////////////////////////////////////////

	// Write the Path Tables
	WritePathTablesSectors(0, NO_UNICODE,  USE_LITTLEENDIAN);
	WritePathTablesSectors(1, NO_UNICODE,  USE_BIGENDIAN);
	WritePathTablesSectors(2, USE_UNICODE, USE_LITTLEENDIAN);
	WritePathTablesSectors(3, USE_UNICODE, USE_BIGENDIAN);

	///////////////////////////////////////////////////////////////////////////////////////////
	
	// Write the Directory Records
	WriteDirectorySector(NULL, NO_UNICODE);
	WriteDirectorySector(NULL, USE_UNICODE);

	///////////////////////////////////////////////////////////////////////////////////////////

	// Write the binary boot sector
	if(IsBootSectorFileLoaded())
		WriteBootSector();

	///////////////////////////////////////////////////////////////////////////////////////////

	// Write the Files Directory Records
	WriteFilesSector(NULL);

	///////////////////////////////////////////////////////////////////////////////////////////
	
	// Close the file
	CloseOutputFile();
	
	///////////////////////////////////////////////////////////////////////////////////////////
	
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Create the .iso file
//-----------------------------------------------------------------------------
bool CISOMaker::CreateOutputFile(char *fname)
{
	if(!IsOutputFileLoaded())
		OutF = fopen(fname, "wb+");
		
	return IsOutputFileLoaded();
}

//-----------------------------------------------------------------------------
// Close the .iso file
//-----------------------------------------------------------------------------
void CISOMaker::CloseOutputFile()
{
	if(IsOutputFileLoaded()){
		fclose(OutF);
		OutF = NULL;
	}
}

//-----------------------------------------------------------------------------
// Load the boot sector file
//-----------------------------------------------------------------------------
bool CISOMaker::LoadBootSectorFile(char *fname)
{
	if(!IsBootSectorFileLoaded())
		BootF = fopen(fname, "rb");
		
	return IsBootSectorFileLoaded();
}

//-----------------------------------------------------------------------------
// Close the boot sector file
//-----------------------------------------------------------------------------
void CISOMaker::CloseBootSectorFile()
{
	if(IsBootSectorFileLoaded()){
		fclose(BootF);
		BootF = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Write the first useless 16 sectors
//-----------------------------------------------------------------------------
void CISOMaker::WriteFirst32k()
{
	BYTE EmptyBuf[SECTOR_SIZE];
	ZeroMemory(&EmptyBuf[0], SECTOR_SIZE);

	for(int Cpt = 0; Cpt < 16; Cpt++){
		fwrite(&EmptyBuf[0], 1, SECTOR_SIZE, OutF);
		PostProgressBarMessage(&TotalSectorsWriten, TotalSectorsCount, 1);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Write the Volume Descriptors Sectors
//-----------------------------------------------------------------------------
void CISOMaker::WriteVolumeDescriptorsSectors(bool IsBootable)
{
	//const BYTE VDSet[3][7] = {{0x01, 0x43, 0x44, 0x30, 0x30, 0x31, 0x01}, {0x02, 0x43, 0x44, 0x30, 0x30, 0x31, 0x01}, {0xFF, 0x43, 0x44, 0x30, 0x30, 0x31, 0x01}};
	const BYTE VDSignature[6] = {0x43, 0x44, 0x30, 0x30, 0x31, 0x01};
	const BYTE DefSystemIdentifier[2][32] = {{0x57, 0x49, 0x4E, 0x33, 0x32, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20}, {0x00, 0x57, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x33, 0x00, 0x32, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20}};
	const LPCSTR szDefAppIdentifier = "VortezISO Maker - All fucking right reserved!";
	const LPCSTR szElTorinoIdentifier = "EL TORITO SPECIFICATION";

	/////////////////////////////////////////
	// Get the current time 
	/////////////////////////////////////////

	SYSTEMTIME RecTime;
	GetSystemTime(&RecTime);

	/////////////////////////////////////////
	// Generate the Primary Volume Descriptor 
	/////////////////////////////////////////

	// Create a pointer to our Primary Volume Descriptor
	CVolumeDescriptor PVD;
	ZeroMemory(&PVD, SECTOR_SIZE);

	//
	PVD.VolumeDescriptorSet[0] = 1;
	memcpy(&PVD.VolumeDescriptorSet[1], &VDSignature[0], 6);
	memcpy(&PVD.SystemIdentifier[0], &DefSystemIdentifier[0][0], 32);   
	
	// Create a volume identifier using the current time values
	{
		memset(&PVD.VolumeIdentifier[0], ' ', 32);

		if(strlen(&CustomVolumeIdentifier[0]) == 0){
			char Buf[32];
			ZeroMemory(&Buf[0], 32);
			sprintf(&Buf[0], "%d%2.2d%2.2d_%2.2d%2.2d%2.2d", RecTime.wYear, RecTime.wMonth, RecTime.wDay,  RecTime.wHour, RecTime.wMinute, RecTime.wSecond);
			memcpy(&PVD.VolumeIdentifier[0], &Buf[0], strlen(&Buf[0]));   
		} else {
			memcpy(&PVD.VolumeIdentifier[0], &CustomVolumeIdentifier[0], strlen(&CustomVolumeIdentifier[0]));   
		}
	}

	// Write the total number of sectors number
	PVD.TotalSectorsCount_LE = TotalSectorsCount;
	PVD.TotalSectorsCount_BE = Utils.MakeBigEndian4b(PVD.TotalSectorsCount_LE);

	// Write useless data...
	PVD.VolumeSetSize_LE = 1;
	PVD.VolumeSetSize_BE = Utils.MakeBigEndian2b(1);
	PVD.VolumeSequenceNumber_LE = 1;
	PVD.VolumeSequenceNumber_BE = Utils.MakeBigEndian2b(1);

	// Write the default sector size
	PVD.SectorSize_LE = SECTOR_SIZE;
	PVD.SectorSize_BE = Utils.MakeBigEndian2b(SECTOR_SIZE);

	// Write the "normal" path table length
	PVD.PathTableLength_LE = PathTableSize[0];
	PVD.PathTableLength_BE = Utils.MakeBigEndian4b(PVD.PathTableLength_LE);

	// Write the path tables 1st sectors
	PVD.FirstSectorOf1stPathTable_LE = PathTableFirstSector[0];
	PVD.FirstSectorOf2ndPathTable_LE = 0;
	PVD.FirstSectorOf1stPathTable_BE = Utils.MakeBigEndian4b(PathTableFirstSector[1]);
	PVD.FirstSectorOf2ndPathTable_BE = 0;

	// Write the root directory record
	{
		CDirectoryRecord DirRec;
		ZeroMemory(&DirRec, sizeof(CDirectoryRecord));

		DirRec.RecordSize = 34;
		
		DirRec.FirstSector_LE = FirstDirectoryRecordSector[0];
		DirRec.FirstSector_BE = Utils.MakeBigEndian4b(DirRec.FirstSector_LE);

		DirRec.FileSize_LE = SECTOR_SIZE;
		DirRec.FileSize_BE = Utils.MakeBigEndian4b(DirRec.FileSize_LE);

		DirRec.Flags = 2;

		DirRec._VolumeSequenceNumber_LE = 1;
		DirRec._VolumeSequenceNumber_BE = Utils.MakeBigEndian2b(DirRec._VolumeSequenceNumber_LE);

		DirRec.IdentifierLen = 1;

		memcpy(&PVD.RootDirectoryRecord[0], &DirRec, 34);
	}

	// Write useless data...
	memset(&PVD.VolumeSetIdentifier[0], ' ', 128);
	memset(&PVD.PublishedIdentifier[0], ' ', 128);
	memset(&PVD.DataPreparerIdentifier[0], ' ', 128);

	// Write the application identifier
	{
		memset(&PVD.ApplicationIdentifier[0], ' ', 128);
		memcpy(&PVD.ApplicationIdentifier[0], &szDefAppIdentifier[0], strlen(&szDefAppIdentifier[0]));
	}

	// Write more useless data...
	memset(&PVD.CopyrightFileIdentifier[0], ' ', 37);
	memset(&PVD.AbstractFileIdentifier[0], ' ', 37);
	memset(&PVD.BibliographicalFile[0], ' ', 37);
	
  	// Write the volume creation time, wich is right now!
	{
		char Buf[17];
		ZeroMemory(&Buf[0], 17);
		
		sprintf(&Buf[0], "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d%2.2d%c", RecTime.wYear, RecTime.wMonth, RecTime.wDay,  RecTime.wHour, RecTime.wMinute, RecTime.wSecond, RecTime.wMilliseconds, 0xF0);
		memcpy(&PVD.VolumeCreationTime[0], &Buf[0], 17);   
		memcpy(&PVD.LastVolumeModificationTime[0], &Buf[0], 17);   
	}
		
	// Write even more useless data...
    memset(&PVD.VolumeExpirationTime[0], '0', 16);
    memset(&PVD.VolumeEffectiveTime[0], '0', 16);

	// This padding byte must be 1
	PVD._PadBytes4 = 1;


	// Write the App. Reseverved data string
	{
		// Default UltraISO string
		/*BYTE UltraIsoDefAppData[44] = {0x55, 0x4C, 0x54, 0x52, 0x41, 0x49, 0x53, 0x4F, 0x00, 0x38, 0x2E, 0x36, 0x2E, 0x31, 0x2E, 0x31, 0x39, 0x38, 0x35, 0x00, 0xC8, 0x14, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

		memcpy(&PVD.AppReserved[0], &UltraIsoDefAppData[0], 44);
		memset(&PVD.AppReserved[44], ' ', 212);
		PVD.AppReserved[256] = 0x2F;
		memset(&PVD.AppReserved[257], 0, 255);*/

		// My custom string
		BYTE DefAppData[14] = {0x56, 0x6F, 0x72, 0x74, 0x65, 0x7A, 0x49, 0x53, 0x4F, 0x20, 0x76, 0x31, 0x2E, 0x30};
		memset(&PVD.AppReserved[0], 0x20, 512);
		memcpy(&PVD.AppReserved[0], &DefAppData[0], 14);
	}

	/////////////////////////////////////////
	// Generate the Boot Volume Descriptor 
	/////////////////////////////////////////
	// Copy the first one over the 2nd one
	CBootVolumeDescriptor BVD;
	if(IsBootable){
		ZeroMemory(&BVD, SECTOR_SIZE);
		
		memcpy(&BVD.VolumeDescriptorSet[1], &VDSignature[0], 6);
		memcpy(&BVD.BootSystemIdentifier[0], &szElTorinoIdentifier[0], strlen(szElTorinoIdentifier));
		BVD.BootCatSector = 20;
	}

	/////////////////////////////////////////
	// Generate the Secondary Volume Descriptor 
	/////////////////////////////////////////
	// Copy the first one over the 2nd one
	CVolumeDescriptor SVD;
	memcpy(&SVD, &PVD, SECTOR_SIZE);	

	SVD.VolumeDescriptorSet[0] = 2;
	memcpy(&SVD.VolumeDescriptorSet[1], &VDSignature[0], 6);

	memcpy(&SVD.SystemIdentifier[0], &DefSystemIdentifier[1][0], 32);   
	Utils.ConvertToUnicode(&SVD.VolumeIdentifier[0], &PVD.VolumeIdentifier[0], 32);

	// Add the "Joliet" signature to the 2nd volume descriptor
	BYTE JolietBytes[3] = {0x25, 0x2F, 0x45};
	memcpy((BYTE*)&SVD._PadBytes3[0], &JolietBytes[0], 3);

	// Update the path table length
	SVD.PathTableLength_LE = PathTableSize[2];
	SVD.PathTableLength_BE = Utils.MakeBigEndian4b(SVD.PathTableLength_LE);

	// Update the path table 1st sectors
	SVD.FirstSectorOf1stPathTable_LE = PathTableFirstSector[2];
	SVD.FirstSectorOf2ndPathTable_LE = 0;
	SVD.FirstSectorOf1stPathTable_BE = Utils.MakeBigEndian4b(PathTableFirstSector[3]);
	SVD.FirstSectorOf2ndPathTable_BE = 0;

	// Change the first directory record
	BYTE *pRootTable1stDirectoryRecordSector = (BYTE*)&SVD;
	DWORD FirstDirSector = FirstDirectoryRecordSector[1];
	memcpy(&pRootTable1stDirectoryRecordSector[158], &FirstDirSector, 4); 
	FirstDirSector = Utils.MakeBigEndian4b(FirstDirectoryRecordSector[1]);
	memcpy(&pRootTable1stDirectoryRecordSector[162], &FirstDirSector, 4); 

	// Convert all normal text to unicode where needed
	Utils.ConvertToUnicode(&SVD.VolumeSetIdentifier[0], &PVD.VolumeSetIdentifier[0], 128);
	Utils.ConvertToUnicode(&SVD.PublishedIdentifier[0], &PVD.PublishedIdentifier[0], 128);
	Utils.ConvertToUnicode(&SVD.DataPreparerIdentifier[0], &PVD.DataPreparerIdentifier[0], 128);
	Utils.ConvertToUnicode(&SVD.ApplicationIdentifier[0], &PVD.ApplicationIdentifier[0], 128);
	Utils.ConvertToUppercase((BYTE*)&PVD.ApplicationIdentifier[0], 128);
	
	Utils.ConvertToUnicode(&SVD.CopyrightFileIdentifier[0], &PVD.CopyrightFileIdentifier[0], 37);
	Utils.ConvertToUnicode(&SVD.AbstractFileIdentifier[0], &PVD.AbstractFileIdentifier[0], 37);
	Utils.ConvertToUnicode(&SVD.BibliographicalFile[0], &PVD.BibliographicalFile[0], 37);

	/////////////////////////////////////////
	// Generate the Final Volume Descriptor 
	/////////////////////////////////////////
	// Only ad the VDSet Bytes
	CVolumeDescriptor FVD;
	ZeroMemory(&FVD, SECTOR_SIZE);

	FVD.VolumeDescriptorSet[0] = 0xFF;
	memcpy(&FVD.VolumeDescriptorSet[1], &VDSignature[0], 6);

	/////////////////////////////////////////
	// Write the Volumes Descriptors
	/////////////////////////////////////////

	// Write the Volume descriptors sectors
	fwrite(&PVD, 1, SECTOR_SIZE, OutF); PostProgressBarMessage(&TotalSectorsWriten, TotalSectorsCount, 1);
	if(IsBootable){
		fwrite(&BVD, 1, SECTOR_SIZE, OutF); 
		PostProgressBarMessage(&TotalSectorsWriten, TotalSectorsCount, 1);
	}
	fwrite(&SVD, 1, SECTOR_SIZE, OutF); PostProgressBarMessage(&TotalSectorsWriten, TotalSectorsCount, 1);
	fwrite(&FVD, 1, SECTOR_SIZE, OutF); PostProgressBarMessage(&TotalSectorsWriten, TotalSectorsCount, 1);	
}

///////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Write the Path Tables Sectors
//-----------------------------------------------------------------------------
void CISOMaker::WritePathTablesSectors(DWORD Indx, BOOL UseUnicode, BOOL UseBigEndian)
{
	DWORD NodeIndx = 0;
	DWORD DataIndx = 0;

	DWORD BufLen = Utils.MakeSectorEven(PathTableSize[Indx]);
	BYTE *pBuffer = new BYTE[BufLen];
	ZeroMemory(&pBuffer[0], BufLen);

	CFolderNode* pNode = NULL;

	while(1){

		// Go to the next node from the node vector and repeat the process
		if(NodeIndx < FoldersNodes.size()){
			pNode = (CFolderNode*)FoldersNodes[NodeIndx];
			NodeIndx++;
		} else 
			break; // Break the loop if we have processed all the nodes

		//
		CPathTableRecord PathTableRecord;
		ZeroMemory(&PathTableRecord, sizeof(PathTableRecord));

		PathTableRecord.NameLen = !UseUnicode ? pNode->ShortNameLen : pNode->LongNameLen;
		PathTableRecord.FirstSector = !UseBigEndian ? pNode->FirstSector.dw[UseUnicode] : Utils.MakeBigEndian4b(pNode->FirstSector.dw[UseUnicode]);
		PathTableRecord.ParentDirectoryID = !UseBigEndian ? pNode->DirectoryID : Utils.MakeBigEndian2b(pNode->DirectoryID);

		//
		switch(pNode->IsSpecialDirectory){
		case _NORMAL_DIR_:
			if(!UseUnicode){
				PathTableRecord.NameLen = pNode->ShortNameLen;
				memcpy(&PathTableRecord.Name[0], &pNode->ShortName[0], PathTableRecord.NameLen);			
			} else {
				PathTableRecord.NameLen = pNode->LongNameLen;
				memcpy(&PathTableRecord.Name[0], &pNode->LongName[0], PathTableRecord.NameLen);			
			}	
			break;

		case _THIS_DIR_:
			PathTableRecord.NameLen = 1;
			break;
		case _PREV_DIR_:
			PathTableRecord.NameLen = 1;
			PathTableRecord.Name[0] = 1;
			break;
		}

		//
		DWORD Size = 0;		
		Utils.GetStructSizeByNodeName(&Size, 8, pNode->ShortNameLen, pNode->LongNameLen, pNode->IsSpecialDirectory, UseUnicode);
		
		//
		memcpy(&pBuffer[DataIndx], &PathTableRecord, Size);

		DataIndx += Size;

		// Just in case...
		if(DataIndx >= PathTableSize[Indx])
			break;
	}

	//
	fwrite(&pBuffer[0], 1, BufLen, OutF);
	PostProgressBarMessage(&TotalSectorsWriten, TotalSectorsCount, Utils.GetSectorTaken(PathTableSize[Indx]));

	SAFE_DELETE_ARRAY(pBuffer);
}

///////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Generate the Directory Records for each folders, then each files...
//-----------------------------------------------------------------------------
void CISOMaker::WriteDirectorySector(CFolderNode *pNode, BOOL UseUnicode)
{
	// Select the root node if pNode is NULL
	if(pNode == NULL)
		pNode = FoldersTree.GetRootNode();

	// Save the current node
	CFolderNode *pOriginalNode = pNode;

	DWORD BufLen = Utils.MakeSectorEven(pNode->Size.dw[UseUnicode]);
	BYTE *pBuffer = new BYTE[BufLen];
	ZeroMemory(&pBuffer[0], BufLen);

	// Fill up a CDirectoryRecord
	DWORD DataIndx = 0;

	//Fill it with data
	pNode = FoldersTree.GetFirstChild(pNode);
	while(pNode != NULL){

		CDirectoryRecord DirRec;
		ZeroMemory(&DirRec, sizeof(DirRec));
		
		DirRec.RecordSize = (BYTE)Utils.MakeEven(33 + (!UseUnicode ? pNode->ShortNameLen : pNode->LongNameLen));
		
		DirRec.FirstSector_LE = pNode->FirstSector.dw[UseUnicode];
		DirRec.FirstSector_BE = Utils.MakeBigEndian4b(DirRec.FirstSector_LE);

		DirRec.FileSize_LE = Utils.MakeSectorEven(pNode->Size.dw[UseUnicode]);
		DirRec.FileSize_BE = Utils.MakeBigEndian4b(DirRec.FileSize_LE);
		
		// Get the current time and write it into the directory record Time[7] bytes
		{
			SYSTEMTIME RecTime;
			GetSystemTime(&RecTime);
			// Year, Month, Day, Hour, Min, Sec
			DirRec.Time[0] = (BYTE)(RecTime.wYear - 1900);
			DirRec.Time[1] = (BYTE)RecTime.wMonth;
			DirRec.Time[2] = (BYTE)RecTime.wDay;
			DirRec.Time[3] = (BYTE)RecTime.wHour;
			DirRec.Time[4] = (BYTE)RecTime.wMinute;
			DirRec.Time[5] = (BYTE)RecTime.wSecond;
			
			TIME_ZONE_INFORMATION tzi;
			GetTimeZoneInformation(&tzi);
			// Offset from Greenwich Mean Time, in 15-minute intervals
			DirRec.Time[6] = (BYTE)((tzi.Bias / 15) * (-1));
		}
		
		// Flags
		if(pNode->IsDirectory)
			DirRec.Flags = 0x02;

		// Useless...
		DirRec._VolumeSequenceNumber_LE = 1;
		DirRec._VolumeSequenceNumber_BE = Utils.MakeBigEndian2b(1);

		// Write the identifier
		switch(pNode->IsSpecialDirectory){
		case _NORMAL_DIR_:
			if(!UseUnicode){
				DirRec.IdentifierLen = pNode->ShortNameLen;
				memcpy(&DirRec.Identifier[0], &pNode->ShortName[0], DirRec.IdentifierLen);			
			} else {
				DirRec.IdentifierLen = pNode->LongNameLen;
				memcpy(&DirRec.Identifier[0], &pNode->LongName[0], DirRec.IdentifierLen);			
			}	
			break;

		case _THIS_DIR_:
			DirRec.IdentifierLen = 1;
			break;
		case _PREV_DIR_:
			DirRec.IdentifierLen = 1;
			DirRec.Identifier[0] = 1;
			break;
		}

		/////////////////////////////////////////////////////////////////////////

		// Copy the directory record chunk
		memcpy(&pBuffer[DataIndx], &DirRec, DirRec.RecordSize);
		DataIndx += DirRec.RecordSize;

		/////////////////////////////////////////////////////////////////////////

		// Go to the next node
		pNode = pNode->pNext;
	}

	// Write the entire directory records
	fwrite(&pBuffer[0], 1, BufLen, OutF);
	PostProgressBarMessage(&TotalSectorsWriten, TotalSectorsCount, Utils.GetSectorTaken(BufLen));

	// Recurse through sub-directory
	pNode = FoldersTree.GetFirstChild(pOriginalNode);
	while(pNode != NULL){
		if(FoldersTree.GetFirstChild(pNode)){
			if(pNode->IsDirectory == TRUE && pNode->IsSpecialDirectory == FALSE)
				WriteDirectorySector(pNode, UseUnicode);
		}
		pNode = pNode->pNext;
	}

	pNode = pOriginalNode;
}

///////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Generate the Directory Records for each folders, then each files...
//-----------------------------------------------------------------------------
void CISOMaker::WriteFilesSector(CFolderNode *pNode)
{
	// Select the root node if pNode is NULL
	if(pNode == NULL)
		pNode = FoldersTree.GetRootNode();

	// Save the current node
	CFolderNode *pOriginalNode = pNode;

	// Get the first child of this node
	pNode = FoldersTree.GetFirstChild(pNode);

	// Calculate the size
	while(pNode != NULL){

		DWORD FSize = 0;
		if(pNode->IsDirectory == FALSE){

			// Save the file size
			FSize = Utils.MakeSectorEven(pNode->Size._32.Lo);

			// Allocate a "Sector" memory block
			BYTE *pBuffer = new BYTE[SECTOR_SIZE];

			// Open the target file
			FILE *InF = fopen(&pNode->FullFileName[0], "rb");

			// Write the file data, sector by sector
			for(DWORD Cpt = 0; Cpt < FSize; Cpt += SECTOR_SIZE){
				ZeroMemory(&pBuffer[0], SECTOR_SIZE);
				fread(&pBuffer[0], 1, SECTOR_SIZE, InF);
				fwrite(&pBuffer[0], 1, SECTOR_SIZE, OutF);
				PostProgressBarMessage(&TotalSectorsWriten, TotalSectorsCount, 1);
			}

			// Close the file
			fclose(InF);

			// Free the memory buffer
			SAFE_DELETE_ARRAY(pBuffer);

		} else if(pNode->IsDirectory == TRUE && pNode->IsSpecialDirectory == FALSE){
			// Recurse through the childs nodes
			WriteFilesSector(pNode);
		}

		// Go to the next node
		pNode = pNode->pNext;
	}

	pNode = pOriginalNode;
}

//-----------------------------------------------------------------------------
// Write the Boot Catalog Sectors
//-----------------------------------------------------------------------------
void CISOMaker::WriteBootCatSector()
{
	static const BYTE BootSig[4] = {0xAA, 0x55, 0x55, 0xAA};

	CBootCatalog BootCat;
	ZeroMemory(&BootCat, sizeof(CBootCatalog));
	BYTE *pByte = (BYTE*)&BootCat.ValidationEntry.Checksum;

	BootCat.ValidationEntry.HeaderID = 1;
	memcpy(&pByte[0], &BootSig[0], 4);

	BootCat.DefaultEntry.BootIndicator = 0x88;
	BootCat.DefaultEntry.BootMediaType = 0;
	BootCat.DefaultEntry.LoadSegment   = 0x07C0;
	BootCat.DefaultEntry.SectorCount   = 4;
	BootCat.DefaultEntry.LoadRBA       = BootSectorID;

	fwrite(&BootCat, 1, SECTOR_SIZE, OutF);
	PostProgressBarMessage(&TotalSectorsWriten, TotalSectorsCount, 1);
}

//-----------------------------------------------------------------------------
// Write the Boot Sector
//-----------------------------------------------------------------------------
void CISOMaker::WriteBootSector()
{
	if(IsBootSectorFileLoaded()){
		BYTE BootImgBuf[SECTOR_SIZE];
		ZeroMemory(&BootImgBuf[0], SECTOR_SIZE);

		// Read the boot sector 
		DWORD NumBytesRead = fread(&BootImgBuf[0], 1, SECTOR_SIZE, BootF);

		// Make sure we've read 2048 bytes or warn the user
		if(NumBytesRead != SECTOR_SIZE)
			FoldersTree.ErrorsLog.LogError(_WARNING_BOOT_FILE_TOO_SMALL_, NULL);

		// Write the boot sector
		fwrite(&BootImgBuf[0], 1, SECTOR_SIZE, OutF);
		PostProgressBarMessage(&TotalSectorsWriten, TotalSectorsCount, 1);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////PRE-CALCULATE STUFFS///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Calculate all the stuffs we need to know before we can generate the .iso
//-----------------------------------------------------------------------------
void CISOMaker::PreCalcStuffs()
{
	// Reset the progress bar 
	PostProgressBarMessage(&TotalSectorsWriten, 1);

	// Add 2 sector if the iso is bootable
	DWORD SectorUsed = !IsBootSectorFileLoaded() ? 19 : 21;

	// Generate a list of all folders
	GenPathTableFoldersList();

	// Calculate the size for each Path Table
	PathTableSize[0] = CalcPathTableSize(NO_UNICODE);
	PathTableSize[1] = PathTableSize[0];
	PathTableSize[2] = CalcPathTableSize(USE_UNICODE);
	PathTableSize[3] = PathTableSize[2];

	PathTableFirstSector[0] = SectorUsed;
	PathTableFirstSector[1] = PathTableFirstSector[0] + Utils.GetSectorTaken(PathTableSize[0]);
	PathTableFirstSector[2] = PathTableFirstSector[1] + Utils.GetSectorTaken(PathTableSize[1]);
	PathTableFirstSector[3] = PathTableFirstSector[2] + Utils.GetSectorTaken(PathTableSize[2]);

	// Calculate the size taken by the 4 path tables
	DWORD TotalPathTableSectorsCount = (PathTableFirstSector[3] + Utils.GetSectorTaken(PathTableSize[3])) - PathTableFirstSector[0];
	SectorUsed += TotalPathTableSectorsCount;

	// Calculate the size taken by the directory records
	FirstDirectoryRecordSector[0] = SectorUsed;
	SectorUsed += Utils.GetSectorTaken(CalcDirectoryRecordsSize(NULL, NO_UNICODE, TRUE));
	FirstDirectoryRecordSector[1] = SectorUsed;
	SectorUsed += Utils.GetSectorTaken(CalcDirectoryRecordsSize(NULL, USE_UNICODE, TRUE));

	// Add 1 sector if the iso is bootable
	if(IsBootSectorFileLoaded()){
		BootSectorID = SectorUsed;
		SectorUsed++;
	}

	// Calculate the sectors taken by files 
	FilesFirstDirectoryRecordSector = SectorUsed;
	SectorUsed += Utils.GetSectorTaken(CalcFilesRecordsSize(NULL));
	
	// Here we have the total number of sectors our iso will need
	TotalSectorsCount = SectorUsed;
	
	/////////////////////////////////////////////////////////////////////////////////

	// Generate the directory ID required for the path tables
	GenDirectoryID();
	// Hold the first sector # for the functions above
	DWORD CurrentSector = FirstDirectoryRecordSector[0];
	
	// Generate the size and first sector # for each valid directory nodes
	GenFoldersRecordsSizeAndFirstSectorData(NULL, &CurrentSector, NO_UNICODE);
	GenFoldersRecordsSizeAndFirstSectorData(NULL, &CurrentSector, USE_UNICODE);
	
	// Allocate 1 sector here for the boot sector
	if(IsBootSectorFileLoaded())
		CurrentSector++;

	// Generate the size and first sector # for each valid files nodes
	GenFilesRecordsFirstSectorData(NULL, &CurrentSector);
}

//-----------------------------------------------------------------------------
// Generate the Path Table size
//-----------------------------------------------------------------------------
void CISOMaker::GenPathTableFoldersList()
{
	CFolderNode* pNode = FoldersTree.GetRootNode();

	DWORD NodeIndx = 0;
	FoldersNodes.clear();

	FoldersNodes.push_back((void*)pNode);
	NodeIndx++;
	
	pNode = FoldersTree.GetFirstChild(pNode);

	// Store all the FoldersTree directory nodes by level and order
	while(1){

		// Check all the parent's node childs
		while(pNode != NULL){

			if(pNode->IsDirectory == TRUE && pNode->IsSpecialDirectory == FALSE){				
				FoldersNodes.push_back((void*)pNode);
			}
			
			// Go to the next node
			pNode = pNode->pNext;
		}

		// Go to the next node from the node vector and repeat the process
		if(NodeIndx < FoldersNodes.size()){
			pNode = (CFolderNode*)FoldersNodes[NodeIndx];
			NodeIndx++;
			pNode = FoldersTree.GetFirstChild(pNode);
		} else 
			break;
	}
}

//-----------------------------------------------------------------------------
// Generate the Path Table size
//-----------------------------------------------------------------------------
DWORD CISOMaker::CalcPathTableSize(BOOL UseUnicode)
{
	CFolderNode* pNode = NULL;

	DWORD PathTableSize = 0;
	
	for(DWORD Cpt = 0; Cpt < FoldersNodes.size(); Cpt++){
		pNode = (CFolderNode*)FoldersNodes[Cpt];
		Utils.GetStructSizeByNodeName(&PathTableSize, 8, pNode->ShortNameLen, pNode->LongNameLen, pNode->IsSpecialDirectory, UseUnicode);
	}

	return PathTableSize;
}

///////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Generate the Directory Record size
//-----------------------------------------------------------------------------
DWORD CISOMaker::CalcDirectoryRecordsSize(CFolderNode *pNode, BOOL UseUnicode, BOOL Recurse)
{
	// Select the root node if pNode is NULL
	if(pNode == NULL)
		pNode = FoldersTree.GetRootNode();

	// Save the current node
	CFolderNode *pOriginalNode = pNode;

	// Init. the size taken variable
	DWORD Size = 0;
	DWORD RecursionResult = 0;
	// Get the first child of this node
	pNode = FoldersTree.GetFirstChild(pNode);

	// Calculate the size
	while(pNode != NULL){
		// Accumulate the size taken
		Utils.GetStructSizeByNodeName(&Size, 33, pNode->ShortNameLen, pNode->LongNameLen, pNode->IsSpecialDirectory, UseUnicode);
		// Recurse through the childs nodes
		if(Recurse)
			RecursionResult += Utils.MakeSectorEven(CalcDirectoryRecordsSize(pNode, UseUnicode, Recurse));
		// Go to the next node
		pNode = pNode->pNext;
	}

	// Restore the node
	pNode = pOriginalNode;

	return Utils.MakeSectorEven(Size) + Utils.MakeSectorEven(RecursionResult);
}

//-----------------------------------------------------------------------------
// Generate the Directory Record size
//-----------------------------------------------------------------------------
DWORD CISOMaker::CalcFilesRecordsSize(CFolderNode *pNode)
{
	// Select the root node if pNode is NULL
	if(pNode == NULL)
		pNode = FoldersTree.GetRootNode();

	// Save the current node
	CFolderNode *pOriginalNode = pNode;

	// Init. the size taken variable
	DWORD Size = 0;
	DWORD RecursionResult = 0;
	// Get the first child of this node
	pNode = FoldersTree.GetFirstChild(pNode);

	// Calculate the size
	while(pNode != NULL){

		if(pNode->IsDirectory == FALSE){
			Size += Utils.MakeSectorEven(pNode->Size._32.Lo);
		} else if(pNode->IsDirectory == TRUE && pNode->IsSpecialDirectory == FALSE){
			// Recurse through the childs nodes
			RecursionResult += CalcFilesRecordsSize(pNode);
		}

		// Go to the next node
		pNode = pNode->pNext;
	}

	// Restore the node
	pNode = pOriginalNode;

	return Size + Utils.MakeSectorEven(RecursionResult);
}

//-----------------------------------------------------------------------------
// Generate the directory sector for the given node
//-----------------------------------------------------------------------------
void CISOMaker::GenDirectoryID()
{
	vector<void*> DirNodes;

	CFolderNode* pNode = FoldersTree.GetRootNode();

	WORD DirID = 1;
	DWORD NodeIndx = 0;

	DirNodes.clear();
	DirNodes.push_back((void*)pNode);
	pNode->DirectoryID = DirID;
	NodeIndx++;
	
	pNode = FoldersTree.GetFirstChild(pNode);

	// Store all the FoldersTree directory nodes by level and order
	while(1){

		// Check all the parent's node childs
		while(pNode != NULL){

			if(pNode->IsDirectory == TRUE && pNode->IsSpecialDirectory == FALSE){				
				DirNodes.push_back((void*)pNode);
				pNode->DirectoryID = DirID;
			}
			
			// Go to the next node
			pNode = pNode->pNext;
		}

		DirID++;

		// Go to the next node from the node vector and repeat the process
		if(NodeIndx < DirNodes.size()){
			pNode = (CFolderNode*)DirNodes[NodeIndx];
			NodeIndx++;
			pNode = FoldersTree.GetFirstChild(pNode);
		} else 
			break;
	}

	DirNodes.clear();
}

//-----------------------------------------------------------------------------
// Generate the directory sector for the given node
//-----------------------------------------------------------------------------
void CISOMaker::GenFoldersRecordsSizeAndFirstSectorData(CFolderNode *pNode, DWORD *pFirstSector, BOOL UseUnicode)
{
	// Select the root node if pNode is NULL
	if(pNode == NULL)
		pNode = FoldersTree.GetRootNode();

	// Save the current node
	CFolderNode *pOriginalNode = pNode;

	// Calculate the size
	DWORD Size = CalcDirectoryRecordsSize(pNode, UseUnicode, FALSE);

	// Add the size and First Sector info
	pNode->FirstSector.dw[UseUnicode] = *pFirstSector;
	pNode->Size.dw[UseUnicode]        = Utils.MakeSectorEven(Size);
	pNode->SectorUsed.dw[UseUnicode]  = Utils.GetSectorTaken(Size);

	//
	*pFirstSector += Utils.GetSectorTaken(Size);

	//
	pNode = FoldersTree.GetFirstChild(pOriginalNode);

	while(pNode != NULL){
		
		if(pNode->IsSpecialDirectory){
			if(pNode->IsSpecialDirectory == _THIS_DIR_){
				pNode->FirstSector.dw[UseUnicode] = *pFirstSector -  Utils.GetSectorTaken(Size);			
				pNode->Size.dw[UseUnicode] = Utils.MakeSectorEven(Size);
				pNode->SectorUsed.dw[UseUnicode] = Utils.GetSectorTaken(Size);
			} else if(pNode->IsSpecialDirectory == _PREV_DIR_){
				if(pNode->pParent->pParent != NULL){
					pNode->FirstSector.dw[UseUnicode] = pNode->pParent->pParent->FirstSector.dw[UseUnicode];			
					pNode->Size.dw[UseUnicode] = pNode->pParent->pParent->Size.dw[UseUnicode];
					pNode->SectorUsed.dw[UseUnicode] = pNode->pParent->pParent->SectorUsed.dw[UseUnicode];
				} else {
					pNode->FirstSector.dw[UseUnicode] = *pFirstSector -  Utils.GetSectorTaken(Size);	
					pNode->Size.dw[UseUnicode] = Utils.MakeSectorEven(Size);
					pNode->SectorUsed.dw[UseUnicode] = Utils.GetSectorTaken(Size);
				}
			}
		}


		// Recurse through sub-directory
		if(FoldersTree.GetFirstChild(pNode)){
			if(pNode->IsDirectory == TRUE && pNode->IsSpecialDirectory == FALSE)
				GenFoldersRecordsSizeAndFirstSectorData(pNode, pFirstSector, UseUnicode);
		}

		// Go to the next node
		pNode = pNode->pNext;
	}

	pNode = pOriginalNode;
}

//-----------------------------------------------------------------------------
// Generate the files sector for the given node
//-----------------------------------------------------------------------------
void CISOMaker::GenFilesRecordsFirstSectorData(CFolderNode *pNode, DWORD *pFirstSector)
{
	// Select the root node if pNode is NULL
	if(pNode == NULL)
		pNode = FoldersTree.GetRootNode();

	// Save the current node
	CFolderNode *pOriginalNode = pNode;

	// Get the first child of this node
	pNode = FoldersTree.GetFirstChild(pNode);

	// Calculate the size
	while(pNode != NULL){

		if(pNode->IsDirectory == FALSE){
			pNode->FirstSector._32.Lo = *pFirstSector;
			*pFirstSector += Utils.GetSectorTaken(pNode->Size._32.Lo);
		}

		// Go to the next node
		pNode = pNode->pNext;
	}

	pNode = FoldersTree.GetFirstChild(pOriginalNode);
	while(pNode != NULL){

		if(pNode->IsDirectory == TRUE && pNode->IsSpecialDirectory == FALSE)
			GenFilesRecordsFirstSectorData(pNode, pFirstSector);

		// Go to the next node
		pNode = pNode->pNext;
	}

	// Restore the node
	pNode = pOriginalNode;
}


