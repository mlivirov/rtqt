/**************************************************************************/
/*                                                                        */
/*  File: USBMouse.C                             Copyright (c) 2002,2006  */
/*  Version: 5.0                                 On Time Informatik GmbH  */
/*                                                                        */
/*                                                                        */
/*                                      On Time        /////////////----- */
/*                                    Informatik GmbH /////////////       */
/* --------------------------------------------------/////////////        */
/*                                  Real-Time and System Software         */
/*                                                                        */
/**************************************************************************/

/* USB Mouse Boot Device Protocol Driver.

   This driver implements a mouse driver for RTUSB-32 and
   RTTarget-32 for devices supporting the HID boot device protocol.
*/

#include <stdlib.h>
#include <string.h>

#include <Rtusb.h>
#include <Rtusys.h>

#ifdef EVALVER
#include <evalchk.h>
#else
#define RTEVALCHECK(n, m)
#endif

#define MAX_MICE 4

#define ENDPOINT_HALT 0

// class specific descriptors
#define HID_DESCRIPTOR       0x21
#define REPORT_DESCRIPTOR    0x22
#define PHYSICAL_DESCRIPTOR  0x23

// subclasses
#define SUBCLASS_BOOTDEVICE  1

// protocols, only used with subclass SUBCLASS_BOOTDEVICE
#define PROT_KEYBOARD        1
#define PROT_MOUSE           2

// class specific requests
#define SET_REPORT  0x09
#define SET_IDLE    0x0A
#define SET_PROTOCOL 0x0B

typedef struct {
   BYTE bLength;           // Numeric expression that is the total size of the HID descriptor.
   BYTE bDescriptorType;   // Constant name specifying type of HID descriptor.
   WORD bcdHID;            // Numeric expression identifying the HID Class Specification release.
   BYTE bCountryCode;      // Numeric expression identifying country code of the localized hardware.
   BYTE bNumDescriptors;   // Numeric expression specifying the number of class descriptors (always at least one i.e. Report descriptor.)
   BYTE bDescriptorType2;  // Constant name identifying type of class descriptor. See Section 7.1.2: Set_Descriptor Request for a table of class descriptor constants.
   WORD wDescriptorLength; // Numeric expression that is the total size of the Report descriptor.
} HIDDescriptor;

typedef struct _MouseData {
   RTUPipeHandle ControlPipe,
                 MouseIn;
   int  PacketSize;       // may not exceed 8
   BYTE MouseData[8];
   BYTE MInterface;
   BYTE EPAddress;
} MouseData;

typedef struct {
   int X, Y;
} Pos;

static MouseData Data[MAX_MICE];
static RTUQHandle Q,Q2;
static Pos PointerPos;
static DWORD Buttons;
static DWORD LastLeftClickTime;

/*-----------------------------------*/
static void SetRepeatRate(RTUPipeHandle ControlPipe, BYTE IFace, int Milliseconds)
{
   RTUSendSetup(ControlPipe,
                RTU_BMRT_INTERFACE | RTU_BMRT_CLASS | RTU_BMRT_HOST_TO_DEVICE,
                SET_IDLE,
                (Milliseconds/4) << 8,
                IFace,
                0,
                NULL);
}

/*-----------------------------------*/
static int SpeedUp(int Delta, int Scale)
{
   if (Scale <= 0)
      return Delta;
   if (abs(Scale) > abs(Delta))
      Scale = abs(Delta);
   return Scale * Delta;
}

