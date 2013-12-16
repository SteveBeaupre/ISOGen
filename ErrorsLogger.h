#ifndef _CERRORS_LOG_H_
#define _CERRORS_LOG_H_
#ifdef __cplusplus

#include "Windows.h"
#include "stdio.h"

#include "SafeKill.h"

#define ERRORS_ROOTNODE_INDEX -1

#define _ERROR_MGS_      1
#define _WARNING_MGS_    2

#define _ERROR_FILE_SIZE_TOO_LARGE_    1
#define _ERROR_DIRECTORY_LIMIT_HIT_    2
#define _ERROR_FILE_CREATION_FAIL_     3
#define _ERROR_BOOT_FILE_LOAD_FAIL_    4
#define _WARNING_BOOT_FILE_TOO_SMALL_  5
#define _WARNING_FILE_DOSENT_EXIST_    6
#define _WARNING_DIR_DOSENT_EXIST_     7
#define _WARNING_FILE_NAME_TOO_LARGE_  8

#define MAX_ERR_TXT_BUF_SIZE     512

struct CErrorNode {
	// Index of the node
	DWORD Index;

	// The type (error or warning)
	DWORD Type;
	// The kind of error/warning (Error)
	DWORD ID;

	// The error message buffer
	char TextBuf[MAX_ERR_TXT_BUF_SIZE];

	// Prev./Next node pointers
	struct CErrorNode *pPrev;
	struct CErrorNode *pNext;
};

// Main class
class CErrorsLogger {
public:
	CErrorsLogger();
	~CErrorsLogger();
private:
	DWORD NumErrors;
	DWORD NumWarnings;
private:
	CErrorNode RootNode;
private:
	void InitRootNode();
private:
	CErrorNode* GetRootNode();
	bool DelNode();
public:
	CErrorNode* GetNode(DWORD Indx);
	CErrorNode* GetLastNode();
	
	DWORD GetNodeCount();
public:
	DWORD GetNumErrors(){return NumErrors;}
	DWORD GetNumWarnings(){return NumWarnings;}
public:
	CErrorNode* LogError(DWORD ErrID, char *txt);
	void  ClearAll();
	char* GetErrorMsg(DWORD Indx);
};

#endif
#endif //_CERRORS_LOG_H_
