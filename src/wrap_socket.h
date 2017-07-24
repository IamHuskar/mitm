#ifndef _WRAP_SOCKET_H_
#define _WRAP_SOCKET_H_

#include "common.h"
#include "ring_buffer.h"

enum DIRECTION{D_CLIENT,D_SERVER};
#define TX_RX_BLOCK_LEN (1024*2)

typedef struct _SEND_PACKET
{
	int total_bytes;
	int bytes_transferred;
	char p_buffer[1];
}SEND_PACKET,*PSEND_PACKET;

class connection;
typedef boost::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&> > SSL_SHARED_STREAM_PTR;
typedef boost::shared_ptr<boost::asio::ssl::context> SSL_SHARED_CONTEXT_PTR;

class wrap_socket:public boost::asio::ip::tcp::socket
{
public:
	static boost::atomic<unsigned int> g_malloc_num;
	static PSEND_PACKET malloc_send_packet(int total_bytes);
	static void         free_send_packet(PSEND_PACKET packet);


	wrap_socket(boost::asio::io_service& m_io,DIRECTION direction);
	~wrap_socket();

	DIRECTION direction();
	void set_direction(DIRECTION direction);
	void cleanup_io();
	void shutdown_both();


	bool push_s_packet(char* buf,int len);
	PSEND_PACKET pop_s_packet();


	PSEND_PACKET create_packet(char*buf,int len);

	void set_send_on_progress(bool bset);
	bool get_send_on_progress();

	void lock_res(){m_res_mutex.lock();};
	void unlock_res(){m_res_mutex.unlock();};


	boost::asio::deadline_timer& timer(){return m_deadline_timer;};
	friend connection;
	bool init_ssl_ctx(boost::asio::ssl::context::method);
	bool init_ssl_stream();
	bool is_ssl_socket(){return m_using_ssl;};

	SSL_SHARED_CONTEXT_PTR ssl_context(){return m_ssl_context_ptr;};
	SSL_SHARED_STREAM_PTR  ssl_stream(){return m_ssl_stream_ptr;};
private:
	void free_packets_in_list();
	bool m_using_ssl;
	boost::thread::id dbg_thread_id;
	char m_recv_block[TX_RX_BLOCK_LEN];
	bool                    m_connect_event_handled;
	bool                    m_is_doing_ssl_hs;
	DIRECTION m_direction;
	std::list<PSEND_PACKET> m_s_packet_list;
	boost::mutex            m_res_mutex;
	bool                    m_send_on_progress;
	boost::asio::deadline_timer m_deadline_timer;

	SSL_SHARED_STREAM_PTR m_ssl_stream_ptr;
	SSL_SHARED_CONTEXT_PTR m_ssl_context_ptr;
};
#endif