/*-----------------------------------*/
static void RTTAPI MakeMouseEvent(BYTE * D)
/*
Byte Bits     Description
0     0       Button 1
      1       Button 2
      2       Button 3
      4 to 7  Device-specific
1     0 to 7  X displacement
2     0 to 7  Y displacement
3+    0 to 7  Device specific (optional)
*/
{
   Pos LastPos = PointerPos; // screen coordinates
   DWORD NewButtons = D[0];
   int DeltaX = (signed char)D[1];
   int DeltaY = (signed char)D[2];
   int DeltaZ = (signed char)D[3];

   PointerPos.X += SpeedUp(DeltaX, RTUSBMouseConfig.MouseScaleX);
   PointerPos.Y += SpeedUp(DeltaY, RTUSBMouseConfig.MouseScaleY);

   if (PointerPos.X < 0)
      PointerPos.X = 0;
   else if (RTUSBMouseConfig.MaxX && (PointerPos.X > RTUSBMouseConfig.MaxX))
      PointerPos.X = RTUSBMouseConfig.MaxX;

   if (PointerPos.Y < 0)
      PointerPos.Y = 0;
   else if (RTUSBMouseConfig.MaxY && (PointerPos.Y > RTUSBMouseConfig.MaxY))
      PointerPos.Y = RTUSBMouseConfig.MaxY;

   // did the pointer pos change?
   if ((LastPos.X != PointerPos.X) || (LastPos.Y != PointerPos.Y))
      RTUSYSMouseEvent(PointerPos.X, PointerPos.Y, 0, Buttons, USB_MOUSE_MOVED);

   // did the buttons change
   if (Buttons != NewButtons)
   {
      DWORD Type = USB_SINGLE_CLICK;

      if ((NewButtons & USB_BUTTON_LEFT) && // its a left button press
          !(Buttons & USB_BUTTON_LEFT) &&
          RTUSBMouseConfig.DoubleClickTime)
      {
         DWORD EventTime = RTUSYSGetTime();
         if ((EventTime - LastLeftClickTime) <= (DWORD)RTUSBMouseConfig.DoubleClickTime)
         {
            Type = USB_DOUBLE_CLICK;
            LastLeftClickTime = 0;
         }
         else
            LastLeftClickTime = EventTime;
      }

      Buttons = NewButtons;
      RTUSYSMouseEvent(PointerPos.X, PointerPos.Y, 0, Buttons, Type);
   }

   // wheel event?
   if (DeltaZ)
      RTUSYSMouseEvent(PointerPos.X, PointerPos.Y, DeltaZ, Buttons, USB_MOUSE_WHEELED);
}

/*-----------------------------------*/
static void CloseMouse(MouseData * MD)
{
   RTUClosePipe(MD->MouseIn);
   RTUClosePipe(MD->ControlPipe);
   memset(MD, 0, sizeof(*MD));
}

/*-----------------------------------*/
static void RTTAPI USBMouseThread(void)
{
   while (1)
   {
      RTUSBMessage * E  = RTUGetMsgQ(Q);
      int          Code = E->ErrorCode;
      MouseData    * MD = RTUGetClientData(RTUDeviceOfPipe(E->Pipe), USBMouse);

      RTUFreeBuffer(E);

      switch (Code)
      {
         case RTU_CANCELED:
            CloseMouse(MD);
            continue; // do not try to receive more data
         case RTU_SUCCESS:
         case RTU_UNDERRUN:
            MakeMouseEvent(MD->MouseData);
            break;
         case RTU_STALL:
            if (RTUSendSetup(MD->ControlPipe,
                             RTU_BMRT_ENDPOINT | RTU_BMRT_STANDARD | RTU_BMRT_HOST_TO_DEVICE,
                             RTU_CLEAR_FEATURE,
                             ENDPOINT_HALT,
                             MD->EPAddress,
                             0,
                             NULL) < RTU_SUCCESS)
            {
               RTUDiagMessage("USB mouse stalled, resetting\n");
               RTUResetDevice(RTUDeviceOfPipe(MD->ControlPipe));
            }
            break;
         default:
            RTUDiagMessage("USB mouse error: ");
            RTUDiagMessage(RTUErrorMessage(Code));
            RTUDiagMessage(", resetting\n");
            RTUResetDevice(RTUDeviceOfPipe(MD->ControlPipe));
            break; // listen for data, required to call CloseKB
      }
      // wait for more data
      if (RTUStartIO(MD->MouseIn, MD->MouseData, MD->PacketSize) < RTU_SUCCESS)
         CloseMouse(MD);
   }
}

