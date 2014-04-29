#ifdef __GNUC__
#include <signal.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#else

#endif

#include <cassert>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "event_buffer.hh"
#include "inspect_exception.hh"
#include "scheduler_setting.hh"
#include "inspect_pthread.hh"
#include "yices_path_computer_singleton.hh"

using namespace std;
using namespace __gnu_cxx;

extern int verboseLevel;
extern bool trace_complete;
#ifdef RSS_EXTENSION
// Used by Scheduler, to decide whether debugging info is printed
static bool config_echo_approved_event = false;
// Used by Scheduler, to set the flag in EventBuffer::approve()
static bool config_log_approved_event = true;
static bool config_log_approved_color = false;
// Used by Scheduler, to set the flag in EventBuffer::approve()
static bool config_target_trace_flag = false;

bool config_lin_check_flag = false;
bool config_lin_serial_flag = false;
bool config_lin_quasi_flag = false;
//extern int max_spins;
//extern SchedulerSetting setting1;

void set_config_echo_approved_event(bool val) {
	config_echo_approved_event = val;
}
void set_config_log_approved_color(bool val) {
	config_log_approved_color = val;
}
void set_config_log_approved_event(bool val) {
	config_log_approved_event = val;
}

void set_config_target_trace_flag(bool val) {
	config_target_trace_flag = val;
}
void set_config_lin_check_flag(bool val) {
	config_lin_check_flag = val;
}
void set_config_lin_serial_flag(bool val) {
	config_lin_serial_flag = val;
}
void set_config_lin_quasi_flag(bool val) {
	config_lin_quasi_flag = val;
}

string color_of_event(InspectEvent& ev) {

	if (!config_log_approved_color) {
		//disable coloring
		return "";
	}

	string color = "\033[0;";
	switch (ev.type) {
	case OBJ_WRITE:
		color = "\033[1;";
		break;
	case OBJ_READ:
		color = "\033[1;";
		break;
	case MUTEX_LOCK:
		color = "\033[1;";
		break;
	case MUTEX_UNLOCK:
		color = "\033[1;";
		break;
	default:
		color = "\033[0;";
		break;
	}

	switch (ev.thread_id) {
	case 1:
		color += "32m";
		break;
	case 2:
		color += "33m";
		break;
	case 3:
		color += "34m";
		break;
	case 4:
		color += "35m";
		break;
	case 5:
		color += "36m";
		break;
	case 6:
		color += "37m";
		break;
	case 7:
		color += "38m";
		break;
	case 8:
		color += "39m";
		break;
	default:
		color += "37m";
		break;
	}
	return color;
}

