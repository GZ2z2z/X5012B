/*
*********************************************************************************************************
* @file    	ethernet.c
* @author  	SY
* @version 	V1.0.0
* @date    	2016-7-10 14:39:55
* @IDE	 	Keil V4.72.10.0
* @Chip    	STM32F407VE
* @brief   	以太网源文件
*********************************************************************************************************
* @attention
*
* 
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private Includes
*********************************************************************************************************
*/
#include "app.h"
#include "wizchip_conf.h"
#include "socket.h"
#include "W5500/w5500.h"
#include "para.h"
#include "dhcp.h"

/*
*********************************************************************************************************
*                              				Private define
*********************************************************************************************************
*/
#define	H_SOCKET_DHCP						(0)
#define	H_SOCKET_BASE						(1)
#define SERVER_PORT							(9999)
#define BROADCAST_PORT						(6999)
#define	UDP_RX_BUFF_SIZE					(1024)
#define DHCP_DELAY_TIME						(50 * TIME_BASE_UNIT)

#define SPI_Dev								DEV_SPI1
#define ENABLE_W5500_GPIO_RCC()				do {\
												__HAL_RCC_GPIOA_CLK_ENABLE();\
												__HAL_RCC_GPIOC_CLK_ENABLE();\
											} while (0)
#define W5500_CS_GPIO_PORT					GPIOC			
#define W5500_CS_PIN						GPIO_PIN_4
#define W5500_RESET_GPIO_PORT				GPIOA
#define W5500_RESET_PIN						GPIO_PIN_2

/* 片选口线置低选中  */
#define W5500_CS_LOW()       				HAL_GPIO_WritePin(W5500_CS_GPIO_PORT, W5500_CS_PIN, GPIO_PIN_RESET)
/* 片选口线置高不选中 */
#define W5500_CS_HIGH()      				HAL_GPIO_WritePin(W5500_CS_GPIO_PORT, W5500_CS_PIN, GPIO_PIN_SET)
/* 复位引脚置低  */
#define W5500_RESET_LOW()       			HAL_GPIO_WritePin(W5500_RESET_GPIO_PORT, W5500_RESET_PIN, GPIO_PIN_RESET)
/* 复位引脚置高 */
#define W5500_RESET_HIGH()      			HAL_GPIO_WritePin(W5500_RESET_GPIO_PORT, W5500_RESET_PIN, GPIO_PIN_SET)

/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/
enum DHCP_Status {
	eDHCP_Idle = 0,
	eDHCP_Auto,
	eDHCP_Hand,
	eDHCP_Done,
};											
											
struct W5500_DHCP {
	enum DHCP_Status status;
	uint8_t sn;
	RIP_MSG rip;
	TIMER_TASK_TypeDef timer;
};											
											
struct W5500_Drv {
	W5500_DevEnum dev;
	uint8_t sn;
	uint16_t localPort;
	uint16_t remotePort;
	uint8_t remoteIP[4];
	SEQUEUE_TypeDef queue;
	bool connect;
	struct W5500_DHCP dhcp;
	uint8_t rxBuffer[UDP_RX_BUFF_SIZE];
};

