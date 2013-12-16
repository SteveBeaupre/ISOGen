#ifndef _CVOLUMEDESCRIPTOR_H_
#define _CVOLUMEDESCRIPTOR_H_
#ifdef __cplusplus

#define SECTOR_SIZE  2048

// Define a raw sector byte data block
struct CRawSector {
	BYTE Data[SECTOR_SIZE];
};

// Define a Volume Descriptor sector data block
#pragma pack(1)
struct CVolumeDescriptor {
    BYTE  VolumeDescriptorSet[7];            // 0
    BYTE  _PadBytes1;
    BYTE  SystemIdentifier[32];              // 8
    BYTE  VolumeIdentifier[32];              // 40
    BYTE  _PadBytes2[8];
    DWORD TotalSectorsCount_LE;              // 80
    DWORD TotalSectorsCount_BE;
    BYTE  _PadBytes3[32];
    WORD  VolumeSetSize_LE;                  // 120
    WORD  VolumeSetSize_BE;
    WORD  VolumeSequenceNumber_LE;           // 124
    WORD  VolumeSequenceNumber_BE;
    WORD  SectorSize_LE;                     // 128
    WORD  SectorSize_BE;
    DWORD PathTableLength_LE;                // 132
    DWORD PathTableLength_BE;
    DWORD FirstSectorOf1stPathTable_LE;      // 140
    DWORD FirstSectorOf2ndPathTable_LE;
    DWORD FirstSectorOf1stPathTable_BE;      // 148
    DWORD FirstSectorOf2ndPathTable_BE;
    BYTE  RootDirectoryRecord[34];           // 156
    BYTE  VolumeSetIdentifier[128];          // 190
    BYTE  PublishedIdentifier[128];          // 318
    BYTE  DataPreparerIdentifier[128];       // 446
    BYTE  ApplicationIdentifier[128];        // 574
    BYTE  CopyrightFileIdentifier[37];       // 702
    BYTE  AbstractFileIdentifier[37];        // 739
    BYTE  BibliographicalFile[37];           // 776
    BYTE  VolumeCreationTime[17];            // 813
    BYTE  LastVolumeModificationTime[17];    // 830
    BYTE  VolumeExpirationTime[17];          // 847
    BYTE  VolumeEffectiveTime[17];           // 864
    BYTE  _PadBytes4;                        // 881
    BYTE  _PadBytes5;                        // 882
    BYTE  AppReserved[512];                  // 883
    BYTE  _PadBytes6[653];                   // 1395
};

struct CBootVolumeDescriptor {
    BYTE  VolumeDescriptorSet[7];            // 0
    BYTE  BootSystemIdentifier[32];          // 0
    BYTE  _PadBytes1[32];
	DWORD BootCatSector;
    BYTE  _PadBytes2[1973];
};

struct CBootCatalog {
	struct CValidationEntry {
		BYTE  HeaderID;
		BYTE  PlatformID;
		WORD  Reserved;
		BYTE  IDString[24];
		WORD  Checksum;
		BYTE  KeyBytes[2];
	};
	struct CDefaultEntry {
		BYTE  BootIndicator;
		BYTE  BootMediaType;
		WORD  LoadSegment;
		BYTE  SystemType;
		BYTE  Unused1;
		WORD  SectorCount;
		DWORD LoadRBA;
		DWORD Unused2;
	};

	CValidationEntry ValidationEntry;
	CDefaultEntry    DefaultEntry;
    BYTE  _PadBytes[2000];
};

// Define a Path Table Record data block
struct CPathTableRecord {
	BYTE  NameLen;              // 0
    BYTE  _PadBytes1;           // 1
    DWORD FirstSector;          // 2
	WORD  ParentDirectoryID;    // 6
	//////////////////////////////
	BYTE  Name[256];            // 8
};

// Define a Directory Record data block
struct CDirectoryRecord {
	BYTE  RecordSize;                // 0
    BYTE  _PadBytes1;                // 1
	DWORD FirstSector_LE;            // 2
	DWORD FirstSector_BE;            // 6
 	DWORD FileSize_LE;               // 10
	DWORD FileSize_BE;               // 14
	BYTE  Time[7];                   // 18  // Year Since 1900, Month, Day, Hour, Minute, Second, Offset From Greenwich
	BYTE  Flags;                     // 25
    WORD  _PadBytes2;                // 26
    WORD  _VolumeSequenceNumber_LE;  // 28
    WORD  _VolumeSequenceNumber_BE;  // 30
	BYTE  IdentifierLen;             // 32
	//////////////////////////////
	BYTE  Identifier[256];           // 33
};
#pragma pack()


#endif
#endif //--_CVOLUMEDESCRIPTOR_H_
