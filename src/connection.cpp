#include "connection.h"
#include "wrap_socket.h"
#include "cross_platform_reserved.h"



boost::atomic<unsigned int> connection::g_connection_guid(0);
boost::atomic<unsigned int> connection::g_write_operation(0);

unsigned int connection::generate_conn_guid()
{
	return connection::g_connection_guid++;
}

filter_mgr* connection::filter_instance()
{
	return &connection::g_filter_mgr;
}

filter_mgr connection::g_filter_mgr;


connection::connection(boost::asio::io_service& io_srv):\
m_io(io_srv),\
m_client(io_srv,D_CLIENT),\
m_server(io_srv,D_SERVER)
{
	m_guid=generate_conn_guid();
	/*
	 * copy filter
	 */
	m_do_ssl_mitm=false;
	m_undetermined_filter_list=filter_instance()->m_filter_mgr_filter_list;
	printf("connection %p init guid=%d \n",this,m_guid);
}

bool connection::set_do_ssl_mitm(boost::asio::ssl::context::method ssl_method)
{
	m_do_ssl_mitm=true;
	return m_client.init_ssl_ctx(ssl_method)&&m_server.init_ssl_ctx(ssl_method);
}

void connection::post_packet_in_list(wrap_socket* socket)
{
	PSEND_PACKET packet=NULL;
	socket->lock_res();
	packet=socket->pop_s_packet();
	if(!packet)
	{
		socket->set_send_on_progress(false);
	}
	socket->unlock_res();
	if(packet)
	{
		real_async_write(socket,packet);
	}
}

void connection::handle_write(wrap_socket* socket,\
		PSEND_PACKET packet,\
		const boost::system::error_code& ec,\
		std::size_t bytes_transferred)
{


	bool free_packet=true;
	do
	{

		if(ec)
		{
			printf("send error occcur && %s %s packet len=%d conn=%p\n",(socket->direction()==D_CLIENT)?"client":"server",ec.message().c_str(),packet->total_bytes,this);
			handle_socket_error(socket);
			break;
		}

		int bytes_send=(int)bytes_transferred;
		packet->bytes_transferred+=bytes_send;
		if(packet->bytes_transferred<packet->total_bytes)
		{
			free_packet=false;
			g_write_operation--;
			real_async_write(socket,packet);
			break;
		}

		post_packet_in_list(socket);

	}while(false);
	if(free_packet)
	{
		g_write_operation--;
		socket->free_send_packet(packet);
	}
}


bool connection::real_async_write(wrap_socket* socket,char* buf,int len)
{
	PSEND_PACKET packet=socket->create_packet(buf,len);
	if(packet)
	{
		real_async_write(socket,packet);
		return true;
	}
	return false;
}

void connection::real_async_write(wrap_socket* socket,PSEND_PACKET packet)
{
	socket->set_send_on_progress(true);
	g_write_operation++;
	if(is_ssl_mitm())
	{
		socket->m_ssl_stream_ptr->async_write_some(\
				boost::asio::buffer(\
						&(packet->p_buffer[packet->bytes_transferred]),\
						packet->total_bytes-packet->bytes_transferred),\
						boost::bind(&connection::handle_write,shared_from_this(),socket,packet,_1,_2));
	}
	else
	{
		socket->async_write_some(\
				boost::asio::buffer(\
						&(packet->p_buffer[packet->bytes_transferred]),\
						packet->total_bytes-packet->bytes_transferred),\
						boost::bind(&connection::handle_write,shared_from_this(),socket,packet,_1,_2));
	}

}


bool connection::post_write(wrap_socket* socket,char* buf,int len)
{
	bool bret=false;
	do
	{

		if(!len)
		{
			return true;
		}
		/*
		 * lock
		 * if send on progress
		 * 	push_back
		 * unlock
		 *
		 * async_send()
		 *
		 */
		bool on_progress=false;
		socket->lock_res();
		on_progress=socket->get_send_on_progress();
		if(socket->m_connect_event_handled)
		{
			if(on_progress)
			{
				bret=socket->push_s_packet(buf,len);
			}
			else
			{
				bret=real_async_write(socket,buf,len);
			}
		}
		else
		{
			socket->set_send_on_progress(true);
			bret=socket->push_s_packet(buf,len);
		}
		socket->unlock_res();
	}while(false);
	return bret;
}


void connection::handle_error_close_socket_timeout(wrap_socket* socket,const boost::system::error_code& ec)
{
	socket->cleanup_io();
}