/*
*********************************************************************************************************
*                              				Private constants
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private macro
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
static struct W5500_Drv g_W5500_Drv[] = {
	//通讯
	{
		.dev 					= DEV_W5500_0,
		.sn						= H_SOCKET_BASE,
		.localPort 				= SERVER_PORT,
	},
	//广播
	{
		.dev 					= DEV_W5500_1,
		.sn						= H_SOCKET_BASE+1,
		.localPort 				= BROADCAST_PORT,
	},
};


/*
*********************************************************************************************************
*                              				Private function prototypes
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                              				Private functions
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : GetW5500DevHandle
* Description	: 获取W5500设备句柄
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static struct W5500_Drv *GetW5500DevHandle(W5500_DevEnum dev)
{
	struct W5500_Drv *handle = NULL;
	for (uint8_t i=0; i<ARRAY_SIZE(g_W5500_Drv); ++i) {		
		if (dev == g_W5500_Drv[i].dev) {
			handle = &g_W5500_Drv[i];
			break;
		}
	}
	return handle;
}

/*
*********************************************************************************************************
* Function Name : Ethernet_Task
* Description	: 以太网任务
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void __Ethernet_Task(struct W5500_Drv *this)
{
	switch (getSn_SR(this->sn))
	{
		case SOCK_UDP:
		{
			int32_t recvSize = getSn_RX_RSR(this->sn);
			const uint8_t UDP_HEAD_SIZE = 8;
			if (recvSize <= UDP_HEAD_SIZE)
			{
				break;
			}
			recvSize -= UDP_HEAD_SIZE;
			
			static uint8_t tempBuff[UDP_RX_BUFF_SIZE];
			if (recvSize > UDP_RX_BUFF_SIZE)
			{
				recvSize = UDP_RX_BUFF_SIZE;
			}
			
			uint16_t remotePort;
			uint8_t remoteIP[4];
			recvSize = recvfrom(this->sn, tempBuff, recvSize, remoteIP, &remotePort);				
			if (recvSize > 0)
			{
				if (this->connect) 
				{
					if ((remotePort != this->remotePort) || (memcmp(remoteIP, this->remoteIP, sizeof(remoteIP)))) {
						ECHO(ECHO_DEBUG,"[BSP][ETH] Drop illegal data. Bind IP = %d.%d.%d.%d, Port = %d", 
							this->remoteIP[0], this->remoteIP[1], this->remoteIP[2], this->remoteIP[3], this->remotePort);
						break;
					}
				}
				else 
				{
					memcpy(this->remoteIP, remoteIP, sizeof(remoteIP));
					this->remotePort = remotePort;
				}
				
				for (uint32_t i=0; i<recvSize; ++i) {
					if (PushSeqQueueByte(&this->queue, &tempBuff[i]) != true) {
						ECHO(ECHO_DEBUG,"%s, Full!", __FUNCTION__);
						break;
					}
				}
			}
			else
			{
				ECHO(ECHO_DEBUG,"%s, Rx Error!", __FUNCTION__);
				break;
			}
			break;
		}	
		case SOCK_CLOSED:
		{
			/* 创建Socket */
			if(socket(this->sn, Sn_MR_UDP, this->localPort, 0x00) == this->sn) {
				ECHO(ECHO_DEBUG,"Create SOCKET Success!");
			} else {
				ECHO(ECHO_DEBUG,"Create SOCKET Error!");
			}
			break;
		}
		default:
			break;
	}
}

