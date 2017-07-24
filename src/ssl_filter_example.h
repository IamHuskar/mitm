#ifndef _SSL_FILTER_EXAMPLE_H_
#define _SSL_FILTER_EXAMPLE_H_
#include "common.h"
#include "wrap_socket.h"
#include "conn_filter.h"


class https_mitm_filter:public conn_filter
{
public:
	https_mitm_filter();
	~https_mitm_filter();
public:
	virtual FILTER_OPTION on_new_conn_coming(connection* current_conn,unsigned int guid,boost::asio::ip::tcp::endpoint ed_client,\
			boost::asio::ip::tcp::endpoint ed_server);

	virtual FILTER_OPTION on_new_data_coming(connection* current_conn,unsigned int guid,wrap_socket* current_socket,\
			wrap_socket* the_other_socket,char* buffer,int buflen);

	virtual FILTER_OPTION on_conn_close(connection* current_conn,unsigned int guid,boost::asio::ip::tcp::endpoint ed_client,\
			boost::asio::ip::tcp::endpoint ed_server);

};

#endif
