#ifndef _CROSS_PLAT_FORM_H_
#define _CROSS_PLAT_FORM_H_
#include "common.h"
#include "wrap_socket.h"

#if (defined _WIN32)||(defined _WIN64)

/*
 * for windows
 */










#else

/*
 * for linux
 */
bool get_remote_server_ep(wrap_socket* client_socket,boost::asio::ip::tcp::endpoint&ed);















#endif




#endif
