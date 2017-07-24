#include "common.h"
#include "connection.h"

class proxy_server
{
public:
	//prepare()
	proxy_server(const char* address,int thread_num,unsigned short port);
	~proxy_server();
	//run server.return immediately
	bool async_run();
	//stop server.return immediately
	void async_stop();
	//wait for all thread stop
	void block_until_stop();
	

private:
	//local addr
	boost::asio::ip::tcp::endpoint m_endpoint;
	//io_service
	boost::asio::io_service m_io;
	boost::asio::ip::tcp::acceptor m_acceptor;

	void handle_accept(boost::shared_ptr<connection> current_connection,const boost::system::error_code& error);

private:
	bool post_accept();
	int m_thread_num;
	std::list<boost::shared_ptr<boost::thread> > m_threads;
};