void connection::handle_socket_error(wrap_socket* err_socket)
{
	/*
	 * if there is data in another socket.
	 * flush all data;
	 * close the socket async;
	 */



	wrap_socket* another_socket=NULL;
	if(err_socket->direction()==D_CLIENT)
	{
		another_socket=&m_server;
	}
	else
	{
		another_socket=&m_client;
	}

	printf("handle_socket_error %s conn=%p\n",(err_socket->direction()==D_CLIENT)?"client":"server",this);
	perform_filter_connection_callback(boost::bind(&conn_filter::on_conn_close,_1,this,conn_guid(),m_client_endpoint,m_server_endpoint));

	/*
	 * wait for 500 ms  to finish other operation
	 */
	err_socket->cleanup_io();

	another_socket->timer().expires_from_now(boost::posix_time::milliseconds(1500));
	another_socket->timer().async_wait(boost::bind(\
				&connection::handle_error_close_socket_timeout,shared_from_this(),another_socket,_1));

}

/*
 * key point
 */
void connection::handle_read(wrap_socket* socket,const boost::system::error_code& ec,std::size_t bytes_transferred)
{

	wrap_socket* the_other_socket=NULL;
	if(socket->direction()==D_CLIENT)
	{
		//printf("recv data from cleint %d\n",(int)bytes_transferred);
		/*
		 * read from client post it to server
		 */
		the_other_socket=&m_server;
	}
	else
	{
		//D_SERVER
		/*
		 * recv data from server,post it to client
		 *
		 */
		//printf("recv data from server %d\n",(int)bytes_transferred);
		the_other_socket=&m_client;
	}


	bool should_handle_error=false;
	do
	{
		if(ec&&!bytes_transferred)
		{
			printf("ec error bytes_transferred=0 %p\n",this);
			should_handle_error=true;
			break;
		}


		/*
		 * sometimes remote peer send all data and close the socket.
		 * error code is true,but bytes_transferred is not zero.
		 * we should transmit thoes data to the other peer;
		 */

		FILTER_OPTION option;
		option=perform_filter_connection_callback(boost::bind(&conn_filter::on_new_data_coming,_1,this,conn_guid(),socket,the_other_socket,\
					socket->m_recv_block,\
					(int)bytes_transferred));
		if(option!=FILTER_HANDLED_BY_CALLBACK)
		{
			post_write(the_other_socket,socket->m_recv_block,(int)bytes_transferred);
		}

		if(ec)
		{
			printf("ec error %p\n",this);
			should_handle_error=true;
			break;
		}
		post_read(socket);
	}while(false);


	if(should_handle_error)
	{
		handle_socket_error(socket);
		printf("recv from %s error %d %p %s\n",(socket->direction()==D_CLIENT)?"client":"server",(int)bytes_transferred,this,ec.message().c_str());
	}


}

void connection::post_read(wrap_socket* socket)
{
	socket->dbg_thread_id=boost::this_thread::get_id();

	if(this->is_ssl_mitm())
	{
		socket->m_ssl_stream_ptr->async_read_some(\
				 			boost::asio::buffer(socket->m_recv_block,TX_RX_BLOCK_LEN),\
							boost::bind(&connection::handle_read,shared_from_this(),socket,_1,_2)
				 			);
	}
	else
	{
		socket->async_read_some(\
				 			boost::asio::buffer(socket->m_recv_block,TX_RX_BLOCK_LEN),\
							boost::bind(&connection::handle_read,shared_from_this(),socket,_1,_2)
				 			);
	}

}

void connection::handle_connect_to_remote_server(const boost::system::error_code& ec)
{

	bool should_handle=false;
	m_server.lock_res();
	if(!m_server.m_connect_event_handled)
	{
		if(!m_server.is_ssl_socket())
		{
			m_server.m_connect_event_handled=true;
		}
		else
		{
			m_server.m_is_doing_ssl_hs=true;
		}
		should_handle=true;
	}
	m_server.unlock_res();
	if(!should_handle)
	{
		return;
	}

	if(ec)
	{
		printf("can't connect to remote server\n");
		handle_socket_error(&m_server);
		return;
	}

	if(this->is_ssl_mitm())
	{
		ssl_shake_hand(&m_server);
	}
	else
	{
		post_packet_in_list(&m_server);
		post_read(&m_server);
	}
}


void connection::handle_connect_to_server_timeout(const boost::system::error_code& ec)
{
	bool should_handle=false;
	m_server.lock_res();
	if(!m_server.m_connect_event_handled)
	{

		if(!m_server.is_ssl_socket())
		{
			m_server.m_connect_event_handled=true;
		}
		else
		{
			if(!m_server.m_is_doing_ssl_hs)
				should_handle=true;
		}


	}
	m_server.unlock_res();
	if(!should_handle)
	{
		return;
	}
	/*
	 * close socket and return;
	 * error handle
	 */
	handle_socket_error(&m_server);
}


#ifdef TEST_CALLBACK
#define DBG(x) printf(x)
#else
#define DBG(x)
#endif

