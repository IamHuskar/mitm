#include "common.h"
#include "proxy_server.h"
#include "ssl_filter_example.h"

int main()
{




	/*
	 *
	 * add all filter
	 */
	boost::shared_ptr<conn_filter> https(new https_mitm_filter());
	connection::filter_instance()->add_filter(https);



	proxy_server server("0.0.0.0",4,20010);
	if(!server.async_run())
	{
		printf("run server error\n");
		return 0;
	};
	printf("press enter to exit\n");
	fgetc(stdin);
	printf("send stop msg to server\n");
	server.async_stop();
	printf("wait for server stop\n");
	server.block_until_stop();	
	printf("main function exit\n");
	return 0;
}
