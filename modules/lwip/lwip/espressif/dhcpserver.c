#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/mem.h"

#include <c_types.h>

#include <etstimer.h>

#include "espressif/esp_wifi.h"

//#include "crypto/common.h"
#include "osapi.h"
#include "lwip/app/dhcpserver.h"

#define LWIP_OPEN_SRC 1

//#ifndef LWIP_OPEN_SRC
//#include "net80211/ieee80211_var.h"
//#endif
#include "user_interface.h"

////////////////////////////////////////////////////////////////////////////////////
static /*const*/ uint8_t xid[4] = {0xad, 0xde, 0x12, 0x23};
static u8_t old_xid[4] = {0};
static const uint8_t magic_cookie[4] = {99, 130, 83, 99};
static struct udp_pcb *pcb_dhcps = NULL;
static struct ip_addr broadcast_dhcps;
static struct ip_addr server_address;
static struct ip_addr client_address;//added
static struct ip_addr client_address_plus;
//static struct dhcps_msg msg_dhcps;
static struct dhcps_msg *pmsg_dhcps = NULL;
struct dhcps_state s;

static struct dhcps_lease dhcps_lease;
static bool dhcps_lease_flag = true;
static list_node *plist = NULL;

#define ICACHE_FLASH_ATTR 
#define DHCPS_DEBUG 	0//1

#define DBG(...) 		//printf(__VA_ARGS__)

/******************************************************************************
 * FunctionName : node_insert_to_list
 * Description  : insert the node to the list
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR node_insert_to_list(list_node **phead, list_node* pinsert)
{
	int i = 0;
DBG("%s: %d\n", __func__, __LINE__);
	list_node *plist = NULL;

	if (*phead == NULL)
		*phead = pinsert;
	else {
		plist = *phead;
		while (plist->pnext != NULL) {
			i++;
			plist = plist->pnext;
		}
		plist->pnext = pinsert;
	}
	pinsert->pnext = NULL;
	
	printf("%s: %d i = %d\n", __func__, __LINE__, i);
}

/******************************************************************************
 * FunctionName : node_delete_from_list
 * Description  : remove the node from list
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR node_remove_from_list(list_node **phead, list_node* pdelete)
{
DBG("%s: %d\n", __func__, __LINE__);
	list_node *plist = NULL;

	plist = *phead;
	if (plist == NULL){
		*phead = NULL;
	} else {
		if (plist == pdelete){
			*phead = plist->pnext;
		} else {
			while (plist != NULL) {
				if (plist->pnext == pdelete){
					plist->pnext = pdelete->pnext;
				}
				plist = plist->pnext;
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ��DHCP msg��Ϣ�ṹ����������
 *
 * @param optptr -- DHCP msg��Ϣλ��
 * @param type -- Ҫ��ӵ�����option
 *
 * @return uint8_t* ����DHCP msgƫ�Ƶ�ַ
 */
