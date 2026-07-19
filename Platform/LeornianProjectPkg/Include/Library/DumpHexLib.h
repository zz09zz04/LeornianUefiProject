/** @file
  Dump Hex library instance base on DebugLib and PrintLib.
  It uses PrintLib/DebugLib to dump memory buffer to graphics console or serial port device .

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

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
  );

#define DEBUG_DUMP_HEX  CommonDumpHex