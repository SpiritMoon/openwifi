/*
	ע�⣬����ļ�ʵ�ֵĹ��ܲ������Ự���ƣ�
	���Ƕ��û���Ϊ���ƣ���ûʵ�ֶ�ÿ���û��������ɻỰ��ʹ�ù���

*/



#include <linux/stddef.h>
#include <linux/skbuff.h> 
#include <linux/netfilter_ipv4.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/icmp.h>
#include <linux/spinlock.h>
#include <linux/inetdevice.h>
#include <linux/slab.h>

//#include "cloud_wlan_types.h"
#include "cloud_wlan_nl_out_pub.h"
#include "cloud_wlan_nl_in_pub.h"
#include "cloud_wlan_main.h"
#include "cloud_wlan_session.h"
#include "cloud_wlan_log.h"


u32 get_tuple_hash(u32 a, u32 b, u16 c, u16 d, u32 mask)
{
	a += c;
	b += d;
	c = 0;
	a = a^b;
	b = a;
	return (jhash_3words(a,(a^c),(a|(b<<16)),0x5123598) & mask);
}

struct timer_list g_flow_ageing_timer;
cwlan_flow_session_hash_t *g_cw_fs_hash;
cwlan_base_cfg_t g_cw_base_cfg;
/******************************************************************************* 
����: ���Ựhash��㳬ʱ������
 -------------------------------------------------------------------------------
����:	
-------------------------------------------------------------------------------
����ֵ:	
*******************************************************************************/
void flow_session_del_overtime(unsigned long data)
{
	u32 hash_key;
	cwlan_flow_session_hash_t *hash_head;
	cwlan_flow_session_node_t *node;
	cwlan_flow_session_node_t *temp_node;
	u32 online_time;

	for(hash_key= 0 ;hash_key <CWLAN_FLOW_SESSION_HASH_LEN; hash_key++)
	{
		hash_head = g_cw_fs_hash + hash_key;
		write_lock_bh(&hash_head->rwlock);

		list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
		{
		/*С���ϻ�ʱ��ʲô������*/
		/*�������ֱ����jiffies - node->last_access_time/HZ
			���Ҳ�������__udivdi3, ��64λ�����й�
		*/
			online_time = jiffies - node->last_access_time;
			if(online_time/HZ <= node->available_time)
			{
				//����web״̬
				cloud_wlan_generate_klog_main(NULL, &node->original_tuple, KLOG_USER, g_klog_descrip[CW_USER_ONLINE_STATE_UPDATE]);
				continue;
			}
			/*�����ϻ�ʱ�䣬ɾ�����*/
			cloud_wlan_generate_klog_main(NULL, &node->original_tuple, KLOG_USER, g_klog_descrip[CW_USER_ONLINE_STATE_DOWN]);
			list_del(&node->list);
			kfree(node);
		}	
		write_unlock_bh(&hash_head->rwlock);
	}
	
    mod_timer( &(g_flow_ageing_timer),(jiffies + g_cw_base_cfg.interval_timer*HZ) );
	return;
}

/******************************************************************************* 
����: ���Ựhash���ȫ��ɾ��
 -------------------------------------------------------------------------------
����:	
-------------------------------------------------------------------------------
����ֵ:	
*******************************************************************************/
void flow_session_del_all(void)
{
	u32 hash_key;
	cwlan_flow_session_hash_t *hash_head;
	cwlan_flow_session_node_t *node;
	cwlan_flow_session_node_t *temp_node;

	for(hash_key= 0 ;hash_key <CWLAN_FLOW_SESSION_HASH_LEN; hash_key++)
	{
		hash_head = g_cw_fs_hash + hash_key;
		write_lock_bh(&hash_head->rwlock);

		list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
		{
			list_del(&node->list);
			kfree(node);
		}	
		write_unlock_bh(&hash_head->rwlock);
	}
	
	return;
}

