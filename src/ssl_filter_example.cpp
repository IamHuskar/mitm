#include "ssl_filter_example.h"
#include "connection.h"

https_mitm_filter::https_mitm_filter()
{
	;
}

https_mitm_filter::~https_mitm_filter()
{
	;
}


FILTER_OPTION https_mitm_filter::on_new_conn_coming(connection* current_conn,unsigned int guid,boost::asio::ip::tcp::endpoint ed_client,\
		boost::asio::ip::tcp::endpoint ed_server)
{

	if(ed_server.port()!=443)
	{
		return FILTER_OUT;
	}

	if(current_conn->set_do_ssl_mitm(boost::asio::ssl::context::tlsv1))
	{
		/*
		 * set ca and cert
		 */
		SSL_SHARED_CONTEXT_PTR server_ctx=current_conn->s2client().ssl_context();
		SSL_SHARED_CONTEXT_PTR client_ctx=current_conn->s2server().ssl_context();

		server_ctx->use_certificate_chain_file("server-cert.pem");
		server_ctx->use_private_key_file("server-private-key.pem", boost::asio::ssl::context::pem);
		server_ctx->load_verify_file("root-cert.pem");
		server_ctx->set_verify_mode(boost::asio::ssl::context_base::verify_none);
		server_ctx->set_options(boost::asio::ssl::context::default_workarounds);


		client_ctx->use_certificate_file("client-cert.pem",boost::asio::ssl::context::pem);
		client_ctx->use_private_key_file("client-private-key.pem", boost::asio::ssl::context::pem);
		client_ctx->load_verify_file("root-cert.pem");
		client_ctx->set_verify_mode(boost::asio::ssl::context_base::verify_none);
		client_ctx->set_options(boost::asio::ssl::context::default_workarounds);


		printf("load cert file ok\n");
	}


	return FILTER_IN;
}

FILTER_OPTION https_mitm_filter::on_new_data_coming(connection* current_conn,unsigned int guid,wrap_socket* current_socket,\
		wrap_socket* the_other_socket,char* buffer,int buflen)
{
	if(current_socket->direction()==D_CLIENT)
	{
	//	printf("443 client %s %p \n",buffer,current_conn);
	}
	else
	{
	//	printf("443 server %s %p\n",buffer,current_conn);
	}
	//current_conn->post_write(the_other_socket,buffer,buflen);
	//return FILTER_HANDLED_BY_CALLBACK;
	return FILTER_UNDETERMINED;
}

FILTER_OPTION https_mitm_filter::on_conn_close(connection* current_conn,unsigned int guid,boost::asio::ip::tcp::endpoint ed_client,\
		boost::asio::ip::tcp::endpoint ed_server)
{
	return FILTER_UNDETERMINED;
}