/*
*********************************************************************************************************
* Function Name : W5500_WriteByte
* Description	: W5500写数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void W5500_WriteByte(uint8_t _ucValue)
{
	SPI_WriteByte(SPI_Dev, _ucValue);
}

/*
*********************************************************************************************************
* Function Name : W5500_ReadByte
* Description	: W5500读取数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint8_t W5500_ReadByte(void)
{
	return SPI_ReadByte(SPI_Dev);
}

/*
*********************************************************************************************************
* Function Name : W5500_WriteBytes
* Description	: W5500写数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void W5500_WriteBytes(uint8_t *_ucValue, uint16_t length)
{
	SPI_WriteBytes(SPI_Dev, _ucValue, length);
}

/*
*********************************************************************************************************
* Function Name : W5500_ReadByte
* Description	: W5500读取数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void W5500_ReadBytes(uint8_t *_ucValue, uint16_t length)
{
	SPI_ReadBytes(SPI_Dev, _ucValue, length);
}

/*
*********************************************************************************************************
* Function Name : W5500_Set_CS_High
* Description	: W5500拉高片选
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void W5500_Set_CS_High(void)
{
	W5500_CS_HIGH();
}

/*
*********************************************************************************************************
* Function Name : W5500_Set_CS_Low
* Description	: W5500拉低片选
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void W5500_Set_CS_Low(void)
{
	W5500_CS_LOW();
}

/*
*********************************************************************************************************
* Function Name : Reset_W5500
* Description	: 复位W5500
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void Reset_W5500(void)
{
	W5500_RESET_LOW();
	bsp_DelayUS(500);
	W5500_RESET_HIGH();
	bsp_DelayMS(1);
}

static wiz_NetInfo gWIZNETINFO = { 	
	.dhcp 	= 	NETINFO_STATIC, 	   
};

/*
*********************************************************************************************************
* Function Name : PrintNetworkInfo
* Description	: 打印网卡信息
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void PrintNetworkInfo(void)
{
	wiz_NetInfo info = {0};
	// Display Network Information
	ctlnetwork(CN_GET_NETINFO, (void*)&info);
	
	uint8_t tmpstr[6] = {0};
	ctlwizchip(CW_GET_ID,(void*)tmpstr);
	ECHO(ECHO_DEBUG,"********************* %s NET CONF *********************",(char*)tmpstr);
	ECHO(ECHO_DEBUG,"MAC: %02X:%02X:%02X:%02X:%02X:%02X",info.mac[0],info.mac[1],info.mac[2],
		  info.mac[3],info.mac[4],info.mac[5]);
	ECHO(ECHO_DEBUG," IP: %d.%d.%d.%d", info.ip[0],info.ip[1],info.ip[2],info.ip[3]);
	ECHO(ECHO_DEBUG,"SUB: %d.%d.%d.%d", info.sn[0],info.sn[1],info.sn[2],info.sn[3]);
	ECHO(ECHO_DEBUG,"GAR: %d.%d.%d.%d", info.gw[0],info.gw[1],info.gw[2],info.gw[3]);
	ECHO(ECHO_DEBUG,"DNS: %d.%d.%d.%d", info.dns[0],info.dns[1],info.dns[2],info.dns[3]);
	ECHO(ECHO_DEBUG, "**********************************************************");
}

/*
*********************************************************************************************************
* Function Name : Network_Init
* Description	: 网络初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void Network_Init(void)
{	
	bsp_GetMACAdress(gWIZNETINFO.mac);
	uint32_t ip = devc_ip_get();
	gWIZNETINFO.ip[0] = (ip >> 24) & 0xff;
	gWIZNETINFO.ip[1] = (ip >> 16) & 0xff;
	gWIZNETINFO.ip[2] = (ip >> 8) & 0xff;
	gWIZNETINFO.ip[3] = (ip >> 0) & 0xff;
	
	gWIZNETINFO.sn[0] = 255;
	gWIZNETINFO.sn[1] = 255;
	gWIZNETINFO.sn[2] = 255;
	gWIZNETINFO.sn[3] = 0;
	
	gWIZNETINFO.gw[0] = gWIZNETINFO.ip[0];
	gWIZNETINFO.gw[1] = gWIZNETINFO.ip[1];
	gWIZNETINFO.gw[2] = gWIZNETINFO.ip[2];
	gWIZNETINFO.gw[3] = 1;
	
	gWIZNETINFO.dns[0] = gWIZNETINFO.ip[0];
	gWIZNETINFO.dns[1] = gWIZNETINFO.ip[1];
	gWIZNETINFO.dns[2] = gWIZNETINFO.ip[2];
	gWIZNETINFO.dns[3] = 1;
	
	ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);
	PrintNetworkInfo();
}

/*
*********************************************************************************************************
* Function Name : NIC_Init
* Description	: 网卡初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void NIC_Init(void)
{
	/* 硬件复位 */
	Reset_W5500();
	
	/* 配置回调函数 */
#if   _WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_SPI_VDM_
	reg_wizchip_cs_cbfunc(W5500_Set_CS_Low, W5500_Set_CS_High);
#elif _WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_SPI_FDM_
	reg_wizchip_cs_cbfunc(W5500_Set_CS_Low, W5500_Set_CS_Low);  // CS must be tried with LOW.
#else
   #if (_WIZCHIP_IO_MODE_ & _WIZCHIP_IO_MODE_SIP_) != _WIZCHIP_IO_MODE_SIP_
	  #error "Unknown _WIZCHIP_IO_MODE_"
   #else
	  reg_wizchip_cs_cbfunc(W5500_Set_CS_Low, W5500_Set_CS_High);
   #endif