/******************************************************************************* 
����: ���Ựƥ��
 -------------------------------------------------------------------------------
����:	
-------------------------------------------------------------------------------
����ֵ:	
*******************************************************************************/
u32 flow_session_down_node(pkt_tuple_info_t *quintuple_info)
{
	u32 hash_key;
	cwlan_flow_session_hash_t *hash_head;
	cwlan_flow_session_node_t *node;
	cwlan_flow_session_node_t *temp_node;

	hash_key = get_tuple_hash(quintuple_info->saddr,(u32)quintuple_info->smac[0],
		(u16)quintuple_info->smac[4],(u16)quintuple_info->smac[5], CWLAN_FLOW_SESSION_HASH_LEN-1);

	hash_head = g_cw_fs_hash + hash_key;
	write_lock_bh(&hash_head->rwlock);

    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
	{
		if( memcmp(node->original_tuple.smac, quintuple_info->smac, 6)
			|| node->original_tuple.saddr!= quintuple_info->saddr)
		{
			continue;
		}
		node->status = CW_USER_ONLINE_STATE_DOWN;	
		cloud_wlan_generate_klog_main(NULL, &node->original_tuple, KLOG_USER, g_klog_descrip[node->status]);
		break;
	}
	
	write_unlock_bh(&hash_head->rwlock);

	return CWLAN_FAIL;
}
/******************************************************************************* 
����: ���ո�����֤��������
 -------------------------------------------------------------------------------
����:	
-------------------------------------------------------------------------------
����ֵ:	
*******************************************************************************/
u32 flow_session_up_node(pkt_tuple_info_t *quintuple_info, int over_time)
{
	u32 hash_key;
	cwlan_flow_session_hash_t *hash_head;
	cwlan_flow_session_node_t *node;
	cwlan_flow_session_node_t *temp_node;
	cwlan_flow_session_node_t *new_node;

	hash_key = get_tuple_hash(quintuple_info->saddr,(u32)quintuple_info->smac[0],
		(u16)quintuple_info->smac[4],(u16)quintuple_info->smac[5], CWLAN_FLOW_SESSION_HASH_LEN-1);

	hash_head = g_cw_fs_hash + hash_key;
	write_lock_bh(&hash_head->rwlock);

    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
	{

		if( memcmp(node->original_tuple.smac, quintuple_info->smac, 6)
			|| node->original_tuple.saddr!= quintuple_info->saddr)
		{
			continue;
		}
		new_node->status = CW_USER_ONLINE_STATE_UP;
		cloud_wlan_generate_klog_main(NULL, quintuple_info, KLOG_USER, g_klog_descrip[CW_USER_ONLINE_STATE_UP]);

		write_unlock_bh(&hash_head->rwlock);
		return CWLAN_OK;
	}

	CLOUD_WLAN_DEBUG("\tnot find and add node.\n");
	if(g_cw_base_cfg.usage_model == USAGE_MODEL_ONLINE)
	{
		new_node = kmalloc(sizeof(cwlan_flow_session_node_t),GFP_KERNEL);
		if(new_node != NULL)
		{
			memcpy((void *)&new_node->original_tuple, (void *)quintuple_info, sizeof(pkt_tuple_info_t));
			new_node->fragment_num = 0;
			new_node->add_time = jiffies;
			new_node->last_access_time = jiffies;
			new_node->flow_B = 0;
			new_node->status = CW_USER_ONLINE_STATE_UP;
			
			list_add(&new_node->list,&hash_head->h_head);
		
			new_node->available_time = over_time;
			cloud_wlan_generate_klog_main(NULL, quintuple_info, KLOG_USER, g_klog_descrip[CW_USER_ONLINE_STATE_UP]);
		}
	}

	write_unlock_bh(&hash_head->rwlock);
	return CWLAN_OK;
}
/******************************************************************************* 
����: ���Ựtcp ��Ƭhash���Ҹ�������
 -------------------------------------------------------------------------------
����:	
-------------------------------------------------------------------------------
����ֵ:	
*******************************************************************************/
u32 flow_session_match_online_list(pkt_tuple_info_t *quintuple_info)
{
	u32 hash_key;
	u32 user_status = CW_USER_ONLINE_STATE_DOWN;
	cwlan_flow_session_hash_t *hash_head;
	cwlan_flow_session_node_t *node;
	cwlan_flow_session_node_t *temp_node;
	cwlan_flow_session_node_t *new_node;

	hash_key = get_tuple_hash(quintuple_info->saddr,(u32)quintuple_info->smac[0],
		(u16)quintuple_info->smac[4],(u16)quintuple_info->smac[5], CWLAN_FLOW_SESSION_HASH_LEN-1);

	hash_head = g_cw_fs_hash + hash_key;
	write_lock_bh(&hash_head->rwlock);

    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
	{
	
		CLOUD_WLAN_DEBUG("in skb [%8x][%8x][%3d]   [%2x][%2x][%2x][%2x][%2x][%2x]\n",
			quintuple_info->saddr,quintuple_info->daddr,
			quintuple_info->protocol,
			quintuple_info->smac[0],quintuple_info->smac[1],
			quintuple_info->smac[2],quintuple_info->smac[3],
			quintuple_info->smac[4],quintuple_info->smac[5]
			);

		CLOUD_WLAN_DEBUG("in fs  [%8x][%8x][%3d]   [%2x][%2x][%2x][%2x][%2x][%2x]\n",
			node->original_tuple.saddr,node->original_tuple.daddr,
			node->original_tuple.protocol,
			node->original_tuple.smac[0],node->original_tuple.smac[1],
			node->original_tuple.smac[2],node->original_tuple.smac[3],
			node->original_tuple.smac[4],node->original_tuple.smac[5]
			);

		if( memcmp(node->original_tuple.smac, quintuple_info->smac, 6)
			|| node->original_tuple.saddr!= quintuple_info->saddr)
		{
			continue;
		}
		//Ҫ��������
		if(jiffies - node->last_access_time < node->available_time)
		{
			user_status = node->status;
		}
		
		CLOUD_WLAN_DEBUG("\t\tthis is node status : %d\n",user_status);

		write_unlock_bh(&hash_head->rwlock);
		return user_status;
	}

	CLOUD_WLAN_DEBUG("\tnot find and add node.\n");
	if(g_cw_base_cfg.usage_model == USAGE_MODEL_LOCAL)
	{
		new_node = kmalloc(sizeof(cwlan_flow_session_node_t),GFP_KERNEL);
		if(new_node != NULL)
		{
			memcpy((void *)&new_node->original_tuple, (void *)quintuple_info, sizeof(pkt_tuple_info_t));
			new_node->fragment_num = 0;
			new_node->add_time = jiffies;
			new_node->last_access_time = jiffies;
			new_node->flow_B = 0;
			new_node->status = CW_USER_ONLINE_STATE_UP;
			
			list_add(&new_node->list,&hash_head->h_head);
			user_status = new_node->status;
		
			new_node->available_time = g_cw_base_cfg.over_time;
			cloud_wlan_generate_klog_main(NULL, quintuple_info, KLOG_USER, g_klog_descrip[user_status]);
		}
	}

	
	write_unlock_bh(&hash_head->rwlock);
	return user_status;
}
/******************************************************************************* 
����: ���Ựtcp ��Ƭhash���Ҹ�������
 -------------------------------------------------------------------------------
����:	
-------------------------------------------------------------------------------
����ֵ:	
*******************************************************************************/
u32 flow_session_show_online_list(void)
{
	u32 hash_key;
	cwlan_flow_session_hash_t *hash_head;
	cwlan_flow_session_node_t *node;
	cwlan_flow_session_node_t *temp_node;

	for(hash_key= 0 ;hash_key <CWLAN_FLOW_SESSION_HASH_LEN; hash_key++)
	{
		hash_head = g_cw_fs_hash + hash_key;
		read_lock_bh(&hash_head->rwlock);

		list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
		{
			printk("\n++++++++++++++++\n");
			printk("MAC    :[%2x][%2x][%2x][%2x][%2x][%2x]\n",
				node->original_tuple.smac[0],node->original_tuple.smac[1],
				node->original_tuple.smac[2],node->original_tuple.smac[3],
				node->original_tuple.smac[4],node->original_tuple.smac[5]);
			printk("saddr				:[%x]\n", node->original_tuple.saddr);
			printk("state				:[%d]	1:is down 0:is up\n", node->status);
			printk("flow				:[%llu]B	The maximum available flow\n", node->flow_B);
			printk("add_time			:[%llu]s\n",node->add_time);
			printk("last_access_time	:[%llu]s\n",node->last_access_time);
		}	
		read_unlock_bh(&hash_head->rwlock);
	}
	
	return CWLAN_OK;
}

