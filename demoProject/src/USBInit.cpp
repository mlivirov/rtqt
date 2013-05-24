/**************************************************************************/
/*                                                                        */
/*  File: USBInit.cpp                            Copyright (c) 2002,2006  */
/*  Version: 5.0                                 On Time Informatik GmbH  */
/*                                                                        */
/*                                                                        */
/*                                      On Time        /////////////----- */
/*                                    Informatik GmbH /////////////       */
/* --------------------------------------------------/////////////        */
/*                                  Real-Time and System Software         */
/*                                                                        */
/**************************************************************************/

/* Source file to initialize RTUSB-32.

   Function FindUSBControllers() is used by the RTUSB-32 demos to
   initialize RTUSB-32. It is recommended that this file is copied
   and then customized for every project which will use RTUSB-32.

   For more information, please see section "Function FindUSBControllers"
   in the RTUSB-32 User's Manual.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Rttarget.h>
#include <Rttbios.h>

#include <Rtk32.h>
#include <Rtkeybrd.h>

#include <Rtusb.h>
#include <Rtusys.h>     // for RTUSYSErrorExit()

#ifndef __cplusplus
   typedef int bool;
   #define true 1
   #define false 0
#endif

// fix PCI latency timers if they are too low
#define MIN_UHCI_PCI_TIMER_LATENCY  32
#define MIN_OHCI_PCI_TIMER_LATENCY  32
#define MIN_EHCI_PCI_TIMER_LATENCY 128

// #define PEG        // don't call KBInit() for RTPEG-32 programs, which contains its own keyboard handling
#define VERBOSE       // tell us what controller have been found
#define INCLUDE_UHCI  // look for UHCI controllers
#define INCLUDE_OHCI  // look for OHCI controllers
#define INCLUDE_EHCI  // look for EHCI controllers

#define MAX_EHCI_MEM_SPACE 1024

#ifdef INCLUDE_EHCI
/*-----------------------------------*/
static bool TakeEHCIOwnership(const BYTE * CapRegs, BYTE Bus, BYTE DeviceFunc)
{
   int EECP = CapRegs[9];  // EHCI Extended Capabilities Pointer
   DWORD EEC;              // USB Legacy Support Extended Capability Register
   DWORD T = RTUSYSGetTime();

   if (EECP < 0x40)
      return true; // no BIOS support

   // see if this controller supports Pre-OS to OS Handoff Synchronization
   RTT_BIOS_ReadConfigData(Bus, DeviceFunc, EECP, 4, (void *) &EEC);
   if ((EEC & 0xFF) != 1)
      return true; // no Pre-OS to OS Handoff Synchronization

   // request ownership by setting HC OS Owned Semaphore
   RTT_BIOS_WriteConfigData(Bus, DeviceFunc, EECP+3, 1, 1);

   // wait up to 1 second until BIOS gives up ownership
   while ((RTUSYSGetTime() - T) < 1000)
   {
      BYTE BIOSSemaphore;
      // Read HC BIOS Owned Semaphore
      RTT_BIOS_ReadConfigData(Bus, DeviceFunc, EECP+2, 1, (void *) &BIOSSemaphore);
      if ((BIOSSemaphore & 1) == 0)
         return true; // the BIOS has released it
   }
   RTUDiagMessage("warning: The BIOS failed to release ownership of the EHCI controller,\n"
                  "will try to use it anyway...\n");
// return false; // use this if you prefer to fail if the BIOS does not let loose.
   return true;
}
#endif

/*-----------------------------------*/
static bool IsPCIDevice(BYTE Bus, BYTE DeviceFunc, WORD VendorID, WORD DeviceID)
{
   WORD V, D;

   RTT_BIOS_ReadConfigData(Bus, DeviceFunc, RTT_BIOS_VENDOR_ID, sizeof(WORD), &V);
   RTT_BIOS_ReadConfigData(Bus, DeviceFunc, RTT_BIOS_DEVICE_ID, sizeof(WORD), &D);
   return (VendorID == V) && (DeviceID == D);
}

/*-----------------------------------*/
static bool GetHCResources(const char * Name, BYTE Bus, BYTE DeviceFunc, BYTE * IRQ, void * * MemAddr, WORD * IOBase, DWORD AddrMask, int TimerLatency)
{
   DWORD Temp;
   BYTE Latency;
   WORD Cmd;

   // verify that the BIOS enabled bus mastering
   RTT_BIOS_ReadConfigData(Bus, DeviceFunc, RTT_BIOS_COMMAND, 2, &Cmd);
   if ((Cmd & 0x04) == 0)
      RTT_BIOS_WriteConfigData(Bus, DeviceFunc, RTT_BIOS_COMMAND, 2, Cmd | 0x04);

   // fix latency timer
   RTT_BIOS_ReadConfigData(Bus, DeviceFunc, RTT_BIOS_LATTIMER, 1, &Latency);
   if (Latency < TimerLatency)
      RTT_BIOS_WriteConfigData(Bus, DeviceFunc, RTT_BIOS_LATTIMER, 1, TimerLatency);

   // make sure the IRQ value makes sense
   RTT_BIOS_ReadConfigData(Bus, DeviceFunc, RTT_BIOS_IRQ, sizeof(*IRQ), IRQ);
   if ((*IRQ == 0) || (*IRQ >= 32))
   {
      RTUDiagMessage("HC with invalid IRQ\n");
      return false;
   }

   if (MemAddr)
   {
      int Result;

      RTT_BIOS_ReadConfigData(Bus, DeviceFunc, 0x10, sizeof(Temp), &Temp);
      *MemAddr = (void*) (Temp &= AddrMask);
      if (*MemAddr == NULL)
      {
         RTUDiagMessage("HC with invalid memory address\n");
         return false;
      }
      *MemAddr = RTFindPhysMem((const void*)Temp, MAX_EHCI_MEM_SPACE, RT_PG_USERREADWRITE);
      if (*MemAddr == NULL)
      {
         *MemAddr = (void*) Temp;
         Result = RTReserveVirtualAddress(MemAddr, MAX_EHCI_MEM_SPACE, 0);
         switch (Result)
         {
            case RT_MAP_NO_PAGING:
            case RT_MAP_SUCCESS:
               RTMapMem((void*)Temp, *MemAddr, MAX_EHCI_MEM_SPACE, RT_PG_USERREADWRITE);
               break;
            default:
               RTUDiagMessage("RTReserveVirtualAddr failed\n");
               return false;
         }
      }
      #ifdef VERBOSE
      {
         char B[80];
         sprintf(B, "%s controller at %08X, IRQ %i\n", Name, *MemAddr, *IRQ);
         RTUDiagMessage(B);
      }
      #endif
   }

   if (IOBase)
   {
      RTT_BIOS_ReadConfigData(Bus, DeviceFunc, 0x20, sizeof(Temp), &Temp);
      *IOBase = (WORD)Temp & (WORD)AddrMask;
      if (*IOBase == 0)
      {
         RTUDiagMessage("HCD with invalid I/O base address\n");
         return false;
      }
      #ifdef VERBOSE
      {
         char B[80];
         sprintf(B, "%s controller at %04X, IRQ %i\n", Name, *IOBase, *IRQ);
         RTUDiagMessage(B);
      }
      #endif
   }
   return true;
}

