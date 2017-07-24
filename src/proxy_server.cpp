#include "proxy_server.h"
proxy_server::proxy_server(const char* ip_str,int thread_num,unsigned short port):m_acceptor(m_io)
{
	boost::system::error_code ec;
	printf("server init %p\n",this);
	if(!ip_str)
	{
		exit(0);
	}

	boost::asio::ip::address addr=boost::asio::ip::address::from_string(ip_str,ec);
	if(ec)
	{
		printf("invalid address %s: using default 0.0.0.0\n",ip_str);
		addr=boost::asio::ip::address::from_string("0.0.0.0",ec);
		if(ec)
		{
			exit(0);
		}
	}
	m_endpoint.address(addr);
	m_endpoint.port(port);

	if(thread_num>=1&&thread_num<=16)
	{
		m_thread_num=thread_num;
	}
	else
	{
		m_thread_num=4;
	}


};

proxy_server::~proxy_server()
{
	printf("server UnInit %p\n",this);
};


void proxy_server::handle_accept(boost::shared_ptr<connection> current_connection,const boost::system::error_code& error)
{
	post_accept();
	if(error)
	{
		printf("error accept %s\n",error.message().c_str());
		/*
		 * can't do anything but return
		 */
		return;
	}
	/*
	 *
	 * start recv data .connect to remote port;
	 * please remember current_connection is a shared ptr
	 */
	current_connection->start_to_work();
}




bool proxy_server::post_accept()
{
	/*
	 * post async_accept
	 * create a new shared connection ptr;
	 */

	boost::shared_ptr<connection> p_incomming_conn(new connection(m_io));

	m_acceptor.async_accept(\
			p_incomming_conn->s2client(),\
			boost::bind(\
					&proxy_server::handle_accept,this,p_incomming_conn,_1\
					)\
			);


	return true;
};


//run server.return immediately
bool proxy_server::async_run()
{
	//start listen
	//run work thread;
	//return immediately
	boost::system::error_code ec;
	bool bret=false;
	do
	{
		m_acceptor.open(m_endpoint.protocol());
		m_acceptor.bind(m_endpoint,ec);

		if(ec)
		{
			printf("can't bind to this address %s\n",ec.message().c_str());
			break;
		}
		m_acceptor.listen(boost::asio::socket_base::max_connections,ec);

		if(ec)
		{
			printf("can't listen on this address\n");
			break;
		}

		post_accept();

		for(int i=0;i<m_thread_num;i++)
		{
			boost::shared_ptr<boost::thread> thread(\
					new boost::thread(boost::bind(&boost::asio::io_service::run, &m_io))\
					);
			m_threads.push_back(thread);
		}

		bret=true;
	}while(false);
	return bret;
};

//stop server.return immediately
void proxy_server::async_stop()
{
	m_io.stop();
};


//wait for all thread stop
void proxy_server::block_until_stop()
{
	printf("block_until_stop wait for all threads exit\n");
	while(!m_threads.empty())
	{
		boost::shared_ptr<boost::thread> thread=m_threads.front();
		m_threads.pop_front();
		thread->join();
	}
	printf("block_until_stop return\n");
};