u32 flow_sesison_init(void)
{
	u32 size;
	u32 i;
	
	size = sizeof(cwlan_flow_session_hash_t)*CWLAN_FLOW_SESSION_HASH_LEN;
	g_cw_fs_hash = (cwlan_flow_session_hash_t*)kmalloc(size, GFP_KERNEL);
	if( NULL == g_cw_fs_hash )
	{
		return CWLAN_FAIL;
	}
	memset(g_cw_fs_hash, 0, size);
	for(i = 0; i < CWLAN_FLOW_SESSION_HASH_LEN; i++)
	{
		INIT_LIST_HEAD(&g_cw_fs_hash[i].h_head);
        rwlock_init(&g_cw_fs_hash[i].rwlock);
	}
	printk("cw init g_cw_fs_hash ok\n");

	init_timer(&(g_flow_ageing_timer));
	g_flow_ageing_timer.function = flow_session_del_overtime;
	g_flow_ageing_timer.data = 0;
	g_flow_ageing_timer.expires = jiffies + g_cw_base_cfg.interval_timer*HZ;
	add_timer( &(g_flow_ageing_timer) );

	printk("cw init g_flow_ageing_timer ok\n");
	return CWLAN_OK;
}
u32 flow_session_exit(void)
{
	flow_session_del_all();
	printk("cw exit free flow_session ok\n");
	kfree(g_cw_fs_hash);
	printk("cw exit free g_cw_fs_hash ok\n");
	del_timer( &(g_flow_ageing_timer) );
	printk("cw exit free g_flow_ageing_timer ok\n");
	return CWLAN_OK;
}
