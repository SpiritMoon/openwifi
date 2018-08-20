#include <signal.h>
#include <stdlib.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <linux/netlink.h>  
#include <sys/socket.h>  
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdint.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>

#include "mysql/mysql.h"

#include "cloud_wlan_types.h"
#include "cloud_wlan_nl_out_pub.h"
#include "cloud_wlan_nl_in_pub.h"
#include "cloud_wlan_sqlite.h"
#include "cloud_wlan_ap_com.h"
#include "cloud_wlan_ap_local.h"
#include "udp-send.h"

//�������ݷ�Ƭ�������������
s32 cw_ap_recv_ac_update_url_wl(dcma_udp_skb_info_t *buff)
{
	ac_udp_white_list_t *node = (ac_udp_white_list_t *)buff->data;
	dns_white_list_t dns_white_list = {0,NULL};

	s8 *url;
	u32 i;
	u32 ip;
	u32 maxnumber = buff->number<CLOUD_WLAN_WHITE_LIST_MAX_U?buff->number : CLOUD_WLAN_WHITE_LIST_MAX_U;

	/*��Ҫɾ�����ݿ��������ض�����е�����*/
//	snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "delete from %s;",CWLAN_AP_WHITE_TABLE);
	sqlite3_exec_unres(g_cw_db, g_cw_sql);

	memset(&dns_white_list, 0, sizeof(dns_white_list));
	
	get_interface_info("br-lan", &ip, NULL);
	dns_white_list.addr[dns_white_list.number++] = ip;
	
	for(i=0; i<maxnumber; i++)
	{
		url = (s8 *)node->data;
		if(url != NULL)
		{
			/*�������ݿ��������ض�����е�����*/
			//snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "INSERT INTO %s (url) VALUES('%s');",CWLAN_AP_WHITE_TABLE, url);
			sqlite3_exec_unres(g_cw_db, g_cw_sql);
			
			cw_get_url_dns(url, &dns_white_list);
		}
		node = (ac_udp_white_list_t *)(url + node->len);
	}
	
	cloud_wlan_sendto_kmod(CW_NLMSG_UPDATE_URL_WHITE_LIST, (s8 *)&dns_white_list, sizeof(dns_white_list));
    return CWLAN_OK;
}

u32 cw_ap_recv_ac_set_default_switch(u32 ap_cwlan_sw)
{
	/*
		�������ݿ���Ĭ�Ͽ��ص�����
	*/

	snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "update com_cfg set ap_cwlan_sw=%d;",ap_cwlan_sw);
	sqlite3_exec_unres(g_cw_db, g_cw_sql);
	cloud_wlan_sendto_kmod(ap_cwlan_sw, NULL, 0);
	return CWLAN_OK;
}
u32 cw_ap_recv_ac_set_klog_switch(u32 ap_klog_sw)
{
	/*
		�������ݿ���Ĭ�Ͽ��ص�����
	*/

	snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "update com_cfg set ap_klog_sw=%d;",ap_klog_sw);
	sqlite3_exec_unres(g_cw_db, g_cw_sql);
	cloud_wlan_sendto_kmod(ap_klog_sw, NULL, 0);
	return CWLAN_OK;
}

