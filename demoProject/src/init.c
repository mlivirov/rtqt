/**************************************************************************/
/*                                                                        */
/*  File: INIT.C                                 Copyright (c) 1998,2006  */
/*  Version: 5.0                                 On Time Informatik GmbH  */
/*                                                                        */
/*                                                                        */
/*                                      On Time        /////////////----- */
/*                                    Informatik GmbH /////////////       */
/* --------------------------------------------------/////////////        */
/*                                  Real-Time and System Software         */
/*                                                                        */
/**************************************************************************/

#include <Rttarget.h>
#include <Rtusb.h>
#include <Rtfiles.h>


//-----------------------------------------------------------------------------
// QT use Win32 emulation. Default handles count is not enouch for QT
// so we need change it.

#define MAXHANDLES 1024 
#define MAXOBJECTS 1024
#define MAXTYPES   1024

RTW32Handle  RTHandleTable[MAXHANDLES] = {{0}}; 
int          RTHandleCount = MAXHANDLES; 

RTW32Object  RTObjectTable[MAXOBJECTS] = {{0}}; 
int          RTObjectCount = MAXOBJECTS; 

RTW32Types   RTTypeTable[MAXTYPES] = {{0}}; 
int          RTTypeCount = MAXTYPES;

//-----------------------------------------------------------------------------

#define RTF_MAX_DRIVES    16  // max logical drives, can be set up to 32
#define RTF_MAX_FILES     100  // max open file, can be set to any value >= 2
#define RTF_MAX_BUFFERS  256  // number of  512 byte sector buffers
#define RTCD_MAX_BUFFERS  16  // number of 2048 byte sector buffers,
                              // set to 8, 16, or more for CD-ROM support

#define RTF_BUFFERS_IN_BSS 0  // don't need initialized buffers

#include <Rtfdata.c>          // replace default tables with our own

static RTFDrvFLPYData Floppy[1] = {0};
static RTFDrvIDEData  IDE[4]    = {0};
static RTFDrvIDEData  SATA[8]   = {0};
static RTFDrvAHCIData AHCI[8]   = {0};

// let's support 1 USB floppy, 2 standard disk plus 1 card reader with 4 slots
static RTFDrvUSBData USBFloppy[1]  = {0};
static RTFDrvUSBData USBDisks[2]   = {0};
static RTFDrvUSBData CardReader[4] = {0};

// In this demo, lots of directory and FAT data is read and written.
// For best performance, we will enable RTF_DEVICE_LAZY_WRITE. In most
// user application, this is probably not what you want. The RTFiles-32
// Reference Manual explains in detail what these device flags mean.

#define DEVICE_FLAGS RTF_DEVICE_LAZY_WRITE

// The RTFiles-32 device list. RTFiles-32 will scan this device listed at
// program startup to mount disk volumes.

RTFDevice RTFDeviceList[] = { 
   // one legacy floppy drive
   { RTF_DEVICE_FLOPPY, 0, DEVICE_FLAGS,        &RTFDrvFloppy, Floppy + 0 }, //0

   // one USB floppy
   { RTF_DEVICE_FLOPPY, 0, DEVICE_FLAGS,        &RTFDrvUSB,    USBFloppy + 0 },//1

   // IDE primary master and slave
   { RTF_DEVICE_FDISK , 0, DEVICE_FLAGS,        &RTFDrvIDE,    IDE + 0 }, //2
   { RTF_DEVICE_FDISK , 1, DEVICE_FLAGS,        &RTFDrvIDE,    IDE + 1 }, //3 

   // IDE secondary master and slave
   { RTF_DEVICE_FDISK , 2, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvIDE,    IDE + 2 }, //4
   { RTF_DEVICE_FDISK , 3, DEVICE_FLAGS, &RTFDrvIDE,    IDE + 3 }, //5

   // four SATA ports
   { RTF_DEVICE_FDISK , 8, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvIDE,    SATA + 0 },//6
   { RTF_DEVICE_FDISK , 9, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvIDE,    SATA + 1 },//7
   { RTF_DEVICE_FDISK ,10, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvIDE,    SATA + 2 },//8
   { RTF_DEVICE_FDISK ,11, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvIDE,    SATA + 3 },//9
   { RTF_DEVICE_FDISK ,12, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvIDE,    SATA + 4 },//10
   { RTF_DEVICE_FDISK ,13, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvIDE,    SATA + 5 },//11
   { RTF_DEVICE_FDISK ,14, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvIDE,    SATA + 6 },//12
   { RTF_DEVICE_FDISK ,15, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvIDE,    SATA + 7 },//13
   // four AHCI ports
   { RTF_DEVICE_FDISK , 0, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvAHCI,   AHCI + 0 },//14
   { RTF_DEVICE_FDISK , 1, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvAHCI,   AHCI + 1 },//15
   { RTF_DEVICE_FDISK , 2, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvAHCI,   AHCI + 2 },//16
   { RTF_DEVICE_FDISK , 3, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvAHCI,   AHCI + 3 },//17
   { RTF_DEVICE_FDISK , 4, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvAHCI,   AHCI + 4 },//18
   { RTF_DEVICE_FDISK , 5, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvAHCI,   AHCI + 5 },//19
   { RTF_DEVICE_FDISK , 6, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvAHCI,   AHCI + 6 },//20
   { RTF_DEVICE_FDISK , 7, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvAHCI,   AHCI + 7 },//21
   // standard USB disks
   { RTF_DEVICE_FDISK , 0, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvUSB,    USBDisks + 0},//22
   { RTF_DEVICE_FDISK , 0, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvUSB,    USBDisks + 1},//23

   /*
   { RTF_DEVICE_FDISK , 20, DEVICE_FLAGS |
                           RTF_DEVICE_NEW_LOCK, &RTFDrvUSB,    USBDisks + 2},
   { RTF_DEVICE_FDISK , 21, DEVICE_FLAGS |
                           RTF_DEVICE_NEW_LOCK, &RTFDrvUSB,    USBDisks + 3},

   */
   // this is the card reader
   { RTF_DEVICE_FDISK , 0, DEVICE_FLAGS | RTF_DEVICE_NEW_LOCK, &RTFDrvUSB,    CardReader + 0},//24
   { RTF_DEVICE_FDISK , 1, DEVICE_FLAGS,        &RTFDrvUSB,    CardReader + 1},//25
   { RTF_DEVICE_FDISK , 2, DEVICE_FLAGS,        &RTFDrvUSB,    CardReader + 2},//26
   { RTF_DEVICE_FDISK , 3, DEVICE_FLAGS,        &RTFDrvUSB,    CardReader + 3},//27
   { 0 } // end of list
};


// The exported init function below is executed before the run-time system
// startup code.
#ifdef _MSC_VER
__declspec(dllexport) void          Init(void)
#else
void __export Init(void)
#endif
{
   RTSetFlags(RT_MM_VIRTUAL | RT_CLOSE_FIND_HANDLES, 1);
   RTCMOSExtendHeap();            // get as much memory as we can
   RTCMOSSetSystemTime();         // get the right date and time

   //RTEmuInit();                   // only if you need it
}
