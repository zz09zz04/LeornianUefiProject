/** @file

  UEFI FV Parser Tool in Host OS execution.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <stdlib.h>

#include "SetScreenColor.h"

#include <Base.h>
#include <Pi/PiFirmwareVolume.h>
#include <Pi/PiFirmwareFile.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/FirmwareFileSystem3.h>
#include <Guid/FspHeaderFile.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FvLib.h>

#define   DEBUG_STR   "[Debug]"
#define	  ERROR_EXIT	-1

#define   FV_COUNT    32
EFI_FIRMWARE_VOLUME_HEADER *FvBaseAddrBuffer[FV_COUNT];

typedef struct {
  UINTN Offset;
  UINT8 Color;
} FV_COLOR_MAP;

STATIC CONST FV_COLOR_MAP mFvColorMap[] = {
  { OFFSET_OF(EFI_FIRMWARE_VOLUME_HEADER, FileSystemGuid),  BLUE_TEXT       },
  { OFFSET_OF(EFI_FIRMWARE_VOLUME_HEADER, FvLength),        GREEN_TEXT      },
  { OFFSET_OF(EFI_FIRMWARE_VOLUME_HEADER, Signature),       RED_TEXT        },
  { OFFSET_OF(EFI_FIRMWARE_VOLUME_HEADER, Attributes),      CYAN_TEXT       },
  { OFFSET_OF(EFI_FIRMWARE_VOLUME_HEADER, HeaderLength),    PURPLE_TEXT     },
  { OFFSET_OF(EFI_FIRMWARE_VOLUME_HEADER, Checksum),        LIGHTBLUE_TEXT  },
  { OFFSET_OF(EFI_FIRMWARE_VOLUME_HEADER, ExtHeaderOffset), LIGHTGREEN_TEXT },
  { OFFSET_OF(EFI_FIRMWARE_VOLUME_HEADER, Reserved),        LIGHTRED_TEXT   },
  { OFFSET_OF(EFI_FIRMWARE_VOLUME_HEADER, Revision),        LIGHTCYAN_TEXT  },
  { OFFSET_OF(EFI_FIRMWARE_VOLUME_HEADER, BlockMap),        WHITE_TEXT      },
  { sizeof(EFI_FIRMWARE_VOLUME_HEADER),                     LIGHTPURPLE_TEXT}
};

STATIC
VOID
SetColorForFvRaw (
  IN UINTN Offset
  )
{
  UINTN Index;

  for (Index = 0; Index < ARRAY_SIZE(mFvColorMap); Index++) {
    if (Offset == mFvColorMap[Index].Offset) {
      SetScreenColor (mFvColorMap[Index].Color);
      break;
    }
  }
}

void ShowRawData(UINT8 *Buffer, UINT64 size)
{
  int i,j;
  for (i = 0 ; i < size / 16 ; i++) {
    SetScreenColor (ORANGE_TEXT);
    printf ("%08x:",i * 16);
    SetScreenColor (WHITE_TEXT);
  	for (j = 0 ; j < 16 ; j++) {
  	  printf (" %02x", Buffer[i*16+j]);
  	}
  	printf ("  ");
  	for (j = 0 ; j < 16 ; j++) {
  	  UINT8 data = Buffer[i*16+j];
  	  if (data >= 0x20 && data <= 0x7E) {
  	    printf ("%c",data);
  	  } else {
  	    printf (".");
  	  }
    }
    printf ("\n");
  }
  if (size % 16 != 0) {
    SetScreenColor (ORANGE_TEXT);
  	printf ("%08x:", (int)size / 16 * 16);
    SetScreenColor (WHITE_TEXT);
  	for (j = 0 ; j < 16 ; j++) {
  	  if (j > size % 16 - 1) {
  	    printf ("   ");
  	  } else {
  	    printf (" %02x", Buffer[size / 16 * 16 + j]);
  	  }
  	}
    printf ("  ");
    for (j = 0 ; j < size % 16 ; j++) {
      UINT8 data = Buffer[size / 16 * 16 + j];
      if (data >= 0x20 && data <= 0x7E) {
  	    printf ("%c",data);
      } else {
        printf (".");
  	  }
    }
    printf ("\n");
  }
}

void ShowFvRawData(UINT8 *Buffer, UINT64 size)
{
  int i, j;
  for (i = 0 ; i < size / 16 ; i++) {
    SetScreenColor (ORANGE_TEXT);
    printf ("%08x:",i * 16);
    SetScreenColor (WHITE_TEXT);
    for (j = 0 ; j < 16 ; j++) {
      SetColorForFvRaw (i * 16 + j);
      printf (" %02x", Buffer[i*16+j]);
    }
    printf ("  ");
    for (j = 0 ; j < 16 ; j++) {
      SetColorForFvRaw (i * 16 + j);
      UINT8 data = Buffer[i*16+j];

      if (data >= 0x20 && data <= 0x7E) {
        printf ("%c",data);
      } else {
        printf (".");
      }
    }
    printf ("\n");
  }
  if (size % 16 != 0) {
    SetScreenColor (ORANGE_TEXT);
    printf ("%08x:", (int)size / 16 * 16);
    SetScreenColor (WHITE_TEXT);
    
    for (j = 0 ; j < 16 ; j++) {
      if (j > size % 16 - 1) {
        printf ("   ");
      } else {
        printf (" %02x", Buffer[size / 16 * 16 + j]);
      }
    }
    printf ("  ");
    for (j = 0 ; j < size % 16 ; j++) {
      UINT8 data = Buffer[size / 16 * 16 + j];
      if (data >= 0x20 && data <= 0x7E) {
        printf ("%c",data);
      } else {
        printf (".");
      }
    }
    printf ("\n");
  }
}

void PrintGuid (EFI_GUID *Guid)
{
  printf (
    "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
    Guid->Data1,
    Guid->Data2,
    Guid->Data3,
    Guid->Data4[0],
    Guid->Data4[1],
    Guid->Data4[2],
    Guid->Data4[3],
    Guid->Data4[4],
    Guid->Data4[5],
    Guid->Data4[6],
    Guid->Data4[7]
    );
}

const char* ParseFvFileTypeWorker (EFI_FV_FILETYPE Type) {
  const char* FvFileType[0x10] = { 
                        NULL,
                        "Raw",
                        "FreeForm",
                        "SEC Core",
                        "PEI Core",
                        "DXE Core",
                        "PEIM",
                        "Driver",
                        "Combined PEIM Driver",
                        "Application",
                        "MM",
                        "FV Image",
                        "Combined MM DXE",
                        "MM Core",
                        "MM Stand alone",
                        "MM Core Stand alone"
                        };
  if (Type == 0xF0) {
    return "Pad File";
  }
  if (Type >= 0x10) {
    return NULL;
  }
  return FvFileType[Type];
}

int ParseFfsHeader (EFI_FFS_FILE_HEADER *FfsFileHeader) 
{
  if (FFS_FILE_SIZE(FfsFileHeader) == 0xffffff) {
    return FFS_FILE_SIZE(FfsFileHeader);
  }

  printf ("File Name(GUID): ");
  SetScreenColor (LIGHTORANGE_TEXT);
  PrintGuid (&FfsFileHeader->Name);
  if (CompareGuid (&FfsFileHeader->Name, &gEfiFirmwareVolumeTopFileGuid)) {
    SetScreenColor (RED_TEXT);
    printf ("  Volume Top File Guid");
    SetScreenColor (WHITE_TEXT);
  }
  if (CompareGuid (&FfsFileHeader->Name, &gFspHeaderFileGuid)) {
    SetScreenColor (CYAN_TEXT);
    printf ("  Firmware Support Package FFS Guid");
    SetScreenColor (WHITE_TEXT);
    printf ("\n");
  }
  SetScreenColor (WHITE_TEXT);
  printf ("\n");

  printf ("Integrity Check                 = 0x%04x\n", FfsFileHeader->IntegrityCheck.Checksum16);
  printf ("Type                            = 0x%02x (%s)\n", FfsFileHeader->Type, ParseFvFileTypeWorker(FfsFileHeader->Type));
  printf ("Attribute                       = 0x%02x\n", FfsFileHeader->Attributes);
  printf ("FFS Size (including FFS Header) = 0x%x (%d)\n", FFS_FILE_SIZE(FfsFileHeader), FFS_FILE_SIZE(FfsFileHeader));
  printf ("State                           = 0x%02x\n", FfsFileHeader->State);
  printf ("\n");

  if (CompareGuid (&FfsFileHeader->Name, &gFspHeaderFileGuid)) {
    FSP_INFO_HEADER *FspInfoHeader;
    FspInfoHeader = (FSP_INFO_HEADER*)((UINT8*)FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER) + 4);
    ShowRawData ((UINT8*)FspInfoHeader, FFS_FILE_SIZE(FfsFileHeader) - sizeof (EFI_FFS_FILE_HEADER) - 4);
    printf ("Signature                       = 0x%x\n", FspInfoHeader->Signature);
    printf ("HeaderLength                    = 0x%x\n", FspInfoHeader->HeaderLength);
    printf ("SpecVersion                     = 0x%x\n", FspInfoHeader->SpecVersion);
    printf ("HeaderRevision                  = 0x%x\n", FspInfoHeader->HeaderRevision);
    printf ("ImageRevision                   = 0x%x\n", FspInfoHeader->ImageRevision);
    printf ("ImageId                         = \n");
    ShowRawData ((UINT8*)&FspInfoHeader->ImageId, 8);
    printf ("ImageSize                       = 0x%x\n", FspInfoHeader->ImageSize);
    printf ("ImageBase                       = 0x%x\n", FspInfoHeader->ImageBase);
    printf ("ImageAttribute                  = 0x%x\n", FspInfoHeader->ImageAttribute);
    printf ("ComponentAttribute              = 0x%x\n", FspInfoHeader->ComponentAttribute);
    printf ("CfgRegionOffset                 = 0x%x\n", FspInfoHeader->CfgRegionOffset);
    printf ("CfgRegionSize                   = 0x%x\n", FspInfoHeader->CfgRegionSize);
    printf ("TempRamInitEntryOffset          = 0x%x\n", FspInfoHeader->TempRamInitEntryOffset);
    printf ("NotifyPhaseEntryOffset          = 0x%x\n", FspInfoHeader->NotifyPhaseEntryOffset);
    printf ("FspMemoryInitEntryOffset        = 0x%x\n", FspInfoHeader->FspMemoryInitEntryOffset);
    printf ("TempRamExitEntryOffset          = 0x%x\n", FspInfoHeader->TempRamExitEntryOffset);
    printf ("FspSiliconInitEntryOffset       = 0x%x\n", FspInfoHeader->FspSiliconInitEntryOffset);
    printf ("FspMultiPhaseSiInitEntryOffset  = 0x%x\n", FspInfoHeader->FspMultiPhaseSiInitEntryOffset);

    printf ("ExtendedImageRevision           = 0x%x\n", FspInfoHeader->ExtendedImageRevision);
//    printf ("Signature = 0x%x\n", FspInfoHeader->);

  }

  return FFS_FILE_SIZE(FfsFileHeader);
}

void ParseFvHeader(EFI_FIRMWARE_VOLUME_HEADER *FvHeader)
{
  int     i;
  UINT8	  *data;

  data = (UINT8*)&FvHeader->Signature;

  printf ("Start to Parse FV Header\n");

  ShowFvRawData ((UINT8*)FvHeader, FvHeader->HeaderLength);

  printf("\n");
  printf("Zero Vector:");
  for(i = 0 ; i < 16 ; i++)
  printf ("%02x ",FvHeader->ZeroVector[i]);
  printf("\n");
  printf ("Guid: ");
  SetScreenColor (BLUE_TEXT);
  PrintGuid (&FvHeader->FileSystemGuid);
  if (CompareGuid (&FvHeader->FileSystemGuid, &gEfiFirmwareFileSystem2Guid)) {
    SetScreenColor (LIGHTBLUE_TEXT);
    printf ("  (gEfiFirmwareFileSystem2Guid)");
  } else if (CompareGuid (&FvHeader->FileSystemGuid, &gEfiFirmwareFileSystem3Guid)) {
    SetScreenColor (LIGHTBLUE_TEXT);
    printf ("  (gEfiFirmwareFileSystem3Guid)");
  }
  printf ("\n");
  SetScreenColor (WHITE_TEXT);

  printf ("FV Length            = 0x%llx (%lld)\n", FvHeader->FvLength, FvHeader->FvLength);	

  printf ("Signature            = %c%c%c%c\n", *data, *(data+1), *(data+2), *(data+3));

  printf ("FV Attribute         = 0x%08x\n", FvHeader->Attributes);
  printf ("Header Length        = 0x%x (%d)\n", FvHeader->HeaderLength, FvHeader->HeaderLength);
  printf ("Check Sum            = 0x%04x\n", FvHeader->Checksum);
  printf ("Extend Header Offset = 0x%04x\n",FvHeader->ExtHeaderOffset);
  printf ("Revision             = 0x%02x\n", FvHeader->Revision);
  printf ("Number of Block      = 0x%08x\n", FvHeader->BlockMap[0].NumBlocks);
  printf ("Block Length         = 0x%08x\n", FvHeader->BlockMap[0].Length);
  printf ("\n\n");

  printf (DEBUG_STR"%p\n", FvHeader);
  printf (DEBUG_STR"%p\n", (UINT8*)FvHeader + FvHeader->HeaderLength);


  //
  //  Start to parse FFS for this FV.
  //
  EFI_FFS_FILE_HEADER  *FileHeader;
  EFI_STATUS           Status;

  FileHeader = NULL;

  do {
    Status = FfsFindNextFile (
              EFI_FV_FILETYPE_ALL,
              FvHeader,
              &FileHeader
              );
    if (!EFI_ERROR (Status)) {
      ParseFfsHeader (FileHeader);
    }
  } while (!EFI_ERROR (Status));

/*
  UINTN FvSize;
  UINTN FfsSize = 0;
  UINT8 *DummyPointer;

  DummyPointer = (UINT8*)FvHeader + FvHeader->HeaderLength;
  FvSize = FvHeader->FvLength - FvHeader->HeaderLength;

  while ((FvSize) > 0) {
    //
    //	Debug
    //
    printf ("Address: 0x%llx\n", DummyPointer - (UINT8*)FvHeader);

    FfsSize = ParseFfsHeader (DummyPointer);
    if (FfsSize == 0xffffff || FfsSize == 0) {
      printf ("The Final FFS.\n");
      break;
    }
    if (FfsSize % 8 != 0) {
      FfsSize += (8 - FfsSize % 8);
    }
    FvSize -= FfsSize;
    DummyPointer += FfsSize;
    printf (DEBUG_STR"remaining size = 0x%llx\n",FvSize);
  }
*/
}

