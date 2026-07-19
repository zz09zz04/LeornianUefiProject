#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/RngLib.h>

#define NARROW_CHAR  0xFFF0
#define WIDE_CHAR    0xFFF1

/**
  Count the storage space of a Unicode string.

  This function handles the Unicode string with NARROW_CHAR
  and WIDE_CHAR control characters. NARROW_HCAR and WIDE_CHAR
  does not count in the resultant output. If a WIDE_CHAR is
  hit, then 2 Unicode character will consume an output storage
  space with size of CHAR16 till a NARROW_CHAR is hit.

  @param String          The input string to be counted.
  @param LimitLen        Whether need to limit the string length.
  @param MaxWidth        The max length this function supported.
  @param Offset          The max index of the string can be show out.

  @return Storage space for the input string.

**/
UINTN

InternalGetStringWidth (
  IN  CHAR16   *String,
  IN  BOOLEAN  LimitLen,
  IN  UINTN    MaxWidth,
  OUT UINTN    *Offset
  )
{
  UINTN  Index;
  UINTN  Count;
  UINTN  IncrementValue;

  if (String == NULL) {
    return 0;
  }

  Index          = 0;
  Count          = 0;
  IncrementValue = 1;

  do {
    //
    // Advance to the null-terminator or to the first width directive
    //
    for ( ; (String[Index] != NARROW_CHAR) && (String[Index] != WIDE_CHAR) && (String[Index] != 0); Index++) {
      Count = Count + IncrementValue;

      if (LimitLen && (Count > MaxWidth)) {
        break;
      }
    }

    //
    // We hit the null-terminator, we now have a count
    //
    if (String[Index] == 0) {
      break;
    }

    if (LimitLen && (Count > MaxWidth)) {
      *Offset = Index;
      break;
    }

    //
    // We encountered a narrow directive - strip it from the size calculation since it doesn't get printed
    // and also set the flag that determines what we increment by.(if narrow, increment by 1, if wide increment by 2)
    //
    if (String[Index] == NARROW_CHAR) {
      //
      // Skip to the next character
      //
      Index++;
      IncrementValue = 1;
    } else {
      //
      // Skip to the next character
      //
      Index++;
      IncrementValue = 2;
    }
  } while (String[Index] != 0);

  return Count * sizeof (CHAR16);
}

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
  )
{
  EFI_STATUS                       Status;
  VA_LIST                          Args;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *ConOut;
  EFI_SIMPLE_TEXT_OUTPUT_MODE      SavedConsoleMode;
  UINTN                            Columns;
  UINTN                            Rows;
  UINTN                            Column;
  UINTN                            Row;
  UINTN                            NumberOfLines;
  UINTN                            MaxLength;
  CHAR16                           *String;
  UINTN                            Length;
  CHAR16                           *Line;
  UINTN                            EventIndex;
  CHAR16                           *TmpString;
  UINT16                           RandX;
  UINT16                           RandY;

  //
  // Determine the length of the longest line in the popup and the the total
  // number of lines in the popup
  //
  VA_START (Args, Key);
  MaxLength     = 0;
  NumberOfLines = 0;
  while ((String = VA_ARG (Args, CHAR16 *)) != NULL) {
    MaxLength = MAX (MaxLength, InternalGetStringWidth (String, FALSE, 0, NULL) / 2);
    NumberOfLines++;
  }

  VA_END (Args);

  //
  // If the total number of lines in the popup is zero, then ASSERT()
  //
  ASSERT (NumberOfLines != 0);

  //
  // If the maximum length of all the strings is zero, then ASSERT()
  //
  ASSERT (MaxLength != 0);

  //
  // Cache a pointer to the Simple Text Output Protocol in the EFI System Table
  //
  ConOut = gST->ConOut;

  //
  // Save the current console cursor position and attributes
  //
  CopyMem (&SavedConsoleMode, ConOut->Mode, sizeof (SavedConsoleMode));

  //
  // Retrieve the number of columns and rows in the current console mode
  //
  ConOut->QueryMode (ConOut, SavedConsoleMode.Mode, &Columns, &Rows);

  //
  // Disable cursor and set the foreground and background colors specified by Attribute
  //
  ConOut->EnableCursor (ConOut, FALSE);
  ConOut->SetAttribute (ConOut, Attribute);

  //
  // Limit NumberOfLines to height of the screen minus 3 rows for the box itself
  //
  NumberOfLines = MIN (NumberOfLines, Rows - 3);

  //
  // Limit MaxLength to width of the screen minus 2 columns for the box itself
  //
  MaxLength = MIN (MaxLength, Columns - 2);

  //
  // Compute the starting row and starting column for the popup
  //
  if (GetRandomNumber16 (&RandX) && GetRandomNumber16 (&RandY)) {
    Row    = ((Rows - (NumberOfLines + 3)) == 0)? 0 : (RandX % (Rows - (NumberOfLines + 3)));
    Column = ((Columns - (MaxLength + 2)) == 0)? 0 : (RandY % (Columns - (MaxLength + 2)));
  } else {
    Row    = (Rows - (NumberOfLines + 3)) / 2;
    Column = (Columns - (MaxLength + 2)) / 2;
  }
//  DEBUG((DEBUG_INFO, "[%a] Row:    %d\n", __FUNCTION__, Row));
//  DEBUG((DEBUG_INFO, "[%a] Column: %d\n", __FUNCTION__, Column));

//
  // Allocate a buffer for a single line of the popup with borders and a Null-terminator
  //
  Line = AllocateZeroPool ((MaxLength + 3) * sizeof (CHAR16));

  if (Line == NULL) {
    ASSERT (Line != NULL);
    return;
  }

  //
  // Draw top of popup box
  //
  SetMem16 (Line, (MaxLength + 2) * 2, BOXDRAW_HORIZONTAL);
  Line[0]             = BOXDRAW_DOWN_RIGHT;
  Line[MaxLength + 1] = BOXDRAW_DOWN_LEFT;
  Line[MaxLength + 2] = L'\0';
  ConOut->SetCursorPosition (ConOut, Column, Row++);
  ConOut->OutputString (ConOut, Line);

  //
  // Draw middle of the popup with strings
  //
  VA_START (Args, Key);
  while ((String = VA_ARG (Args, CHAR16 *)) != NULL && NumberOfLines > 0) {
    SetMem16 (Line, (MaxLength + 2) * 2, L' ');
    Line[0]             = BOXDRAW_VERTICAL;
    Line[MaxLength + 1] = BOXDRAW_VERTICAL;
    Line[MaxLength + 2] = L'\0';
    ConOut->SetCursorPosition (ConOut, Column, Row);
    ConOut->OutputString (ConOut, Line);
    Length = InternalGetStringWidth (String, FALSE, 0, NULL) / 2;
    if (Length <= MaxLength) {
      //
      // Length <= MaxLength
      //
      ConOut->SetCursorPosition (ConOut, Column + 1 + (MaxLength - Length) / 2, Row++);
      ConOut->OutputString (ConOut, String);
    } else {
      //
      // Length > MaxLength
      //
      InternalGetStringWidth (String, TRUE, MaxLength, &Length);
      TmpString = AllocateZeroPool ((Length + 1) * sizeof (CHAR16));

      if (TmpString == NULL) {
        ASSERT (TmpString != NULL);
        break;
      }

      StrnCpyS (TmpString, Length + 1, String, Length - 3);
      StrCatS (TmpString, Length + 1, L"...");

      ConOut->SetCursorPosition (ConOut, Column + 1, Row++);
      ConOut->OutputString (ConOut, TmpString);
      FreePool (TmpString);
    }

    NumberOfLines--;
  }

  VA_END (Args);

  //
  // Draw bottom of popup box
  //
  SetMem16 (Line, (MaxLength + 2) * 2, BOXDRAW_HORIZONTAL);
  Line[0]             = BOXDRAW_UP_RIGHT;
  Line[MaxLength + 1] = BOXDRAW_UP_LEFT;
  Line[MaxLength + 2] = L'\0';
  ConOut->SetCursorPosition (ConOut, Column, Row++);
  ConOut->OutputString (ConOut, Line);

  //
  // Free the allocated line buffer
  //
  FreePool (Line);

  //
  // Restore the cursor visibility, position, and attributes
  //
  ConOut->EnableCursor (ConOut, SavedConsoleMode.CursorVisible);
  ConOut->SetCursorPosition (ConOut, SavedConsoleMode.CursorColumn, SavedConsoleMode.CursorRow);
  ConOut->SetAttribute (ConOut, SavedConsoleMode.Attribute);

  //
  // Wait for a keystroke
  //
  if (Key != NULL) {
    while (TRUE) {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, Key);
      if (!EFI_ERROR (Status)) {
        break;
      }

      //
      // If we encounter error, continue to read another key in.
      //
      if (Status != EFI_NOT_READY) {
        continue;
      }

      gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
    }
  }

}
