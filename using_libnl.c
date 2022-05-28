
// Based on (but modified from)
/*************************************************************
* Description: We get some wifi info using nl80211           *
* Licence    : Public Domain.                                *
* Author     : Antonios Tsolis (2016)                        *
*************************************************************/
//
// Modifications by C. Lohr (still under public domain)
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netlink/netlink.h>   //lots of netlink functions
#include <netlink/genl/genl.h> //genl_connect, genlmsg_put
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h> //genl_ctrl_resolve
#include <linux/nl80211.h>     //NL80211 definitions
#include "netlink_names.h"

static volatile int keepRunning = 1;

typedef struct {
	int             id;
	struct nl_sock *socket;
	struct nl_cb *cb3;
	int             result3;
} Netlink;

typedef struct {
	char ifname[30];
	int  ifindex;
	int  signal;
	uint64_t  txrate;
} Wifi;

static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
	[NL80211_STA_INFO_INACTIVE_TIME] = { .type = NLA_U32 },
	[NL80211_STA_INFO_RX_BYTES]      = { .type = NLA_U32 },
	[NL80211_STA_INFO_TX_BYTES]      = { .type = NLA_U32 },
	[NL80211_STA_INFO_RX_PACKETS]    = { .type = NLA_U32 },
	[NL80211_STA_INFO_TX_PACKETS]    = { .type = NLA_U32 },
	[NL80211_STA_INFO_SIGNAL]        = { .type = NLA_U8 },
	[NL80211_STA_INFO_TX_BITRATE]    = { .type = NLA_NESTED },
	[NL80211_STA_INFO_LLID]          = { .type = NLA_U16 },
	[NL80211_STA_INFO_PLID]          = { .type = NLA_U16 },
	[NL80211_STA_INFO_PLINK_STATE]   = { .type = NLA_U8 },
};

static struct nla_policy freq_policy[NL80211_FREQUENCY_ATTR_MAX + 1] = {
	[NL80211_FREQUENCY_ATTR_FREQ]      = { .type = NLA_U32 },
};

static int initNl80211(Netlink *nl, Wifi *w);
static int finish_handler(struct nl_msg *msg, void *arg);
static int getWifiName_callback(struct nl_msg *msg, void *arg);
static int getWifiInfo_callback(struct nl_msg *msg, void *arg);
static int getWifiInfo_callback2(struct nl_msg *msg, void *arg);
static int getWifiStatus(Netlink *nl, Wifi *w);

static int initNl80211(Netlink *nl, Wifi *w)
{
	nl->socket = nl_socket_alloc();
	if (!nl->socket) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}
	printf( "---------------1\n" );
	nl_socket_set_buffer_size(nl->socket, 8192, 8192);
	printf( "---------------2\n" );

	if (genl_connect(nl->socket)) {
		fprintf(stderr, "Failed to connect to netlink socket.\n");
		nl_close(nl->socket);
		nl_socket_free(nl->socket);
		return -ENOLINK;
	}
	printf( "---------------3\n" );

	nl->id = genl_ctrl_resolve(nl->socket, "nl80211");
	if (nl->id < 0) {
		fprintf(stderr, "Nl80211 interface not found.\n");
		nl_close(nl->socket);
		nl_socket_free(nl->socket);
		return -ENOENT;
	}

	printf( "---------------4\n" );
	nl->cb3 = nl_cb_alloc(NL_CB_DEFAULT);
	nl_cb_set(nl->cb3, NL_CB_VALID, NL_CB_CUSTOM, getWifiInfo_callback2, w);
	nl_cb_set(nl->cb3, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &(nl->result3));

	return nl->id;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret     = 0;
	return NL_SKIP;
}



static int getWifiInfo_callback2(struct nl_msg *msg, void *arg)
{
	struct nlattr *    tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *    sinfo[NL80211_STA_INFO_MAX + 1];
	struct nlattr *    rinfo[NL80211_FREQUENCY_ATTR_MAX + 1];
	//nl_msg_dump(msg, stdout);

	nla_parse(tb,
	          NL80211_ATTR_MAX,
	          genlmsg_attrdata(gnlh, 0),
	          genlmsg_attrlen(gnlh, 0),
	          NULL);

	if (tb[NL80211_ATTR_IFNAME]) {
		printf( "IfName: %s\n", nla_get_string(tb[NL80211_ATTR_IFNAME]));
	}

	int i;
	for( i = 0; i < /*NL80211_ATTR_MAX*/sizeof(attrnames)/sizeof(attrnames[0]); i++ )
	{
		if( tb[i] )
		{
			int type = nla_len( tb[i] );
			printf( "%d = %p (%s) (%d) %d\n", i, tb[i], attrnames[i], type, nla_get_u32( tb[i] ) );
		}
	}
	return NL_SKIP;
}

static int getWifiStatus(Netlink *nl, Wifi *w)
{
	nl->result3 = 1;

	struct nl_msg *msg3 = nlmsg_alloc();

	if (!msg3) {
		fprintf(stderr, "Failed to allocate netlink message.\n");
		return -2;
	}

	printf( "%d\n", nl->id );

	genlmsg_put(msg3,
	            NL_AUTO_PORT,
	            NL_AUTO_SEQ,
	            nl->id,
	            0,
	            NLM_F_DUMP,
	            NL80211_CMD_GET_INTERFACE,
	            0);

	nl_send_auto(nl->socket, msg3);
	while (nl->result3 > 0) {
		nl_recvmsgs(nl->socket, nl->cb3);
	}

	nlmsg_free(msg3);

	return 0;
}

int main(int argc, char **argv)
{
	Netlink nl;
	Wifi    w;


	nl.id = initNl80211(&nl, &w);
	if (nl.id < 0) {
		fprintf(stderr, "Error initializing netlink 802.11\n");
		return -1;
	}

		getWifiStatus(&nl, &w);

	printf("Exiting gracefully... ");
	nl_cb_put(nl.cb3);
	nl_close(nl.socket);
	nl_socket_free(nl.socket);
	printf("OK\n");
	return 0;
}