void SearchFvHeaderAddress (UINT8 *Buffer, int BufferSize, int *NumberOfFvHeader)
{
  int i;
  int Count = 0;

  UINT32                     *SigPointer;
  EFI_FIRMWARE_VOLUME_HEADER *FvHeader;

  Count = 0;
  for(i = 0 ; i < BufferSize - sizeof(UINT32) ; i += (sizeof(UINT8) / sizeof(UINT8))) {
    SigPointer = (UINT32*)&Buffer[i];
//    FvHeader = BASE_CR(&Buffer[i], EFI_FIRMWARE_VOLUME_HEADER, Signature);
    if (*SigPointer == EFI_FVH_SIGNATURE) {
      FvHeader = BASE_CR(&Buffer[i], EFI_FIRMWARE_VOLUME_HEADER, Signature);
      if (CompareGuid (&FvHeader->FileSystemGuid, &gEfiFirmwareFileSystem2Guid) ||
          CompareGuid (&FvHeader->FileSystemGuid, &gEfiFirmwareFileSystem3Guid)) {
        FvBaseAddrBuffer[Count] = (EFI_FIRMWARE_VOLUME_HEADER*) FvHeader;
        printf ("[Buffer Address %d: 0x%08llx][ROM Address %d: 0x%llx]\n", Count, (UINT64)(FvHeader), Count, ((UINT64)FvHeader - (UINT64)Buffer));
        Count++;
      }
    }
    if (Count >= FV_COUNT) {
      printf ("Fv Count is large than FV_COUNT: %d\n",Count);
      return;
    }
  }
  *NumberOfFvHeader = Count;
  return;
}