string color_of_default_console() {
	return "\033[0m";
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  the implementation of SchedulerSocketIndex 
//
//////////////////////////////////////////////////////////////////////////////

SchedulerSocketIndex::SchedulerSocketIndex() {
	sockets.clear();
}

SchedulerSocketIndex::~SchedulerSocketIndex() {
	this->clear();
}

Socket * SchedulerSocketIndex::get_socket(int fd) {
	hash_map<int, Socket*, ::hash<int> >::iterator it;
	it = sockets.find(fd);
	assert( it != sockets.end());
	return it->second;
}

Socket * SchedulerSocketIndex::get_socket_by_thread_id(int tid) {
	hash_map<int, Socket*, ::hash<int> >::iterator it;

	it = tid2socket.find(tid);
	assert( it != tid2socket.end());
	return it->second;
}

Socket * SchedulerSocketIndex::get_socket(InspectEvent &ev) {
	hash_map<int, Socket*, ::hash<int> >::iterator it;

	it = tid2socket.find(ev.thread_id);
	assert( it != tid2socket.end());
	return it->second;
}

void SchedulerSocketIndex::add(Socket * socket) {
	int fd;
	assert( socket != NULL);
	fd = socket->get_fd();
	sockets.insert(make_pair(fd, socket));
}

void SchedulerSocketIndex::update(int thread_id, Socket * socket) {
	assert( socket != NULL);
	if (tid2socket.find(thread_id) == tid2socket.end())
		tid2socket.insert(make_pair(thread_id, socket));
}

void SchedulerSocketIndex::remove(int fd) {
	hash_map<int, Socket*, ::hash<int> >::iterator it;
	Socket * socket;

	assert( fd > 0);
	it = sockets.find(fd);
	socket = it->second;
	socket->close();
	delete socket;
	sockets.erase(it);
}

//used only by stateful model checking
void SchedulerSocketIndex::remove_sc_fd(int fd) {
	hash_map<int, Socket*, ::hash<int> >::iterator it;
	Socket * socket;

	assert( fd > 0);
	it = sockets.find(fd);
	socket = it->second;
	socket->close(); //added by chao, not sure, 4/7/2012
	delete socket; //added by chao, not sure, 4/7/2012
	sockets.erase(it);
}

/**
 * close all other sockets except the server socket  
 */
void SchedulerSocketIndex::clear() {
	hash_map<int, Socket*, ::hash<int> >::iterator it;
	Socket * socket;
	for (it = sockets.begin(); it != sockets.end(); it++) {
		socket = it->second;
		socket->close();
		delete socket;
	}

	sockets.clear();
	tid2socket.clear();
}

bool SchedulerSocketIndex::has_fd(int fd) {
	iterator it;
	it = sockets.find(fd);
	return (it != sockets.end());
}

////////////////////////////////////////////////////////////////////////////////////
//
//  the implementation of EventBuffer
//
////////////////////////////////////////////////////////////////////////////////////
void EventBuffer::update_output_folders() {
	if (config_lin_check_flag) {
		if (config_lin_serial_flag)
			output_serial();
		else
			output_concur();
	} else
		output_normal();
}

void EventBuffer::output_serial() {
	static int trace_count = 1;
	cout << "count: " << trace_count << endl;
	char tmpstr[64];
	if (trace_count == 1) {
		system("rm -rf serial_trace");
		system("mkdir serial_trace");
	}
	sprintf(tmpstr, "serial_trace/trace%d", trace_count++);

	toEventLog.close();
	toEventLogXml.close();
	toEventLogJet.close();

	logFileName = string(tmpstr) + ".log"; //"./trace.log";
	logXmlFileName = string(tmpstr) + ".log.xml"; //"./trace.log.xml";
	logJetFileName = string(tmpstr) + ".jet"; //"./trace.jet";

	toEventLog.open(logFileName.c_str(), ios::out);
	toEventLogXml.open(logXmlFileName.c_str(), ios::out);
	toEventLogJet.open(logJetFileName.c_str(), ios::out);

	if (toEventLog.fail())
		cerr << "Error: fail to open file '" << logFileName << "'\n";
	if (toEventLogXml.fail())
		cerr << "Error: fail to open file '" << logXmlFileName << "'\n";
	if (toEventLogJet.fail())
		cerr << "Error: fail to open file '" << logJetFileName << "'\n";
}

void EventBuffer::output_concur() {
	static int trace_count = 1;
	cout << "count: " << trace_count << endl;
	char tmpstr[64];
	if (trace_count == 1) {
		system("rm -rf concur_trace");
		system("mkdir concur_trace");
	}
	sprintf(tmpstr, "concur_trace/trace%d", trace_count++);

	toEventLog.close();
	toEventLogXml.close();
	toEventLogJet.close();

	logFileName = string(tmpstr) + ".log"; //"./trace.log";
	logXmlFileName = string(tmpstr) + ".log.xml"; //"./trace.log.xml";
	logJetFileName = string(tmpstr) + ".jet"; //"./trace.jet";

	toEventLog.open(logFileName.c_str(), ios::out);
	toEventLogXml.open(logXmlFileName.c_str(), ios::out);
	toEventLogJet.open(logJetFileName.c_str(), ios::out);

	if (toEventLog.fail())
		cerr << "Error: fail to open file '" << logFileName << "'\n";
	if (toEventLogXml.fail())
		cerr << "Error: fail to open file '" << logXmlFileName << "'\n";
	if (toEventLogJet.fail())
		cerr << "Error: fail to open file '" << logJetFileName << "'\n";
}

void EventBuffer::output_normal() {
	static int trace_count = 1;
	cout << "count: " << trace_count << endl;
	char tmpstr[64];
	if (trace_count == 1) {
		system("rm -rf normal_trace");
		system("mkdir normal_trace");
	}
	sprintf(tmpstr, "normal_trace/trace%d", trace_count++);

	toEventLog.close();
	toEventLogXml.close();
	toEventLogJet.close();

	logFileName = string(tmpstr) + ".log"; //"./trace.log";
	logXmlFileName = string(tmpstr) + ".log.xml"; //"./trace.log.xml";
	logJetFileName = string(tmpstr) + ".jet"; //"./trace.jet";

	toEventLog.open(logFileName.c_str(), ios::out);
	toEventLogXml.open(logXmlFileName.c_str(), ios::out);
	toEventLogJet.open(logJetFileName.c_str(), ios::out);

	if (toEventLog.fail())
		cerr << "Error: fail to open file '" << logFileName << "'\n";
	if (toEventLogXml.fail())
		cerr << "Error: fail to open file '" << logXmlFileName << "'\n";
	if (toEventLogJet.fail())
		cerr << "Error: fail to open file '" << logJetFileName << "'\n";
}

EventBuffer::EventBuffer() :
		server_socket(SOCK_UNIX), sc_socket(NULL) {

#ifdef RSS_EXTENSION
	if (config_lin_check_flag)
		linChecker = new lin_checker();
	else
		linChecker = NULL;

	update_output_folders();
	//	trace_count++;

#endif //RSS_EXTENSION
}

EventBuffer::~EventBuffer() {
	server_socket.close();
	if (sc_socket != NULL) {
		sc_socket->close();
		delete sc_socket;
	}
	if (config_lin_check_flag)
		delete linChecker;
}

bool EventBuffer::init(string socket_file, int time_out, int backlog) {
	int retval;

	server_socket.set_timeout_val(time_out);

	retval = server_socket.bind(socket_file);

	if (retval == -1) {
		cerr << "Inspect initialization error: fail to bind the socket "
				<< socket_file << endl;
		return false;
	}

	retval = server_socket.listen(backlog);
	if (retval == -1) {
		cerr << "Inspect initialization error: fail to listen to the socket "
				<< endl;
		return false;
	}

	return true;
}

void EventBuffer::reset() {
	socket_index.clear();
	buffer.clear();
	if (sc_socket != NULL) {
		sc_socket->close();
		delete sc_socket;
		sc_socket = NULL;
	}
}

/**
 *   the first event must be MAIN_START 
 */
InspectEvent EventBuffer::get_the_first_event() {
	Socket * socket;
	InspectEvent event;
	InspectEvent sc_ev1, sc_ev2;

	socket = server_socket.accept();
	assert( socket != NULL);

	(*socket) >> event;
	socket_index.add(socket);

	socket_index.update(event.thread_id, socket);

	assert( event.type == THREAD_START);
	return event;
}

bool EventBuffer::has_event(int tid) {
	EventBuffer::iterator it;
	it = buffer.find(tid);
	if (it == buffer.end())
		return false;
	return it->second.valid();
}

InspectEvent EventBuffer::get_event(int tid) {
	EventBuffer::iterator it;
	InspectEvent event;

	while (!has_event(tid))
		collect_events();

	it = buffer.find(tid);
	assert(it != buffer.end());

	event = it->second;
	it->second.clear();

	if (event.type == LOCAL_INFO) {
		int pos = event.location_id;
		bool flag = event.local_state_changed_flag;

		approve(event);

		while (!has_event(tid))
			collect_events();
		it = buffer.find(tid);
		assert(it != buffer.end());
		event = it->second;
		it->second.clear();

		event.location_id = pos;
		event.local_state_changed_flag = flag;
	}

	return event;
}

/**
 *  receive the content of a thread event 
 * 
 *  Handling the local binary datais a specical case of 
 *  receiving event
 */
InspectEvent EventBuffer::receive_event(Socket * socket) {
	InspectEvent event;

	assert(socket != NULL);
	(*socket) >> event;

	if (event.type == LOCAL_BINARY) {
		int size;
		void * local_binary;

		socket->recv_int(&size);
		local_binary = new char[size];
		event.thread_arg = local_binary;
		event.obj_id = size;
	}

#ifdef RSS_EXTENSION

//	if (config_log_approved_event) {
//		write_event_log((string) "       ", event);
//	}
	if (config_echo_approved_event) {
		//cout << "[       ]: " << event.toString() << endl;
	}

#endif//RSS_EXTENSION
	return event;
}

/**
 *  set the readfs set that is going to be monitored by the 
 *  ::select system call 
 */
pair<int, int> EventBuffer::set_read_fds() {
	SchedulerSocketIndex::iterator it;
	int max_fd, min_fd, server_fd;

	FD_ZERO( &readfds);

	max_fd = -1;
	min_fd = 9999999;

	server_fd = server_socket.get_fd();
	FD_SET( server_fd, &readfds);
	max_fd = (server_fd > max_fd) ? server_fd : max_fd;
	min_fd = (server_fd < min_fd) ? server_fd : min_fd;

	for (it = socket_index.begin(); it != socket_index.end(); it++) {
		FD_SET( it->first, &readfds);
		max_fd = (it->first > max_fd) ? it->first : max_fd;
		min_fd = (it->first < min_fd) ? it->first : min_fd;
	}

	return pair<int, int>(min_fd, max_fd);
}

/**
 *   monitor the sockets, and collect the coming events from them. 
 *   the porblem is that the events may come in different order. 
 *
 */
void EventBuffer::collect_events() {
	pair<int, int> min_max;
	int min_fd, max_fd; //, num_valid, num_readable;
	InspectEvent event;
	Socket * socket;
	hash_map<int, InspectEvent, ::hash<int> >::iterator ebit;

	min_max = this->set_read_fds();
	min_fd = min_max.first;
	max_fd = min_max.second;
	//num_valid = ::select(max_fd + 1, &readfds, NULL, NULL, NULL);
	::select(max_fd + 1, &readfds, NULL, NULL, NULL);

	for (int i = min_fd; i <= max_fd; i++) {
		if (!FD_ISSET(i, &readfds))
			continue;
		if (is_listening_socket(i)) {
			socket = server_socket.accept();
			socket_index.add(socket);
			continue;
		}

		/* Chao: removed on 4/7/2012
		 ::ioctl(i, FIONREAD, &num_readable);
		 if (num_readable == 0){
		 socket_index.remove( i );
		 continue;
		 }
		 */

		socket = socket_index.get_socket(i);
		event = this->receive_event(socket);

		//cout << event.toString () << endl;

		if (event.thread_id == SC_THREAD_ID) {
			assert(event.type == STATE_CAPTURING_THREAD_START);
			socket_index.remove_sc_fd(i);
			sc_socket = socket;
			assert(! socket_index.has_fd(i));

			(*sc_socket) << (int) STATE_CAPTURING_THREAD_START_PERM;
		} else {
			socket_index.update(event.thread_id, socket);
			// when prev event is SLEEP/YIELD/BLOCKING, assertion no longer holds
			// assert( !has_event(event.thread_id) );
			buffer[event.thread_id] = event;
		}
	}
}

vector<string> EventBuffer::split_string(string input, string split_by) {

	vector<string> str_list;
	if (input.size() < 1)
		return str_list;
	int comma_n = 0;
	do {
		std::string tmp_s = "";
		comma_n = input.find(split_by);
		if (-1 == comma_n) {
			tmp_s = input.substr(0, input.length());
			str_list.push_back(tmp_s);
			break;
		}
		tmp_s = input.substr(0, comma_n);
		input.erase(0, comma_n + split_by.size());
		str_list.push_back(tmp_s);
	} while (true);
	return str_list;
}

bool EventBuffer::filter_constructor(string event) {

	vector<string> split_event = split_string(event, "::");

	string class_name = split_event[split_event.size() - 2]; //class name

	string function_name = split_string(split_event[split_event.size() - 1],
			"(")[0]; //remove anything after()

	split_event = split_string(function_name, "~");
	function_name = split_event[split_event.size() - 1]; //remove ~ for deconstruct function

//	cout << "class: " << class_name << ", fuction: " << function_name << endl;

	if (class_name.compare(function_name) == 0) //filter the construct and deconstruct functions
			{
		return false;
	} else
		return true;
}

void EventBuffer::approve(InspectEvent & event) {
	Socket * socket = NULL;

	assert( event.valid());
	socket = socket_index.get_socket(event);
	assert( socket != NULL);

	EventPermitType permit;
	permit = permit_type(event.type);

	*socket << (int) permit;

#ifdef RSS_EXTENSION
	//----------------------
	//RSS: added on 4/7/2012
	//----------------------
	if (permit == THREAD_START_PERM) {
		int config = 0;
		config |= ((config_target_trace_flag ? 1 : 0));
		config |= ((config_lin_check_flag ? 1 : 0)) << 1;
		config |= ((config_lin_serial_flag ? 1 : 0)) << 2;
//		cout<<"config : "<<config<<endl;
		*socket << (int) config;
	}
	if (event.may_fail) {
		assert(
				permit == COND_WAIT_PERM || permit == MUTEX_LOCK_PERM || permit == MUTEX_UNLOCK_PERM);
		assert(event.wakeup_time_sec != 0 || event.wakeup_time_nsec != 0);
		*socket << (int) event.return_status;
	}
	if (config_log_approved_event) {
		if (config_lin_check_flag) {
			if (event.type == OBJ_CALL || event.type == OBJ_RESP) {

				if (filter_constructor(event.method_name)) {

					write_event_log("approve", event);
					write_event_log_xml(event);
					write_event_log_jet(event);
				}
			}
		} else {
			write_event_log("approve", event);
			write_event_log_xml(event);
			write_event_log_jet(event);
		}

	}
//	if (event.type == OBJ_CALL) {
//		event.index_of_OBJ_CALL++;
//
//	} else if (event.type == OBJ_RESP) {
////		event.index_of_OBJ_CALL++;
//
//	}
	//chao: 6/12/2012, for debugging purpose only

	if (config_lin_check_flag) {

		if (event.type == THREAD_END && event.thread_id == 0)
			trace_complete = true;

		if (verboseLevel == 0) {
			if (event.type == OBJ_CALL) {
//			event.index_of_OBJ_CALL++;

				cout << "[  l-call  ]: " << event.toString() << endl;
			} else if (event.type == OBJ_RESP) {
//			event.index_of_OBJ_CALL++;
				cout << "<  l-resp  >: " << event.toString() << endl;
			} else if ((event.type == MUTEX_LOCK) && event.mutex_id == 1) {
//			event.index_of_OBJ_CALL++;
//			cout << "[  l-Lock  ]: " << event.toString() << endl;
			} else if ((event.type == MUTEX_UNLOCK) && event.mutex_id == 1) {
//			event.index_of_OBJ_CALL++;
//			cout << "[  l-unLk  ]: " << event.toString() << endl;
			}
		} else if (verboseLevel >= 1) {
			event.index_of_OBJ_CALL++;
			if (event.type == OBJ_CALL) {

				cout << "[  l-call  ]: " << event.toString() << endl;
			} else if (event.type == OBJ_RESP) {

				cout << "<  l-resp  >: " << event.toString() << endl;
			} else if ((event.type == MUTEX_LOCK) && event.mutex_id == 1) {

				cout << "[  l-Lock  ]: " << event.toString() << endl;
			} else if ((event.type == MUTEX_UNLOCK) && event.mutex_id == 1) {

				cout << "[  l-unLk  ]: " << event.toString() << endl;
			} else {
				cout << color_of_event(event);
				cout << "[ approve  ]: " << event.toString() << endl;
				cout << color_of_default_console();
			}
		}
	} else {
		event.index_of_OBJ_CALL++;

		if (verboseLevel >= 0) {
			cout << color_of_event(event);
			cout << "[ approve  ]: " << event.toString() << endl;
			cout << color_of_default_console();
		}
	}

	if (true) {
		if (yices_path_computer_singleton::getInstance()->run_yices_replay) {
			if (event.type == 23) {
			} else if (event.type == 20 && event.thread_id != 0) {
			} else if (event.type == 21 && event.thread_id != 0) {
			} else {//after filtered the pre_create, thread start and thread end
				yices_path_computer_singleton::getInstance()->event_map[yices_path_computer_singleton::getInstance()->event_map.size()
						+ 1] = event;
//
			}

		}

//    string event_info=event.toString();
//
//
//
//    int size_map=yices_path_computer_singleton::getInstance()->result;
//    yices_path_computer_singleton::getInstance()->event_map.insert(pair<string, int>(event_info, size_map));
//    yices_path_computer_singleton::getInstance()->result++;
//
//   cout <<"_____________event info: "<< yices_path_computer_singleton::getInstance()->event_map[event_info] <<endl;

		//mapWordRecVerb.insert(map<string, int>::value_type("look", 5)); /* value_type方式 */
		// mapWordRecVerb.insert(pair<string, int>("walk", 6)); /* pair方式 */

	}
#endif //RSS_EXTENSION
//chao: added on 4/7/2012
	if (event.type == THREAD_END) {
          socket_index.remove(socket->get_fd());
	}


}

bool EventBuffer::is_listening_socket(int fd) {
	return (fd == server_socket.sock_fd);
}

string EventBuffer::toString() {
	stringstream ss;
	iterator it;

	ss << "{";
	for (it = buffer.begin(); it != buffer.end(); it++) {
		if (it->second.valid())
			ss << it->second.toString() << "  ";
	}
	ss << "}";
	return ss.str();
}

#ifdef RSS_EXTENSION

void EventBuffer::write_event_log(string typstr, InspectEvent & event) {
	if (toEventLog.good()) {
		if (event.thread_id == 0 && event.type == THREAD_START) {
			toEventLog.close();
			toEventLog.open(logFileName.c_str(), ios::out);
		}
		if (config_lin_check_flag) {

			if (linChecker->function_filter.find(event.method_name)
					!= linChecker->function_filter.end()) {

				toEventLog << event.toString_line() << endl;
			}

			if (!config_lin_serial_flag || config_lin_quasi_flag) {

				linChecker->add_to_trace(event);
			}

		} else {
			toEventLog << "[" << typstr << "]: " << event.toString() << endl;
		}
		toEventLog.flush();
	}
}

void EventBuffer::write_event_log_xml(InspectEvent & event) {
	static int count_event;
	if (toEventLogXml.good()) {
		if (event.thread_id == 0 && event.type == THREAD_START) {
			toEventLogXml.close();
			toEventLogXml.open(logXmlFileName.c_str(), ios::out);
			count_event = 0;
		}

		count_event++;
		toEventLogXml << "<ExecutionStep id = \"" << count_event << "\"> "
				<< "\n   " << event.toXML() << "\n    </ExecutionStep> "
				<< endl;
		toEventLogXml.flush();
	}
}

void EventBuffer::write_event_log_jet(InspectEvent & event) {
	static bool done = false;

	if (done)
		return;

	if (toEventLogJet.good()) {

		if (event.thread_id == 0 && event.type == THREAD_START) {
			toEventLogJet.close();
			toEventLogJet.open(logJetFileName.c_str(), ios::out);
			toEventLogJet << "graph[" << endl;
		}

		string str = event.toJet();
		if (str != "")
			toEventLogJet << str << endl;

		bool donejet = ((event.type == MISC_EXIT)
				|| (event.type == PROPERTY_ASSERT)
				|| (event.type == THREAD_END && event.thread_id == 0));
		if (donejet) {
			toEventLogJet << "]" << endl;
			done = true;
		}

		toEventLogJet.flush();
	}

}

#endif //RSS_EXTENSION
