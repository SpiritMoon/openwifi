#ifndef CLOUD_WLAN_NL_H_
#define CLOUD_WLAN_NL_H_



#define NETLINK_CWLAN		31	/*netlink����ţ����Ϊ32*/
/** ������ݸ���(�̶�) **/  
#define MAX_DATA_PAYLOAD 512
/** ���Э�鸺��(�̶�) **/  
#define MAX_PROTOCOL_LPAYLOAD (MAX_DATA_PAYLOAD + 8)
/*��������̫�󳤶�*/
#define CW_DES_LEN (MAX_DATA_PAYLOAD/2)
/*URL��󳤶�*/
#define CW_LOCATION_URL_DATA_LEN 1024
/*URL, ע�����ǲ���Ҫ��"/"��*/
#define CW_LOCATION_URL_DATA "http://www.108608.com/portal/index.jsp"
/*IP*/
#define CW_LOCATION_PORT 8080
/* ����ip*/
#define CW_LOCATION_URL_IP_MAX 10

#define CLOUD_WLAN_WHITE_LIST_MAX_U 50

//��������ò�����Ӧ�����õ� dbֱ�ӾͿ����е��������ٽ���
#define GET_TYPE_3(src)	(src & 0x000F)
#define GET_TYPE_2(src)	((src & 0x00FF)>>8)
#define GET_TYPE_1(src) ((src)>>16)
#define SET_TYPE_3(src, add1, add2) (((src)<<16)|((add1)<<8)|(add2))
#define SET_TYPE_2(src, add1) (((src)<<16)|(add1))
#define SET_TYPE_1(src) (src)



enum cmd_type
{
	CW_CMD_TYPE_AP_KMOD,
	CW_CMD_TYPE_AP_LOCAL,
	CW_CMD_TYPE_MAX
};
enum cmd_kmod
{
	/*����debug���鿴�ں˵Ļ�����Ϣ*/
	CW_NLMSG_RES_OK=0	,					//�ظ�ok
	CW_NLMSG_RES_FAIL ,					//�ظ�FAIL
	CW_NLMSG_DEBUG_SHOW_ONLINE_USER,
	CW_NLMSG_DEBUG_SHOW_URL_WHITE_LIST,
	CW_NLMSG_DEBUG_SHOW_USER_WHITE_LIST,
	CW_NLMSG_DEBUG_SHOW_PORTAL,
	CW_NLMSG_GET_TEST,					//����ǲ����õ�
	
	CW_NLMSG_SET_OFF,					//ȫ�ֿ���
	CW_NLMSG_SET_ON	,				
	CW_NLMSG_SET_DEBUG_OFF,				//ȫ�ֵ��Կ���
	CW_NLMSG_SET_DEBUG_ON,
	
	CW_NLMSG_UPDATE_PORTAL,			//����portal���ĵ�localtion����
	CW_NLMSG_UPDATE_URL_WHITE_LIST,			//ȫ�ֵ�Ŀ�ĵ�ַ������,����������������,������̫��
	CW_NLMSG_USER_WHITE_LIST_ADD,			//���������������
	CW_NLMSG_USER_WHITE_LIST_DEL,
	CW_NLMSG_USER_WHITE_LIST_UPDATE,		//������������
	CW_NLMSG_UPDATE_SESSION_CFG,			//ȫ�ֵĻỰ��������
	CW_NLMSG_SET_KLOG_OFF,		//ȫ����־��Ϣ����
	CW_NLMSG_SET_KLOG_ON,
	CW_NLMSG_PUT_KLOG_INFO,
	CW_NLMSG_SET_USER_PID,		//����һ��ȫ�ֵ��û�̫pid

	CW_NLMSG_AP_KMOD_MAX,
};

enum cmd_local
{
	/****************************************/
	
	/*ap��ac֮��ͨ������*/
		
	CW_NLMSG_SET_REBOOT = 0x7fff0000,		// ����ap
	CW_NLMSG_SET_WAN_PPPOE, 	//����ap wan�ӿ�Ϊpppoe����ģʽ
	CW_NLMSG_SET_WAN_DHCP,		//����ap wan�ӿ�Ϊdhcp ģʽ
	CW_NLMSG_SET_WIFI_INFO, 	//����ap wifi��Ϣ15
	
