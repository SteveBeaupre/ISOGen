#ifndef _CFOLDERSTREE_H_
#define _CFOLDERSTREE_H_
#ifdef __cplusplus

#define WIN32_LEAN_AND_MEAN // Trim libraies.
#define VC_LEANMEAN         // Trim even further.

#include "Windows.h"
#include "Stdio.h"
#include "SafeKill.h"

#include "ErrorsLogger.h"
#include "Utils.h"

#include <Vector>
using namespace std;


#define ROOT_NODE_INDEX  0
#define MAX_PATH_LEN   256

#define _NORMAL_FILE_  0
#define _NORMAL_DIR_   _NORMAL_FILE_
#define _THIS_DIR_     1
#define _PREV_DIR_     2

union UNIONDAT {
	LONGLONG x;
	struct {
		DWORD Hi; 
		DWORD Lo;
	} _32;
	DWORD dw[2];
};


struct CFolderNode {

	// Store the depth of the nodes
	DWORD Level;

	///////////////////////////////////////

	// File/Folder/SpecialDir flags
	BOOL  IsDirectory;
	DWORD IsSpecialDirectory;

	///////////////////////////////////////

	// Used for path tables
	WORD DirectoryID;

	///////////////////////////////////////

	// Size/Sectors stuffs
	UNIONDAT FirstSector;
	UNIONDAT SectorUsed;
    UNIONDAT Size;

	///////////////////////////////////////

	// Names length
	BYTE NameLen;
	BYTE ShortNameLen;
	BYTE LongNameLen;

	// Names Buffers
	char Name[MAX_PATH_LEN];
	char FullFileName[MAX_PATH_LEN];
	char ShortName[32];
	char LongName[224];
	
	///////////////////////////////////////

	// Parent/Child Pointers
	struct CFolderNode *pParent;
	struct CFolderNode *pFirstChild;
	// Next/Prev Pointers
	struct CFolderNode *pPrev;
	struct CFolderNode *pNext;
};

//----------------------------------------------------------------------//
// The base class used to control the custom data stucture
//----------------------------------------------------------------------//
class CFoldersClass {
public:
	CFoldersClass();
	~CFoldersClass();
private:
	CUtils Utils;
public:
	CErrorsLogger ErrorsLog;
private:
	CFolderNode RootNode;
	DWORD NumFolders;
private:
	void InitNode(CFolderNode* pNode, CFolderNode* pParentNode, CFolderNode* pPrevNode, CFolderNode* pNextNode, DWORD Level, BOOL IsDir, DWORD IsSpecialDir, char *pName);
	void InitNode(CFolderNode* pNode, CFolderNode* pParentNode, CFolderNode* pPrevNode, CFolderNode* pNextNode, DWORD Level, WIN32_FIND_DATA *fdata, char *dir);
	void SetNodePointers(CFolderNode* pNode, CFolderNode* ParentNode, CFolderNode* FirstChild, CFolderNode* Prev, CFolderNode* Next); 
public:
	CFolderNode* GetRootNode();

	CFolderNode* GetFirstChild(CFolderNode *pNode);
	CFolderNode* GetLastChild(CFolderNode *pNode);
private:
	void ExtractFileName(const char *szOldPath, char *szNewPath, DWORD NewPathBufferLen);
private:
	BOOL FileExist(const char *szFileName, WIN32_FIND_DATA *pData, BOOL IsDir = FALSE);
	BOOL DirExist(const char *szFileName, WIN32_FIND_DATA *pData);
	void RecurseDirectory(CFolderNode *pNode, char *dir, DWORD Level);
public:
	void CreateRootDirectory();
	BOOL AddDirectory(CFolderNode *pNode, char *dir);
	BOOL AddFile(CFolderNode *pNode, char *fname);
	
	void ClearNodes(CFolderNode *pNode, DWORD Level);
	void PrintNodes(CFolderNode *pNode, DWORD Level);
};

#endif
#endif //_CFOLDERSTREE_H_
