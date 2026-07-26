/** @file

  UEFI Firmware Device Parser Tool in Host OS execution.

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
#include <Guid/LzmaDecompress.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FvLib.h>
#include <Library/UefiDecompressLib.h>

//
// These interfaces are implemented by edk2's LzmaCustomDecompressLib.  That
// library exposes the decoder primarily through guided-section registration,
// but a host application can call the environment-independent decoder directly.
//
RETURN_STATUS
EFIAPI
LzmaUefiDecompressGetInfo (
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  );

RETURN_STATUS
EFIAPI
LzmaUefiDecompress (
  IN CONST VOID  *Source,
  IN UINTN       SourceSize,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch
  );

#define INDENT_PRINTF(Depth, ...)          \
  do {                                     \
    printf("%*s", (int)((Depth) * 2), ""); \
    printf(__VA_ARGS__);                   \
  } while (0)

#define   DEBUG_STR   "[Debug]"
#define	  ERROR_EXIT	-1

#define   FV_COUNT    32
#define   MAX_SECTION_NESTING_DEPTH  16
EFI_FIRMWARE_VOLUME_HEADER *mFvBaseAddrBuffer[FV_COUNT];

STATIC UINT8  *mFdBuffer;
STATIC UINTN  mFdBufferSize;
STATIC EFI_FIRMWARE_VOLUME_HEADER  *mParsedFvBuffer[FV_COUNT];
STATIC UINTN  mParsedFvCount;

STATIC
VOID
ParseFvHeader (
  IN EFI_FIRMWARE_VOLUME_HEADER  *FvHeader,
  IN UINTN                       AvailableSize,
  IN UINTN                       Depth
  );

typedef struct {
  UINTN StartOffset;
  UINT8 Color;
} RAW_COLOR_ENTRY;

STATIC CONST RAW_COLOR_ENTRY mFvColorMap[] = {
  { OFFSET_OF(EFI_FIRMWARE_VOLUME_HEADER, ZeroVector),      WHITE_TEXT      },
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
  { sizeof(EFI_FIRMWARE_VOLUME_HEADER),                     WHITE_TEXT      },
  { MAX_UINTN,                                              WHITE_TEXT      }
};

STATIC CONST RAW_COLOR_ENTRY mFfsColorMap[] = {
  { OFFSET_OF(EFI_FFS_FILE_HEADER, Name),                   LIGHTORANGE_TEXT},
  { OFFSET_OF(EFI_FFS_FILE_HEADER, IntegrityCheck),         BLUE_TEXT       },
  { OFFSET_OF(EFI_FFS_FILE_HEADER, Type),                   GREEN_TEXT      },
  { OFFSET_OF(EFI_FFS_FILE_HEADER, Attributes),             RED_TEXT        },
  { OFFSET_OF(EFI_FFS_FILE_HEADER, Size),                   CYAN_TEXT       },
  { OFFSET_OF(EFI_FFS_FILE_HEADER, State),                  PURPLE_TEXT     },
  { MAX_UINTN,                                              WHITE_TEXT      }
};

UINT8
GetColorByOffset (
  UINTN Offset,
  CONST RAW_COLOR_ENTRY *Map
  )
{
  UINTN Index = 0;

  while (Map[Index + 1].StartOffset != MAX_UINTN &&
         Offset >= Map[Index + 1].StartOffset) {
    Index++;
  }

  return Map[Index].Color;
}

VOID ShowRawData(
  UINT8                     *Buffer, 
  UINT64                    Size,
  UINTN                     Depth,
  IN CONST RAW_COLOR_ENTRY  *ColorMap
  )
{
  UINT64 Offset;
  UINTN  Index;
  UINTN  LineSize;
  UINT8  Data;
  UINTN  ColorIndex;

  ColorIndex = 0;
  
  for (Offset = 0; Offset < Size; Offset += 16) {

    LineSize = (UINTN)((Size - Offset >= 16) ? 16 : (Size - Offset));

    SetScreenColor (ORANGE_TEXT);
    INDENT_PRINTF (Depth, "%08llx:", Offset);

    SetScreenColor (WHITE_TEXT);

    //
    // Hex
    //
    for (Index = 0; Index < 16; Index++) {
      if (Index < LineSize) {
        if (ColorMap != NULL) {
          SetScreenColor(GetColorByOffset(Offset + Index, ColorMap));
        }
        printf (" %02x", Buffer[Offset + Index]);
      } else {
        printf ("   ");
      }
    }

    printf ("  ");

    //
    // ASCII
    //
    for (Index = 0; Index < LineSize; Index++) {
      if (ColorMap != NULL) {
        SetScreenColor(GetColorByOffset(Offset + Index, ColorMap));
      }
      Data = Buffer[Offset + Index];
      if ((Data >= 0x20) && (Data <= 0x7E)) {
        printf ("%c", Data);
      } else {
        printf (".");
      }
    }

    printf ("\n");
  }
  SetScreenColor (WHITE_TEXT);
}

VOID PrintGuid (EFI_GUID *Guid)
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

STATIC
CONST CHAR8 *
ParseFvFileTypeWorker (
  IN EFI_FV_FILETYPE  Type
  )
{
  switch (Type) {
    case EFI_FV_FILETYPE_RAW:
      return "Raw";
    case EFI_FV_FILETYPE_FREEFORM:
      return "FreeForm";
    case EFI_FV_FILETYPE_SECURITY_CORE:
      return "SEC Core";
    case EFI_FV_FILETYPE_PEI_CORE:
      return "PEI Core";
    case EFI_FV_FILETYPE_DXE_CORE:
      return "DXE Core";
    case EFI_FV_FILETYPE_PEIM:
      return "PEIM";
    case EFI_FV_FILETYPE_DRIVER:
      return "Driver";
    case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
      return "Combined PEIM Driver";
    case EFI_FV_FILETYPE_APPLICATION:
      return "Application";
    case EFI_FV_FILETYPE_MM:
      return "MM";
    case EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE:
      return "FV Image";
    case EFI_FV_FILETYPE_COMBINED_MM_DXE:
      return "Combined MM DXE";
    case EFI_FV_FILETYPE_MM_CORE:
      return "MM Core";
    case EFI_FV_FILETYPE_MM_STANDALONE:
      return "MM Stand alone";
    case EFI_FV_FILETYPE_MM_CORE_STANDALONE:
      return "MM Core Stand alone";
    case EFI_FV_FILETYPE_FFS_PAD:
      return "Pad File";
    default:
      return "Unknown";
  }
}

STATIC
CONST CHAR8 *
ParseSectionTypeWorker (
  IN EFI_SECTION_TYPE  Type
  )
{
  switch (Type) {
    case EFI_SECTION_COMPRESSION:
      return "Compression";
    case EFI_SECTION_GUID_DEFINED:
      return "GUID Defined";
    case EFI_SECTION_DISPOSABLE:
      return "Disposable";
    case EFI_SECTION_PE32:
      return "PE32";
    case EFI_SECTION_PIC:
      return "PIC";
    case EFI_SECTION_TE:
      return "TE";
    case EFI_SECTION_DXE_DEPEX:
      return "DXE Dependency";
    case EFI_SECTION_VERSION:
      return "Version";
    case EFI_SECTION_USER_INTERFACE:
      return "User Interface";
    case EFI_SECTION_COMPATIBILITY16:
      return "Compatibility16";
    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:
      return "Firmware Volume Image";
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID:
      return "Freeform Subtype GUID";
    case EFI_SECTION_RAW:
      return "Raw";
    case EFI_SECTION_PEI_DEPEX:
      return "PEI Dependency";
    case EFI_SECTION_MM_DEPEX:
      return "MM Dependency";
    default:
      return "Unknown";
  }
}

STATIC
UINT32
GetSectionSize (
  IN EFI_COMMON_SECTION_HEADER  *Section
  )
{
  return IS_SECTION2 (Section) ? SECTION2_SIZE (Section) : SECTION_SIZE (Section);
}

STATIC
UINTN
GetSectionHeaderSize (
  IN EFI_COMMON_SECTION_HEADER  *Section
  )
{
  return IS_SECTION2 (Section) ? sizeof (EFI_COMMON_SECTION_HEADER2) : sizeof (EFI_COMMON_SECTION_HEADER);
}

STATIC
BOOLEAN
IsFvInFdBuffer (
  IN EFI_FIRMWARE_VOLUME_HEADER  *FvHeader
  )
{
  UINTN  Address;
  UINTN  BufferAddress;

  Address       = (UINTN)FvHeader;
  BufferAddress = (UINTN)mFdBuffer;
  return (BOOLEAN)(
                    (mFdBuffer != NULL) &&
                    (Address >= BufferAddress) &&
                    (Address - BufferAddress < mFdBufferSize)
                    );
}

STATIC
BOOLEAN
IsFvAlreadyParsed (
  IN EFI_FIRMWARE_VOLUME_HEADER  *FvHeader
  )
{
  UINTN  Index;

  if (!IsFvInFdBuffer (FvHeader)) {
    return FALSE;
  }

  for (Index = 0; Index < mParsedFvCount; Index++) {
    if (mParsedFvBuffer[Index] == FvHeader) {
      return TRUE;
    }
  }

  if (mParsedFvCount < ARRAY_SIZE (mParsedFvBuffer)) {
    mParsedFvBuffer[mParsedFvCount++] = FvHeader;
  }

  return FALSE;
}

STATIC
VOID
ParseSections (
  IN UINT8  *Sections,
  IN UINTN  SectionsSize,
  IN UINTN  Depth
  )
{
  EFI_COMMON_SECTION_HEADER  *Section;
  EFI_COMPRESSION_SECTION    *CompressionSection;
  EFI_COMPRESSION_SECTION2   *CompressionSection2;
  EFI_GUID_DEFINED_SECTION   *GuidSection;
  EFI_GUID_DEFINED_SECTION2  *GuidSection2;
  RETURN_STATUS              Status;
  UINT8                      *SectionData;
  UINT8                      *UncompressedBuffer;
  VOID                       *ScratchBuffer;
  UINT32                     SectionSize;
  UINT32                     UncompressedSize;
  UINT32                     DestinationSize;
  UINT32                     ScratchSize;
  UINTN                      SectionHeaderSize;
  UINTN                      EncapsulationHeaderSize;
  UINTN                      OccupiedSize;
  UINTN                      Offset;
  UINT16                     DataOffset;
  UINT16                     Attributes;
  UINT8                      CompressionType;
  EFI_GUID                   *SectionDefinitionGuid;

  if (Depth > MAX_SECTION_NESTING_DEPTH) {
    printf ("Section nesting exceeds the supported depth of %d.\n", MAX_SECTION_NESTING_DEPTH);
    return;
  }

  INDENT_PRINTF (Depth, "--------------------------- Sections Start ---------------------------\n");
  Offset = 0;
  while (Offset + sizeof (EFI_COMMON_SECTION_HEADER) <= SectionsSize) {
    Section = (EFI_COMMON_SECTION_HEADER *)(Sections + Offset);
    if (IS_SECTION2 (Section) &&
        (Offset + sizeof (EFI_COMMON_SECTION_HEADER2) > SectionsSize)) {
      printf ("Invalid extended section header at offset 0x%llx.\n", (UINT64)Offset);
      return;
    }

    SectionSize       = GetSectionSize (Section);
    SectionHeaderSize = GetSectionHeaderSize (Section);
    if ((SectionSize < SectionHeaderSize) || (SectionSize > SectionsSize - Offset)) {
      printf ("Invalid section size 0x%x at offset 0x%llx.\n", SectionSize, (UINT64)Offset);
      return;
    }

    INDENT_PRINTF (
      Depth, 
      "Section offset %#12llx, size %#10x, type %#4x (%s)\n",
      (UINT64)Offset,
      SectionSize,
      Section->Type,
      ParseSectionTypeWorker (Section->Type)
      );
    //ShowRawData ((UINT8*)Section, SectionSize, Depth, NULL);

    SectionData = (UINT8 *)Section + SectionHeaderSize;
    if (Section->Type == EFI_SECTION_FIRMWARE_VOLUME_IMAGE) {
      if ((SectionSize - SectionHeaderSize >= sizeof (EFI_FIRMWARE_VOLUME_HEADER)) &&
          (((EFI_FIRMWARE_VOLUME_HEADER *)SectionData)->Signature == EFI_FVH_SIGNATURE)) {
        ParseFvHeader (
          (EFI_FIRMWARE_VOLUME_HEADER *)SectionData,
          SectionSize - SectionHeaderSize,
          Depth + 1
          );
      } else {
        printf ("%*sInvalid FV image section.\n", (int)((Depth + 1) * 2), "");
      }
    } else if (Section->Type == EFI_SECTION_COMPRESSION) {
      if (IS_SECTION2 (Section)) {
        EncapsulationHeaderSize = sizeof (EFI_COMPRESSION_SECTION2);
        if (SectionSize < EncapsulationHeaderSize) {
          printf ("%*sInvalid compression section header.\n", (int)((Depth + 1) * 2), "");
          return;
        }

        CompressionSection2 = (EFI_COMPRESSION_SECTION2 *)Section;
        UncompressedSize     = CompressionSection2->UncompressedLength;
        CompressionType      = CompressionSection2->CompressionType;
      } else {
        EncapsulationHeaderSize = sizeof (EFI_COMPRESSION_SECTION);
        if (SectionSize < EncapsulationHeaderSize) {
          printf ("%*sInvalid compression section header.\n", (int)((Depth + 1) * 2), "");
          return;
        }

        CompressionSection = (EFI_COMPRESSION_SECTION *)Section;
        UncompressedSize   = CompressionSection->UncompressedLength;
        CompressionType    = CompressionSection->CompressionType;
      }

      SectionData = (UINT8 *)Section + EncapsulationHeaderSize;
      printf (
        "%*sCompression type %u, uncompressed size 0x%x\n",
        (int)((Depth + 1) * 2),
        "",
        CompressionType,
        UncompressedSize
        );

      if (CompressionType == EFI_NOT_COMPRESSED) {
        if (UncompressedSize != SectionSize - EncapsulationHeaderSize) {
          printf ("%*sUncompressed section length mismatch.\n", (int)((Depth + 1) * 2), "");
        } else {
          ParseSections (SectionData, UncompressedSize, Depth + 1);
        }
      } else if (CompressionType == EFI_STANDARD_COMPRESSION) {
        DestinationSize = 0;
        ScratchSize     = 0;
        Status = UefiDecompressGetInfo (
                   SectionData,
                   SectionSize - (UINT32)EncapsulationHeaderSize,
                   &DestinationSize,
                   &ScratchSize
                   );
        if (RETURN_ERROR (Status) || (DestinationSize != UncompressedSize)) {
          printf ("%*sUnable to get valid UEFI decompression information.\n", (int)((Depth + 1) * 2), "");
        } else {
          UncompressedBuffer = malloc (DestinationSize);
          ScratchBuffer      = (ScratchSize == 0) ? NULL : malloc (ScratchSize);
          if ((UncompressedBuffer == NULL) || ((ScratchSize != 0) && (ScratchBuffer == NULL))) {
            printf ("%*sUnable to allocate decompression buffers.\n", (int)((Depth + 1) * 2), "");
          } else {
            Status = UefiDecompress (SectionData, UncompressedBuffer, ScratchBuffer);
            if (RETURN_ERROR (Status)) {
              printf ("%*sUEFI decompression failed.\n", (int)((Depth + 1) * 2), "");
            } else {
              ParseSections (UncompressedBuffer, DestinationSize, Depth + 1);
            }
          }

          free (ScratchBuffer);
          free (UncompressedBuffer);
        }
      } else {
        printf ("%*sUnsupported compression type 0x%02x.\n", (int)((Depth + 1) * 2), "", CompressionType);
      }
    } else if (Section->Type == EFI_SECTION_GUID_DEFINED) {
      if (IS_SECTION2 (Section)) {
        GuidSection2 = (EFI_GUID_DEFINED_SECTION2 *)Section;
        DataOffset            = GuidSection2->DataOffset;
        Attributes            = GuidSection2->Attributes;
        SectionDefinitionGuid = &GuidSection2->SectionDefinitionGuid;
      } else {
        GuidSection = (EFI_GUID_DEFINED_SECTION *)Section;
        DataOffset            = GuidSection->DataOffset;
        Attributes            = GuidSection->Attributes;
        SectionDefinitionGuid = &GuidSection->SectionDefinitionGuid;
      }

      if ((DataOffset < SectionHeaderSize) || (DataOffset > SectionSize)) {
        printf ("%*sInvalid GUID-defined section data offset.\n", (int)((Depth + 1) * 2), "");
      } else if (CompareGuid (SectionDefinitionGuid, &gLzmaCustomDecompressGuid)) {
        DestinationSize = 0;
        ScratchSize     = 0;
        SectionData     = (UINT8 *)Section + DataOffset;
        Status = LzmaUefiDecompressGetInfo (
                   SectionData,
                   SectionSize - DataOffset,
                   &DestinationSize,
                   &ScratchSize
                   );
        printf (
          "%*sLZMA guided section, uncompressed size 0x%x (compress size 0x%x)\n",
          (int)((Depth + 1) * 2),
          "",
          DestinationSize,
          SectionSize - DataOffset
          );
        if (RETURN_ERROR (Status)) {
          printf ("%*sUnable to get LZMA decompression information.\n", (int)((Depth + 1) * 2), "");
        } else {
          UncompressedBuffer = calloc (DestinationSize, sizeof(UINT8));
          ScratchBuffer      = (ScratchSize == 0) ? NULL : malloc (ScratchSize);
          if ((UncompressedBuffer == NULL) || ((ScratchSize != 0) && (ScratchBuffer == NULL))) {
            printf ("%*sUnable to allocate LZMA decompression buffers.\n", (int)((Depth + 1) * 2), "");
          } else {
            Status = LzmaUefiDecompress (
                       SectionData,
                       SectionSize - DataOffset,
                       UncompressedBuffer,
                       ScratchBuffer
                       );
            if (RETURN_ERROR (Status)) {
              printf ("%*sLZMA decompression failed.\n", (int)((Depth + 1) * 2), "");
            } else {
              printf ("%*sLZMA decompression Success.\n", (int)((Depth + 1) * 2), "");
              ParseSections (UncompressedBuffer, DestinationSize, Depth + 1);
            }
          }

          free (ScratchBuffer);
          free (UncompressedBuffer);
        }
      } else if ((Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0) {
        ParseSections ((UINT8 *)Section + DataOffset, SectionSize - DataOffset, Depth + 1);
      } else {
        printf ("%*sGUID-defined section requires an extraction handler; skipped.\n", (int)((Depth + 1) * 2), "");
      }
    }

    OccupiedSize = ALIGN_VALUE ((UINTN)SectionSize, 4);
    if ((OccupiedSize < SectionSize) || (OccupiedSize > SectionsSize - Offset)) {
      INDENT_PRINTF (Depth, "--------------------------- Sections End ---------------------------\n");
      return;
    }

    Offset += OccupiedSize;
  }
  INDENT_PRINTF (Depth, "--------------------------- Sections End ---------------------------\n");
}

STATIC
VOID
ParseFfsSections (
  IN EFI_FFS_FILE_HEADER  *FfsFileHeader,
  IN UINTN                Depth
  )
{
  UINT32  FileSize;
  UINTN   FileHeaderSize;

  if ((FfsFileHeader->Type == EFI_FV_FILETYPE_RAW) ||
      (FfsFileHeader->Type == EFI_FV_FILETYPE_FFS_PAD)) {
    return;
  }

  FileHeaderSize = IS_FFS_FILE2 (FfsFileHeader) ? sizeof (EFI_FFS_FILE_HEADER2) : sizeof (EFI_FFS_FILE_HEADER);
  FileSize       = IS_FFS_FILE2 (FfsFileHeader) ? FFS_FILE2_SIZE (FfsFileHeader) : FFS_FILE_SIZE (FfsFileHeader);
  if (FileSize <= FileHeaderSize) {
    return;
  }

  ParseSections ((UINT8 *)FfsFileHeader + FileHeaderSize, FileSize - FileHeaderSize, Depth);
}

VOID
FfsInfoPrintWorker (
  EFI_FFS_FILE_HEADER  *FfsFileHeader,
  IN UINTN             Depth
  )
{
  UINTN   FileHeaderSize;
  FileHeaderSize = IS_FFS_FILE2 (FfsFileHeader) ? sizeof (EFI_FFS_FILE_HEADER2) : sizeof (EFI_FFS_FILE_HEADER);

  ShowRawData ((UINT8*)FfsFileHeader, FileHeaderSize, Depth, mFfsColorMap);
  INDENT_PRINTF (Depth, "\n");

  INDENT_PRINTF (Depth, "File Name(GUID): ");
  SetScreenColor (LIGHTORANGE_TEXT);
  PrintGuid (&FfsFileHeader->Name);
  if (CompareGuid (&FfsFileHeader->Name, &gEfiFirmwareVolumeTopFileGuid)) {
    SetScreenColor (RED_TEXT);
    INDENT_PRINTF (Depth, "  Volume Top File Guid");
    SetScreenColor (WHITE_TEXT);
  }
  if (CompareGuid (&FfsFileHeader->Name, &gFspHeaderFileGuid)) {
    SetScreenColor (CYAN_TEXT);
    INDENT_PRINTF (Depth, "  Firmware Support Package FFS Guid");
    SetScreenColor (WHITE_TEXT);
    INDENT_PRINTF (Depth, "\n");
  }
  SetScreenColor (WHITE_TEXT);
  INDENT_PRINTF (Depth, "\n");

  INDENT_PRINTF (Depth, "Integrity Check                 = 0x%04x\n", FfsFileHeader->IntegrityCheck.Checksum16);
  INDENT_PRINTF (Depth, "Type                            = 0x%02x (%s)\n", FfsFileHeader->Type, ParseFvFileTypeWorker(FfsFileHeader->Type));
  INDENT_PRINTF (Depth, "Attribute                       = 0x%02x\n", FfsFileHeader->Attributes);
  INDENT_PRINTF (Depth, "FFS Size (including FFS Header) = 0x%x (%d)\n", FFS_FILE_SIZE(FfsFileHeader), FFS_FILE_SIZE(FfsFileHeader));
  INDENT_PRINTF (Depth, "State                           = 0x%02x\n", FfsFileHeader->State);
//  INDENT_PRINTF (Depth, "\n");

  if (CompareGuid (&FfsFileHeader->Name, &gFspHeaderFileGuid)) {
    FSP_INFO_HEADER *FspInfoHeader;
    FspInfoHeader = (FSP_INFO_HEADER*)((UINT8*)FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER) + 4);
    ShowRawData ((UINT8*)FspInfoHeader, FFS_FILE_SIZE(FfsFileHeader) - sizeof (EFI_FFS_FILE_HEADER) - 4, Depth, NULL);
    INDENT_PRINTF (Depth, "Signature                       = 0x%x\n", FspInfoHeader->Signature);
    INDENT_PRINTF (Depth, "HeaderLength                    = 0x%x\n", FspInfoHeader->HeaderLength);
    INDENT_PRINTF (Depth, "SpecVersion                     = 0x%x\n", FspInfoHeader->SpecVersion);
    INDENT_PRINTF (Depth, "HeaderRevision                  = 0x%x\n", FspInfoHeader->HeaderRevision);
    INDENT_PRINTF (Depth, "ImageRevision                   = 0x%x\n", FspInfoHeader->ImageRevision);
    INDENT_PRINTF (Depth, "ImageId                         = \n");
    ShowRawData ((UINT8*)&FspInfoHeader->ImageId, 8, Depth, NULL);
    INDENT_PRINTF (Depth, "ImageSize                       = 0x%x\n", FspInfoHeader->ImageSize);
    INDENT_PRINTF (Depth, "ImageBase                       = 0x%x\n", FspInfoHeader->ImageBase);
    INDENT_PRINTF (Depth, "ImageAttribute                  = 0x%x\n", FspInfoHeader->ImageAttribute);
    INDENT_PRINTF (Depth, "ComponentAttribute              = 0x%x\n", FspInfoHeader->ComponentAttribute);
    INDENT_PRINTF (Depth, "CfgRegionOffset                 = 0x%x\n", FspInfoHeader->CfgRegionOffset);
    INDENT_PRINTF (Depth, "CfgRegionSize                   = 0x%x\n", FspInfoHeader->CfgRegionSize);
    INDENT_PRINTF (Depth, "TempRamInitEntryOffset          = 0x%x\n", FspInfoHeader->TempRamInitEntryOffset);
    INDENT_PRINTF (Depth, "NotifyPhaseEntryOffset          = 0x%x\n", FspInfoHeader->NotifyPhaseEntryOffset);
    INDENT_PRINTF (Depth, "FspMemoryInitEntryOffset        = 0x%x\n", FspInfoHeader->FspMemoryInitEntryOffset);
    INDENT_PRINTF (Depth, "TempRamExitEntryOffset          = 0x%x\n", FspInfoHeader->TempRamExitEntryOffset);
    INDENT_PRINTF (Depth, "FspSiliconInitEntryOffset       = 0x%x\n", FspInfoHeader->FspSiliconInitEntryOffset);
    INDENT_PRINTF (Depth, "FspMultiPhaseSiInitEntryOffset  = 0x%x\n", FspInfoHeader->FspMultiPhaseSiInitEntryOffset);

    INDENT_PRINTF (Depth, "ExtendedImageRevision           = 0x%x\n", FspInfoHeader->ExtendedImageRevision);
//    printf ("Signature = 0x%x\n", FspInfoHeader->);

  }
}

VOID ParseFfsHeader (
  EFI_FFS_FILE_HEADER  *FfsFileHeader,
  UINTN                Depth
  ) 
{
  FfsInfoPrintWorker (FfsFileHeader, Depth);

  ParseFfsSections (FfsFileHeader, Depth);

  INDENT_PRINTF (Depth, "\n");
}

VOID
FvInfoPrintWorker (
  IN EFI_FIRMWARE_VOLUME_HEADER  *FvHeader,
  IN UINTN                       Depth
  )
{
  int i;
  UINT8	  *data;

  ShowRawData ((UINT8*)FvHeader, FvHeader->HeaderLength, Depth, mFvColorMap);
  INDENT_PRINTF (Depth, "\n");

  INDENT_PRINTF (Depth, "Zero Vector:");
  for(i = 0 ; i < 16 ; i++) printf ("%02x ",FvHeader->ZeroVector[i]);
  INDENT_PRINTF (Depth, "\n");
  INDENT_PRINTF (Depth, "File System Guid     = ");
  SetScreenColor (BLUE_TEXT);
  PrintGuid (&FvHeader->FileSystemGuid);
  if (CompareGuid (&FvHeader->FileSystemGuid, &gEfiFirmwareFileSystem2Guid)) {
    SetScreenColor (LIGHTBLUE_TEXT);
    printf ("  (gEfiFirmwareFileSystem2Guid)");
  } else if (CompareGuid (&FvHeader->FileSystemGuid, &gEfiFirmwareFileSystem3Guid)) {
    SetScreenColor (LIGHTBLUE_TEXT);
    printf ("  (gEfiFirmwareFileSystem3Guid)");
  }
  INDENT_PRINTF (Depth, "\n");
  SetScreenColor (WHITE_TEXT);

  INDENT_PRINTF (Depth, "FV Length            = 0x%llx (%lld)\n", FvHeader->FvLength, FvHeader->FvLength);	

  data = (UINT8*)&FvHeader->Signature;
  INDENT_PRINTF (Depth, "Signature            = %c%c%c%c\n", *data, *(data+1), *(data+2), *(data+3));
  INDENT_PRINTF (Depth, "FV Attribute         = 0x%08x\n", FvHeader->Attributes);
  INDENT_PRINTF (Depth, "Header Length        = 0x%x (%d)\n", FvHeader->HeaderLength, FvHeader->HeaderLength);
  INDENT_PRINTF (Depth, "Check Sum            = 0x%04x\n", FvHeader->Checksum);
  INDENT_PRINTF (Depth, "Extend Header Offset = 0x%04x\n", FvHeader->ExtHeaderOffset);
  INDENT_PRINTF (Depth, "Revision             = 0x%02x\n", FvHeader->Revision);
  INDENT_PRINTF (Depth, "Number of Block      = 0x%08x\n", FvHeader->BlockMap[0].NumBlocks);
  INDENT_PRINTF (Depth, "Block Length         = 0x%08x\n", FvHeader->BlockMap[0].Length);
  INDENT_PRINTF (Depth, "\n");

  EFI_FIRMWARE_VOLUME_EXT_HEADER  *FvExtHeader;
  FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER*)((UINT8*)FvHeader + FvHeader->ExtHeaderOffset);
  INDENT_PRINTF (Depth, "FV Name Guid         = ");
  SetScreenColor (LIGHTGREEN_TEXT);
  PrintGuid (&FvExtHeader->FvName);
  SetScreenColor (WHITE_TEXT);
  INDENT_PRINTF (Depth, "\n\n");
}

STATIC
VOID
ParseFvHeader (
  IN EFI_FIRMWARE_VOLUME_HEADER  *FvHeader,
  IN UINTN                       AvailableSize,
  IN UINTN                       Depth
  )
{

  if ((AvailableSize < sizeof (EFI_FIRMWARE_VOLUME_HEADER)) ||
      (FvHeader->Signature != EFI_FVH_SIGNATURE) ||
      (FvHeader->HeaderLength < sizeof (EFI_FIRMWARE_VOLUME_HEADER)) ||
      (FvHeader->FvLength > AvailableSize) ||
      (FvHeader->HeaderLength > FvHeader->FvLength)) {
    printf ("%*sInvalid or truncated FV header.\n", (int)(Depth * 2), "");
    return;
  }

  if (IsFvAlreadyParsed (FvHeader)) {
    printf ("%*sFV at %p was already parsed; skipping duplicate.\n", (int)(Depth * 2), "", FvHeader);
    return;
  }

  INDENT_PRINTF (Depth, "Start to Parse FV Header\n");
  FvInfoPrintWorker (FvHeader, Depth);

//  printf (DEBUG_STR"%p\n", FvHeader);
//  printf (DEBUG_STR"%p\n", (UINT8*)FvHeader + FvHeader->HeaderLength);

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
      INDENT_PRINTF (Depth, "FFS offset: 0x%llx\n", (UINT64)((UINT8 *)FileHeader - (UINT8 *)FvHeader));
      ParseFfsHeader (FileHeader, Depth);
    }
  } while (!EFI_ERROR (Status));
}

void SearchFvHeaderAddress (UINT8 *Buffer, int BufferSize, int *NumberOfFvHeader)
{
  int i;
  int Count = 0;

  UINT32                     *SigPointer;
  EFI_FIRMWARE_VOLUME_HEADER *FvHeader;

  Count = 0;
  for(i = 0 ; i < BufferSize - sizeof(UINT32) ; i += (sizeof(UINT8) / sizeof(UINT8))) {
    if (i < OFFSET_OF(EFI_FIRMWARE_VOLUME_HEADER, Signature))
        continue;

    SigPointer = (UINT32*)&Buffer[i];
//    FvHeader = BASE_CR(&Buffer[i], EFI_FIRMWARE_VOLUME_HEADER, Signature);
    if (*SigPointer == EFI_FVH_SIGNATURE) {
      FvHeader = BASE_CR(&Buffer[i], EFI_FIRMWARE_VOLUME_HEADER, Signature);
      if (CompareGuid (&FvHeader->FileSystemGuid, &gEfiFirmwareFileSystem2Guid) ||
          CompareGuid (&FvHeader->FileSystemGuid, &gEfiFirmwareFileSystem3Guid)) {
        mFvBaseAddrBuffer[Count] = (EFI_FIRMWARE_VOLUME_HEADER*) FvHeader;
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

  mFdBuffer     = FvRawData;
  mFdBufferSize = FvRawSize;
  SearchFvHeaderAddress (FvRawData, FvRawSize, &FirmwareVolumeCount);

  for (i = 0 ; i < FirmwareVolumeCount ; i++) {
    printf ("[FV index %d ROM Address: 0x%llx]\n", i, (UINT64)(mFvBaseAddrBuffer[i] - mFvBaseAddrBuffer[0]));
    ParseFvHeader (
      (EFI_FIRMWARE_VOLUME_HEADER *)mFvBaseAddrBuffer[i],
      FvRawSize - (UINTN)((UINT8 *)mFvBaseAddrBuffer[i] - FvRawData),
      0
      );
  }

  printf ("Done to Parse FV file %s\n", argv[1]);

  free(FvRawData);
}
