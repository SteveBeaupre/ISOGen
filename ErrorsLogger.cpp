#include "ErrorsLogger.h"

CErrorsLogger::CErrorsLogger()
{
	NumErrors = 0;
	NumWarnings = 0;

	InitRootNode();
}

CErrorsLogger::~CErrorsLogger()
{
	ClearAll();
}

void CErrorsLogger::InitRootNode()
{
	RootNode.Index = ERRORS_ROOTNODE_INDEX;
	RootNode.Type = 0;
	RootNode.ID = 0;
	ZeroMemory(&RootNode.TextBuf[0], MAX_ERR_TXT_BUF_SIZE);

	RootNode.pPrev = NULL;
	RootNode.pNext = NULL;
}

CErrorNode* CErrorsLogger::GetRootNode()
{
	return &RootNode;
}

CErrorNode* CErrorsLogger::GetNode(DWORD Indx)
{
	UINT Count = 0;
	CErrorNode *pNode = &RootNode;
	
	// Loop throught the nodes until we hit the one we want
	while(pNode->pNext != NULL){
		pNode = pNode->pNext;
		if(Count == Indx)
			return pNode;
		Count++;
	}

	return NULL;
}

DWORD CErrorsLogger::GetNodeCount()
{
	UINT Count = 0;
	CErrorNode *pNode = &RootNode;
	
	// Loop throught the nodes until we hit the one we want
	while(pNode->pNext != NULL){
		pNode = pNode->pNext;
		Count++;
	}

	return Count;
}

CErrorNode* CErrorsLogger::GetLastNode()
{
	CErrorNode *pNode = &RootNode;
	
	// Loop throught the nodes until we hit the last one
	while(pNode->pNext != NULL)
		pNode = pNode->pNext;

	// Return the last node pointer
	return pNode;
}

CErrorNode* CErrorsLogger::LogError(DWORD ErrID, char *txt)
{
	CErrorNode *pLastNode = GetLastNode();

	// Allocate memory for the new node
	pLastNode->pNext = new CErrorNode;
	CErrorNode *pNewNode = pLastNode->pNext;

	// Set the node index
	pNewNode->Index = pLastNode->Index + 1;
	
	// Init. the path name buffer
	if(ErrID >= 1 && ErrID <= 3){
		pNewNode->Type = _ERROR_MGS_;
	} else if(ErrID >= 4 && ErrID <= 6){
		pNewNode->Type = _WARNING_MGS_;
	}

	// Inc. the errors/warnings counter
	switch(pNewNode->Type)
	{
	case _ERROR_MGS_:   NumErrors++;   break;
	case _WARNING_MGS_: NumWarnings++; break;
	}

	// Save the error/warning id
	pNewNode->ID = ErrID;

	// Erase the buffer
	ZeroMemory(&pNewNode->TextBuf[0], MAX_ERR_TXT_BUF_SIZE);
	
	// Write the error/warning message text in the buffer
	switch(pNewNode->ID)
	{
	case _ERROR_FILE_SIZE_TOO_LARGE_: 
		sprintf(&pNewNode->TextBuf[0], "Error... The file \"%s\" is larger than %d bytes.", txt, 0xFFFFFFFF);
		break;
	case _ERROR_DIRECTORY_LIMIT_HIT_: 
		sprintf(&pNewNode->TextBuf[0], "Error... Folders count exceeding %d.", 0x0000FFFF);
		break;
	case _ERROR_FILE_CREATION_FAIL_: 
		sprintf(&pNewNode->TextBuf[0], "Error... Unable to create output file \"%s\".", txt);
		break;
	case _ERROR_BOOT_FILE_LOAD_FAIL_:
		sprintf(&pNewNode->TextBuf[0], "Error... Unable to load the boot sector file \"%s\".", txt);
		break;
	case _WARNING_BOOT_FILE_TOO_SMALL_: 
		sprintf(&pNewNode->TextBuf[0], "Warning... The boot sector file size isn't %d bytes", 0x00000800);
		break;
	case _WARNING_FILE_DOSENT_EXIST_: 
		sprintf(&pNewNode->TextBuf[0], "Warning... The file \"%s\" dosen't exist.", txt);
		break;
	case _WARNING_DIR_DOSENT_EXIST_: 
		sprintf(&pNewNode->TextBuf[0], "Warning... The folder \"%s\" dosen't exist.", txt);
		break;
	case _WARNING_FILE_NAME_TOO_LARGE_: 
		sprintf(&pNewNode->TextBuf[0], "Warning... The filename \"%s\" exceed 110 characters.", txt);
		break;
	}

	// Initialize parent and child pointers
	pNewNode->pPrev = pLastNode;
	pNewNode->pNext = NULL;

	return pNewNode;
}

bool CErrorsLogger::DelNode()
{
	CErrorNode *pNode = GetLastNode();

	// If this the root node, we're done...
	if(pNode == NULL || pNode->Index == ERRORS_ROOTNODE_INDEX)
		return false; // return faillure

	//
	switch(pNode->Type)
	{
	case _ERROR_MGS_:   NumErrors--;   break;
	case _WARNING_MGS_: NumWarnings--; break;
	}

	// Update the parent node child pointer
	CErrorNode *parentNode = pNode->pPrev;
	parentNode->pNext = NULL;

	// Delete the node
	SAFE_DELETE_OBJECT(pNode);

	// return success
	return true;
}

// Delete everything...
void CErrorsLogger::ClearAll()
{
	while(DelNode());
}

// Return a pointer to the text msg
char* CErrorsLogger::GetErrorMsg(DWORD Indx)
{
	CErrorNode *pNode = GetNode(Indx);

	if(pNode != NULL)
		return &pNode->TextBuf[0];

	return NULL;
}


