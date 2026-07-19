## @file
# UEFI/PI Emulation Platform with UEFI HII interface supported.
#
# The Emulation Platform can be used to debug individual modules, prior to creating
# a real platform. This also provides an example for how an DSC is created.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = Leornian
  PLATFORM_GUID                  = ae51897f-a3ab-4979-98ea-ba7d6323c43e
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  DEFINE  PROJECT_PACKAGE        = $(PLATFORM_NAME)ProjectPkg
  OUTPUT_DIRECTORY               = Build/$(PROJECT_PACKAGE)

  SUPPORTED_ARCHITECTURES        = X64|IA32
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

  DEFINE ANSI_COLOR_DEBUG        = TRUE
  DEFINE ACPIVIEW_ENABLE         = TRUE
  DEFINE QEMU_SUPPORT            = FALSE

!if $(QEMU_SUPPORT) == TRUE
  FLASH_DEFINITION               = $(PROJECT_PACKAGE)/Include/ReferencePlatform/OvmfPkg/OvmfPkgX64.fdf
!else
  FLASH_DEFINITION               = $(PROJECT_PACKAGE)/Include/ReferencePlatform/EmulatorPkg/EmulatorPkg.fdf
!endif # End of $(QEMU_SUPPORT) == TRUE

!if $(QEMU_SUPPORT) == TRUE
!include $(PROJECT_PACKAGE)/Include/ReferencePlatform/OvmfPkg/OvmfPkgX64.dsc
!else
!include $(PROJECT_PACKAGE)/Include/ReferencePlatform/EmulatorPkg/EmulatorPkg.dsc
!endif # End of $(QEMU_SUPPORT) == TRUE

!include $(PROJECT_PACKAGE)/Include/FeatureIncludes.dsc.inc

## Common PCD settings
[PcdsFeatureFlag]

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask                        | 0xf
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel                     | 0x800000CF
#  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel                     | 0x8FFFFFFF
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiExposedTableVersions           | 0x3C

[PcdsDynamicDefault]
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution          | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution            | 0

  # Set video resolution for text setup.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoHorizontalResolution     | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoVerticalResolution       | 0

## Reference Project PCD settings
!if $(QEMU_SUPPORT) == TRUE
!else
[PcdsDynamicHii.common.DEFAULT]
  # Correct setting
#  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|L"Setup"|gEmuSystemConfigGuid|0x0|160
#  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|L"Setup"|gEmuSystemConfigGuid|0x4|53
  
  # W.A for [Bug]: Row/column arguments in Console Out are transposed #12766 
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|L"Setup"|gEmuSystemConfigGuid|0x0|53
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|L"Setup"|gEmuSystemConfigGuid|0x4|160

  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|L"Setup"|gEmuSystemConfigGuid|0x0|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|L"Setup"|gEmuSystemConfigGuid|0x4|0
!endif # End of $(QEMU_SUPPORT) == TRUE

## Common Instances
[LibraryClasses]
  DumpHexLib|$(PROJECT_PACKAGE)/Library/DumpHexLib/DumpHexLib.inf

[LibraryClasses.common.HOST_APPLICATION]
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf

!if $(ANSI_COLOR_DEBUG) == TRUE
[LibraryClasses.common.SEC]
  DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf

[LibraryClasses.common.PEI_CORE]
  DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf

[LibraryClasses.common.PEIM]
  DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf

[LibraryClasses.common.DXE_CORE]
  DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf

[LibraryClasses.common.UEFI_DRIVER]
  DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf

[LibraryClasses.common.DXE_DRIVER]
  DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf

[LibraryClasses.common.UEFI_APPLICATION]
  DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf

[LibraryClasses.common.SMM_CORE]
  DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf

[LibraryClasses.common.MM_STANDALONE]
  DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf

[LibraryClasses.common.MM_CORE_STANDALONE]
  DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf

!endif # End of $(ANSI_COLOR_DEBUG) == TRUE

[Components]
  $(PROJECT_PACKAGE)/Applications/MischiefApp/MischiefApp.inf
  $(PROJECT_PACKAGE)/Applications/UefiFvParser/UefiFvParser.inf
  UefiCpuPkg/Application/Cpuid/Cpuid.inf

## Reference Project Instances
!if $(QEMU_SUPPORT) == TRUE

!else
[LibraryClasses]
  SerialPortLib|EmulatorPkg/Library/DxeEmuStdErrSerialPortLib/DxeEmuStdErrSerialPortLib.inf

[LibraryClasses.common.UEFI_DRIVER]
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf

[Components]
!if $(ANSI_COLOR_DEBUG) == TRUE
  ##
  #  DXE Phase modules
  ##
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf
      SerialPortLib|EmulatorPkg/Library/DxeEmuStdErrSerialPortLib/DxeEmuStdErrSerialPortLib.inf
      DxeEmuLib|EmulatorPkg/Library/DxeEmuLib/DxeEmuLib.inf
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }

  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf {
   <LibraryClasses>
      DebugLib|$(PROJECT_PACKAGE)/Library/AnsiColorDebugLibSerialPort/AnsiColorDebugLibSerialPort.inf
      SerialPortLib|EmulatorPkg/Library/DxeEmuStdErrSerialPortLib/DxeEmuStdErrSerialPortLib.inf
  }
!endif

!if $(ACPIVIEW_ENABLE) == TRUE
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
!endif

!endif # End of $(QEMU_SUPPORT) == TRUE