/*-----------------------------------*/
static void RTTAPI USBMouseThread2(void)
{
   while (1)
   {
      RTUSBMessage * E  = RTUGetMsgQ(Q2);
      int          Code = E->ErrorCode;
      MouseData    * MD = RTUGetClientData(RTUDeviceOfPipe(E->Pipe), USBMouse);

      RTUFreeBuffer(E);

      switch (Code)
      {
         case RTU_CANCELED:
            CloseMouse(MD);
            continue; // do not try to receive more data
         case RTU_SUCCESS:
         case RTU_UNDERRUN:
            MD->MouseData[2] = (MD->MouseData[2]>>4) | (MD->MouseData[3]<<4);// !!! X,Y 12bit -> X,Y 8 bit
            MD->MouseData[3] = MD->MouseData[4];// !!! Колесо (Вертикаль)
            MakeMouseEvent(MD->MouseData);
            break;
         case RTU_STALL:
            if (RTUSendSetup(MD->ControlPipe,
                             RTU_BMRT_ENDPOINT | RTU_BMRT_STANDARD | RTU_BMRT_HOST_TO_DEVICE,
                             RTU_CLEAR_FEATURE,
                             ENDPOINT_HALT,
                             MD->EPAddress,
                             0,
                             NULL) < RTU_SUCCESS)
            {
               RTUDiagMessage("USB mouse stalled, resetting\n");
               RTUResetDevice(RTUDeviceOfPipe(MD->ControlPipe));
            }
            break;
         default:
            RTUDiagMessage("USB mouse error: ");
            RTUDiagMessage(RTUErrorMessage(Code));
            RTUDiagMessage(", resetting\n");
            RTUResetDevice(RTUDeviceOfPipe(MD->ControlPipe));
            break; // listen for data, required to call CloseKB
      }
      // wait for more data
      if (RTUStartIO(MD->MouseIn, MD->MouseData, MD->PacketSize) < RTU_SUCCESS)
         CloseMouse(MD);
   }
}
/*-----------------------------------*/
void RTTAPI USBMouse(RTUDeviceHandle Device, RTUSBEvent Event)
{
   RTEVALCHECK(5, 0x39E7CB01)
   switch (Event)
   {
      case RTUConnect:
         {
            const RTUConfigurationDescriptor * C;
            const RTUInterfaceDescriptor     * I = RTUFindInterface(Device, RTU_USB_CLASS_HID, SUBCLASS_BOOTDEVICE, PROT_MOUSE, 0, &C);
            const RTUEndpointDescriptor      * E = RTUFindEndpoint(I, RTUInterrupt, RTUInput);
            MouseData * MD = NULL;
            int i;

            for (i=0; i<MAX_MICE; i++)
               if (Data[i].ControlPipe == 0)
               {
                  MD = Data + i;
                  break;
               }

            if ((MD == NULL) || (I == NULL) || (E == NULL))
               return;

            if (RTUEndpointPacketSize(E) > 8)
               return;

            if (RTUClaimInterface(Device, C, I, USBMouse) < RTU_SUCCESS)
               return;

            if (C->iConfiguration == 4)
            {
                if (Q2 == NULL)
                {
                    Q2 = RTUCreateMsgQ(MAX_MICE, "USB Mouse");
                    RTUSYSCreateThread(USBMouseThread2, RTUSBMouseConfig.TaskPriority, RTUSBMouseConfig.TaskStackSize, "USB Mouse");
                }
                RTUSetClientData(Device, USBMouse, MD);
                MD->MInterface  = I->bInterfaceNumber;
                MD->EPAddress   = E->bEndPointAddress;
                MD->ControlPipe = RTUOpenPipe(Device, NULL, 1);
                MD->PacketSize  = RTUEndpointPacketSize(E);
                MD->MouseIn     = RTUOpenPipe(Device, E, 1);
                RTUSetPipeMsgQ(MD->MouseIn, Q2);
            }
            else 
            {
                if (Q == NULL)
                {
                   Q = RTUCreateMsgQ(MAX_MICE, "USB Mouse");
                       RTUSYSCreateThread(USBMouseThread, RTUSBMouseConfig.TaskPriority, RTUSBMouseConfig.TaskStackSize, "USB Mouse");
                }
                RTUSetClientData(Device, USBMouse, MD);
                MD->MInterface  = I->bInterfaceNumber;
                MD->EPAddress   = E->bEndPointAddress;
                MD->ControlPipe = RTUOpenPipe(Device, NULL, 1);
                MD->PacketSize  = RTUEndpointPacketSize(E);
                MD->MouseIn     = RTUOpenPipe(Device, E, 1);
                RTUSetPipeMsgQ(MD->MouseIn, Q);
            }

            // enableing the boot protocol disables the wheel on some mice
//          RTUSendSetup(MD->ControlPipe, 0x21, SET_PROTOCOL, 0, I->bInterfaceNumber, 0, NULL);
            SetRepeatRate(MD->ControlPipe, MD->MInterface, 0);
            RTUStartIO(MD->MouseIn, MD->MouseData, MD->PacketSize);
            break;
         }
      case RTUDisconnect:
         {
            MouseData * MD = RTUGetClientData(Device, USBMouse);
            if (MD)
               RTUCancelIO(MD->MouseIn);
            break;
         }
   }
}

/*-----------------------------------*/
void RTTAPI USBSetMousePos(int X, int Y)
{
   PointerPos.X = X;
   PointerPos.Y = Y;
   RTUSYSMouseEvent(PointerPos.X, PointerPos.Y, 0, Buttons, USB_MOUSE_MOVED);
}
