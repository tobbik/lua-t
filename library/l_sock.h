/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * l_socket.h
 * socket functions wrapped for lua
 *
 * data definitions
 */

#ifdef _WIN32
#define QTC_TARGET_WIN32  1
#define QTC_TARGET_FAMILY_WINDOWS  1
#else
#define QTC_TARGET_LINUX  1
#endif

#define CRC16             0xA001
#define MAX_PKT_BYTES     1500

enum sock_type {
	UDP,
	TCP
};

struct udp_socket {
	int                socket;    // socket
	//t_timeout tm;
};

struct tdp {
	int                socket;    // socket
	//enum sock_type     type;      // UDP, TCP ...
	//t_timeout tm;
};
