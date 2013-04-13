#include "state_model.h"

#include <cassert> 

StateModel::StateModel():exit_loop(-1), m_current_state(0){ }
StateModel::~StateModel()
{
	event_base* evt = nullptr;
	while( !m_external_events.empty() )
	{
		evt = m_external_events.front();
		m_external_events.pop();
		delete evt;
	}
}

void StateModel::operator()()
{
	while( true )
	{
		event_base* new_event;
		if( ! m_internal_events.empty() )
		{
			new_event = m_internal_events.front();
			m_internal_events.pop();
		}
		else
		{
			boost::unique_lock<boost::mutex> lock(m_external_queue_is_empty.mx);
			if( m_external_events.empty() )
			{
				m_external_queue_is_empty.cv.wait(lock);
			}
			assert( !m_external_events.empty() );
			new_event = m_external_events.front();
			m_external_events.pop();
		}
		if( new_event->id == exit_loop )
		{
			delete new_event;
			break;
		}
		m_current_state = m_state_transitions[m_current_state][new_event->id];
		m_state_functions[m_current_state](new_event);
	}
}

void StateModel::send( event_base* event , event_queue& q)
{
	if(!event) throw std::runtime_error("invalid event");
	q.push( event );
}

void StateModel::send_internal( event_base* event)
{
	send( event, m_internal_events );
}

void StateModel::send( event_base* event)
{
	try
	{
		boost::lock_guard<boost::mutex> lock(m_external_queue_is_empty.mx);
		send( event, m_external_events );
	}
	catch( const boost::lock_error& e )
	{
		printf("caught lock_error %s\n", e.what()); fflush(stdout);
	}
	m_external_queue_is_empty.cv.notify_one();
}