/*-----------------------------------*/
int FindUSBControllers(void)
{
   int i, Count = 0;
   BYTE Bus, DeviceFunc;
   BYTE IRQ;
   void * MemBase;
   WORD IOBase;

   // The VC++ 8.0 run-time system needs more than 16k stack space for
   // writing to files. Since all USB client callbacks are called from the
   // Hub task, and some use printf() to write to file stdout, we need
   // a little more stack space.
   #if _MSC_VER >= 1400
      RTUSBConfig.HubTaskStackSize = 32 * 1024;
   #endif

#ifndef INCLUDE_UHCI
#ifndef INCLUDE_OHCI
   RTUSBConfig.Flags |= RTU_NO_EHCI_COMPANION;
#endif
#endif

   if (!RTT_BIOS_Installed())
   {
      RTUDiagMessage("No PCI BIOS found\n");
      return 0;
   }

   // initialize the kernel (if not done already)
   RTKernelInit(0);

   if (!RTKDebugVersion())            // switch off all diagnostics messages
   {
      RTUSetMessageHandler(NULL);
      RTUSBConfig.Flags |= RTU_NOEXIT_ON_ERROR; // keep on going after fatal errors
   }

#ifndef PEG
   KBInit();         // we want blocking keyboard I/O
#endif

   // make sure RTUSB-32 is shut down properly
   atexit(RTUShutDown);

#ifdef INCLUDE_UHCI
   // The UHCI spec allows USB legacy emulation using standard IRQs rather
   // through SMIs. We must thus disable such emulation first before enabling
   // and (potentially shared) IRQs of other USB host controllers).
   for (i=0; RTT_BIOS_FindClassCode(RTU_PCI_CLASS_UHCI, i, &Bus, &DeviceFunc) == RTT_BIOS_SUCCESSFUL; i++)
      RTT_BIOS_WriteConfigData(Bus, DeviceFunc, 0xC0, 2, 0x0000);
#endif

#ifdef INCLUDE_EHCI
   for (i=0; RTT_BIOS_FindClassCode(RTU_PCI_CLASS_EHCI, i, &Bus, &DeviceFunc) == RTT_BIOS_SUCCESSFUL; i++)
      if (GetHCResources("EHCI", Bus, DeviceFunc, &IRQ, &MemBase, NULL, 0xFFFFFF00, MIN_EHCI_PCI_TIMER_LATENCY))
         if (TakeEHCIOwnership((const BYTE *) MemBase, Bus, DeviceFunc))
         {
            RTURegisterEHCI(MemBase, IRQ);
            Count++;
         }
#endif

#ifdef INCLUDE_OHCI
   for (i=0; RTT_BIOS_FindClassCode(RTU_PCI_CLASS_OHCI, i, &Bus, &DeviceFunc) == RTT_BIOS_SUCCESSFUL; i++)
      if (GetHCResources("OHCI", Bus, DeviceFunc, &IRQ, &MemBase, NULL, 0xFFFFF000, MIN_OHCI_PCI_TIMER_LATENCY))
      {
         if (IsPCIDevice(Bus, DeviceFunc, 0x1131, 0x1561)) // Philips ISP1561
            RTUSBConfig.Flags |= RTU_OHCI_3_PHASE_SCHED;
         RTURegisterOHCI(MemBase, IRQ);
         Count++;
      }
#endif

#ifdef INCLUDE_UHCI
   for (i=0; RTT_BIOS_FindClassCode(RTU_PCI_CLASS_UHCI, i, &Bus, &DeviceFunc) == RTT_BIOS_SUCCESSFUL; i++)
      if (GetHCResources("UHCI", Bus, DeviceFunc, &IRQ, NULL, &IOBase, 0x0000FFE0, MIN_UHCI_PCI_TIMER_LATENCY))
      {
         RTURegisterUHCI(IOBase, IRQ);
         // enable interrupts after the controller has been initialized
         RTT_BIOS_WriteConfigData(Bus, DeviceFunc, 0xC0, 2, 0x2000);
         Count++;
      }
#endif

   return Count;
}
