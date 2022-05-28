
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

static volatile int keepRunning = 1;

const char * attrnames[] = {
	"NL80211_ATTR_UNSPEC",
	"NL80211_ATTR_WIPHY",
	"NL80211_ATTR_WIPHY_NAME",
	"NL80211_ATTR_IFINDEX",
	"NL80211_ATTR_IFNAME",
	"NL80211_ATTR_IFTYPE",
	"NL80211_ATTR_MAC",
	"NL80211_ATTR_KEY_DATA",
	"NL80211_ATTR_KEY_IDX",
	"NL80211_ATTR_KEY_CIPHER",
	"NL80211_ATTR_KEY_SEQ",
	"NL80211_ATTR_KEY_DEFAULT",
	"NL80211_ATTR_BEACON_INTERVAL",
	"NL80211_ATTR_DTIM_PERIOD",
	"NL80211_ATTR_BEACON_HEAD",
	"NL80211_ATTR_BEACON_TAIL",
	"NL80211_ATTR_STA_AID",
	"NL80211_ATTR_STA_FLAGS",
	"NL80211_ATTR_STA_LISTEN_INTERVAL",
	"NL80211_ATTR_STA_SUPPORTED_RATES",
	"NL80211_ATTR_STA_VLAN",
	"NL80211_ATTR_STA_INFO",
	"NL80211_ATTR_WIPHY_BANDS",
	"NL80211_ATTR_MNTR_FLAGS",
	"NL80211_ATTR_MESH_ID",
	"NL80211_ATTR_STA_PLINK_ACTION",
	"NL80211_ATTR_MPATH_NEXT_HOP",
	"NL80211_ATTR_MPATH_INFO",
	"NL80211_ATTR_BSS_CTS_PROT",
	"NL80211_ATTR_BSS_SHORT_PREAMBLE",
	"NL80211_ATTR_BSS_SHORT_SLOT_TIME",
	"NL80211_ATTR_HT_CAPABILITY",
	"NL80211_ATTR_SUPPORTED_IFTYPES",
	"NL80211_ATTR_REG_ALPHA2",
	"NL80211_ATTR_REG_RULES",
	"NL80211_ATTR_MESH_CONFIG",
	"NL80211_ATTR_BSS_BASIC_RATES",
	"NL80211_ATTR_WIPHY_TXQ_PARAMS",
	"NL80211_ATTR_WIPHY_FREQ",
	"NL80211_ATTR_WIPHY_CHANNEL_TYPE",
	"NL80211_ATTR_KEY_DEFAULT_MGMT",
	"NL80211_ATTR_MGMT_SUBTYPE",
	"NL80211_ATTR_IE",
	"NL80211_ATTR_MAX_NUM_SCAN_SSIDS",
	"NL80211_ATTR_SCAN_FREQUENCIES",
	"NL80211_ATTR_SCAN_SSIDS",
	"NL80211_ATTR_GENERATION",
	"NL80211_ATTR_BSS",
	"NL80211_ATTR_REG_INITIATOR",
	"NL80211_ATTR_REG_TYPE",
	"NL80211_ATTR_SUPPORTED_COMMANDS",
	"NL80211_ATTR_FRAME",
	"NL80211_ATTR_SSID",
	"NL80211_ATTR_AUTH_TYPE",
	"NL80211_ATTR_REASON_CODE",
	"NL80211_ATTR_KEY_TYPE",
	"NL80211_ATTR_MAX_SCAN_IE_LEN",
	"NL80211_ATTR_CIPHER_SUITES",
	"NL80211_ATTR_FREQ_BEFORE",
	"NL80211_ATTR_FREQ_AFTER",
	"NL80211_ATTR_FREQ_FIXED",
	"NL80211_ATTR_WIPHY_RETRY_SHORT",
	"NL80211_ATTR_WIPHY_RETRY_LONG",
	"NL80211_ATTR_WIPHY_FRAG_THRESHOLD",
	"NL80211_ATTR_WIPHY_RTS_THRESHOLD",
	"NL80211_ATTR_TIMED_OUT",
	"NL80211_ATTR_USE_MFP",
	"NL80211_ATTR_STA_FLAGS2",
	"NL80211_ATTR_CONTROL_PORT",
	"NL80211_ATTR_TESTDATA",
	"NL80211_ATTR_PRIVACY",
	"NL80211_ATTR_DISCONNECTED_BY_AP",
	"NL80211_ATTR_STATUS_CODE",
	"NL80211_ATTR_CIPHER_SUITES_PAIRWISE",
	"NL80211_ATTR_CIPHER_SUITE_GROUP",
	"NL80211_ATTR_WPA_VERSIONS",
	"NL80211_ATTR_AKM_SUITES",
	"NL80211_ATTR_REQ_IE",
	"NL80211_ATTR_RESP_IE",
	"NL80211_ATTR_PREV_BSSID",
	"NL80211_ATTR_KEY",
	"NL80211_ATTR_KEYS",
	"NL80211_ATTR_PID",
	"NL80211_ATTR_4ADDR",
	"NL80211_ATTR_SURVEY_INFO",
	"NL80211_ATTR_PMKID",
	"NL80211_ATTR_MAX_NUM_PMKIDS",
	"NL80211_ATTR_DURATION",
	"NL80211_ATTR_COOKIE",
	"NL80211_ATTR_WIPHY_COVERAGE_CLASS",
	"NL80211_ATTR_TX_RATES",
	"NL80211_ATTR_FRAME_MATCH",
	"NL80211_ATTR_ACK",
	"NL80211_ATTR_PS_STATE",
	"NL80211_ATTR_CQM",
	"NL80211_ATTR_LOCAL_STATE_CHANGE",
	"NL80211_ATTR_AP_ISOLATE",
	"NL80211_ATTR_WIPHY_TX_POWER_SETTING",
	"NL80211_ATTR_WIPHY_TX_POWER_LEVEL",
	"NL80211_ATTR_TX_FRAME_TYPES",
	"NL80211_ATTR_RX_FRAME_TYPES",
	"NL80211_ATTR_FRAME_TYPE",
	"NL80211_ATTR_CONTROL_PORT_ETHERTYPE",
	"NL80211_ATTR_CONTROL_PORT_NO_ENCRYPT",
	"NL80211_ATTR_SUPPORT_IBSS_RSN",
	"NL80211_ATTR_WIPHY_ANTENNA_TX",
	"NL80211_ATTR_WIPHY_ANTENNA_RX",
	"NL80211_ATTR_MCAST_RATE",
	"NL80211_ATTR_OFFCHANNEL_TX_OK",
	"NL80211_ATTR_BSS_HT_OPMODE",
	"NL80211_ATTR_KEY_DEFAULT_TYPES",
	"NL80211_ATTR_MAX_REMAIN_ON_CHANNEL_DURATION",
	"NL80211_ATTR_MESH_SETUP",
	"NL80211_ATTR_WIPHY_ANTENNA_AVAIL_TX",
	"NL80211_ATTR_WIPHY_ANTENNA_AVAIL_RX",
	"NL80211_ATTR_SUPPORT_MESH_AUTH",
	"NL80211_ATTR_STA_PLINK_STATE",
	"NL80211_ATTR_WOWLAN_TRIGGERS",
	"NL80211_ATTR_WOWLAN_TRIGGERS_SUPPORTED",
	"NL80211_ATTR_SCHED_SCAN_INTERVAL",
	"NL80211_ATTR_INTERFACE_COMBINATIONS",
	"NL80211_ATTR_SOFTWARE_IFTYPES",
	"NL80211_ATTR_REKEY_DATA",
	"NL80211_ATTR_MAX_NUM_SCHED_SCAN_SSIDS",
	"NL80211_ATTR_MAX_SCHED_SCAN_IE_LEN",
	"NL80211_ATTR_SCAN_SUPP_RATES",
	"NL80211_ATTR_HIDDEN_SSID",
	"NL80211_ATTR_IE_PROBE_RESP",
	"NL80211_ATTR_IE_ASSOC_RESP",
	"NL80211_ATTR_STA_WME",
	"NL80211_ATTR_SUPPORT_AP_UAPSD",
	"NL80211_ATTR_ROAM_SUPPORT",
	"NL80211_ATTR_SCHED_SCAN_MATCH",
	"NL80211_ATTR_MAX_MATCH_SETS",
	"NL80211_ATTR_PMKSA_CANDIDATE",
	"NL80211_ATTR_TX_NO_CCK_RATE",
	"NL80211_ATTR_TDLS_ACTION",
	"NL80211_ATTR_TDLS_DIALOG_TOKEN",
	"NL80211_ATTR_TDLS_OPERATION",
	"NL80211_ATTR_TDLS_SUPPORT",
	"NL80211_ATTR_TDLS_EXTERNAL_SETUP",
	"NL80211_ATTR_DEVICE_AP_SME",
	"NL80211_ATTR_DONT_WAIT_FOR_ACK",
	"NL80211_ATTR_FEATURE_FLAGS",
	"NL80211_ATTR_PROBE_RESP_OFFLOAD",
	"NL80211_ATTR_PROBE_RESP",
	"NL80211_ATTR_DFS_REGION",
	"NL80211_ATTR_DISABLE_HT",
	"NL80211_ATTR_HT_CAPABILITY_MASK",
	"NL80211_ATTR_NOACK_MAP",
	"NL80211_ATTR_INACTIVITY_TIMEOUT",
	"NL80211_ATTR_RX_SIGNAL_DBM",
	"NL80211_ATTR_BG_SCAN_PERIOD",
	"NL80211_ATTR_WDEV",
	"NL80211_ATTR_USER_REG_HINT_TYPE",
	"NL80211_ATTR_CONN_FAILED_REASON",
	"NL80211_ATTR_SAE_DATA",
	"NL80211_ATTR_VHT_CAPABILITY",
	"NL80211_ATTR_SCAN_FLAGS",
	"NL80211_ATTR_CHANNEL_WIDTH",
	"NL80211_ATTR_CENTER_FREQ1",
	"NL80211_ATTR_CENTER_FREQ2",
	"NL80211_ATTR_P2P_CTWINDOW",
	"NL80211_ATTR_P2P_OPPPS",
	"NL80211_ATTR_LOCAL_MESH_POWER_MODE",
	"NL80211_ATTR_ACL_POLICY",
	"NL80211_ATTR_MAC_ADDRS",
	"NL80211_ATTR_MAC_ACL_MAX",
	"NL80211_ATTR_RADAR_EVENT",
	"NL80211_ATTR_EXT_CAPA",
	"NL80211_ATTR_EXT_CAPA_MASK",
	"NL80211_ATTR_STA_CAPABILITY",
	"NL80211_ATTR_STA_EXT_CAPABILITY",
	"NL80211_ATTR_PROTOCOL_FEATURES",
	"NL80211_ATTR_SPLIT_WIPHY_DUMP",
	"NL80211_ATTR_DISABLE_VHT",
	"NL80211_ATTR_VHT_CAPABILITY_MASK",
	"NL80211_ATTR_MDID",
	"NL80211_ATTR_IE_RIC",
	"NL80211_ATTR_CRIT_PROT_ID",
	"NL80211_ATTR_MAX_CRIT_PROT_DURATION",
	"NL80211_ATTR_PEER_AID",
	"NL80211_ATTR_COALESCE_RULE",
	"NL80211_ATTR_CH_SWITCH_COUNT",
	"NL80211_ATTR_CH_SWITCH_BLOCK_TX",
	"NL80211_ATTR_CSA_IES",
	"NL80211_ATTR_CSA_C_OFF_BEACON",
	"NL80211_ATTR_CSA_C_OFF_PRESP",
	"NL80211_ATTR_RXMGMT_FLAGS",
	"NL80211_ATTR_STA_SUPPORTED_CHANNELS",
	"NL80211_ATTR_STA_SUPPORTED_OPER_CLASSES",
	"NL80211_ATTR_HANDLE_DFS",
	"NL80211_ATTR_SUPPORT_5_MHZ",
	"NL80211_ATTR_SUPPORT_10_MHZ",
	"NL80211_ATTR_OPMODE_NOTIF",
	"NL80211_ATTR_VENDOR_ID",
	"NL80211_ATTR_VENDOR_SUBCMD",
	"NL80211_ATTR_VENDOR_DATA",
	"NL80211_ATTR_VENDOR_EVENTS",
	"NL80211_ATTR_QOS_MAP",
	"NL80211_ATTR_MAC_HINT",
	"NL80211_ATTR_WIPHY_FREQ_HINT",
	"NL80211_ATTR_MAX_AP_ASSOC_STA",
	"NL80211_ATTR_TDLS_PEER_CAPABILITY",
	"NL80211_ATTR_SOCKET_OWNER",
	"NL80211_ATTR_CSA_C_OFFSETS_TX",
	"NL80211_ATTR_MAX_CSA_COUNTERS",
	"NL80211_ATTR_TDLS_INITIATOR",
	"NL80211_ATTR_USE_RRM",
	"NL80211_ATTR_WIPHY_DYN_ACK",
	"NL80211_ATTR_TSID",
	"NL80211_ATTR_USER_PRIO",
	"NL80211_ATTR_ADMITTED_TIME",
	"NL80211_ATTR_SMPS_MODE",
	"NL80211_ATTR_OPER_CLASS",
	"NL80211_ATTR_MAC_MASK",
	"NL80211_ATTR_WIPHY_SELF_MANAGED_REG",
	"NL80211_ATTR_EXT_FEATURES",
	"NL80211_ATTR_SURVEY_RADIO_STATS",
	"NL80211_ATTR_NETNS_FD",
	"NL80211_ATTR_SCHED_SCAN_DELAY",
	"NL80211_ATTR_REG_INDOOR",
	"NL80211_ATTR_MAX_NUM_SCHED_SCAN_PLANS",
	"NL80211_ATTR_MAX_SCAN_PLAN_INTERVAL",
	"NL80211_ATTR_MAX_SCAN_PLAN_ITERATIONS",
	"NL80211_ATTR_SCHED_SCAN_PLANS",
	"__NL80211_ATTR_AFTER_LAST",
};


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
		printf( "IfName: %s\n",  nla_get_string(tb[NL80211_ATTR_IFNAME]));
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