template<typename callback>
FILTER_OPTION  connection::perform_filter_connection_callback(callback filter_callback)
{


	std::list<boost::shared_ptr<conn_filter> >::iterator it;

	FILTER_OPTION option=FILTER_UNDETERMINED;

	if(!m_undetermined_filter_list.empty())
	{

		for(it=m_undetermined_filter_list.begin();it!=m_undetermined_filter_list.end();)
		{


			DBG(("________%s:%d\n",__FILE__,__LINE__));

			FILTER_OPTION option=filter_callback(*it);
			DBG(("________%s:%d\n",__FILE__,__LINE__));
			if(option==FILTER_OUT)
			{
				DBG(("________%s:%d\n",__FILE__,__LINE__));
				it=m_undetermined_filter_list.erase(it);
				DBG(("________%s:%d\n",__FILE__,__LINE__));
			}
			else if(option==FILTER_IN)
			{
				DBG(("________%s:%d\n",__FILE__,__LINE__));
				m_inject_filter_list.push_back(*it);
				DBG(("________%s:%d\n",__FILE__,__LINE__));
				it=m_undetermined_filter_list.erase(it);
				DBG(("________%s:%d\n",__FILE__,__LINE__));
			}
			else
			{
				it++;
			}

		}
	}
	DBG(("________%s:%d\n",__FILE__,__LINE__));
	if(!this->m_inject_filter_list.empty())
	{

		for(it=m_inject_filter_list.begin();it!=m_inject_filter_list.end();)
		{
			DBG(("________%s:%d\n",__FILE__,__LINE__));
			option=filter_callback(*it);
			DBG(("________%s:%d\n",__FILE__,__LINE__));
			if(option==FILTER_OUT)
			{
				DBG(("________%s:%d\n",__FILE__,__LINE__));
				it=m_inject_filter_list.erase(it);
				DBG(("________%s:%d\n",__FILE__,__LINE__));

			}
			else
			{
				it++;
			}

		}
	}
	return option;
}


void connection::start_to_work()
{
	/*
	 * get remote server addr
	 * async connect to remote server
	 * post recv request on client
	 */
	/*
	 * fill address
	 */
	m_client_endpoint=m_client.remote_endpoint();
	m_client.m_connect_event_handled=true;
	if(!get_remote_server_ep(&m_client,m_server_endpoint))
	{
		/*
		 *
		 */
		printf("can't get remote server\n");
		return;
	}



	/*
	 * call filter
	 */
	perform_filter_connection_callback(boost::bind(&conn_filter::on_new_conn_coming,_1,this,conn_guid(),m_client_endpoint,m_server_endpoint));


	printf("try to redirect packet from %s:%d to %s:%d\n",\
			m_client_endpoint.address().to_string().c_str(),m_client_endpoint.port(),\
			m_server_endpoint.address().to_string().c_str(),m_server_endpoint.port());
	/*
	 * async wait connect for 1.5 sec;
	 */
	m_server.timer().expires_from_now(boost::posix_time::milliseconds(1500));
	m_server.timer().async_wait(boost::bind(\
			&connection::handle_connect_to_server_timeout,shared_from_this(),_1));
	m_server.async_connect(m_server_endpoint,\
			boost::bind(&connection::handle_connect_to_remote_server,shared_from_this(),_1)\
			);

	if(is_ssl_mitm())
	{
		/*
		 * ssl  hand shake
		 */
		ssl_shake_hand(&m_client);
	}
	else
	{
		post_read(&m_client);
	}

}

void connection::handle_handshake(wrap_socket* socket,const boost::system::error_code& error)
{

	if(error)
	{
		handle_socket_error(socket);
		printf("shake hand with %s error %s %p\n",(socket->direction()==D_CLIENT)?"client":"server",error.message().c_str(),this);
		return;
	}
	else
	{
		if(socket->direction()==D_CLIENT)
		{
			printf("shake hand with client ok %p\n",this);

		}
		else
		{
			printf("shake hand with server ok %p\n",this);
		}
	}
	socket->m_connect_event_handled=true;
	post_packet_in_list(socket);
	post_read(socket);
}


void connection::ssl_shake_hand(wrap_socket* socket)
{
	socket->init_ssl_stream();
	boost::asio::ssl::stream_base::handshake_type type=boost::asio::ssl::stream_base::client;
	if(socket->direction()==D_CLIENT)
	{
		printf("ssl as server\n");
		type=boost::asio::ssl::stream_base::server;
	}
	else
	{
		printf("ssl as client\n");
	}

	socket->m_ssl_stream_ptr->async_handshake(type,\
			boost::bind(&connection::handle_handshake,shared_from_this(),socket,_1));
}

connection::~connection()
{


	unsigned int v=g_write_operation;
	printf("connection %p uninit write_operation %d\n",this,v);
}

//connection to client
wrap_socket& connection::s2client()
{
	return m_client;
}
//connection to server
wrap_socket& connection::s2server()
{
	return m_server;
}
