#include "common.h"
#include "wrap_socket.h"
#include "filter_mgr.h"

class connection:public boost::enable_shared_from_this<connection>
{
public:
	/*
	 * static filter
	 */
	static unsigned int generate_conn_guid();
	static boost::atomic<unsigned int> g_connection_guid;
	static boost::atomic<unsigned int> g_write_operation;

	static filter_mgr* filter_instance();
	static filter_mgr g_filter_mgr;





	connection(boost::asio::io_service& io_srv);
	~connection();
	//connection to client
	wrap_socket& s2client();
	//connection to server
	wrap_socket& s2server();
	/*
	 * start to do his work
	 */
	void start_to_work();
	unsigned int conn_guid(){return m_guid;};
	bool post_write(wrap_socket* socket,char* buf,int len);

	bool set_do_ssl_mitm(boost::asio::ssl::context::method ssl_method);

	bool is_ssl_mitm(){return m_do_ssl_mitm;};
private:
	boost::asio::io_service& m_io;
	wrap_socket m_client;
	boost::asio::ip::tcp::endpoint m_client_endpoint;
	wrap_socket m_server;
	boost::asio::ip::tcp::endpoint m_server_endpoint;


	unsigned int m_guid;
	bool m_do_ssl_mitm;

	void post_read(wrap_socket* socket);
	void post_packet_in_list(wrap_socket* socket);

	bool real_async_write(wrap_socket* socket,char* buf,int len);
	void real_async_write(wrap_socket* socket,PSEND_PACKET packet);


	void handle_read(wrap_socket* socket,const boost::system::error_code& ec,std::size_t bytes_transferred);
	void handle_write(wrap_socket* socket,\
			PSEND_PACKET packet,\
			const boost::system::error_code& ec,\
			std::size_t bytes_transferred);


	void ssl_shake_hand(wrap_socket* socket);
	void handle_handshake(wrap_socket* socket,const boost::system::error_code& error);

	void handle_connect_to_remote_server(const boost::system::error_code& ec);
	void handle_connect_to_server_timeout(const boost::system::error_code& ec);
	void handle_error_close_socket_timeout(wrap_socket* socket,const boost::system::error_code& ec);
	void connect_to_remote_server(boost::asio::ip::tcp::endpoint& ed);


	void handle_socket_error(wrap_socket* err_socket);

	std::list<boost::shared_ptr<conn_filter> > m_inject_filter_list;
	std::list<boost::shared_ptr<conn_filter> > m_undetermined_filter_list;


	template<typename callback>
	FILTER_OPTION perform_filter_connection_callback(callback filter_callback);



};
