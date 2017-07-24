#ifndef _CONN_FILTER_H_
#define _CONN_FILTER_H_
#include "common.h"


enum FILTER_OPTION{FILTER_OUT,FILTER_IN,FILTER_UNDETERMINED,FILTER_HANDLED_BY_CALLBACK};

class wrap_socket;
class connection;
class conn_filter
{
public:
	conn_filter(){};
	virtual ~conn_filter(){};

public:
	virtual FILTER_OPTION on_new_conn_coming(connection* current_conn,unsigned int guid,boost::asio::ip::tcp::endpoint ed_client,\
			boost::asio::ip::tcp::endpoint ed_server)=0;

	virtual FILTER_OPTION on_new_data_coming(connection* current_conn,unsigned int guid,wrap_socket* current_socket,\
			wrap_socket* the_other_socket,char* buffer,int buflen)=0;

	virtual FILTER_OPTION on_conn_close(connection* current_conn,unsigned int guid,boost::asio::ip::tcp::endpoint ed_client,\
			boost::asio::ip::tcp::endpoint ed_server)=0;

};

#endif
