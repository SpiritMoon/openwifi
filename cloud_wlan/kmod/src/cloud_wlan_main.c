#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <net/ip.h>
#include <linux/udp.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>
#include <linux/if.h>
#include <linux/socket.h>

#include <net/arp.h>


//#include "cloud_wlan_types.h"
#include "cloud_wlan_nl_out_pub.h"
#include "cloud_wlan_nl_in_pub.h"

#include "cloud_wlan_main.h"
#include "cloud_wlan_session.h"
#include "cloud_wlan_http_pub.h"
#include "cloud_wlan_log.h"
#include "cloud_wlan_dns_wl.h"
#include "cloud_wlan_user_wl.h"

rwlock_t g_rwlock;

u32 g_cloud_wlan_debug = CWLAN_CLOSE;
u32 g_cloud_wlan_nlmsg_pid = 0;

/* IP Hooks */
/* After promisc drops, checksum checks. */
#define NF_IP_PRE_ROUTING	0
/* If the packet is destined for this box. */
#define NF_IP_LOCAL_IN		1
/* If the packet is destined for another interface. */
#define NF_IP_FORWARD		2
/* Packets coming from a local process. */
#define NF_IP_LOCAL_OUT		3
/* Packets about to hit the wire. */
#define NF_IP_POST_ROUTING	4
#define NF_IP_NUMHOOKS		5

/* ����ע�����ǵĺ��������ݽṹ */ 
struct nf_hook_ops g_cwlan_in_hook_prer; 
struct nf_hook_ops g_cwlan_out_hook_prer; 

u32 cloud_wlan_get_quintuple(struct sk_buff *skb, pkt_tuple_info_t *quintuple_info)
{
	struct iphdr *iphdr;
	struct tcphdr *tcphdr;
	struct ethhdr *eth_hdr;
	
	struct dst_entry *dst = skb_dst(skb);
	struct net_device *dev = dst->dev;
	struct neighbour *neigh;

	
	iphdr = ip_hdr(skb);
	tcphdr = tcp_hdr(skb);
	quintuple_info->saddr = iphdr->saddr;
	quintuple_info->daddr = iphdr->daddr;
	quintuple_info->source = tcphdr->source;
	quintuple_info->dest = tcphdr->dest;
	quintuple_info->protocol = iphdr->protocol;

	//eth_hdr = eth_hdr(skb);
	neigh = __ipv4_neigh_lookup_noref(dev, iphdr->daddr);
	if(neigh != NULL)
	{
		if(neigh->hh.hh_len > ETH_HLEN)
		{
			eth_hdr = (struct ethhdr *)neigh->hh.hh_data;
		}
		else
		{
			eth_hdr = (struct ethhdr *)(neigh->hh.hh_data+2);
		}
	}
	memcpy(quintuple_info->smac, eth_hdr->h_source, 6); 
	memcpy(quintuple_info->dmac, eth_hdr->h_dest, 6); 

	
	return CWLAN_OK;
}
/******************************************************************************* 
����:���������ж�
 -------------------------------------------------------------------------------
����:	pdesc ԭ������Ϣ
-------------------------------------------------------------------------------
����ֵ:	CWLAN_FAIL	
			CWLAN_OK	forwar����
*******************************************************************************/
u32 cloud_wlan_packet_transparent_forward(struct sk_buff *skb)
{
	//struct ethhdr *ethhdr;
	struct iphdr *iphdr;
	struct tcphdr *tcphdr;

	//struct eth8021hdr *ethhdr_8021;
	/*
	1��cloud wlan ��һ��ȫ�ֿ���
		�ж����Ƿ�Ϊap����������
	*/

	if( g_cw_base_cfg.cwlan_sw== CWLAN_CLOSE|| skb == NULL || skb->dev == NULL )
	{
		return CWLAN_OK;
	}
	/*
	ethhdr_8021 = (struct eth8021hdr *)eth_hdr(skb);
	if(ethhdr_8021->ethhdr.h_proto != htons(ETH_P_IP) && ethhdr_8021->ethhdr.h_proto != htons(ETH_P_8021Q))
	{
		CLOUD_WLAN_DEBUG("not ETH_P_IP or ETH_P_8021Q\n");
		return CWLAN_OK;
	}
	
	if(ethhdr_8021->ethhdr.h_proto == htons(ETH_P_8021Q) && ethhdr_8021->proto != htons(ETH_P_IP))
	{
		CLOUD_WLAN_DEBUG("is ETH_P_8021Q but not ETH_P_IP\n");
		return CWLAN_OK;
	}
*/
	if( memcmp("br-lan", skb->dev->name, 6) )
	{
		return CWLAN_OK;
	}
	
	iphdr = ip_hdr(skb);
	tcphdr = tcp_hdr(skb);

	//������Ŀ�Ķ˿�Ϊsnmp,dns�˿ں�����͸��
	/* DNS DHCP CAPWAP SNMP Needs to be forwarded, you can add remove or make a corresponding interface*/
	if( iphdr->protocol == IPPROTO_ICMP )
	{
		return CWLAN_OK;
	}
	switch(ntohs(tcphdr->dest))
	{
		case PROTO_DNS:
		case PROTO_DHCP67:
		case PROTO_DHCP68:
		case PROTO_CAPWAP_C:
		case PROTO_CAPWAP_D:
		case PROTO_SNMP1:
		case PROTO_SNMP2:
		case PROTO_SSH:
		//case PROTO_HTTP:
		//case PROTO_HTTPS:
		//case PROTO_HTTP2:
			return CWLAN_OK;
		default:
			break;
	}

	return CWLAN_FAIL;

}

