#include "ring_buffer.h"

ring_buffer::ring_buffer(int size)
{
	if(size<=0||size>SIZE_4MB)
	{
		m_total_size=SIZE_4MB;
	}
	else
	{
		m_total_size=size;
	}

	m_buffer=new char[m_total_size];

	if(!m_buffer)
		exit(0);

	m_read_pos=m_write_pos=0;
}

ring_buffer::~ring_buffer()
{
	if(m_buffer)
		delete[] m_buffer;
}

bool ring_buffer::write_buf(char* buf,int size)
{
	bool bret=false;
	do
	{
		if(size<=0||size>m_total_size)
		{
			break;
		}
		int rest_len=m_total_size-m_write_pos;
		if(size<=rest_len)
		{

			memcpy(&m_buffer[m_write_pos],buf,size);
			m_write_pos+=size;
			bret=true;
			break;
		}
		else
		{

			if((datasize()+size)>m_total_size)
			{
				printf("buffer over flow\n");
				break;
			}
			memmove(m_buffer,&m_buffer[m_read_pos],datasize());
			m_write_pos=datasize();
			m_read_pos=0;
			memcpy(&m_buffer[m_write_pos],buf,size);
			m_write_pos+=size;
		}

	}while(false);
	return bret;
}

char* ring_buffer::read_buf(int bytes_to_read)
{
	char* result=NULL;
	if(bytes_to_read<=datasize())
	{
		result=&m_buffer[m_read_pos];
		m_read_pos+=bytes_to_read;
	}
	return result;
}

int ring_buffer::datasize()
{
	return m_write_pos-m_read_pos;
}
int ring_buffer::freespace()
{
	return m_total_size-datasize();
}
