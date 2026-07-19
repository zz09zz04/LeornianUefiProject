/** @file
  Dump Hex library instance base on DebugLib and PrintLib.
  It uses PrintLib/DebugLib to dump memory buffer to graphics console or serial port device .

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Base.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
//#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DumpHexLib.h>

static CONST CHAR8  mHex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

/**
  Dump some hexadecimal data to the screen.

  @param[in] Indent     How many spaces to indent the output.
  @param[in] Offset     The offset of the printing.
  @param[in] DataSize   The size in bytes of UserData.
  @param[in] UserData   The data to print out.
**/
VOID
CommonDumpHex (
  IN UINTN    ErrorLevel,
  IN BOOLEAN  GraphicsConsolePrint,
  IN UINTN    Indent,
  IN UINTN    Offset,
  IN UINTN    DataSize,
  IN VOID     *UserData
  )
{
  UINT8  *Data;

  CHAR8  Val[50];

  CHAR8  Str[20];

  UINT8  TempByte;
  UINTN  Size;
  UINTN  Index;

  Data = UserData;
  while (DataSize != 0) {
    Size = 16;
    if (Size > DataSize) {
      Size = DataSize;
    }

    for (Index = 0; Index < Size; Index += 1) {
      TempByte           = Data[Index];
      Val[Index * 3 + 0] = mHex[TempByte >> 4];
      Val[Index * 3 + 1] = mHex[TempByte & 0xF];
      Val[Index * 3 + 2] = (CHAR8)((Index == 7) ? '-' : ' ');
      Str[Index]         = (CHAR8)((TempByte < ' ' || TempByte > 'z') ? '.' : TempByte);
    }

    Val[Index * 3] = 0;
    Str[Index]     = 0;
    if (GraphicsConsolePrint) {
      Print (L"%*a%08X: %-48a *%a*\r\n", Indent, "", Offset, Val, Str);
    }
    DEBUG((ErrorLevel, "%*a%08X: %-48a *%a*\r\n", Indent, "", Offset, Val, Str));

    Data     += Size;
    Offset   += Size;
    DataSize -= Size;
  }
}