s32 cw_ap_recv_ac_update_portal_wl(dcma_udp_skb_info_t *buff)
{
	s32 ret;
	dns_white_list_t *portal_white_list;
	reHttp_t *portal_cfg;
	s8 out[CW_DES_LEN]={0};
	dns_protal_url_t *portal_url = (dns_protal_url_t *)buff->data;

	snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "UPDATE %s SET url=?;",CWLAN_AP_PORTAL_TABLE);
	//sqlite3_binary_write1(g_cw_db,g_cw_sql, (s8 *)buff->data, CW_DES_LEN);

	portal_cfg = malloc(sizeof(reHttp_t));
	portal_white_list = malloc(sizeof(dns_white_list_t));

	if(portal_white_list == NULL || portal_cfg == NULL)
	{
		cw_local_log_writeto_file("cw_ap_local_update_portal_wl fail\n");
		return CWLAN_FAIL;
	}
	
	memset((u8 *)portal_cfg, 0, sizeof(reHttp_t));
	memset((u8 *)portal_white_list, 0, sizeof(dns_white_list_t));


	//DES_Act(out, (s8 *)portal_url->data, portal_url->data_len,g_des_key, DES_KEY_LEN, DECRYPT);

	ret = cw_get_url_dns(out, portal_white_list);
	if(ret == CWLAN_FAIL)
	{
		cw_local_log_writeto_file("cw_get_url_dns fail\n");
		goto local_exit;
	}
	memcpy((u8 *)portal_cfg->destIp, (u8 *)portal_white_list->addr, sizeof(u32)*( CW_LOCATION_URL_IP_MAX-1));
	portal_cfg->destPort = CW_LOCATION_PORT;
	memcpy(portal_cfg->Location, out, CW_LOCATION_URL_DATA_LEN);
	cloud_wlan_sendto_kmod(CW_NLMSG_UPDATE_PORTAL, (s8 *)portal_cfg, sizeof(reHttp_t));

local_exit:
	free(portal_white_list);
	free(portal_cfg);
	return CWLAN_OK;
}
u32 cw_ap_recv_ac_set_reboot()
{
	system("reboot");
	return CWLAN_OK;
}
/*���µ��õ�uci��Ҫluci��֧��
�Ժ�����ǲ�Ҫʹ��uci����̫��������������ֲ����
*/
u32 cw_ap_recv_ac_set_wan_pppoe(pppoe_cfg_t *pppoe)
{
	s8 cmd[256];
	
	snprintf(cmd, 256, "uci set network.wan.proto=pppoe; uci set network.wan.username=%s;uci set network.wan.password=%s",
		pppoe->username, pppoe->password);
	system(cmd);
	system("uci commit; /etc/init.d/network restart");

	return CWLAN_OK;
}
u32 cw_ap_recv_ac_set_wan_dhcp()
{
	system("uci set network.wan.proto=dhcp; uci commit; /etc/init.d/network restart");
	return CWLAN_OK;
}

u32 cw_ap_recv_ac_set_wifi_info(wifi_cfg_t *wifi_cfg)
{
	s8 cmd[256];
	
	snprintf(cmd, 256, "uci set wireless.@wifi-device[%d].disabled=%d",
		wifi_cfg->wlan_id, wifi_cfg->disabled);
	system(cmd);
	snprintf(cmd, 256, "uci set wireless.@wifi-device[%d].txpower=%d",
		wifi_cfg->wlan_id, wifi_cfg->txpower);
	system(cmd);
	snprintf(cmd, 256, "uci set wireless.@wifi-device[%d].channel=%d",
		wifi_cfg->wlan_id, wifi_cfg->channel);
	system(cmd);

	if(wifi_cfg->ssid_len !=0 )
	{
		snprintf(cmd, 256, "uci set wireless.@wifi-iface[%d].ssid=%s",
			wifi_cfg->wlan_id, wifi_cfg->ssid);
		system(cmd);
	}
	if(wifi_cfg->en_type == EN_NONE)
	{
		snprintf(cmd, 256, "uci set wireless.@wifi-iface[%d].encryption=none",
			wifi_cfg->wlan_id);
		system(cmd);

	}
	snprintf(cmd, 256, "uci set wireless.@wifi-iface[%d].wds=1",
		wifi_cfg->wlan_id);
	system(cmd);//��������
	
	system("uci commit; /etc/init.d/network restart");

	return CWLAN_OK;

/*
	uci set wireless.@wifi-device[0].disabled=0    //������
	uci set wireless.@wifi-device[0].txpower=17    //���ù���Ϊ17dbm ̫�߻�������ģ��
	uci set wireless.@wifi-device[0].channel=6	  //���������ŵ�Ϊ6
	uci set wireless.@wifi-iface[0].mode=ap    //��������ģʽΪap
	uci set wireless.@wifi-iface[0].ssid=[�Լ�����SSID]    //��������SSID
	uci set wireless.@wifi-iface[0].network=lan    //�������ӵ�lan��
	uci set wireless.@wifi-iface[0].encryption=psk2    //���ü���ΪWPA2-PSK
	uci set wireless.@wifi-iface[0].key=[����]	  //������������
*/
}
u32 cw_ap_recv_ac_update_user_wl_update(dcma_udp_skb_info_t *buff)
{
	user_wl_buf_t *ubuff = buff->data;
	cloud_wlan_sendto_kmod(CW_NLMSG_USER_WHITE_LIST_UPDATE, (s8 *)ubuff, sizeof(32)+ubuff->number*6);
	return CWLAN_OK;
}

