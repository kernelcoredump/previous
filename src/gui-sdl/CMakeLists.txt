
include_directories(. ../.. ../debug ../includes ${SDL2_INCLUDE_DIR} ${PCAP_INCLUDE_DIR}) 

add_library(GuiSdl
    dlgAbout.c dlgAdvanced.c dlgAlert.c dlgBoot.c dlgDimension.c
    dlgDiskSelect.c dlgEthernet.c dlgEthernetAdvanced.c dlgFileSelect.c
    dlgFloppy.c dlgGraphics.c dlgKeyboard.c dlgMain.c dlgMemory.c 
    dlgMemoryAdvanced.c dlgMissingFile.c dlgMouse.c dlgOpticalDisk.c 
    dlgPrinter.c dlgRom.c dlgSCSI.c dlgSound.c dlgSystem.c sdlgui.c
)
