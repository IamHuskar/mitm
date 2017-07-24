#include "cross_platform_reserved.h"


#if (defined _WIN32)||(defined _WIN64)

#error 'fix me!!! get_remote_server_ep'

#else
#include <linux/netfilter_ipv4.h>
#include <arpa/inet.h>

bool get_remote_server_ep(wrap_socket* client_socket,boost::asio::ip::tcp::endpoint& ed)
{
	bool bret=false;
	boost::asio::ip::address addr;
	unsigned short port=0;
	struct sockaddr_in original_addr;

	socklen_t len=sizeof(original_addr);
	memset(&original_addr,0,len);

	do
	{
		/*
		 * test purpose
		 */
#if 0
		addr=boost::asio::ip::address::from_string("127.0.0.1");
		port=10010;


#else
		//boost::asio::ip::
		boost::asio::ip::tcp::socket::native_handle_type native_socket;
		native_socket=client_socket->native_handle();
		if(getsockopt(native_socket,SOL_IP,SO_ORIGINAL_DST,&original_addr,&len))
		{
			break;
		}

		unsigned long ipv4addr=ntohl(original_addr.sin_addr.s_addr);
		addr=boost::asio::ip::address(boost::asio::ip::address_v4(ipv4addr));
		port=ntohs(original_addr.sin_port);
#endif
		ed.address(addr);
		ed.port(port);

		bret=true;
	}while(false);
	return bret;
}
#endif