/*
	���� ��ac �����ø��¹���
*/
static void *cw_recv_ac_info_branch(void *arg)  
{  
	u8 recv_buff[MAX_PROTOCOL_LPAYLOAD]={0};
	memcpy(recv_buff, arg, MAX_PROTOCOL_LPAYLOAD);
	dcma_udp_skb_info_t *buff = (dcma_udp_skb_info_t *)recv_buff;
	switch(buff->type)  
	{  

		case CW_NLMSG_HEART_BEAT:
			break;

		case CW_NLMSG_SET_DEBUG_OFF:
			cloud_wlan_sendto_kmod(buff->type, NULL, 0);
			cw_local_log_writeto_file("cw_recv_ac debuf off\n");
			break;
		case CW_NLMSG_SET_DEBUG_ON:
			cloud_wlan_sendto_kmod(buff->type, NULL, 0);
			cw_local_log_writeto_file("cw_recv_ac debuf on\n");
			break;
		case CW_NLMSG_SET_OFF:
			cw_ap_recv_ac_set_default_switch(buff->type);
			cw_local_log_writeto_file("cw_recv_ac set off\n");
			break;
		case CW_NLMSG_SET_ON:
			cw_ap_recv_ac_set_default_switch(buff->type);
			cw_local_log_writeto_file("cw_recv_ac set on\n");
			break;
		case CW_NLMSG_SET_KLOG_OFF:
			cw_ap_recv_ac_set_klog_switch(buff->type);
			cw_local_log_writeto_file("cw_recv_ac set klog off\n");
			break;
		case CW_NLMSG_SET_KLOG_ON:
			cw_ap_recv_ac_set_klog_switch(buff->type);
			cw_local_log_writeto_file("cw_recv_ac set klog on\n");
			break;
		case CW_NLMSG_UPDATE_URL_WHITE_LIST:
			cw_ap_recv_ac_update_url_wl(buff);
			cw_local_log_writeto_file("cw_ap_recv_ac_update_url_wl\n");
			break;
		case CW_NLMSG_USER_WHITE_LIST_UPDATE:
			cw_ap_recv_ac_update_user_wl_update(buff);
			break;
		case CW_NLMSG_UPDATE_PORTAL:
			cw_ap_recv_ac_update_portal_wl(buff);
			cw_local_log_writeto_file("cw_ap_recv_ac_update_portal_wl\n");
			break;
		case CW_NLMSG_SET_REBOOT:
			cw_ap_recv_ac_set_reboot();
			cw_local_log_writeto_file("cw_ap_recv_ac_set_reboot\n");
			break;
		case CW_NLMSG_SET_WAN_PPPOE:
			cw_ap_recv_ac_set_wan_pppoe((pppoe_cfg_t *)buff->data);
			cw_local_log_writeto_file("cw_ap_recv_ac_set_wan_pppoe\n");
			break;
		case CW_NLMSG_SET_WAN_DHCP:
			cw_ap_recv_ac_set_wan_dhcp();
			cw_local_log_writeto_file("cw_ap_recv_ac_set_wan_dhcp\n");
			break;
		case CW_NLMSG_SET_WIFI_INFO:
			cw_ap_recv_ac_set_wifi_info((wifi_cfg_t *)buff->data);
			cw_local_log_writeto_file("cw_ap_recv_ac_set_wifi_info\n");
			break;

        default:  
			cw_local_log_writeto_file("unkown cmd [%d]\n", buff->type);
			break;

			*/
    }  
    return;  
}  

