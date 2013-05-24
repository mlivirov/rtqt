#include "xrt_init.h"
#ifdef XRT_NETWORK
static int (*NET_BIND_DRIVER[])(int i)={
xn_bind_ne2000, //0
xn_bind_ne2000_pci, //1
xn_bind_n83815, //2
xn_bind_tc90x, //3 //3COM
xn_bind_smc91c9x, //4  //SMC
xn_bind_rtlance, //5 
xn_bind_lance_isa,//6
xn_bind_cs,//7 //CRYSTAL
xn_bind_i82559, //8 
xn_bind_i8254x,//9
xn_bind_r8139,//10
xn_bind_r8168, //11
xn_bind_rdc6040, //12
xn_bind_davicom, //13
xn_bind_rhine,//14
xn_bind_ax172,//15
xn_bind_ax772,//16
//xn_bind_prism,
//xn_bind_prism_pcmcia,
//xn_bind_rtvmf,
//xn_bind_rtrth,
};

static const int NET_DEVICE_CONFIG[]={
NE2000_DEVICE, //0
NE2000_PCI_DEVICE,//1
N83815_DEVICE,//2
TC90X_DEVICE,//3
SMC91C9X_DEVICE,//4
LANCE_DEVICE,//5
LANCE_ISA_DEVICE,//6
LAN_CS89X0_DEVICE,//7
I82559_DEVICE,//8
I8254X_DEVICE,//VM DRIVER //9
R8139_DEVICE,//10
R8168_DEVICE,//11
RDC6040_DEVICE,//12
DAVICOM_DEVICE,//13
RHINE_DEVICE,//14
AX172_DEVICE,//15
AX772_DEVICE,//16
PRISM_DEVICE,//17
PRISM_PCMCIA_DEVICE, //18
RTVMF_DEVICE, //19
RTRTH_DEVICE, //20
};

static const int NetDriverCount = 16;

#define XN_I_MAX 5
int xn_interfaces[XN_I_MAX];
int xn_count = 0;

void xn_close()
{
	while(xn_count--)
		xn_interface_close(xn_interfaces[xn_count]);
}
#endif

#ifdef XRT_MONITORING
void RTKAPI monitoring(void *)
{
	static int started = 0;
	if(!started)
	{
		RTKCreateThread(monitoring, 0, 1024*1024, 0, 0, "mon");
		started++;
	}else
	{
		char tasks[32768];
		while(1)
		{
			RTKTaskInfo(tasks, 32768, LF_TASK_NAME|LF_TASK_HANDLE|LF_REL_CPU_TIME);
			printf(tasks);
			RTKClearStatistic();
			RTKDelay(CLKMilliSecsToTicks(1000));

		}
	}
}
#endif

void xrt_init()
{
	RTKernelInit(0);
	RTSetDisplayHandler(RT_DISP_HOST);

//

#ifdef XRT_PS2MOUSE
	RTInitMouse(-1, 12, 0, 5, 5);
	atexit(RTMouseDone);
#endif

#ifdef XRT_USB
	RTURegisterCallback(USBKeyboard);
	RTURegisterCallback(USBMouse);
	RTURegisterCallback(USBAX172);     // ax172 and ax772 drivers
	RTURegisterCallback(USBAX772);
	RTURegisterCallback(USBDisk);
	
	FindUSBControllers();              // install USB host controllers
	RTUWaitInitialEnumDone();          // give the USB stack time to enumerate devices
#endif

#ifdef XRT_NETWORK
	CFG_NUM_DEVICES = NetDriverCount * 3;
//
	if(xn_rtip_init()>=0)
	{
		int idriver = 0;
		int minor = 0;
		unsigned char ip[4] = {192, 168, 210, 55};
		unsigned char mask[4] = {255, 255, 255, 0};
		while(idriver<NetDriverCount && xn_count < XN_I_MAX)
		{
			if (NET_DEVICE_CONFIG[idriver] == SMC91C9X_DEVICE || 
				NET_DEVICE_CONFIG[idriver] == LANCE_ISA_DEVICE || 
				NET_DEVICE_CONFIG[idriver] == LAN_CS89X0_DEVICE)
			{
				idriver++;
				continue;
			}

			int result = -1;
			if ((result = NET_BIND_DRIVER[idriver](minor)) >= 0)
			{		
				RTKDelay(CLKMilliSecsToTicks(300)); // give some time
				result = xn_interface_open(NET_DEVICE_CONFIG[idriver], minor);
				if(result != SOCKET_ERROR)
				{
					IFACE_INFO i;

					xn_set_ip(result, (PFCBYTE)ip, (PFCBYTE)mask);
					xn_interfaces[xn_count++] = result;

					xn_interface_info(result, &i);
					printf("%s started, IP: %s\n", i.device_name, inet_ntoa(*(in_addr *)i.my_ip_address));
					minor++;
					continue;
				}
			}
			
			minor = 0;
			idriver++;
		}
		atexit(xn_close);
	}
#endif

#ifdef XRT_MONITORING
	monitoring(0);
#endif
}



