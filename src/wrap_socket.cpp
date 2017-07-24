#include "wrap_socket.h"
boost::atomic<unsigned int> wrap_socket::g_malloc_num(0);

wrap_socket::wrap_socket(boost::asio::io_service& m_io,DIRECTION direction):\
boost::asio::ip::tcp::socket(m_io),\
m_deadline_timer(m_io)
{
	m_direction=direction;
	m_send_on_progress=false;
	m_connect_event_handled=false;
	m_is_doing_ssl_hs=false;
	m_using_ssl=false;
	printf("wrap_socket %p init,direction=%s\n",this,m_direction==D_SERVER?"server":"client");
}

bool wrap_socket::init_ssl_ctx(boost::asio::ssl::context::method method)
{
	m_using_ssl=true;
	SSL_SHARED_CONTEXT_PTR tmpctx(new boost::asio::ssl::context(method));
	this->m_ssl_context_ptr=tmpctx;
	return true;
}

bool wrap_socket::init_ssl_stream()
{
	SSL_SHARED_STREAM_PTR tmpstream(new boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>(*this,*m_ssl_context_ptr));
	this->m_ssl_stream_ptr=tmpstream;
	return true;
}

DIRECTION wrap_socket::direction()
{
	return m_direction;
}


void wrap_socket::set_send_on_progress(bool bset)
{
	m_send_on_progress=bset;
}

bool wrap_socket::get_send_on_progress()
{
	return m_send_on_progress;
}

void wrap_socket::shutdown_both()
{
	boost::system::error_code ignored_ec;
	shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);

}

void wrap_socket::cleanup_io()
{
	free_packets_in_list();
	try
	{
		shutdown_both();
		/*
		 * call close may coz crash
		 * if a operation was called in thread a,and close operation was called in thread b
		 * in /usr/include/boost/asio/detail/impl/epoll_reactor.ipp
		 * close() call epoll_reactor::deregister_descriptor and get the descriptor_data->mutex_ in thread b. thread a ,send op
		 * eration is waiting for the lock
		 * mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);
		 * acquire lock
		 * if (!descriptor_data->shutdown_)
		 * {
		 * 		descriptor_lock.unlock();

    		free_descriptor_state(descriptor_data);
    		descriptor_data = 0;
		 * }
		 * descriptor_data was freed by close()
		 *
		 * then in thread a send operation get the lock .but descriptor_data has already been freed.coz crash.
		 * maybe there is two close() operation
		 *
		 */
		//bug bug fix me???????
		//close();
	}catch(...)
	{

	}

}



PSEND_PACKET wrap_socket::malloc_send_packet(int total_bytes)
{
	PSEND_PACKET ppacket=NULL;
	do
	{
		if(total_bytes<=0||total_bytes>=SIZE_1MB)
		{
			break;
		}
		ppacket=(PSEND_PACKET)malloc(sizeof(SEND_PACKET)-1+total_bytes);
		if(!ppacket)
		{
			printf("malloc error %d\n",total_bytes);
			break;
		}
		g_malloc_num++;
		ppacket->bytes_transferred=0;
		ppacket->total_bytes=total_bytes;
		memset(ppacket->p_buffer,0,total_bytes);
	}while(false);
	return ppacket;
}
void   wrap_socket::free_send_packet(PSEND_PACKET packet)
{
	if(packet)
	{
		g_malloc_num--;
		free(packet);
	}
}






PSEND_PACKET wrap_socket::pop_s_packet()
{
	PSEND_PACKET packet=NULL;
	if(!m_s_packet_list.empty())
	{
		packet=m_s_packet_list.front();
		m_s_packet_list.pop_front();
	}
	return packet;
}

PSEND_PACKET wrap_socket::create_packet(char*buf,int len)
{
	PSEND_PACKET ppacket=NULL;
	do
	{
		ppacket=malloc_send_packet(len);
		if(!ppacket)
		{
			break;
		}
		memcpy(ppacket->p_buffer,buf,len);
	}while(false);
	return ppacket;
}

void wrap_socket::free_packets_in_list()
{
	lock_res();
	while(!m_s_packet_list.empty())
	{
		PSEND_PACKET p=m_s_packet_list.front();
		m_s_packet_list.pop_front();
		free_send_packet(p);
	}
	unlock_res();
}

bool wrap_socket::push_s_packet(char* buf,int len)
{

	PSEND_PACKET ppacket=create_packet(buf,len);
	if(ppacket)
	{
		m_s_packet_list.push_back(ppacket);
		return true;
	}
	return false;
}

wrap_socket::~wrap_socket()
{
	unsigned int v=g_malloc_num;

	printf("wrap_socket %p uninit,direction=%s packet_num=%d listnum=%d\n",this,m_direction==D_SERVER?"server":"client",v,(int)m_s_packet_list.size());
	cleanup_io();
}