//���kernel model������־�ĸ���
static u32 cw_ap_sendto_ac_klog_info(dcma_udp_skb_info_t *buff)
{

	
	u32 sendsize=0;
	u32 msg_len = 0;
	s8 heartbeat_msg[MAX_PROTOCOL_LPAYLOAD]={0};
	kmod_log_info_t *klog_buff = (kmod_log_info_t *)buff->data;


	/* ��������˷���������Ϣ��С */
	msg_len = klog_buff->size + sizeof(dcma_udp_skb_info_t);
	
	time((time_t *)&klog_buff->time);
	//��������
	UdpSend2(g_online_socket.server_addr, g_online_socket.server_port, (s8 *)buff, msg_len);

	return CWLAN_OK;
}
void *cw_ap_online_communication_pthread(void *param)
{
	s32 ret =-1;
	int sockfd = -1;
	struct sockaddr_in client_addr, server_addr;
	int sin_size = sizeof(struct sockaddr_in);
	pthread_t thread_id;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	time_t current_time=0;
	time_t recv_heart_beat_time = 0;
	time(&recv_heart_beat_time);

	s8 heartbeat_msg[MAX_PROTOCOL_LPAYLOAD]={0};
	s8 recv_msg[MAX_PROTOCOL_LPAYLOAD]={0};
	dcma_udp_skb_info_t *buff = (dcma_udp_skb_info_t *)heartbeat_msg;
	ap_local_info_t *ap_info_buff = (ap_local_info_t *)(heartbeat_msg + sizeof(dcma_udp_skb_info_t));
	u32 msg_len = 0;
	msg_len = sizeof(ap_local_info_t) + sizeof(dcma_udp_skb_info_t);

	sockfd = UdpConnectInit(inet_atoi(g_online_socket.server_addr), g_online_socket.server_port, &server_addr);
	if (-1 == sockfd)
	{
		printf("UdpConnectInit %d\n", errno);
		exit(1);
	}
	get_interface_info(g_config_base.wan_eth_name, NULL, (s8 *)g_config_base.wan_eth_mac);
	memcpy(ap_info_buff->apmac, g_config_base.wan_eth_mac, 6);


	buff->number = 1;
	buff->type=CW_NLMSG_HEART_BEAT;
	//memcpy(heartbeat_msg, (s8 *)&ap_info, sizeof(ap_local_info_t));
	 /* �����������������Ϣ */
	struct timeval tv;
	fd_set readfds;
	tv.tv_sec=3;
	tv.tv_usec=10;
	while(1){
		
		cw_get_ap_local_info(ap_info_buff);
		ret = UdpSend(sockfd, server_addr, heartbeat_msg, msg_len);
		if(ret != 0){
			sockfd = UdpConnectInit(inet_atoi(g_online_socket.server_addr), g_online_socket.server_port, &server_addr);
			continue;
		}
		FD_ZERO(&readfds);
		FD_SET(sockfd,&readfds);
		select(sockfd+1,&readfds,NULL,NULL,&tv);
		if(FD_ISSET(sockfd,&readfds))
		{
			if(-1 == recvfrom(sockfd, recv_msg, MAX_PROTOCOL_LPAYLOAD, 0, (struct sockaddr *)&client_addr, &sin_size))
			{
				cw_local_log_writeto_file("recvfrom ac :%d %s\n", errno, strerror(errno));
				continue;
			}
			time(&recv_heart_beat_time);
			//��֧����
			cw_ap_heartbeat_get_cwlan_switch_status();
			pthread_create(&thread_id, &attr, cw_recv_ac_info_branch, (void *)&recv_msg);
		}
	}
	
}
static s32 g_ap_heart_beat_interval = 3;
static time_t g_recv_heart_beat_time=0;
static time_t g_send_heart_beat_time=0;
 u32 cw_ap_heartbeat_get_cwlan_switch_status()
 {
	 time(&g_recv_heart_beat_time);
	 return CWLAN_OK;
 }

