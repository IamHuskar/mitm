#include "common.h"
//4 MB
#define SIZE_1MB (1024*1024)
#define SIZE_4MB (SIZE_1MB*4)


class ring_buffer
{
public:
	ring_buffer(int size);
	~ring_buffer();

	bool write_buf(char* buf,int size);
	char* read_buf(int bytes_to_read);
	int   datasize();
	int   freespace();
private:
	char* m_buffer;
	int m_total_size;
	int m_read_pos;
	int m_write_pos;
};