#endif
	reg_wizchip_spi_cbfunc(W5500_ReadByte,W5500_WriteByte);
	reg_wizchip_spiburst_cbfunc(W5500_ReadBytes, W5500_WriteBytes);
	
	/* SOCKET缓存初始化 */
	static uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2}};
	if(ctlwizchip(CW_INIT_WIZCHIP,(void*)memsize) == -1)
	{
		ECHO(ECHO_DEBUG,"Set SOCKET Buffer Error!");
		while(1);
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_InitSPI_W5500
* Description	: 初始化SPI W5500
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void bsp_InitSPI_W5500(void)
{
	ENABLE_W5500_GPIO_RCC();
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = W5500_CS_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;		
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(W5500_CS_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = W5500_RESET_PIN;
	HAL_GPIO_Init(W5500_RESET_GPIO_PORT, &GPIO_InitStructure);
	
	W5500_CS_HIGH();		
	W5500_RESET_HIGH();
}

/*
*********************************************************************************************************
*                              				DHCP
*********************************************************************************************************
*/

/*
*********************************************************************************************************
* Function Name : IP_AssignHandler
* Description	: IP分配处理程序
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void IP_AssignHandler(void)
{	
	struct W5500_Drv *this = GetW5500DevHandle(DEV_W5500_BROADCAST);
	if (this == NULL) {
		return;
	}
	
	ECHO(ECHO_DEBUG,"[BSP][ETH] DHCP Assign!");	
	getIPfromDHCP(gWIZNETINFO.ip);
	getSNfromDHCP(gWIZNETINFO.sn);
    getGWfromDHCP(gWIZNETINFO.gw);    
    getDNSfromDHCP(gWIZNETINFO.dns);
    gWIZNETINFO.dhcp = NETINFO_DHCP;
	ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);	
	SetTimerCounter(&this->dhcp.timer, DHCP_DELAY_TIME);
}

/*
*********************************************************************************************************
* Function Name : IP_UpdateHandler
* Description	: IP更新处理程序
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void IP_UpdateHandler(void)
{
	ECHO(ECHO_DEBUG,"[BSP][ETH] DHCP Update!");
	IP_AssignHandler();
}

/*
*********************************************************************************************************
* Function Name : IP_ConflictHandler
* Description	: IP冲突处理程序
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void IP_ConflictHandler(void)
{
	ECHO(ECHO_DEBUG,"[BSP][ETH] DHCP IP conflict!");
}

/*
*********************************************************************************************************
* Function Name : Ping
* Description	: Ping命令
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool Ping(struct W5500_Drv *this, uint8_t *IP)
{
	//WIZchip RCR value changed for ARP Timeout count control
	uint8_t tmp = getRCR();
	setRCR(0x03);

	const char *str = "CHECK_IP_EXIST";
	// IP conflict detection : ARP request - ARP reply
	// Broadcasting ARP Request for check the IP conflict using UDP sendto() function
	int32_t ret = sendto(this->sn, (uint8_t *)str, strlen(str), IP, 5000);
	
	// RCR value restore
	setRCR(tmp);
		
	return (ret == strlen(str));
}

/*
*********************************************************************************************************
* Function Name : CheckRouterExist
* Description	: 检测路由器是否存在
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool CheckRouterExist(struct W5500_Drv *this)
{	
	uint8_t IP[4] = {0};
	for (uint8_t i=0; i<sizeof(IP); ++i) {
		IP[i] = this->remoteIP[i];
	}
	IP[3] = 1;
	return Ping(this, IP);
}

/*
*********************************************************************************************************
* Function Name : Check_DHCP_IPLegal
* Description	: 检测DHCP分配的IP地址合法
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool Check_DHCP_IPLegal(struct W5500_Drv *this, uint8_t *IP)
{
	//WIZchip RCR value changed for ARP Timeout count control
	uint8_t tmp = getRCR();
	setRCR(0x03);

	const char *str = "CHECK_IP_CONFLICT";
	// IP conflict detection : ARP request - ARP reply
	// Broadcasting ARP Request for check the IP conflict using UDP sendto() function
	int32_t ret = sendto(this->sn, (uint8_t *)str, strlen(str), IP, 5000);
	
	// RCR value restore
	setRCR(tmp);
	
	return (ret == SOCKERR_TIMEOUT);
}

/*
*********************************************************************************************************
* Function Name : GenenateIP
* Description	: 生成IP地址
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void GenenateIP(struct W5500_Drv *this)
{	
	srand(bsp_GetRunTime() ^ bsp_GetDeviceID());
	int32_t value = 0;
	while (value <= 1) {
		value = rand() % 255;
		if ((value == 111) || (value == 234) || (value == this->remoteIP[3])) {
			value = 0;
		}
	}
	for (uint8_t i=0; i<ARRAY_SIZE(this->remoteIP); ++i) {
		gWIZNETINFO.ip[i] = this->remoteIP[i];
		gWIZNETINFO.gw[i] = this->remoteIP[i];
		gWIZNETINFO.dns[i] = this->remoteIP[i];
	}
	gWIZNETINFO.ip[3] = value;
	gWIZNETINFO.gw[3] = 1;
	gWIZNETINFO.dns[3] = 1;
	ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);
	__Ethernet_Task(this);
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_DHCP_Close
* Description	: 关闭DHCP处理程序
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void __bsp_W5500_DHCP_Close(struct W5500_Drv *this)
{
	this->dhcp.status = eDHCP_Idle;
	DHCP_stop();
	ECHO(ECHO_DEBUG,"[BSP][ETH] DHCP Close!");
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_DHCP_Handler
* Description	: DHCP处理程序
* Input			: None
* Output		: None
* Return		: DHCP_IP_LEASED: 成功; DHCP_FAILED: 失败
*********************************************************************************************************
*/
static void __bsp_W5500_DHCP_Handler(struct W5500_Drv *this)
{
	switch (this->dhcp.status) {
	case eDHCP_Idle:	
		break;
	case eDHCP_Auto:
	{
		uint8_t ret = DHCP_run();
		/* DHCP Failed */
		if (ret == DHCP_FAILED) {
			ECHO(ECHO_DEBUG,"[BSP][ETH] DHCP Failed!");
			__bsp_W5500_DHCP_Close(this);
		} else if (ret == DHCP_IP_LEASED) {
			ECHO(ECHO_DEBUG,"[BSP][ETH] DHCP Success!");
			DHCP_stop();
			SetTimerCounter(&this->dhcp.timer, DHCP_DELAY_TIME);
			this->dhcp.status = eDHCP_Done;
		}
		break;
	}
	case eDHCP_Hand:
	{
		GenenateIP(this);	
		if (Check_DHCP_IPLegal(this, gWIZNETINFO.ip)) {
			ECHO(ECHO_DEBUG,"[BSP][ETH] DHCP OK!");
			SetTimerCounter(&this->dhcp.timer, DHCP_DELAY_TIME);
			this->dhcp.status = eDHCP_Done;
		}
		break;
	}
	case eDHCP_Done:
		TimerTaskCycle(&this->dhcp.timer);
		break;
	}
}

/*
*********************************************************************************************************
* Function Name : DHCP_TimeUpNotify
* Description	: DHCP定时器时间到达通知
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void DHCP_TimeUpNotify(void *privateData)
{
	struct W5500_Drv *this = privateData;
	App_NotifyMasterDeviceReport(DEV_COMMU_BROADCAST);
	this->dhcp.status = eDHCP_Idle;
	PrintNetworkInfo();
//	devc_ip_set(*(uint32_t *)gWIZNETINFO.ip);
//	PRM_Write();
}

/*
*********************************************************************************************************
*                              				API
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_W5500_Handler
* Description	: W5500处理程序
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_W5500_Handler(void)
{
	for (uint8_t i=0; i<ARRAY_SIZE(g_W5500_Drv); ++i) {
		struct W5500_Drv *this = &g_W5500_Drv[i];
		__Ethernet_Task(this);
		__bsp_W5500_DHCP_Handler(this);
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_WriteBytes
* Description	: 以太网发送字节流
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_W5500_WriteBytes(W5500_DevEnum dev, const uint8_t *data, uint32_t lenth)
{
	struct W5500_Drv *this = GetW5500DevHandle(dev);
	if (this == NULL) {
		return false;
	}
	int32_t ret = sendto(this->sn, (uint8_t *)data, lenth, this->remoteIP, this->remotePort);
	return ((ret<0) ? false : true);
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_ReadByte
* Description	: 以太网读取字节流
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_W5500_ReadByte(W5500_DevEnum dev, uint8_t *data)
{
	struct W5500_Drv *this = GetW5500DevHandle(dev);
	if (this == NULL) {
		return false;
	}
	return (PopSeqQueueByte(&this->queue, data) == true);
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_GetLinkStatus
* Description	: 获取以太网连接状态
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_W5500_GetLinkStatus(void)
{
	uint8_t linkStatus = 0;	
	if (ctlwizchip(CW_GET_PHYLINK, (void*)&linkStatus) == -1) {
		return false;
	}
	return true;
}

/*
*********************************************************************************************************
* Function Name : BindingPort
* Description	: 绑定端口
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_W5500_BindingPort(W5500_DevEnum dev, uint16_t port)
{
	struct W5500_Drv *this = GetW5500DevHandle(dev);
	if (this == NULL) {
		return;
	}
	socket(this->sn, Sn_MR_UDP, port, 0);
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_Close
* Description	: 关闭socket
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_W5500_Close(W5500_DevEnum dev)
{
	struct W5500_Drv *this = GetW5500DevHandle(dev);
	if (this == NULL) {
		return;
	}
	close(this->sn);
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_Connect
* Description	: 建立连接
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_W5500_Connect(W5500_DevEnum dev)
{
	struct W5500_Drv *this = GetW5500DevHandle(dev);
	if (this == NULL) {
		return;
	}
	this->connect = true;	
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_DisConnect
* Description	: 断开连接
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_W5500_DisConnect(W5500_DevEnum dev)
{
	struct W5500_Drv *this = GetW5500DevHandle(dev);
	if (this == NULL) {
		return;
	}
	this->connect = false;
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_DHCP_Init
* Description	: DHCP初始化处理程序
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_W5500_DHCP_Init(W5500_DevEnum dev)
{
	struct W5500_Drv *this = GetW5500DevHandle(dev);
	if (this == NULL) {
		return;
	}	
	if (this->dhcp.status == eDHCP_Idle) {
		this->connect = false;
		ECHO(ECHO_DEBUG,"[BSP][ETH] DHCP Start...");
		
		if (CheckRouterExist(this)) {
			this->dhcp.status = eDHCP_Auto;
			this->dhcp.sn = H_SOCKET_DHCP;
			DHCP_init(this->dhcp.sn, (uint8_t *)&this->dhcp.rip);
			reg_dhcp_cbfunc(IP_AssignHandler, IP_UpdateHandler, IP_ConflictHandler);
		} else {
			this->dhcp.status = eDHCP_Hand;
		}
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_DHCP_Close
* Description	: 关闭DHCP处理程序
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_W5500_DHCP_Close(W5500_DevEnum dev)
{
	struct W5500_Drv *this = GetW5500DevHandle(dev);
	if (this == NULL) {
		return;
	}
	__bsp_W5500_DHCP_Close(this);
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_DHCP_Handler
* Description	: DHCP处理程序
* Input			: None
* Output		: None
* Return		: DHCP_IP_LEASED: 成功; DHCP_FAILED: 失败
*********************************************************************************************************
*/
void bsp_W5500_DHCP_Handler(W5500_DevEnum dev)
{
	struct W5500_Drv *this = GetW5500DevHandle(dev);
	if (this == NULL) {
		return;
	}
	__bsp_W5500_DHCP_Handler(this);
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_GetLocalIP
* Description	: 获取本地IP地址
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
uint32_t bsp_W5500_GetLocalIP(void)
{
	return *(uint32_t *)gWIZNETINFO.ip;
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_GetRemoteIP
* Description	: 获取远程IP地址
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
uint32_t bsp_W5500_GetRemoteIP(W5500_DevEnum dev)
{
	struct W5500_Drv *this = GetW5500DevHandle(dev);
	if (this == NULL) {
		return 0;
	}
	return *(uint32_t *)this->remoteIP;
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_SetRemoteIP
* Description	: 设置远程IP地址
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_W5500_SetRemoteIP(W5500_DevEnum dev, uint32_t remoteIP)
{
	struct W5500_Drv *this = GetW5500DevHandle(dev);
	if (this == NULL) {
		return;
	}
	*(uint32_t *)this->remoteIP = remoteIP;
}

/*
*********************************************************************************************************
* Function Name : bsp_W5500_CheckIPRepeat
* Description	: 检测IP重复
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_W5500_CheckIPRepeat(W5500_DevEnum dev, uint8_t *IP)
{
	struct W5500_Drv *this = GetW5500DevHandle(dev);
	if (this == NULL) {
		return false;
	}
	return !Check_DHCP_IPLegal(this, IP);
}

/*
*********************************************************************************************************
* Function Name : bsp_InitW5500
* Description	: 初始化W5500
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitW5500(void)
{
	/* 初始化SPI	 */
	bsp_InitSPI_W5500();		
	
	/* 网卡初始化 */
	NIC_Init();
	
	/* 网络初始化 */
	Network_Init();
	
	for (uint8_t i=0; i<ARRAY_SIZE(g_W5500_Drv); ++i) {
		struct W5500_Drv *this = &g_W5500_Drv[i];		
		CreateSeqQueue(&this->queue, this->rxBuffer, sizeof(this->rxBuffer));
		
		TimerTaskInit(&this->dhcp.timer, CTRL_PERIOD, this);
		RegisterTimerTaskTimeUpNotify_CallBack(&this->dhcp.timer, DHCP_TimeUpNotify);
	}
	
	ECHO(ECHO_DEBUG, "[BSP] W5500 初始化 .......... OK");
}

/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/

