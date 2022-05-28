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

int nl_seqno;
int nl_pid;
int nl_socket;
int nl_80211_type = 0x1f; // Normally 0x1f
struct sockaddr_nl nl_sanl = { 0 };
#define NL80211_MESSAGE_TYPE 0x1f

int ReadNL()
{
	int len;
	char buf[32768];
	struct iovec iov = { buf, sizeof(buf) };
	struct nlmsghdr *nh = alloca(32768);
	struct msghdr msg = { &nl_sanl, sizeof(nl_sanl), &iov, 1, NULL, 0, 0 };
	len = recvmsg( nl_socket, &msg, 0 );

	for( nh = (struct nlmsghdr *) buf; NLMSG_OK( nh, len ); nh = NLMSG_NEXT( nh, len ) )
	{
		/* The end of multipart message. */
		switch( nh->nlmsg_type )
		{
		case NLMSG_DONE:
			return 1;
		case NLMSG_ERROR:
			return 0;
		case 0x10:
			// Control Message
			printf( "Control message\n" );
			if( 0 )
			{
				uint8_t * nlm = (uint8_t*)(nh+1);
				int i;
				for( i = 0; i < nh->nlmsg_len; i++ )
				{
					printf( "%02x%c", nlm[i], ((i&0x1f)!=0x1f)?' ':'\n' );
				}
				printf( "\n" );
			}
			break;
		case 0x1f:
			printf( "NL80211 Reply\n" );
			if( 1 )
			{
				uint8_t * nlm = (uint8_t*)(nh+1);
				int i;
				for( i = 0; i < nh->nlmsg_len; i++ )
				{
					printf( "%02x%c", nlm[i], ((i&0x1f)!=0x1f)?' ':'\n' );
				}
				printf( "\n" );
			}
			return 0;
		default:
			printf( "MSG: %d / %d\n", nh->nlmsg_type, nh->nlmsg_len );
			break;
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

		// Probably not needed - but in libnl.
		//int rlen = sizeof( nl_sanl );
		//getsockname( nl_socket, (struct sockaddr *)&nl_sanl, &rlen );
	}

    struct nlmsghdr *nh = alloca(32);
	nh->nlmsg_pid = nl_pid;
	nh->nlmsg_seq = ++nl_seqno;
	nh->nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK;   	// Request an ack from kernel by setting NLM_F_ACK.
	nh->nlmsg_type = 0x10; //nlctrl
	nh->nlmsg_len = 32;
	uint8_t * nh_buffer = (uint8_t*)(nh+1); 		// Buffer is after header

	// Gotta be honnest.  Not sure what this is.
	memcpy( nh_buffer, "\x03\x01\x00\x00\x0c\x00\x02\x00nl80211\x00", 16 );

    struct iovec iov = { nh, nh->nlmsg_len };
    struct msghdr msg = { &nl_sanl, sizeof(nl_sanl), &iov, 1, NULL, 0, 0 };
	sendmsg( nl_socket, &msg, 0 );

//	sendmsg(3, {msg_name={sa_family=AF_NETLINK, nl_pid=0, nl_groups=00000000}, msg_namelen=12, msg_iov=[{iov_base={{len=32, type=nlctrl, flags=NLM_F_REQUEST|NLM_F_ACK, seq=1653720725, pid=2080396848}, "\x03\x01\x00\x00\x0c\x00\x02\x00nl80211\x00"}, iov_len=32}], msg_iovlen=1, msg_controllen=0, msg_flags=0}, 0) = 32
//	sendmsg(3, {msg_name={sa_family=AF_NETLINK, nl_pid=0, nl_groups=00000000}, msg_namelen=12, msg_iov=[{iov_base={{len=32, type=nlctrl, flags=NLM_F_REQUEST|NLM_F_ACK, seq=1, pid=2080403945}, "\x20\x9f\x6f\xb6\xfe\x7f\x00\x00\x20\x00\x00\x00\x00\x00\x00\x00"}, iov_len=32}], msg_iovlen=1, msg_controllen=0, msg_flags=0}, 0) = 32

	while( ReadNL() );

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
	uint8_t * nh_buffer = (uint8_t*)(nh+1); 		// Buffer is after header

	// Gotta be honnest.  Not sure what this is.
	memcpy( nh_buffer, "\x05\x00\x00\x00", 4 );


    struct iovec iov = { nh, nh->nlmsg_len };
    struct msghdr msg = { &nl_sanl, sizeof(nl_sanl), &iov, 1, NULL, 0, 0 };
	sendmsg( nl_socket, &msg, 0 );

	while( ReadNL() );
//sendmsg(3, {msg_name={sa_family=AF_NETLINK, nl_pid=0, nl_groups=00000000}, msg_namelen=12, msg_iov=[{iov_base={{len=20, type=nl80211, flags=NLM_F_REQUEST|NLM_F_ACK|0x300, seq=2, pid=2080406493}, "\x05\x00\x00\x00"}, iov_len=20}], msg_iovlen=1, msg_controllen=0, msg_flags=0}, 0) = 20
//sendmsg(3, {msg_name={sa_family=AF_NETLINK, nl_pid=0, nl_groups=00000000}, msg_namelen=12, msg_iov=[{iov_base={{len=20, type=nl80211, flags=NLM_F_REQUEST|NLM_F_ACK|0x300, seq=1653728605, pid=2512419777}, "\x05\x00\x00\x00"}, iov_len=20}], msg_iovlen=1, msg_controllen=0, msg_flags=0}, 0) = 20




	printf( "\n" );
	return 0;
}


