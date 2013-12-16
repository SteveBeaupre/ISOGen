#ifndef _CISOMAKER_H_
#define _CISOMAKER_H_
#ifdef __cplusplus

#define WIN32_LEAN_AND_MEAN // Trim libraies.
#define VC_LEANMEAN         // Trim even further.

#include <Windows.h>
#include <Stdio.h>

#include "FoldersTree.h"

#define NO_UNICODE    0
#define USE_UNICODE   1
#define USE_LITTLEENDIAN  0
#define USE_BIGENDIAN     1

#define WM_GAUGE_UPDATE    WM_USER + 175

// Main Class
class CISOMaker {
public:
	CISOMaker();
	~CISOMaker();
private:
	HWND hMainWnd;
	BOOL bGaugePresent;
private:
	CUtils Utils;
private:
	FILE *OutF;
	bool IsOutputFileLoaded(){return OutF != NULL;}
	bool CreateOutputFile(char *fname);
	void CloseOutputFile();
	FILE *BootF;
	bool IsBootSectorFileLoaded(){return BootF != NULL;}
	bool LoadBootSectorFile(char *fname);
	void CloseBootSectorFile();
private:
	CFoldersClass FoldersTree;
	// Cache for a list of FoldersNode pointers for path tables
	vector<void*> FoldersNodes;
private:
	// Store the boot sector #
	DWORD BootSectorID;
	// Store the total number of sectors we have to write
	DWORD TotalSectorsCount;
	// Store the total number of sectors we have writen
	DWORD TotalSectorsWriten;
	
	// Store the path tables size and first sector #
	DWORD PathTableSize[4];
	DWORD PathTableFirstSector[4];
	
	// Store the directory records size and first sector #
	DWORD FirstDirectoryRecordSector[2];
	DWORD FilesFirstDirectoryRecordSector;

	// If used, will replace the auto-generated string
	char CustomVolumeIdentifier[32];
private:
	// Utility functions
	void PreCalcStuffs();
	void GenPathTableFoldersList();
	
	// Sectors writting functions
	void WriteFirst32k();
	void WriteVolumeDescriptorsSectors(bool IsBootable);
	void WritePathTablesSectors(DWORD Indx, BOOL UseUnicode, BOOL UseBigEndian);
	void WriteDirectorySector(CFolderNode *pNode, BOOL UseUnicode);
	void WriteFilesSector(CFolderNode *pNode);
	void WriteBootCatSector();
	void WriteBootSector();
	
	// Sectors and/or size calculation functions
	DWORD CalcPathTableSize(BOOL UseUnicode);
	DWORD CalcDirectoryRecordsSize(CFolderNode *pNode, BOOL UseUnicode, BOOL Recurse);
	DWORD CalcFilesRecordsSize(CFolderNode *pNode);
	
	// Number generators functions
	void GenDirectoryID();
	void GenFoldersRecordsSizeAndFirstSectorData(CFolderNode *pNode, DWORD *pFirstSector, BOOL UseUnicode);
	void GenFilesRecordsFirstSectorData(CFolderNode *pNode, DWORD *pFirstSector);
	
	// Windows Messages Functions
	void PostProgressBarMessage(DWORD *x, DWORD Total, DWORD IncrementBy = 0);
public: // Public functions
	// Reset everithing back to an "empty iso"
	void Reset();

	// Save the handles we need if we want to use the windows messages
	void SetWindowsHandles(HWND MainWnd, BOOL GaugePresent);

	// Overwrite the default volume identifier 
	void SetVolumeIdentifier(char *vid);

	// Add directory and files to the tree
	BOOL AddDirectory(char *pDirName);
	BOOL AddFile(char *fname);

	// Make the .iso file
	BOOL BuildISO(char *fname, char *bootfname);

	// Print the tree in console mode
	void PrintTree();

	// Return errors messages, and how many of them they are
	DWORD GetNumErrorsMsg();
	char* GetErrorsMsg(DWORD Indx);
};

#endif
#endif //_CISOMAKER_H_
