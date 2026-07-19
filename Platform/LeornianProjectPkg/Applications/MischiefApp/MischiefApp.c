/** @file
  This sample application bases on UefiLib CreatePopUp to enhance CreateRandomPopUp
  to print "WARNING!!!" to the UEFI Console.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Draws a dialog box to the console output device specified by
  ConOut defined in the EFI_SYSTEM_TABLE and waits for a keystroke
  from the console input device specified by ConIn defined in the
  EFI_SYSTEM_TABLE.

  If there are no strings in the variable argument list, then ASSERT().
  If all the strings in the variable argument list are empty, then ASSERT().

  @param[in]   Attribute  Specifies the foreground and background color of the popup.
  @param[out]  Key        A pointer to the EFI_KEY value of the key that was
                          pressed.  This is an optional parameter that may be NULL.
                          If it is NULL then no wait for a keypress will be performed.
  @param[in]  ...         The variable argument list that contains pointers to Null-
                          terminated Unicode strings to display in the dialog box.
                          The variable argument list is terminated by a NULL.

**/
VOID
EFIAPI
CreateRandomPopUp (
  IN  UINTN          Attribute,
  OUT EFI_INPUT_KEY  *Key       OPTIONAL,
  ...
  );

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
MischiefAppEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT32           Index;

  for (Index = 0 ; Index < 500 ; Index++) {
    CreateRandomPopUp (
      EFI_YELLOW | EFI_BACKGROUND_RED,
      NULL,
      L"",
      L"",
      L"       WARNING!!!       ",
      L"",
      L"",
      NULL
      );

    gBS->Stall(50000);
  }

//  SystemTable->ConOut->ClearScreen (SystemTable->ConOut);

  return EFI_SUCCESS;
}
