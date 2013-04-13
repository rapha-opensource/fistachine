#ifndef _state_model_h
#define _state_model_h

#include <queue> 
#include <boost/thread.hpp>

template <class ValueT> struct condition
{
	boost::condition_variable cv;
	boost::mutex mx;
	ValueT value;
};

typedef condition<bool> bool_condition;

class event_base
{
	public:
		event_base() :id(0) {};
		virtual ~event_base() {}
	int id;
};

template <int ID, class T> struct event : public event_base
{
	event(const T& data_source):id(ID), data(std::move(data_source)) {}
	int id;
	T data;
};

static const int termination = -1;

#define SEND(name) \
void send( name##_event* event ) \
{ \
	/* printf("sending " #name "event to external q\n"); fflush(stdout); */\
	StateModel::send( (event_base*)event ); \
}

#define EVENT(name) struct name##_event : public event_base { name##_event() { id = name; } }; \
	void send_internal( name##_event* event ) { StateModel::send_internal( (event_base*)event ); } \
	SEND(name) \

class StateModel
{
public:
	int exit_loop;
private:
	typedef std::queue<event_base*> event_queue;
        event_queue m_external_events, m_internal_events;
	int m_current_state;
	bool_condition m_external_queue_is_empty;

protected:
	typedef std::vector< std::function< void (event_base*) > > state_functions_t;
	typedef std::vector<std::vector<int>> state_transitions_t;

	state_functions_t m_state_functions;
	state_transitions_t m_state_transitions;

	void send( event_base* event, event_queue& q);
	void send_internal( event_base* event);
	void send( event_base* event);

public:
	StateModel();
	virtual ~StateModel();

	void operator()();

};

#endif