/* ע���hook������ʵ�� */ 
u32 cwlan_in_hook_prer(u32 hooknum,
			       struct sk_buff *skb,
			       const struct net_device *in,
			       const struct net_device *out,
			       u32 (*okfn)(struct sk_buff *))
{
	u32 ret;
	u32 i;
	//struct ethhdr *ethhdr;
	struct iphdr *iphdr;
	struct tcphdr *tcphdr;
	pkt_tuple_info_t cw_quintuple_info;


	//���˱���
	ret = cloud_wlan_packet_transparent_forward(skb);
	if(ret == CWLAN_OK )
	{
		goto  accept;
	}
	
	iphdr = ip_hdr(skb);
	tcphdr = tcp_hdr(skb);
	/*
	2��Ŀ�ĵ�ַ����dns���������ֱ�ӹ�
	*/
	for(i=0; i<CW_LOCATION_URL_IP_MAX; i++)
	{
		if(ntohl(iphdr->daddr) == g_portal_config.rehttp_conf.destIp[i])
		{
			goto  accept;
		}
	}
	if(cloud_wlan_dns_white_list_find((u32)ntohl(iphdr->daddr)) == CWLAN_OK)
	{
		goto  accept;
	}
	cloud_wlan_get_quintuple(skb, &cw_quintuple_info);
	/*
	2.1��Ŀ�ĵ�ַ����user���������ֱ�ӹ�
	*/

	if(cloud_wlan_user_white_list_find(cw_quintuple_info.smac) == CWLAN_OK)
	{
		goto  accept;
	}
	
	/*
	3�����������û��б�������֤״̬��һЩ��Ϣ���ж��Ƿ�ͨ��
		�����ڣ����½�һ���û��ڵ�
	*/
	ret = flow_session_match_online_list(&cw_quintuple_info);
	switch (ret)
	{
		case CW_USER_ONLINE_STATE_UP:
			CLOUD_WLAN_DEBUG("\t\tmatch accept.\n\n");
/*����������url������־*/
			cloud_wlan_generate_klog_main(skb, &cw_quintuple_info, KLOG_URL, NULL);
			goto  accept;
		case CW_USER_ONLINE_STATE_DOWN:
			CLOUD_WLAN_DEBUG("\t\tmatch protal.\n\n");
			break;
		default :
			CLOUD_WLAN_DEBUG("\t\tmatch drop.\n\n");
			goto  drop;
	}

	//read_unlock_bh(&g_rwlock);
	//Ŀǰֻ�ܴ���80�ض���443ֱ�Ӷ���
	if( iphdr->protocol == IPPROTO_TCP && ntohs(tcphdr->dest) == PROTO_HTTP)
	{
		//Դ��ֱ�Ӷ���
		reply_http_redirector(skb);
	}

drop:
	return NF_DROP;
accept:
	return NF_ACCEPT;
}

