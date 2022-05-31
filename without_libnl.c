// Public domain example of using netlink without libnl.
// Based on https://linux.die.net/man/7/netlink

#include <asm/types.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "netlink_names.h"
#include <linux/nl80211.h>	 //NL80211 definitions

int nl_seqno;
int nl_pid;
int nl_socket;
int nl_80211_type = -1;
struct sockaddr_nl nl_sanl = { 0 };

int ReadNL()
{
	int len;
	char buf[32768];
	struct nlmsghdr *nh;
	int is_first = 1;

	while( 1 )
	{
		// Make sure we clear out the buffer for subsequent calls.
		len = recv( nl_socket, buf, sizeof(buf), MSG_NOSIGNAL | (is_first?0:MSG_DONTWAIT) );
		is_first = 0;
		if( len <= 0 ) break;

		for( nh = (struct nlmsghdr *) buf; NLMSG_OK( nh, len ); nh = NLMSG_NEXT( nh, len ) )
		{
			switch( nh->nlmsg_type )
			{
			case NLMSG_DONE:  // TYPE 3
				break;
			case NLMSG_ERROR: // TYPE 2
				break;
			default:
			{
				uint8_t * nlm = (uint8_t*)(nh+1);
				uint8_t * nlmend = nlm + nh->nlmsg_len;
				nlm += 4; // Not sure what the first 4 bytes are (07 01 00 00)
				printf( "Netlink Reply: Type: %d  Length: %d\n", nh->nlmsg_type, nh->nlmsg_len );
				do
				{
					int msglen = ((uint16_t*)nlm)[0];
					if( msglen == 0 ) break;
					int msglenbuf = (msglen+3)&0xfffc; //All messages are padded.
					int type = nlm[2];
					if( nh->nlmsg_type == 0x10 )
					{
							printf( "    CONTROL: %d:%d: %02x%02x%02x%02x\n", nlm[2], msglen-4, nlm[4], nlm[5], nlm[6], nlm[7] );
							if( type == 1 )
							{
								nl_80211_type = *((uint32_t*)(nlm + 4));
								printf( "Got nl80211 type %d\n", nl_80211_type );
							}
					}
					else
					{
						switch( type )
						{
						case NL80211_ATTR_IFNAME:
							printf( "    ****IFName: %s\n", nlm + 4 );
							break;
						case NL80211_ATTR_SSID:
							printf( "    ****SSDID: %s\n", nlm + 4 );
							break;
						case NL80211_ATTR_WIPHY_FREQ:
							printf( "    ****Frequency: %d\n", *((uint32_t*)(nlm + 4)) );
							break;
						case NL80211_ATTR_CHANNEL_WIDTH:
							printf( "    ****Channel Width: %d\n", *((uint32_t*)(nlm + 4)) );
							break;
						default:
							printf( "    %s: %d: %02x%02x%02x%02x\n", attrnames[type], msglen-4, nlm[4], nlm[5], nlm[6], nlm[7] );
							break;
						}
					}
					nlm += msglenbuf;
					//printf( "%d (%p %p)\n", msglen, nlm, nlmend );
				} while( nlm < nlmend );
			}
			}
		}
	}
	return 1;
}

int SetupNL()
{
	nl_socket = socket( AF_NETLINK, SOCK_RAW|SOCK_CLOEXEC, NETLINK_GENERIC );
	nl_seqno = 0;

	if( nl_socket < 0 )
	{
		fprintf( stderr, "Error: Could not open netlink socket.\n" );
		return -4;
	}

	{
		// libnl likes to make the buffers larger (which is good)
		int buffer_size = 32768;
		setsockopt(nl_socket, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof( buffer_size ) );
		setsockopt(nl_socket, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof( buffer_size ) );
	}

	nl_pid = getpid() | 0x7C000000;

	{
		// We need to bind to the netlink socket.

		nl_sanl.nl_family = AF_NETLINK;
		nl_sanl.nl_pid = 0;
		nl_sanl.nl_groups = 0;
		if( bind( nl_socket, (struct sockaddr *)&nl_sanl, sizeof( nl_sanl ) ) < 0 )
		{
			fprintf( stderr, "Error: Could not bind netlink socket.\n" );
			return -5;
		}
	}

	struct nlmsghdr *nh = alloca(32);
	nh->nlmsg_pid = nl_pid;
	nh->nlmsg_seq = ++nl_seqno;
	nh->nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK;   	// Request an ack from kernel by setting NLM_F_ACK.
	nh->nlmsg_type = 0x10; //nlctrl
	nh->nlmsg_len = 32;
	uint8_t * nh_buffer = (uint8_t*)(nh+1); 		// Buffer is after header

	// Request an 80211 interface.
	memcpy( nh_buffer, "\x03\x01\x00\x00\x0c\x00\x02\x00nl80211\x00", 16 );

	send( nl_socket, nh, nh->nlmsg_len, MSG_NOSIGNAL );

	ReadNL();


	if( nl_80211_type < 0 )
	{
		fprintf( stderr, "Error: Could not find nl80211.\n" );
		return -7;
	}

	return 0;
}

int main()
{
	if( SetupNL() )
	{
		return -1;
	}


	struct nlmsghdr *nh = alloca(16+4);
	nh->nlmsg_pid = nl_pid;
	nh->nlmsg_seq = ++nl_seqno;
	nh->nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK|0x300;   	// Request an ack from kernel by setting NLM_F_ACK.
	nh->nlmsg_type = nl_80211_type; //nlctrl
	nh->nlmsg_len = 16+4;
	uint32_t * command = (uint32_t*)(nh+1); 		// Buffer is after header

	// Code for requesting status.
	*command = NL80211_CMD_GET_INTERFACE;

	send( nl_socket, nh, 16+4, MSG_NOSIGNAL );

	ReadNL();

	printf( "\n" );
	return 0;
}