void *cw_ap_heartbeat_set_cwlan_switch_status(void *arg)
{
	time_t current_time=0;
	static int k_flag = CW_NLMSG_SET_OFF;
	dns_white_list_t dns_white_list = {0,{0}};

	while(1){
		time(&current_time);
		if(g_recv_heart_beat_time != 0)
		{
			//���յ������ظ�
			if(CW_NLMSG_SET_OFF == k_flag)
			{
				/*����֮ǰ��Ҫ����һ��portal��url*/
				//cw_ap_local_db_dns_update(g_cw_db);��Щ���ܲ�����Ҫ
				//cw_ap_local_db_user_white_list_update(g_cw_db);
				
				cw_ap_recv_ac_set_default_switch(CW_NLMSG_SET_ON);
				cw_local_log_writeto_file("cw_recv_ac set on: heart heat \n");
				k_flag = CW_NLMSG_SET_ON;
			}
			g_recv_heart_beat_time = 0;
			g_send_heart_beat_time = current_time;
		}
		else
		{
			//û�н��յ������ظ�
			if(current_time - g_send_heart_beat_time > g_ap_heart_beat_interval * 3)
			{
				if(CW_NLMSG_SET_ON == k_flag)
				{
					k_flag = CW_NLMSG_SET_OFF;
					cw_ap_recv_ac_set_default_switch(CW_NLMSG_SET_OFF);
					cw_local_log_writeto_file("cw_recv_ac set off: heart heat over time\n");
				}
			}
		}
	}
	return CWLAN_OK;
}

extern u32 (*cw_ap_sendto_ac_info)(dcma_udp_skb_info_t *buff);
extern void *cw_ap_local_cfg_update_pthread(void *param);
u32 cw_online_dispose_pthread_cfg_init(struct pthread_id *g_pthread)
{
	u32 ret;
	pthread_attr_t attr;
	
	pthread_t monitor_up_down_id;
	//struct sched_param  params;
	
	ret = pthread_attr_init(&attr);
	
	//�߳��˳�ֱ���ͷ���Դ
	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	/* ���ؽ����ں������߳�
	�������kmod��־����Ϣ�ϱ�*/
	cw_ap_sendto_ac_info = cw_ap_sendto_ac_klog_info;
	ret = pthread_create(&g_pthread->recv_k_id,&attr,cloud_wlan_recv_kmod_info,NULL);
	if(ret!=0){
		printf("init cloud_wlan_recv_kmod_info fail %d!\n",ret);
	}
	ret = pthread_create(&g_pthread->update_id,NULL,cw_ap_online_communication_pthread,NULL);
	if(ret!=0){
		printf("init cw_ap_online_communication_pthread fail %d!\n",ret);
	}
	ret = pthread_create(&monitor_up_down_id,NULL,cw_ap_heartbeat_set_cwlan_switch_status,NULL);
	if(ret!=0){
		printf("init cw_ap_online_communication_pthread fail %d!\n",ret);
	}
	return ret;
}

u32 cw_online_dynamic_update_init(struct pthread_id *g_pthread)
{
	s32 ret = CWLAN_OK;

	ret = cw_online_dispose_pthread_cfg_init(g_pthread);
	return ret;
	
}
u32 cw_online_dynamic_update_exit()
{


}