UINT8* GetFvImageContent (
  char *FileName,
  int  *DataSize
  )
{
  FILE     *fp = NULL;
  UINT8    *FvRawData;

  if (DataSize == NULL) {
    return NULL;
  }

  FvRawData = NULL;
  *DataSize = 0;
  fp = fopen (FileName,"rb");
  if (fp == NULL) {
    printf ("Open File %s Error!!!\n", FileName);
    return NULL;
  }

  fseek (fp, 0, SEEK_END);
  *DataSize = ftell (fp);
  if (*DataSize <= 0) {
    printf ("Fail to get File %s size!!!\n", FileName);
    goto Exit;
  }
  printf ("File size = 0x%x (%d)\n", *DataSize, *DataSize);
  rewind(fp);

  FvRawData = malloc (*DataSize * sizeof(UINT8));
  if (FvRawData == NULL) {
    printf ("Allocate Buffer Error!!!\n");
    goto Exit;
  }

  if (*DataSize != fread (FvRawData, sizeof(UINT8), *DataSize, fp)) {
    printf("Read File Error!!!\n");
    free (FvRawData);
    FvRawData = NULL;
    goto Exit;
  }

Exit:
  fclose(fp);
  return FvRawData;
}

int main (int argc, char **argv)
{
  UINT8    *FvRawData;
  int      FvRawSize;
  int      FirmwareVolumeCount;
  int      i;

  if (argc != 2) {
    printf ("Parameter Error!\n");
    return ERROR_EXIT;
  }

  FvRawData = GetFvImageContent (argv[1], &FvRawSize);
  if (FvRawData == NULL) {
    return ERROR_EXIT;
  }

  SearchFvHeaderAddress (FvRawData, FvRawSize, &FirmwareVolumeCount);

  for (i = 0 ; i < FirmwareVolumeCount ; i++) {
    printf ("[FV index %d ROM Address: 0x%llx]\n", i, (UINT64)(FvBaseAddrBuffer[i] - FvBaseAddrBuffer[0]));
    ParseFvHeader ((EFI_FIRMWARE_VOLUME_HEADER*)FvBaseAddrBuffer[i]);
  }

  printf ("Done to Parse FV file %s\n", argv[1]);

  free(FvRawData);
}
