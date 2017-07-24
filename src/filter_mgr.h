#ifndef _FILTER_MGR_H_
#define _FILTER_MGR_H_
#include "common.h"
#include "conn_filter.h"

class connection;
class filter_mgr
{
public:
	filter_mgr();
	~filter_mgr();
	bool add_filter(boost::shared_ptr<conn_filter> filter_ptr);
	friend connection;
private:
	std::list<boost::shared_ptr<conn_filter> > m_filter_mgr_filter_list;
};

#endif
