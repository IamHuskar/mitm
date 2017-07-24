#include "filter_mgr.h"


filter_mgr::filter_mgr()
{
	printf("filter_mgr %p init\n",this);
}


filter_mgr::~filter_mgr()
{
	printf("filter_mgr %p Uninit\n",this);
}

/*
 *
 * no lock;
 */
bool filter_mgr::add_filter(boost::shared_ptr<conn_filter> filter_ptr)
{
	m_filter_mgr_filter_list.push_back(filter_ptr);
	return true;
}