///////////////////////////////////////////////////////////////////////////////////
static uint8_t* ICACHE_FLASH_ATTR add_msg_type(uint8_t *optptr, uint8_t type)
{
DBG("%s: %d\n", __func__, __LINE__);

        *optptr++ = DHCP_OPTION_MSG_TYPE;
        *optptr++ = 1;
        *optptr++ = type;
        return optptr;
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ��DHCP msg�ṹ������offerӦ������
 *
 * @param optptr -- DHCP msg��Ϣλ��
 *
 * @return uint8_t* ����DHCP msgƫ�Ƶ�ַ
 */
///////////////////////////////////////////////////////////////////////////////////
static uint8_t* ICACHE_FLASH_ATTR add_offer_options(uint8_t *optptr)
{
DBG("%s: %d\n", __func__, __LINE__);
        struct ip_addr ipadd;

        ipadd.addr = *( (uint32_t *) &server_address);

#ifdef USE_CLASS_B_NET
        *optptr++ = DHCP_OPTION_SUBNET_MASK;
        *optptr++ = 4;  //length
        *optptr++ = 255;
        *optptr++ = 240;	
        *optptr++ = 0;
        *optptr++ = 0;
#else
        *optptr++ = DHCP_OPTION_SUBNET_MASK;
        *optptr++ = 4;  
        *optptr++ = 255;
        *optptr++ = 255;	
        *optptr++ = 255;
        *optptr++ = 0;
#endif

        *optptr++ = DHCP_OPTION_LEASE_TIME;
        *optptr++ = 4;  
        *optptr++ = 0x00;
        *optptr++ = 0x01;
        *optptr++ = 0x51;
        *optptr++ = 0x80; 	

        *optptr++ = DHCP_OPTION_SERVER_ID;
        *optptr++ = 4;  
        *optptr++ = ip4_addr1( &ipadd);
        *optptr++ = ip4_addr2( &ipadd);
        *optptr++ = ip4_addr3( &ipadd);
        *optptr++ = ip4_addr4( &ipadd);

	    *optptr++ = DHCP_OPTION_ROUTER;
	    *optptr++ = 4;  
	    *optptr++ = ip4_addr1( &ipadd);
	    *optptr++ = ip4_addr2( &ipadd);
	    *optptr++ = ip4_addr3( &ipadd);
	    *optptr++ = ip4_addr4( &ipadd);

#ifdef USE_DNS
	    *optptr++ = DHCP_OPTION_DNS_SERVER;
	    *optptr++ = 4;
	    *optptr++ = ip4_addr1( &ipadd);
		*optptr++ = ip4_addr2( &ipadd);
		*optptr++ = ip4_addr3( &ipadd);
		*optptr++ = ip4_addr4( &ipadd);
#endif

#ifdef CLASS_B_NET
        *optptr++ = DHCP_OPTION_BROADCAST_ADDRESS;
        *optptr++ = 4;  
        *optptr++ = ip4_addr1( &ipadd);
        *optptr++ = 255;
        *optptr++ = 255;
        *optptr++ = 255;
#else
        *optptr++ = DHCP_OPTION_BROADCAST_ADDRESS;
        *optptr++ = 4;  
        *optptr++ = ip4_addr1( &ipadd);
        *optptr++ = ip4_addr2( &ipadd);
        *optptr++ = ip4_addr3( &ipadd);
        *optptr++ = 255;
#endif

        *optptr++ = DHCP_OPTION_INTERFACE_MTU;
        *optptr++ = 2;  
#ifdef CLASS_B_NET
        *optptr++ = 0x05;	
        *optptr++ = 0xdc;
#else
        *optptr++ = 0x02;	
        *optptr++ = 0x40;
#endif

        *optptr++ = DHCP_OPTION_PERFORM_ROUTER_DISCOVERY;
        *optptr++ = 1;  
        *optptr++ = 0x00; 

        *optptr++ = 43;	
        *optptr++ = 6;	

        *optptr++ = 0x01;	
        *optptr++ = 4;  
        *optptr++ = 0x00;
        *optptr++ = 0x00;
        *optptr++ = 0x00;
        *optptr++ = 0x02; 	

        return optptr;
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ��DHCP msg�ṹ����ӽ����־����
 *
 * @param optptr -- DHCP msg��Ϣλ��
 *
 * @return uint8_t* ����DHCP msgƫ�Ƶ�ַ
 */
///////////////////////////////////////////////////////////////////////////////////
static uint8_t* ICACHE_FLASH_ATTR add_end(uint8_t *optptr)
{
DBG("%s: %d\n", __func__, __LINE__);

        *optptr++ = DHCP_OPTION_END;
        return optptr;
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ����һ��DHCP msg�ṹ��
 *
 * @param -- m ָ�򴴽���DHCP msg�ṹ�����?
 */
///////////////////////////////////////////////////////////////////////////////////
static void ICACHE_FLASH_ATTR create_msg(struct dhcps_msg *m)
{
DBG("%s: %d m=%x\n", __func__, __LINE__, m);
        struct ip_addr client;

        client.addr = *( (uint32_t *) &client_address);

        m->op = DHCP_REPLY;
        m->htype = DHCP_HTYPE_ETHERNET;
        m->hlen = 6;  
        m->hops = 0;
DBG("%s: %d sizeof(m->xid)=%d\n", __func__, __LINE__, sizeof(m->xid));
        memcpy((char *) xid, (char *) m->xid, sizeof(m->xid));
DBG("%s: %d\n", __func__, __LINE__);
        m->secs = 0;
        m->flags = htons(BOOTP_BROADCAST); 

DBG("%s: %d\n", __func__, __LINE__);
        memcpy((char *) m->yiaddr, (char *) &client.addr, sizeof(m->yiaddr));

DBG("%s: %d\n", __func__, __LINE__);
        memset((char *) m->ciaddr, 0, sizeof(m->ciaddr));
        memset((char *) m->siaddr, 0, sizeof(m->siaddr));
        memset((char *) m->giaddr, 0, sizeof(m->giaddr));
        memset((char *) m->sname, 0, sizeof(m->sname));
        memset((char *) m->file, 0, sizeof(m->file));

        memset((char *) m->options, 0, sizeof(m->options));
DBG("%s: %d\n", __func__, __LINE__);
        memcpy((char *) m->options, (char *) magic_cookie, sizeof(magic_cookie));
DBG("%s: %d\n", __func__, __LINE__);
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ����һ��OFFER
 *
 * @param -- m ָ����Ҫ���͵�DHCP msg����
 */
///////////////////////////////////////////////////////////////////////////////////
static void ICACHE_FLASH_ATTR send_offer(struct dhcps_msg *m)
{
DBG("%s: %d\n", __func__, __LINE__);
       uint8_t *end;
	    struct pbuf *p, *q;
	    u8_t *data;
	    u16_t cnt=0;
	    u16_t i;
		err_t SendOffer_err_t;
        create_msg(m);

        end = add_msg_type(&m->options[4], DHCPOFFER);
        end = add_offer_options(end);
        end = add_end(end);

	    p = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct dhcps_msg), PBUF_RAM);
#if DHCPS_DEBUG
		printf("udhcp: send_offer>>p->ref = %d\n", p->ref);
#endif
	    if(p != NULL){
	       
#if DHCPS_DEBUG
	        printf("dhcps: send_offer>>pbuf_alloc succeed\n");
	        printf("dhcps: send_offer>>p->tot_len = %d\n", p->tot_len);
	        printf("dhcps: send_offer>>p->len = %d\n", p->len);
#endif
	        q = p;
	        while(q != NULL){
	            data = (u8_t *)q->payload;
	            for(i=0; i<q->len; i++)
	            {
	                data[i] = ((u8_t *) m)[cnt++];
#if DHCPS_DEBUG
					printf("%02x ",data[i]);
					if((i+1)%16 == 0){
						printf("\n");
					}
#endif
	            }

	            q = q->next;
	        }
	    }else{
	        
#if DHCPS_DEBUG
	        printf("dhcps: send_offer>>pbuf_alloc failed\n");
#endif
	        return;
	    }
        SendOffer_err_t = udp_sendto( pcb_dhcps, p, &broadcast_dhcps, DHCPS_CLIENT_PORT );
#if DHCPS_DEBUG
	        printf("dhcps: send_offer>>udp_sendto result %x\n",SendOffer_err_t);
#endif
	    if(p->ref != 0){	
#if DHCPS_DEBUG
	        printf("udhcp: send_offer>>free pbuf\n");
#endif
	        pbuf_free(p);
	    }
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ����һ��NAK��Ϣ
 *
 * @param m ָ����Ҫ���͵�DHCP msg����
 */
///////////////////////////////////////////////////////////////////////////////////
static void ICACHE_FLASH_ATTR send_nak(struct dhcps_msg *m)
{
DBG("%s: %d sizeof(struct dhcps_msg) = %d\n", __func__, __LINE__, sizeof(struct dhcps_msg));

    	u8_t *end;
	    struct pbuf *p, *q;
	    u8_t *data;
	    u16_t cnt=0;
	    u16_t i;
		err_t SendNak_err_t;
        create_msg(m);

        end = add_msg_type(&m->options[4], DHCPNAK);
        end = add_end(end);

	    p = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct dhcps_msg), PBUF_RAM);
#if DHCPS_DEBUG
		printf("udhcp: send_nak>>p->ref = %d\n", p->ref);
#endif
	    if(p != NULL){
	        
#if DHCPS_DEBUG
	        printf("dhcps: send_nak>>pbuf_alloc succeed\n");
	        printf("dhcps: send_nak>>p->tot_len = %d\n", p->tot_len);
	        printf("dhcps: send_nak>>p->len = %d\n", p->len);
#endif
	        q = p;
	        while(q != NULL){
	            data = (u8_t *)q->payload;
	            for(i=0; i<q->len; i++)
	            {
	                data[i] = ((u8_t *) m)[cnt++];
#if DHCPS_DEBUG					
					printf("%02x ",data[i]);
					if((i+1)%16 == 0){
						printf("\n");
					}
#endif
	            }

	            q = q->next;
	        }
	    }else{
	        
#if DHCPS_DEBUG
	        printf("dhcps: send_nak>>pbuf_alloc failed\n");
#endif
	        return;
    	}
        SendNak_err_t = udp_sendto( pcb_dhcps, p, &broadcast_dhcps, DHCPS_CLIENT_PORT );
#if DHCPS_DEBUG
	        printf("dhcps: send_nak>>udp_sendto result %x\n",SendNak_err_t);
#endif
 	    if(p->ref != 0){
#if DHCPS_DEBUG			
	        printf("udhcp: send_nak>>free pbuf\n");
#endif
	        pbuf_free(p);
	    }
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ����һ��ACK��DHCP�ͻ���
 *
 * @param m ָ����Ҫ���͵�DHCP msg����
 */
///////////////////////////////////////////////////////////////////////////////////
static void ICACHE_FLASH_ATTR send_ack(struct dhcps_msg *m)
{
DBG("%s: %d\n", __func__, __LINE__);

		u8_t *end;
	    struct pbuf *p, *q;
	    u8_t *data;
	    u16_t cnt=0;
	    u16_t i;
		err_t SendAck_err_t;
        create_msg(m);

        end = add_msg_type(&m->options[4], DHCPACK);
        end = add_offer_options(end);
        end = add_end(end);
	    
	    p = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct dhcps_msg), PBUF_RAM);
#if DHCPS_DEBUG
		printf("udhcp: send_ack>>p->ref = %d\n", p->ref);
#endif
	    if(p != NULL){
	        
#if DHCPS_DEBUG
	        printf("dhcps: send_ack>>pbuf_alloc succeed\n");
	        printf("dhcps: send_ack>>p->tot_len = %d\n", p->tot_len);
	        printf("dhcps: send_ack>>p->len = %d\n", p->len);
#endif
	        q = p;
	        while(q != NULL){
	            data = (u8_t *)q->payload;
	            for(i=0; i<q->len; i++)
	            {
	                data[i] = ((u8_t *) m)[cnt++];
#if DHCPS_DEBUG					
					printf("%02x ",data[i]);
					if((i+1)%16 == 0){
						printf("\n");
					}
#endif
	            }

	            q = q->next;
	        }
	    }else{
	    
#if DHCPS_DEBUG
	        printf("dhcps: send_ack>>pbuf_alloc failed\n");
#endif
	        return;
	    }
        SendAck_err_t = udp_sendto( pcb_dhcps, p, &broadcast_dhcps, DHCPS_CLIENT_PORT );
#if DHCPS_DEBUG
	        printf("dhcps: send_ack>>udp_sendto result %x\n",SendAck_err_t);
#endif
	    
	    if(p->ref != 0){
#if DHCPS_DEBUG
	        printf("udhcp: send_ack>>free pbuf\n");
#endif
	        pbuf_free(p);
	    }
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ����DHCP�ͻ��˷�����DHCP����������Ϣ�����Բ�ͬ��DHCP��������������Ӧ��Ӧ��
 *
 * @param optptr DHCP msg�е���������
 * @param len ��������Ĵ��?(byte)
 *
 * @return uint8_t ���ش�����DHCP Server״ֵ̬
 */
///////////////////////////////////////////////////////////////////////////////////
static uint8_t ICACHE_FLASH_ATTR parse_options(uint8_t *optptr, sint16_t len)
{
DBG("%s: %d\n", __func__, __LINE__);
        struct ip_addr client;
    	bool is_dhcp_parse_end = false;

        client.addr = *( (uint32_t *) &client_address);// Ҫ�����DHCP�ͻ��˵�IP

        u8_t *end = optptr + len;
        u16_t type = 0;

        s.state = DHCPS_STATE_IDLE;

        while (optptr < end) {
#if DHCPS_DEBUG
        	printf("dhcps: (sint16_t)*optptr = %d\n", (sint16_t)*optptr);
#endif
        	switch ((sint16_t) *optptr) {

                case DHCP_OPTION_MSG_TYPE:	//53
                        type = *(optptr + 2);
                        break;

                case DHCP_OPTION_REQ_IPADDR://50
                        if( memcmp( (char *) &client.addr, (char *) optptr+2,4)==0 ) {
#if DHCPS_DEBUG
                    		printf("dhcps: DHCP_OPTION_REQ_IPADDR = 0 ok\n");
#endif
                            s.state = DHCPS_STATE_ACK;
                        }else {
#if DHCPS_DEBUG
                    		printf("dhcps: DHCP_OPTION_REQ_IPADDR != 0 err\n");
#endif
                            s.state = DHCPS_STATE_NAK;
                        }
                        break;
                case DHCP_OPTION_END:
			            {
			                is_dhcp_parse_end = true;
			            }
                        break;
            }

		    if(is_dhcp_parse_end){
		            break;
		    }

            optptr += optptr[1] + 2;
        }

        switch (type){
        
        	case DHCPDISCOVER://1
                s.state = DHCPS_STATE_OFFER;
#if DHCPS_DEBUG
            	printf("dhcps: DHCPD_STATE_OFFER\n");
#endif
                break;

        	case DHCPREQUEST://3
                if ( !(s.state == DHCPS_STATE_ACK || s.state == DHCPS_STATE_NAK) ) {
                        s.state = DHCPS_STATE_NAK;
#if DHCPS_DEBUG
                		printf("dhcps: DHCPD_STATE_NAK\n");
#endif
                }
                break;

			case DHCPDECLINE://4
                s.state = DHCPS_STATE_IDLE;
#if DHCPS_DEBUG
            	printf("dhcps: DHCPD_STATE_IDLE\n");
#endif
                break;

        	case DHCPRELEASE://7
                s.state = DHCPS_STATE_IDLE;
#if DHCPS_DEBUG
            	printf("dhcps: DHCPD_STATE_IDLE\n");
#endif
                break;
        }
#if DHCPS_DEBUG
    	printf("dhcps: return s.state = %d\n", s.state);
#endif
        return s.state;
}
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
static sint16_t ICACHE_FLASH_ATTR parse_msg(struct dhcps_msg *m, u16_t len)
{
DBG("%s: %d sizeof(struct dhcps_msg)=%d\n", __func__, __LINE__, sizeof(struct dhcps_msg) );
		if(memcmp((char *)m->options,
              (char *)magic_cookie,
              sizeof(magic_cookie)) == 0){
#if DHCPS_DEBUG
        	printf("dhcps: len = %d\n", len);
#endif
	        /*
         	 * ��¼��ǰ��xid���ﴦ���?
         	 * �˺�ΪDHCP�ͻ����������û�ͳһ��ȡIPʱ��
         	*/
//	        if((old_xid[0] == 0) &&
//	           (old_xid[1] == 0) &&
//	           (old_xid[2] == 0) &&
//	           (old_xid[3] == 0)){
//	            /*
//	             * old_xidδ��¼�κ����?
//	             * �϶��ǵ�һ��ʹ��
//	            */
//	            memcpy((char *)old_xid, (char *)m->xid, sizeof(m->xid));
//	        }else{
//	            /*
//	             * ���δ����DHCP msg��Я���xid���ϴμ�¼�Ĳ�ͬ��
//	             * �϶�Ϊ��ͬ��DHCP�ͻ��˷��ͣ���ʱ����Ҫ����Ŀͻ���IP
//	             * ���� 192.168.4.100(0x6404A8C0) <--> 192.168.4.200(0xC804A8C0)
//	             *
//	            */
//	            if(memcmp((char *)old_xid, (char *)m->xid, sizeof(m->xid)) != 0){
	                /*
                 	 * ��¼���ε�xid�ţ�ͬʱ�����IP����
                 	*/
	                struct ip_addr addr_tmp;    
	                memcpy((char *)old_xid, (char *)m->xid, sizeof(m->xid));

	                {
						struct dhcps_pool *pdhcps_pool = NULL;
						list_node *pnode = NULL;
						list_node *pback_node = NULL;

						POOL_START:
						client_address.addr = client_address_plus.addr;
//							addr_tmp.addr =  htonl(client_address_plus.addr);
//							addr_tmp.addr++;
//							client_address_plus.addr = htonl(addr_tmp.addr);
						for (pback_node = plist; pback_node != NULL;pback_node = pback_node->pnext) {
							pdhcps_pool = pback_node->pnode;
							if (memcmp(pdhcps_pool->mac, m->chaddr, sizeof(pdhcps_pool->mac)) == 0){
//									printf("the same device request ip\n");
								client_address.addr = pdhcps_pool->ip.addr;
								pdhcps_pool->lease_timer = DHCPS_LEASE_TIMER;
								goto POOL_CHECK;
							}else if (pdhcps_pool->ip.addr == client_address_plus.addr){
//									client_address.addr = client_address_plus.addr;
//									printf("the ip addr has been request\n");
								addr_tmp.addr = htonl(client_address_plus.addr);
								addr_tmp.addr++;
								client_address_plus.addr = htonl(addr_tmp.addr);
								client_address.addr = client_address_plus.addr;
							}
						}
						pdhcps_pool = (struct dhcps_pool *)zalloc(sizeof(struct dhcps_pool));
						pdhcps_pool->ip.addr = client_address.addr;
						memcpy(pdhcps_pool->mac, m->chaddr, sizeof(pdhcps_pool->mac));
						pdhcps_pool->lease_timer = DHCPS_LEASE_TIMER;
						pnode = (list_node *)zalloc(sizeof(list_node ));
						pnode->pnode = pdhcps_pool;
						node_insert_to_list(&plist, pnode);

						POOL_CHECK:
						if ((client_address_plus.addr > dhcps_lease.end_ip) || (ip_addr_isany(&client_address))){
							client_address_plus.addr = dhcps_lease.start_ip;
							goto POOL_START;
						}

                        if (sdk_wifi_softap_set_station_info(m->chaddr, &client_address) == false) {
							return 0;
						}
					}

#if DHCPS_DEBUG
	                printf("dhcps: xid changed\n");
	                printf("dhcps: client_address.addr = %x\n", client_address.addr);
#endif
	               
//	            }
	            
//	        }
                    
	        return parse_options(&m->options[4], len);
	    }
        return 0;
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * DHCP ��������ݰ���մ���ص�����˺�����LWIP UDPģ������ʱ������
 * ��Ҫ����udp_recv()������LWIP����ע��.
 *
 * @param arg
 * @param pcb ���յ�UDP��Ŀ��ƿ�?
 * @param p ���յ���UDP�е��������?
 * @param addr ���ʹ�UDP���Դ�����IP��ַ
 * @param port ���ʹ�UDP���Դ�����UDPͨ���˿ں�
 */
///////////////////////////////////////////////////////////////////////////////////
static void ICACHE_FLASH_ATTR handle_dhcp(void *arg, 
									struct udp_pcb *pcb, 
									struct pbuf *p, 
									struct ip_addr *addr, 
									uint16_t port)
{
DBG("%s: %d\n", __func__, __LINE__);
		
		pmsg_dhcps = malloc( sizeof(dhcps_msg) );
		if(pmsg_dhcps == NULL) return;
		
		sint16_t tlen;
        u16_t i;
	    u16_t dhcps_msg_cnt=0;
	    u8_t *p_dhcps_msg = (u8_t *)pmsg_dhcps /*&msg_dhcps*/;
	    u8_t *data;

#if DHCPS_DEBUG
    	printf("dhcps: handle_dhcp-> receive a packet\n");
#endif
	    if (p==NULL) {
			free( pmsg_dhcps );
			pmsg_dhcps = NULL;
			return;
	    }

		tlen = p->tot_len;
	    data = p->payload;

#if DHCPS_DEBUG
	    printf("dhcps: handle_dhcp-> p->tot_len = %d\n", tlen);
	    printf("dhcps: handle_dhcp-> p->len = %d\n", p->len);
#endif		

	    memset(pmsg_dhcps /*&msg_dhcps*/, 0, sizeof(dhcps_msg));
	    for(i=0; i<p->len; i++){
	        p_dhcps_msg[dhcps_msg_cnt++] = data[i];
#if DHCPS_DEBUG					
			printf("%02x ",data[i]);
			if((i+1)%16 == 0){
				printf("\n");
			}
#endif
	    }
		
		if(p->next != NULL) {
#if DHCPS_DEBUG
	        printf("dhcps: handle_dhcp-> p->next != NULL\n");
	        printf("dhcps: handle_dhcp-> p->next->tot_len = %d\n",p->next->tot_len);
	        printf("dhcps: handle_dhcp-> p->next->len = %d\n",p->next->len);
#endif
			
	        data = p->next->payload;
	        for(i=0; i<p->next->len; i++){
	            p_dhcps_msg[dhcps_msg_cnt++] = data[i];
#if DHCPS_DEBUG					
				printf("%02x ",data[i]);
				if((i+1)%16 == 0){
					printf("\n");
				}
#endif
			}
		}

		/*
	     * DHCP �ͻ���������Ϣ����
	    */
#if DHCPS_DEBUG
    	printf("dhcps: handle_dhcp-> parse_msg(p)\n");
#endif
		
        switch(parse_msg(pmsg_dhcps /*&msg_dhcps*/, tlen - 240)) {

	        case DHCPS_STATE_OFFER://1
#if DHCPS_DEBUG            
            	 printf("dhcps: handle_dhcp-> DHCPD_STATE_OFFER\n");
#endif			
	             send_offer(pmsg_dhcps /*&msg_dhcps*/);
	             break;
	        case DHCPS_STATE_ACK://3
#if DHCPS_DEBUG
            	 printf("dhcps: handle_dhcp-> DHCPD_STATE_ACK\n");
#endif			
	             send_ack(pmsg_dhcps /*&msg_dhcps*/);
	             break;
	        case DHCPS_STATE_NAK://4
#if DHCPS_DEBUG            
            	 printf("dhcps: handle_dhcp-> DHCPD_STATE_NAK\n");
#endif
	             send_nak(pmsg_dhcps /*&msg_dhcps*/);
	             break;
			default :
				 break;
        }
#if DHCPS_DEBUG
    	printf("dhcps: handle_dhcp-> pbuf_free(p)\n");
#endif
        pbuf_free(p);

		free( pmsg_dhcps );
		pmsg_dhcps = NULL;
}
///////////////////////////////////////////////////////////////////////////////////
static void ICACHE_FLASH_ATTR wifi_softap_init_dhcps_lease(uint32 ip)
{
DBG("%s: %d\n", __func__, __LINE__);
	uint32 softap_ip = 0,local_ip = 0;

	if (dhcps_lease_flag) {
		local_ip = softap_ip = htonl(ip);
		softap_ip &= 0xFFFFFF00;
		local_ip &= 0xFF;
		if (local_ip >= 0x80)
			local_ip -= DHCPS_MAX_LEASE;
		else
			local_ip ++;

		bzero(&dhcps_lease, sizeof(dhcps_lease));
		dhcps_lease.start_ip = softap_ip | local_ip;
		dhcps_lease.end_ip = softap_ip | (local_ip + DHCPS_MAX_LEASE);
	}
	dhcps_lease.start_ip = htonl(dhcps_lease.start_ip);
	dhcps_lease.end_ip= htonl(dhcps_lease.end_ip);
//	printf("start_ip = 0x%x, end_ip = 0x%x\n",dhcps_lease.start_ip, dhcps_lease.end_ip);
}
///////////////////////////////////////////////////////////////////////////////////
void ICACHE_FLASH_ATTR dhcps_start(struct ip_info *info)
{
DBG("%s: %d\n", __func__, __LINE__);
	pmsg_dhcps = malloc( sizeof(dhcps_msg) );
	if(pmsg_dhcps == NULL) return; 
	memset(pmsg_dhcps /*&msg_dhcps*/, 0, sizeof(dhcps_msg));
DBG("%s: %d\n", __func__, __LINE__);
	pcb_dhcps = udp_new();
	if (pcb_dhcps == NULL || info ==NULL) {
		printf("dhcps_start(): could not obtain pcb\n");
		free( pmsg_dhcps );
		pmsg_dhcps = NULL;
		return;
	}

	IP4_ADDR(&broadcast_dhcps, 255, 255, 255, 255);

	server_address = info->ip;
	wifi_softap_init_dhcps_lease(server_address.addr);
DBG("%s: %d\n", __func__, __LINE__);
	client_address_plus.addr = dhcps_lease.start_ip;

	udp_bind(pcb_dhcps, IP_ADDR_ANY, DHCPS_SERVER_PORT);
DBG("%s: %d\n", __func__, __LINE__);
	udp_recv(pcb_dhcps, handle_dhcp, NULL);
DBG("%s: %d\n", __func__, __LINE__);
#if DHCPS_DEBUG
	printf("dhcps:dhcps_start->udp_recv function Set a receive callback handle_dhcp for UDP_PCB pcb_dhcps\n");
#endif
	free( pmsg_dhcps );
	pmsg_dhcps = NULL;
}

void ICACHE_FLASH_ATTR dhcps_stop(void)
{
DBG("%s: %d\n", __func__, __LINE__);
	if(pcb_dhcps == NULL) return;
	
	udp_disconnect(pcb_dhcps);
	udp_remove(pcb_dhcps);
	pcb_dhcps = NULL;
	
	list_node *pnode = NULL;
	list_node *pback_node = NULL;
	pnode = plist;
	while (pnode != NULL) {
		pback_node = pnode;
		pnode = pback_node->pnext;
		node_remove_from_list(&plist, pback_node);
		free(pback_node->pnode);
		pback_node->pnode = NULL;
		free(pback_node);
		pback_node = NULL;
	}
}

bool ICACHE_FLASH_ATTR wifi_softap_set_dhcps_lease(struct dhcps_lease *please)
{
DBG("%s: %d\n", __func__, __LINE__);
	struct ip_info info;
	uint32 softap_ip = 0;
	if (please == NULL)
		return false;

	bzero(&info, sizeof(struct ip_info));
	sdk_wifi_get_ip_info(SOFTAP_IF, &info);
	softap_ip = htonl(info.ip.addr);
	please->start_ip = htonl(please->start_ip);
	please->end_ip = htonl(please->end_ip);

	/*config ip information can't contain local ip*/
	if ((please->start_ip <= softap_ip) && (softap_ip <= please->end_ip))
		return false;

	/*config ip information must be in the same segment as the local ip*/
	softap_ip >>= 8;
	if ((please->start_ip >> 8 != softap_ip)
			|| (please->end_ip >> 8 != softap_ip)) {
		return false;
	}

	if (please->end_ip - please->start_ip > DHCPS_MAX_LEASE)
		return false;

	bzero(&dhcps_lease, sizeof(dhcps_lease));
	dhcps_lease.start_ip = please->start_ip;
	dhcps_lease.end_ip = please->end_ip;
	dhcps_lease_flag = false;
	return true;
}

void ICACHE_FLASH_ATTR dhcps_coarse_tmr(void)
{
DBG("%s: %d\n", __func__, __LINE__);
	list_node *pback_node = NULL;
	list_node *pnode = NULL;
	struct dhcps_pool *pdhcps_pool = NULL;
	pnode = plist;
	while (pnode != NULL) {
		pdhcps_pool = pnode->pnode;
		pdhcps_pool->lease_timer --;
		if (pdhcps_pool->lease_timer == 0){
			pback_node = pnode;
			pnode = pback_node->pnext;
			node_remove_from_list(&plist,pback_node);
			free(pback_node->pnode);
			pback_node->pnode = NULL;
			free(pback_node);
			pback_node = NULL;
		} else {
			pnode = pnode ->pnext;
		}
	}
}