	CW_NLMSG_HEART_BEAT, //���б���
	
	CW_NLMSG_AP_LOCAL�MAX,

};


enum flow_session_status
{
	CW_USER_ONLINE_STATE_UP,		//����״̬
	CW_USER_ONLINE_STATE_DOWN,		//����״̬
	CW_USER_ONLINE_STATE_UPDATE,
	CW_USER_ONLINE_STATE_MAX
};


enum klog_mode
{
	REAL_TIME,
	UNREAL_TIME
};


typedef struct dns_protal_url
{
	u32 data_len;
	s8 data[];
}dns_protal_url_t;

/*url ���˵�ַ�ṹ*/
typedef struct ac_udp_white_list
{
	u32 id;
	u32 len;	//ֻ�Ǳ��ṹ�����ݵ�data����
	u8 *data;	//�ַ���Ҫ��\0�ṹ����
}ac_udp_white_list_t;

typedef struct cwlan_base_cfg
{
	u32 usage_model;
	u32 cwlan_sw;
	u32 klog_sw;
	u32 over_time;		//��㳬ʱʱ�䣬����Ϊ��λ
	u32 interval_timer;	//��ʱ��ִ�м��ʱ�䣬����Ϊ��λ
	u32 flow_max;		//�������������ֽڼ�
}cwlan_base_cfg_t;


typedef struct reHttp
{
	u32 id;//��ͬ�û��ض���һ��������û��ʵ��
	u32 destIp[CW_LOCATION_URL_IP_MAX];	//�ض���ָ��Ŀ�ĵ�ַ
	u16 destPort;	//�ض���ָ���Ķ˿ں�
	s8 Location[CW_LOCATION_URL_DATA_LEN];	//�ض���ָ����URL
}reHttp_t;

typedef struct pppoe_cfg
{
	s8 username[64];
	s8 password[64];
}pppoe_cfg_t;

enum usage_model
{
	USAGE_MODEL_LOCAL,
	USAGE_MODEL_ONLINE
};

enum cw_switch
{
	CWLAN_OPEN,
	CWLAN_CLOSE = -1
};

enum encryption_type
{
	EN_NONE,
	EN_WEP_OPEN,
	EN_WEP_SHARE,
	EN_WAP_PSK,
	EN_WAP2_PSK,
	EN_WAP_MIX,
	EN_MAX
};
enum arithmetic_type
{
	ALG_CCMP,
	ALG_TKIP,
	ALG_MIX,
	ALG_MAX
};
typedef struct encryption_cfg_info
{
	u8 arithmetic;
	u8 key_len;
	u8 key[128];//������������
}encryption_cfg_info_t;

typedef struct wifi
{
	u8 wlan_id;
	u8 disabled;//������
	u8 txpower;    //���ù���Ϊ17dbm ̫�߻�������ģ��
	u8 channel;	  //���������ŵ�Ϊ6
	//s8 mode;    //��������ģʽΪap
	u8 ssid_len;
	u8 ssid[128];    //��������SSID
	u8 en_type;    //���ü���ΪWPA2-PSK
	encryption_cfg_info_t en_info;
}wifi_cfg_t;


typedef struct ap_local_info 
{
	u8 apmac[6];
	u32 mem_total; // ��λ��K
	u32 mem_free;
	u32 cpu_idle_rate;
	u64 run_time;	//��λ���룬������Ŀǰ�����˶��
}ap_local_info_t;

typedef struct online_user_info
{
	u32 userip;
	u8 usermac[6];
	u8 apmac[6];
	u32 status;
	u64 time;	//�����ߵ�ǰʱ��
}online_user_info_t;

/*��������ͨ�Žṹ*/
typedef struct dcma_udp_info
{
	u32 type;	//��������
	u32 number;	//data ����
	u8 data[];	
}dcma_udp_skb_info_t;

#endif