/* ע���hook������ʵ�� */ 
u32 cwlan_out_hook_prer(u32 hooknum,
			       struct sk_buff *skb,
			       const struct net_device *in,
			       const struct net_device *out,
			       u32 (*okfn)(struct sk_buff *))
{
	u32 ret;
	u32 i;
	

	struct iphdr *iphdr;
	struct tcphdr *tcphdr;
	pkt_tuple_info_t quintuple_info;
	s8 *buff=NULL;
/*��wlanδ�������Ǳ���Ϊnullֱ�ӹ�*/
	if( g_cw_base_cfg.cwlan_sw == CWLAN_CLOSE|| skb == NULL || skb->dev == NULL )
	{
		goto  accept;
	}
	
	iphdr = ip_hdr(skb);
	tcphdr = tcp_hdr(skb);
	/*����http����ֱ�ӹ�*/
	if(	(ntohs(tcphdr->source) != PROTO_HTTP && ntohs(tcphdr->source)!= PROTO_HTTP2))
	{
		goto  accept;
	}
	/*
	2��Ŀ�ĵ�ַ���ڰ��������ֱ�ӹ�
	*/
	for(i=0; i<CW_LOCATION_URL_IP_MAX; i++)
	{
		if(ntohl(iphdr->saddr) == g_portal_config.rehttp_conf.destIp[i] 
			&& tcphdr->syn != 1)
		{
			//�����ǲ�����֤�ɹ�
			buff = (s8 *)tcphdr + tcphdr->doff * 4;
			ret = cloud_wlan_http_skb_parse_reply(buff,NULL);
			if(ret == CWLAN_OK)
			{
				cloud_wlan_get_quintuple(skb, &quintuple_info);
	

				
				CLOUD_WLAN_DEBUG("out [%x][%x][%d][%d]    [%2x][%2x][%2x][%2x][%2x][%2x]	[%2x][%2x][%2x][%2x][%2x][%2x]\n",
					quintuple_info.saddr,quintuple_info.daddr,
					quintuple_info.protocol,tcphdr->dest,

					quintuple_info.smac[0],quintuple_info.smac[1],
					quintuple_info.smac[2],quintuple_info.smac[3],
					quintuple_info.smac[4],quintuple_info.smac[5],

					quintuple_info.dmac[0],quintuple_info.dmac[1],
					quintuple_info.dmac[2],quintuple_info.dmac[3],
					quintuple_info.dmac[4],quintuple_info.dmac[5]
					);
				//ע�⣬֮ǰ��Ҫ���ж�����ʱ������
				flow_session_up_node(&quintuple_info, 1800);
			}
			break;
		}
	}
	
accept:
	return NF_ACCEPT;
}

u32 kmod_hook_init(void)
{
	/* �ù��Ӷ�Ӧ�Ĵ����� */
	g_cwlan_in_hook_prer.hook = (nf_hookfn *)cwlan_in_hook_prer;
	/* ʹ��IPv4�ĵ�һ��hook */
	g_cwlan_in_hook_prer.hooknum  = NF_IP_PRE_ROUTING;
	g_cwlan_in_hook_prer.pf       = PF_INET; 
	g_cwlan_in_hook_prer.priority = NF_IP_PRI_FIRST;   /* �����ǵĺ�������ִ�� */

	/*���û��Լ�����Ĺ���ע�ᵽ�ں���*/ 
	nf_register_hook(&g_cwlan_in_hook_prer);

	
	/* �ù��Ӷ�Ӧ�Ĵ����� */
	g_cwlan_out_hook_prer.hook = (nf_hookfn *)cwlan_out_hook_prer;
	/* ʹ��IPv4�ĵ�һ��hook */
	g_cwlan_out_hook_prer.hooknum  = NF_IP_POST_ROUTING;
	g_cwlan_out_hook_prer.pf       = PF_INET; 
	//g_cwlan_out_hook_prer.priority = NF_IP_PRI_FIRST;   /* �����ǵĺ�������ִ�� */

	/*���û��Լ�����Ĺ���ע�ᵽ�ں���*/ 
	nf_register_hook(&g_cwlan_out_hook_prer);
	return CWLAN_OK;
}
u32 kmod_hook_exit(void)
{
	//���û��Լ�����Ĺ��Ӵ��ں���ɾ�� 
    nf_unregister_hook(&g_cwlan_in_hook_prer);
	//���û��Լ�����Ĺ��Ӵ��ں���ɾ�� 
    nf_unregister_hook(&g_cwlan_out_hook_prer);
	return CWLAN_OK;
}
extern u32 cloud_wlan_nl_init(void);
extern u32 cloud_wlan_nl_exit(void);

/* cloud_module_init ��- ��ʼ��������
��ģ��װ��ʱ�����ã�����ɹ�װ�ط���0 ���򷵻ط�0ֵ */
s32 __init cloud_module_init(void)
{
	cloud_wlan_nl_init();


	//g_user_white_list init
	cloud_wlan_user_white_list_init();
	//g_dns_white_list init
	cloud_wlan_dns_white_list_init();

	flow_sesison_init();
	//�ض������ó�ʼ��Ĭ��ֵ
	reply_http_redirector_init();

	rwlock_init(&g_rwlock);

	kmod_hook_init();
	
	printk("cw init cloud_wlan kmod finish\n");

	return 0;
}
/*cloud_module_exit ��- �˳�������
��ģ��ж��ʱ������*/
void __exit cloud_module_exit(void)
{
	g_cw_base_cfg.cwlan_sw = CWLAN_CLOSE;
	kmod_hook_exit();
	
	cloud_wlan_nl_exit();

	flow_session_exit();
	
	printk("cw exit cloud_wlan finish!\n");

}
module_init(cloud_module_init);
module_exit(cloud_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("cloud_lzc");
