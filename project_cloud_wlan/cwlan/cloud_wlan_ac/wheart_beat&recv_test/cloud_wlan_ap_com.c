#include <stdlib.h>  
#include <stdio.h>  
#include <unistd.h>  
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


#include "cloud_wlan_types.h"
#include "cloud_wlan_nl.h"
#include "cloud_wlan_sqlite.h"
#include "cloud_wlan_cfg_update.h"
#include "cloud_wlan_ap_com.h"

s32 g_cw_debug = 1;


int main(int argc, char **argv)  
{
	s32 ret;
/*
	ret =sqlite3_open(CWLAN_AP_CFG_DB, &g_cw_db);
	if( ret )						//�������������ʾ��Ϣ���˳�����	
	{
		printf("INIT:Can'topen database: %s\n", sqlite3_errmsg(g_cw_db));  
		sqlite3_close(g_cw_db);  
		exit(1);  
	}
*/
	/*�û�̬���ں�̬ͨ�ų�ʼ��
	ret = cloud_wlan_nl_cfg_init();
	if(ret != CWLAN_OK)
	{
		sqlite3_close(g_cw_db);  
		exit(1);  
	}
*/
	/* ����Ĭ��һЩ���õĳ�����*/
	//cw_ap_local_db_com_cfg_init(g_cw_db);
	g_ap_com_cfg.ac_com_addr=htonl(inet_addr("10.9.100.66"));
	printf("INIT:cwlan ac ip          %x\n", g_ap_com_cfg.ac_com_addr);
	g_ap_com_cfg.ac_com_port=22222;
	printf("INIT:cwlan ac port         %d\n", g_ap_com_cfg.ac_com_port);
	strcpy(g_ap_com_cfg.ap_com_eth,"eth6");
	printf("INIT:cwlan ac eth          %s\n", g_ap_com_cfg.ap_com_eth);


	/*��ʼ����̬�����߳�*/
	ret = cw_cfg_dynamic_update_init();
    if (ret != CWLAN_OK)
	{  
        printf("INIT:cw_cfg_dynamic_update_init FAIL��%d\n",ret);
		//cloud_wlan_nl_close();
		//sqlite3_close(g_cw_db); 
		exit(1);  
    }  

	printf("INIT:cw_ap_com init ok\n");

	/*���Ŀ����ڱ������ø��½�����*/
	while(1){};
	
	return CWLAN_OK;
}
