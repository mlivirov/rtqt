

#ifndef PLATFORM_WIN32
DWORD WINAPI ps2_task(void *)
{
	printf(TMW_MESSAGE_FORMAT, "starting ps/2");
	RTInitMouse(-1,12,0,5,5);
	printf("done\n");
	return 0;
}
#endif

DWORD WINAPI usb_task(void *)
{
#ifndef PLATFORM_WIN32
#ifdef PS2MOUSE
	
#pragma message("██████████████████████████████ PS/2 MOUSE ENABLED ███████████████████████████████")
	TMWCreateThread(ps2_task,1024*64,8,0,"ps2");
#endif
	printf(TMW_MESSAGE_FORMAT, "starting usb");
	
	RTURegisterCallback(USBKeyboard);
	RTURegisterCallback(USBMouse);
	RTURegisterCallback(USBAX172);     // ax172 and ax772 drivers
	RTURegisterCallback(USBAX772);
	RTURegisterCallback(USBDisk);
	
	FindUSBControllers();              // install USB host controllers
	RTUWaitInitialEnumDone();          // give the USB stack time to enumerate devices
	printf("done\n");	
#endif
	return 0;
}



void usb_init()
{
	HANDLE h = TMWCreateThread(usb_task,1024*64,0,0,"usb");
	WaitForSingleObject(h,INFINITE);
}

void usb_done()
{
#ifndef PLATFORM_WIN32
	RTUShutDown();
#endif
}