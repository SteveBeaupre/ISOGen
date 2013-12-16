#include "FoldersTree.h"

//-----------------------------------------------------------------------------
// Constructor...
//-----------------------------------------------------------------------------
CFoldersClass::CFoldersClass()
{
	NumFolders = 0;
	InitNode(&RootNode, NULL, NULL, NULL, -1, TRUE, _THIS_DIR_, NULL);
}

//-----------------------------------------------------------------------------
// Destructor...
//-----------------------------------------------------------------------------
CFoldersClass::~CFoldersClass()
{
	ClearNodes(NULL, 0);
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Check if a file path is valid
//-----------------------------------------------------------------------------
BOOL CFoldersClass::FileExist(const char *szFileName, WIN32_FIND_DATA *pData, BOOL IsDir)
{
	//Create a buffer to copy our string
	char Buf[MAX_PATH];
	ZeroMemory(Buf, sizeof(Buf));
	
	// Append "/*" to the string if we search for a directory
	sprintf(Buf, !IsDir ? "%s" : "%s\\*", szFileName);
    
	//
    HANDLE h = FindFirstFile(Buf, pData);

	// Return a valid handle if found
	if(h){	
		FindClose(h); // Close the handle
		return TRUE;
	}

	// Return false if the file or directory dosen't exist
	return FALSE;
}

//-----------------------------------------------------------------------------
// Check if a directory name is valid
//-----------------------------------------------------------------------------
BOOL CFoldersClass::DirExist(const char *szFileName, WIN32_FIND_DATA *pData)
{
	return FileExist(szFileName, pData, TRUE);
}

//-----------------------------------------------------------------------------
// Convert for Ex. "C:\Temp\File1.bin" into "File1.bin"
//-----------------------------------------------------------------------------
void CFoldersClass::ExtractFileName(const char *szOldPath, char *szNewPath, DWORD NewPathBufferLen)
{
	ZeroMemory(szNewPath, NewPathBufferLen);

	DWORD Len = strlen(szOldPath);

	if(Len > 0){
		
		BOOL IsSlashFound = FALSE;
		DWORD CptNewPath = 0;
		
		for(DWORD CptOldPath = 0; CptOldPath < Len; CptOldPath++){
			
			if(CptOldPath >= NewPathBufferLen)
				break;

			if(!IsSlashFound && szOldPath[CptOldPath] == '\\'){
				IsSlashFound = TRUE;
				continue;
			}

			if(IsSlashFound && CptOldPath < Len){
				szNewPath[CptNewPath] = szOldPath[CptOldPath];	
				CptNewPath++;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void AddToPath(char *p1, char *p2)
{
	if(strlen(p1) > 0 && p1[strlen(p1)-1] != '\\'){
		strcat(p1, "\\");
		strcat(p1, p2);
	} else {
		strcat(p1, p2);
	}
}

////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void MergePath(char *pDest, char *p1, char *p2)
{
	if(strlen(p1) > 0 && p1[strlen(p1)-1] != '\\'){
		strcat(pDest, p1);
		strcat(pDest, "\\");
		strcat(pDest, p2);
	} else {
		strcat(pDest, p1);
		strcat(pDest, p2);
	}
}

////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Create a file node on the root
//-----------------------------------------------------------------------------
BOOL CFoldersClass::AddFile(CFolderNode *pNode, char *fname)
{
	if(pNode == NULL)
		pNode = GetRootNode();

	pNode = GetLastChild(pNode);
	if(pNode == NULL) // This shouldn't happen unless CreateRootNode has not been called
		return FALSE;

	CFolderNode *pParentNode = pNode->pParent;
	CFolderNode *pLastNode = pNode;
	
	// Gather directory info
	WIN32_FIND_DATA fdata;
	HANDLE h = FindFirstFile(fname, &fdata);

	// If the file dosen't exist...
	if(h == INVALID_HANDLE_VALUE){
		ErrorsLog.LogError(_WARNING_FILE_DOSENT_EXIST_, fname);
		return FALSE;
	}

	// Create the file node
	pNode->pNext = new CFolderNode;
	pNode = pNode->pNext;

	// Extract the parent directory name...
	DWORD FullNameLen = strlen(fname);
	DWORD FilePathNameLen = FullNameLen - (strlen(fdata.cFileName) + 1);
	char Dir[MAX_PATH_LEN];
	ZeroMemory(&Dir[0], MAX_PATH_LEN);
	for(DWORD Cpt = 0; Cpt < FilePathNameLen; Cpt++)
		Dir[Cpt] = fname[Cpt];
	
	// Init. the node
	InitNode(pNode, pParentNode, pLastNode, NULL, 0, &fdata, &Dir[0]);

	return TRUE;
}

//-----------------------------------------------------------------------------
// Create some nodes from a directory path on the disk
//-----------------------------------------------------------------------------
BOOL CFoldersClass::AddDirectory(CFolderNode *pNode, char *dir)
{
	if(pNode == NULL)
		pNode = GetRootNode();

	pNode = GetLastChild(pNode);
	if(pNode == NULL) // This shouldn't happen unless CreateRootNode has not been called
		return FALSE;

	CFolderNode *pParentNode = pNode->pParent;
	CFolderNode *pLastNode = pNode;
	
	// Gather directory info
	WIN32_FIND_DATA fdata;
	HANDLE h = FindFirstFile(dir, &fdata);

	// If the directory dosen't exist...
	if(h == INVALID_HANDLE_VALUE){
		ErrorsLog.LogError(_WARNING_DIR_DOSENT_EXIST_, dir);
		return FALSE;
	}

	// Create the directory
	pNode->pNext = new CFolderNode;
	pNode = pNode->pNext;

	// Extract the parent directory name...
	DWORD PathLen = strlen(dir);
	DWORD ParentPathNameLen = PathLen - (strlen(fdata.cFileName) + 1);
	char ParentPathName[MAX_PATH_LEN];
	ZeroMemory(&ParentPathName[0], MAX_PATH_LEN);
	for(DWORD Cpt = 0; Cpt < ParentPathNameLen; Cpt++)
		ParentPathName[Cpt] = dir[Cpt];
	
	// Init. the node
	InitNode(pNode, pParentNode, pLastNode, NULL, 0, &fdata, &ParentPathName[0]);

	// Start adding this directory files and folders recursively
	RecurseDirectory(pNode, dir, 1);
	
	return TRUE;
}

//-----------------------------------------------------------------------------
// Build a nodes with recursion
//-----------------------------------------------------------------------------
void CFoldersClass::RecurseDirectory(CFolderNode *pNode, char *dir, DWORD Level)
{
	// Append a "\*" or "*" to the end of the current path
	char path[MAX_PATH_LEN];
	ZeroMemory(&path[0], MAX_PATH_LEN);
	MergePath(path, dir, "*");

	WIN32_FIND_DATA fdata;

	// If the directory is valid...
	if(DirExist(dir, &fdata)){
		
		HANDLE h = FindFirstFile(&path[0], &fdata);

		CFolderNode *pParentNode = pNode;
		CFolderNode *pPrevNode = NULL;

		// Add a sub-directories to the end of the current node childs list
		do{	
			pNode = pParentNode;

			if(pNode->pFirstChild == NULL){
				pNode->pFirstChild = new CFolderNode;
				pPrevNode = NULL;
				pNode = pNode->pFirstChild;
			} else {
				pNode = GetLastChild(pNode);
				pNode->pNext = new CFolderNode;
				pPrevNode = pNode;
				pNode = pNode->pNext;
			}

			InitNode(pNode, pParentNode, pPrevNode, NULL, Level + 1, &fdata, dir);

			if(strcmp(fdata.cFileName, ".") != 0 && strcmp(fdata.cFileName, "..") != 0){

				char newpath[MAX_PATH_LEN];
				ZeroMemory(&newpath[0], MAX_PATH_LEN);
				MergePath(&newpath[0], dir, fdata.cFileName);

				if(fdata.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
					RecurseDirectory(pNode, &newpath[0], Level + 1);
			}
		} while(FindNextFile(h, &fdata) != NULL);

		// Close the handle
		FindClose(h);
		pNode = pParentNode;
	}
}

//-----------------------------------------------------------------------------
// Initialize a node (only the "." and ".." and the root node should be init. here now 
//-----------------------------------------------------------------------------
void CFoldersClass::InitNode(CFolderNode* pNode, CFolderNode* pParentNode, CFolderNode* pPrevNode, CFolderNode* pNextNode, 
							 DWORD Level, 
							 BOOL IsDir, 
							 DWORD IsSpecialDir,
							 char *pName)
{
	// Initialise the class data structure
	pNode->Level = Level;

	pNode->IsDirectory = IsDir;
	pNode->IsSpecialDirectory = IsSpecialDir;

	pNode->DirectoryID = 0;

	pNode->FirstSector.x = 0;
	pNode->SectorUsed.x  = 0;
	pNode->Size.x        = 0;

	ZeroMemory(&pNode->Name[0], MAX_PATH_LEN);
	ZeroMemory(&pNode->FullFileName[0], MAX_PATH_LEN);
	ZeroMemory(&pNode->ShortName[0], 32);
	ZeroMemory(&pNode->LongName[0], 224);

	pNode->ShortNameLen = 0;
	pNode->LongNameLen = 0;

	switch(IsSpecialDir) // <-- Don't put the ZeroMemory() below this ...
	{
	case _THIS_DIR_:
		pNode->NameLen = 0;
		break;
	case _PREV_DIR_:
		pNode->NameLen = 1;
		pNode->Name[0] = 1;
		break;
	case _NORMAL_DIR_:
		pNode->NameLen = strlen(pName);
		sprintf(&pNode->Name[0], pName);
		break;
	}
	
	// Increment the folders counter, if this is a normal directory
	if(pNode->IsDirectory == TRUE && !pNode->IsSpecialDirectory){
		NumFolders++;
		// Raise the "Folders number limitation" error flag
		if(NumFolders > 0x0000FFFF) 
			ErrorsLog.LogError(_ERROR_DIRECTORY_LIMIT_HIT_, NULL);
	}

	// Setup the nodes pointers
	SetNodePointers(pNode, pParentNode, NULL, pPrevNode, pNextNode);
}

//-----------------------------------------------------------------------------
// Initialize a node (Should be used by AddDirectory and Recurse Directory only)
//-----------------------------------------------------------------------------
void CFoldersClass::InitNode(CFolderNode* pNode, CFolderNode* pParentNode, CFolderNode* pPrevNode, CFolderNode* pNextNode, DWORD Level, WIN32_FIND_DATA *fdata, char *dir)
{
	pNode->FirstSector.x = 0;
	pNode->SectorUsed.x  = 0;
	pNode->Size.x        = 0;

	ZeroMemory(&pNode->Name[0], MAX_PATH_LEN);
	ZeroMemory(&pNode->FullFileName[0], MAX_PATH_LEN);
	ZeroMemory(&pNode->ShortName[0], 32);
	ZeroMemory(&pNode->LongName[0], 224);

	// Initialise the class data structure
	pNode->Level = Level;

	pNode->IsDirectory = fdata->dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY;

	if(!pNode->IsDirectory){
		// Raise the "file size too large" error flag...
		if(fdata->nFileSizeHigh != 0) 
			ErrorsLog.LogError(_ERROR_FILE_SIZE_TOO_LARGE_, &fdata->cFileName[0]);

		pNode->IsSpecialDirectory = _NORMAL_FILE_;
		pNode->Size._32.Lo = fdata->nFileSizeLow;
		pNode->SectorUsed._32.Lo = pNode->Size._32.Lo / SECTOR_SIZE;
		if(pNode->Size._32.Lo % SECTOR_SIZE > 0)
			pNode->SectorUsed._32.Lo++;
		MergePath(&pNode->FullFileName[0], dir, fdata->cFileName);
	} else {
		if(strcmp(fdata->cFileName, "..") == 0){
			pNode->IsSpecialDirectory = _PREV_DIR_;
		} else if(strcmp(fdata->cFileName, ".") == 0){
			pNode->IsSpecialDirectory = _THIS_DIR_;
		} else {
			pNode->IsSpecialDirectory = _NORMAL_DIR_;
			MergePath(&pNode->FullFileName[0], dir, fdata->cFileName);
		}
	}

	switch(pNode->IsSpecialDirectory) // <-- Don't put the ZeroMemory() below this ...
	{
	case _THIS_DIR_:
		pNode->NameLen = 0;
		break;
	case _PREV_DIR_:
		pNode->NameLen = 1;
		pNode->Name[0] = 1;
		break;
	case _NORMAL_DIR_:
		pNode->NameLen = strlen(fdata->cFileName);
		sprintf(&pNode->Name[0], fdata->cFileName);
		break;
	}

	// Create the short file name
	pNode->ShortNameLen = strlen(fdata->cAlternateFileName);
	if(pNode->ShortNameLen > 0){
		memcpy(&pNode->ShortName[0], &fdata->cAlternateFileName[0], pNode->ShortNameLen);
	} else {
		pNode->ShortNameLen = strlen(pNode->Name);
		memcpy(&pNode->ShortName[0], &pNode->Name[0], pNode->ShortNameLen);
	}
	Utils.ConvertToUppercase((BYTE*)&pNode->ShortName[0], 32);
	Utils.TranslateIllegalChars((BYTE*)&pNode->ShortName[0], 32);


	// Raise the "name too long" warning flag...
	if(strlen(pNode->Name) > 110)
		ErrorsLog.LogError(_WARNING_FILE_NAME_TOO_LARGE_, &pNode->Name[0]);
	// Create the long file name
	memcpy(&pNode->LongName[0], &pNode->Name[0], 110);
	pNode->LongNameLen = strlen(pNode->LongName) * 2;
	Utils.ConvertToUnicode((BYTE*)&pNode->LongName[0], pNode->LongNameLen);

	// Increment the folders counter, if this is a normal directory
	if(pNode->IsDirectory == TRUE && !pNode->IsSpecialDirectory){
		NumFolders++;
		// Raise the "Folders number limitation" error flag
		if(NumFolders > 0x0000FFFF) 
			ErrorsLog.LogError(_ERROR_DIRECTORY_LIMIT_HIT_, NULL);
	}

	// Setup the nodes pointers
	SetNodePointers(pNode, pParentNode, NULL, pPrevNode, pNextNode);
}

void CFoldersClass::SetNodePointers(CFolderNode* pNode, CFolderNode* ParentNode, CFolderNode* FirstChild, CFolderNode* Prev, CFolderNode* Next)
{
	pNode->pParent     = ParentNode;
	pNode->pFirstChild = FirstChild;
	pNode->pPrev = Prev;
	if(pNode->pPrev != NULL)
		pNode->pPrev->pNext = pNode;
	pNode->pNext = Next;
}

//-----------------------------------------------------------------------------
// Return the address of the root node
//-----------------------------------------------------------------------------
CFolderNode* CFoldersClass::GetRootNode()
{
	// Return the node pointer
	return &RootNode;
}

//-----------------------------------------------------------------------------
// Return the first child of the given node
//-----------------------------------------------------------------------------
CFolderNode* CFoldersClass::GetFirstChild(CFolderNode* pNode)
{
	// Return the node pointer
	return pNode->pFirstChild;
}

//-----------------------------------------------------------------------------
// Return the last child of the given node
//-----------------------------------------------------------------------------
CFolderNode* CFoldersClass::GetLastChild(CFolderNode *pNode)
{
	pNode = GetFirstChild(pNode);

	while(pNode != NULL){
		if(pNode->pNext != NULL)
			pNode = pNode->pNext;
		else
			break;
	}

	// Return the node pointer
	return pNode;
}


//-----------------------------------------------------------------------------
// Create some nodes for the root directory
//-----------------------------------------------------------------------------
void CFoldersClass::CreateRootDirectory()
{
	CFolderNode *pRootNode = GetRootNode();
	CFolderNode *pNode = pRootNode;

	if(pNode->pFirstChild != NULL)
		return;
	
	// Create the "." directory
	pNode->pFirstChild = new CFolderNode;
	pNode = pNode->pFirstChild;
	InitNode(pNode, pRootNode, NULL, NULL, 0, TRUE, _THIS_DIR_, NULL);
	CFolderNode *pLastNode = pNode;

	// Create the ".." directory
	pNode->pNext = new CFolderNode;
	pNode = pNode->pNext;
	InitNode(pNode, pRootNode, pLastNode, NULL, 0, TRUE, _PREV_DIR_, NULL);
}


//-----------------------------------------------------------------------------
// Clear all the nodes
//-----------------------------------------------------------------------------
void CFoldersClass::ClearNodes(CFolderNode *pNode, DWORD Level)
{
	// if pNode == NULL, we'll use the root node 
	if(Level == 0)
		pNode = GetRootNode();
	
	// Get the last child pointer
	CFolderNode *pChild = GetLastChild(pNode);

	// While this child have prev nodes...
	while(pChild != NULL){
	
		// Recruse through the node in reverse order...
		ClearNodes(pChild, Level + 1);

		// Then delete the node and set back the pointers needed to NULL
		if(pChild->pPrev != NULL){
			pChild = pChild->pPrev;
			SAFE_DELETE_OBJECT(pChild->pNext);
		} else {
			SAFE_DELETE_OBJECT(pChild);
			pChild = NULL;
		}
	}

	// Set the first child pointer of this node to NULL
	pNode->pFirstChild = NULL;

	// Reset the Folders number counter
	if(Level == 0){
		NumFolders = 0;
		//Errors = 0;
	};

}

//-----------------------------------------------------------------------------
// Print the tree nodes...
//-----------------------------------------------------------------------------
void CFoldersClass::PrintNodes(CFolderNode *pNode, DWORD Level)
{
	// if pNode == NULL, we'll use the root node 
	if(Level == 0)
		pNode = GetRootNode();

	// Get the first child pointer
	CFolderNode *pChild = GetFirstChild(pNode);

	// While this child have next nodes...
	while(pChild != NULL){
	
		// ... print this node
		if(!pChild->IsSpecialDirectory){
			for(DWORD Cpt = 0; Cpt < Level; Cpt++)
				printf("  ");
			printf("%s - %d Sectors used\n", &pChild->Name[0], pChild->SectorUsed.dw[1]);
		}

		// ... print childs nodes
		PrintNodes(pChild, Level + 1);

		pChild = pChild->pNext;
	}
}

