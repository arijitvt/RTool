#include <cassert>
#include <cstring>

#include <utility>
#include <string>
#include <map>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include <signal.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "scheduler.hh"
#include "next_state.hh"

#include "yices_path_computer_singleton.hh"

#include "predecessor.hh"

#ifdef FLOW_DEP
#include "flow_dep.hpp"

// return codes from LLVM pass
#include "../../llvm-3.2.src/lib/Analysis/Dependence/ReturnCodes.h"

// Paths for OPT and the dependence library file. Modify accordingly.
static const char * const opt_path = "/home/markus/src/install-3.2/bin/opt";
static const char * const dependence_lib_path = "/home/markus/src/install-3.2/lib/Dependence.so";
// Flag to activate dependence query pass
static const char * const depquery_flag = "-depquery";
#endif

// Debug message macro
#ifdef MK_DEBUG
#define DEBUG_MSG(str)  do { cerr << str; } while (false)
#define DEBUG_PAUSE()   do { cerr << "[DEBUG] press enter...\n"; \
                              cin.ignore(); } while (false)
#else
#define DEBUG_MSG(str)  do { } while (false)
#define DEBUG_PAUSE()   do { } while (false)
#endif

using namespace std;

extern Scheduler* g_scheduler;
extern int verboseLevel;

extern bool config_lin_check_flag;
extern bool config_lin_serial_flag;
extern bool config_lin_quasi_flag;
extern bool quasi_flag;

extern int total_check;
extern int total_lin;
extern int total_not_lin;

static PredecessorInfo *g_PredInfo;

void verbose(int level, string str) {
	if (verboseLevel >= level)
		cout << str << endl;
}

using std::vector;
inline void check_target_status(int sut_pid) {
	if (sut_pid != -1) {
		int stat_loc;
		int res = waitpid(sut_pid, &stat_loc, WNOHANG);
		if (WIFSIGNALED(stat_loc)) {
			cout << "target failure (e.g., segfault)" << endl;
		}
		if (WIFEXITED(stat_loc)) {
			cout << "target exit with (" << WEXITSTATUS(stat_loc) << ")"
					<< endl;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// some signal handlers for cleaning up
// 
/////////////////////////////////////////////////////////////////////////////

void sig_alarm(int signo) {
	//g_scheduler->resetReplayStatus();
}

void sig_int(int signo) {
	cout << "===================================" << endl;
	cout << "(SIG_INT) Interrupted!  " << endl;
	cout << "sig_int is fired .... !\n";
	g_scheduler->stop();

	cout << "Total number of runs:  " << g_scheduler->run_counter << endl;
	if (setting.stateful_flag) {
		cout << "Total number of states: " << g_scheduler->num_of_states
				<< endl;
		cout << "Truncated Branches: " << g_scheduler->num_of_truncated_branches
				<< endl;
	}

	cout << "Transitions explored: " << g_scheduler->num_of_transitions << endl;

        if (setting.max_pset != -1) {
          cout << "PSet Pruned: " << g_PredInfo->pruned_ << endl;
        }

	cout << "===================================" << endl;

	//cout << g_scheduler->state_stack.toString() << endl;
	exit(-1);
}

void sig_abort(int signo) {
	cout << "===================================" << endl;
	cout << "sig_abort is fired .... !\n";
	check_target_status(g_scheduler->sut_pid);

	g_scheduler->stop();

	cout << "Total number of runs:  " << g_scheduler->run_counter << endl;
	if (setting.stateful_flag) {
		cout << "Total number of states: " << g_scheduler->num_of_states
				<< endl;
		cout << "Truncated Branches: " << g_scheduler->num_of_truncated_branches
				<< endl;
	}

	cout << "Transitions explored: " << g_scheduler->num_of_transitions << endl;

        if (setting.max_pset != -1) {
          cout << "PSet Pruned: " << g_PredInfo->pruned_ << endl;
        }

	cout << "===================================" << endl;
	//cout << g_scheduler->state_stack.toString() << endl;
	exit(-1);
}

void sig_pipe(int signo) {
	throw SocketException("unknown");
}

//////////////////////////////////////////////////////////////////////////////
//
//     The implementation of Scheduler
// 
//////////////////////////////////////////////////////////////////////////////

Scheduler::Scheduler() :
		max_errors(1000000), num_of_errors_detected(0), num_of_transitions(0), num_of_states(
				0), num_of_truncated_branches(0), num_killed(0), sut_pid(-1), run_counter(
				1) {
#ifdef FLOW_DEP
        flow_dep_readwrite_pruned = 0;
        flow_dep_writewrite_pruned = 0;
#endif

	sigset_t sst;
	int retval;

	sigemptyset(&sst);
	sigaddset(&sst, SIGPIPE);
	retval = sigprocmask(SIG_BLOCK, &sst, NULL);
	assert(retval != -1);

	retval = pthread_sigmask(SIG_BLOCK, &sst, NULL);
	assert(retval != -1);

	signal(SIGINT, sig_int);
	signal(SIGABRT, sig_abort);
}

Scheduler::~Scheduler() {
}


/**
 *  set up the server socket for listening 
 * 
 */
bool Scheduler::init() {
	int retval;
	char buf[64];

#ifdef FLOW_DEP
  DEBUG_MSG("[MK DEBUG] flow dep enabled? "
       << (setting.flow_dep ? "true" : "false") << '\n'
       << "update at end enabled?? "
       << (setting.update_at_end ? "true" : "false") << '\n'
       << "slicedb filename: "  << setting.slicedb_filename << '\n'
       << "cs peak enabled? " << (setting.cs_peak ? "true" : "false")
       << '\n');

        // ensure there is a command processor
        assert(system(NULL) != 0);

        if (setting.flow_dep) {
          // Open the assert slice database and load it into memory
          processSliceDB();
        }
#endif

        // Initialize the predecessor tracker
#ifdef MK_DEBUG
        cerr << "[MK DEBUG] initializing predecessor info\n";
#endif
        g_PredInfo = new PredecessorInfo(setting.max_pset);

	setenv("INSPECT_SOCKET_FILE", setting.socket_file.c_str(), 1);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", setting.timeout_val);
	setenv("INSPECT_TMEOUT_VAL", buf, 1);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", setting.max_threads);
	setenv("INSPECT_BACKLOG_NUM", buf, 1);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", (int) setting.stateful_flag);
	setenv("INSPECT_STATEFUL_FLAG", buf, 1);

	max_errors = setting.max_errors;

	retval = event_buffer.init(setting.socket_file, setting.timeout_val,
			setting.max_threads);

	assert(retval);

	if (setting.replay_file != "") {
		read_replay_file();
	}

	return true;
}

void Scheduler::stop() {
	event_buffer.close();
}

/** 
 *  Restart the system under test
 */
void Scheduler::exec_test_target(const char* path) {
	int retval;
	int pid = fork();
	if (pid == 0) {
		int sub_pid = fork();
		if (sub_pid == 0) {
			//retval = ::execl(path, path, NULL);
			retval = ::execvp(setting.target_argv[0], setting.target_argv);
			assert(retval != -1);
		} else {
			ofstream os(".subpid");
			os << sub_pid << endl;
			os.close();
			exit(0);
		}
	} else {
		waitpid(pid, NULL, 0);
		ifstream is(".subpid");
		int sub_pid;
		is >> sub_pid;
		//cout << " sub_pid = " << sub_pid <<endl;
		system("rm -f .subpid");
		is.close();

		sut_pid = sub_pid;
	}
}

void Scheduler::run() {
	struct timeval start_time, end_time;
	gettimeofday(&start_time, NULL);

	try {

		if (yices_path_computer_singleton::getInstance()->game_mode) {

			this->free_run();
		} else {
			//RUN # 1
			this->monitor_first_run();

			//check argument --yices
			if (yices_path_computer_singleton::getInstance()->run_yices_replay) {

				yices_path_computer_singleton::getInstance()->compute_new_trace();

				if (yices_path_computer_singleton::getInstance()->run_yices_replay) {
					this->yices_run();
				}

			}
			if (config_lin_check_flag
					&& (!config_lin_serial_flag || quasi_flag)) {
				event_buffer.linChecker->print_check_trace();
				event_buffer.linChecker->clear();
			}
			while (state_stack.has_backtrack_points()
					&& yices_path_computer_singleton::getInstance()->run_yices_replay
							== false) {
				verbose(3, "has backtrack points");
				run_counter++;

				if (setting.replay_file != "") {
					cout << "Replayed the trace '" << setting.replay_file << "'"
							<< endl;
					break;
				}
				if (setting.max_runs != -1 && run_counter >= setting.max_runs) {
					cout << "@@@CLAP: Reached MaxRuns (" << run_counter << ")" << endl;
					break;
				}
				if (setting.max_seconds != -1) {
					gettimeofday(&end_time, NULL);
					int sec;
					int usec;
					sec = end_time.tv_sec - start_time.tv_sec;
					usec = end_time.tv_usec - start_time.tv_usec;
					if (usec < 0) {
						usec += 1000000;
						sec--;
					}
					if (sec >= setting.max_seconds) {
						cout << "@@@CLAP: Reached MaxSeconds (" << sec << "." << usec
								<< ")" << endl;
						break;
					}
				}
				/*
				 if (num_of_errors_detected >= max_errors) {
				 cout << "Reached MaxError (" << max_errors << ")" << endl;
				 break;
				 }
				 */

				this->backtrack_checking(); //RUN # 2,3,4,...
				if (config_lin_check_flag
						&& (!config_lin_serial_flag || quasi_flag)) {
					event_buffer.linChecker->print_check_trace();
					event_buffer.linChecker->clear();
				}

				//cout << state_stack.toString();
                                
                                if (setting.max_pset != -1) {
                                  g_PredInfo->AfterEachRun(state_stack.prefixDepth());
                                }
			}
		}

	} catch (AssertException & e) {
		cout << endl;
		cout << "=================================" << endl;
		cout << "@@@CLAP: Caught AssertException" << endl;
		if (verboseLevel >= 1) {
			cout << state_stack.top()->toString() << endl;
		}
		kill(sut_pid, SIGTERM);
		sut_pid = -1;
		exit_status = -1;
	} catch (DeadlockException & e) {
		cout << endl;
		cout << "========== ========================" << endl;
		cout << "@@@CLAP: Caught DeadlockException" << endl;
		if (verboseLevel >= 1) {
			cout << state_stack.top()->toString() << endl;
		}
		kill(sut_pid, SIGTERM);
		sut_pid = -1;
		exit_status = -1;
	} catch (DataraceException & e) {
		cout << endl;
		cout << "===================================" << endl;
		cout << "@@@CLAP: Caught DataraceException (" << e.detail << ")" << endl;
		if (verboseLevel >= 1) {
			cout << state_stack.top()->toString() << endl;
		}
		kill(sut_pid, SIGTERM);
		sut_pid = -1;
		exit_status = -1;
	} catch (IllegalLockException & e) {
		cout << endl;
		cout << "===================================" << endl;
		cout << "@@@CLAP: Caught IllegalLockException (" << e.detail << ")" << endl;
		if (verboseLevel >= 1) {
			cout << state_stack.top()->toString() << endl;
		}
		kill(sut_pid, SIGTERM);
		sut_pid = -1;
		exit_status = -1;
	} catch (SocketException & e) {
		cout << endl;
		cout << "===================================" << endl;
		cout << "@@@CLAP: Caught SocketException (" << e.detail << ")" << endl;
		cout << "could be a target failure (e.g., segfault)" << endl;
		if (verboseLevel >= 1) {
			cout << state_stack.top()->toString() << endl;
		}
		kill(sut_pid, SIGTERM);
		sut_pid = -1;
		exit_status = -1;
	} catch (ThreadException & e) {
		cout << endl;
		cout << "===================================" << endl;
		cout << "Caught ThreadException" << endl;
		if (verboseLevel >= 1) {
			cout << state_stack.top()->toString() << endl;
		}
		kill(sut_pid, SIGTERM);
		sut_pid = -1;
		exit_status = -1;
	} catch (NullObjectException & e) {
		cout << endl;
		cout << "===================================" << endl;
		cout << "@@@CLAP: Caught NullObjectException" << endl;
		if (verboseLevel >= 1) {
			cout << state_stack.top()->toString() << endl;
		}
		kill(sut_pid, SIGTERM);
		sut_pid = -1;
		exit_status = -1;
	}

	if (verboseLevel >= -1) {
		cout << endl;
		cout << "===================================" << endl;
		cout << "Total number of runs: " << run_counter << endl
		     << "Sleepset killed runs: " << num_killed << endl;
		cout << "Transitions explored: " << num_of_transitions << endl;

		if (config_lin_check_flag) {

			cout << "total_check  : " << total_check << endl;
			cout << "total_lin    : " << total_lin << endl;
			cout << "total_not_lin: " << total_not_lin << endl;

		}
                if (setting.max_pset != -1) {
                  cout << "PSet Pruned: " << g_PredInfo->pruned_ << endl;
                }
#ifdef FLOW_DEP
                if (setting.flow_dep) {
                  cout << "Flow Dep Read-Write Pruned: " << flow_dep_readwrite_pruned << '\n';
                  cout << "Flow Dep Write-Write Pruned: " << flow_dep_writewrite_pruned << '\n';
                  cout << "Flow Dep Pruned: " << flow_dep_readwrite_pruned + flow_dep_writewrite_pruned
                       << '\n';
                }
#endif

	}

}

void Scheduler::exec_transition(InspectEvent & event) {
	event_buffer.approve(event);

	if (event.type == THREAD_END || event.type == MISC_EXIT
			|| event.type == PROPERTY_ASSERT)
		return;

#if 1
	//chao: 1/7/2013 -- this wait is necessary; othewise, when replay a
	//trace, events from different threads may race

	// wait for the event to complete before approving another thread
	while (!event_buffer.has_event(event.thread_id)) {
		event_buffer.collect_events();
	}
#endif

}

State * Scheduler::get_initial_state() {
	State * init_state;

	InspectEvent event;

	init_state = new State();
	event = event_buffer.get_the_first_event();
	assert(event.type == THREAD_START);
	assert(event.thread_id == 0);

	init_state->add_to_enabled(event);
	init_state->clock_vectors.add_the_first_thread();

	return init_state;
}

InspectEvent Scheduler::get_latest_executed_event_of_thread(State * state,
		int thread_id) {
	State * ptr;
	InspectEvent dummy;

	ptr = state;
	while (ptr != NULL) {
		if (ptr->sel_event.thread_id == thread_id)
			return ptr->sel_event;
		ptr = ptr->prev;
	}

	return dummy;
}

void Scheduler::check_property(State * state) {
	if (setting.report_race_flag && state->check_race()) {
		bool report_it = true;
		if (setting.race_only_flag) {
			num_of_errors_detected++;
			if (num_of_errors_detected <= setting.max_errors) {
				report_it = false;
			}
		}
		if (report_it == true) {
			throw DataraceException("Found a data race");
		}
	} else if (state->sel_event.type == PROPERTY_ASSERT) {
		throw AssertException();
	}

}

void Scheduler::yices_run() {

	map<int, map<int, InspectEvent> >::iterator it;
	for (it = yices_path_computer_singleton::getInstance()->result_trace.begin();
			it
					!= yices_path_computer_singleton::getInstance()->result_trace.end();
			it++) {

		cout << "replay trace: " << it->first << endl;

		InspectEvent event;
		State * init_state, *new_state, *current_state;
		TransitionSet::iterator tit;

		event_buffer.reset();
		thread_table.reset();
		state_stack.stack.clear();
		yices_path_computer_singleton::getInstance()->event_map.clear();

		run_counter++;

		cout << " === run  " << run_counter
				<< ": the new trace from SMT solver ===\n";

		this->exec_test_target(setting.target.c_str());

		init_state = this->get_initial_state();

		state_stack.push(init_state);

//		cout << "replay size:  "
//				<< yices_path_computer_singleton::getInstance()->event_map.size()
//				<< endl;
		map<int, InspectEvent> replay_trace = it->second;

		//chao: 7/12/2012, do not catch deadlock exception here, throw
		//it up to the caller

		// try {
		current_state = state_stack.top();
		for (unsigned int i = 1; i <= replay_trace.size(); i++) {
			InspectEvent& anticipated_ev = replay_trace[i];

			event = current_state->prog_state->enabled.get_transition(
					anticipated_ev.thread_id);
			if (event == InspectEvent::dummyEvent || event.type == UNKNOWN)
				event = event_buffer.get_event(anticipated_ev.thread_id);
//			else break;

			//replay failed?
			assert(event != InspectEvent::dummyEvent && event.type != UNKNOWN);
			assert(event.type == anticipated_ev.type);

			//update_backtrack_info(current_state);
			assert(current_state->clock_vectors.size() > 0);

			new_state = next_state(current_state, event, event_buffer);
                        // TODO: Update PArcSet
                        if (setting.max_pset != -1) {
                          // event trace is in "state_stack" which is a vector of states
                          // each state has a "state->sel_event"
                          // each state has a "state->prev_state"
                          // TODO:  given the current event "event", first
                          // find backwardly the "immediate predecessor
                          // event", called "pred_ev", and then add the PSET
                          // pair (prev_ev --> event)
                          
                          // current_state is the state we just transitioned
                          // from by selected event
                          g_PredInfo->PSetUpdate(current_state);
                        }
			this->check_property(current_state);
			num_of_transitions++;
			if (event.type == THREAD_PRE_CREATE)
				i += 2;

			state_stack.push(new_state);
			assert(new_state->prev == current_state);
			current_state = new_state;
			//cout <<"aa"<< current_state->toString()<< endl;
		}

		/*
		 } catch (DeadlockException & e) {
		 kill(sut_pid, SIGTERM);
		 sut_pid = -1;
		 }
		 */
                if (setting.max_pset != -1) {
                  g_PredInfo->AfterEachRun(state_stack.prefixDepth());
                }
	}

#ifdef FLOW_DEP
#ifdef MK_DEBUG
        cerr << "[MK DEBUG] Scheduler finished in yices_run()\n";
#endif
        cout << "[ERROR] Flow dependencies should be checked here\n";
        assert(0 && "see error above");
#endif

}

void Scheduler::free_run() {

	InspectEvent event;
	State * init_state, *new_state, *current_state;
	TransitionSet::iterator tit;

	this->exec_test_target(setting.target.c_str());

	init_state = this->get_initial_state();

	state_stack.push(init_state);

	current_state = state_stack.top();
	while (!current_state->is_enabled_empty()) {

		int choice = -1;
		if (current_state->prog_state->enabled.size() > 1) {
			InspectEventMapIterator it;

			map<int, int> available_events;
			for (it = current_state->prog_state->enabled.begin();
					it != current_state->prog_state->enabled.end(); it++) {

				if (it->second.type == 23) {		//thread create first
					choice = it->second.thread_id;
				} else if (it->second.type == 25) {		//thread join first
					choice = it->second.thread_id;
				}
				if (it->second.type != 21) {		//thread end last
					available_events[available_events.size()] =
							it->second.thread_id;
					//cout << "automatically ignored:  " << event.toString() << endl;

				}

				cout << it->second.thread_id << ": ----[enabled]: "
						<< it->second.toString() << endl;

			}
			if (available_events.size() == 1) {
				choice = available_events[0];
			}

			if (choice != -1) {
				event = current_state->prog_state->enabled.get_transition(
						choice);
				cout << "automatically chose:  " << event.toString() << endl;

			} else {
				while (available_events.size() > 1) {
					cout << "Please choose by thread id:   ";
					cin >> choice;

					event = current_state->prog_state->enabled.get_transition(
							choice);
					if (event.valid()) {
						cout << "              chose:  " << event.toString()
								<< endl;
						break;
					}

				}
			}

		}

		if (choice == -1)
			event = current_state->prog_state->enabled.get_transition();
		assert(current_state->clock_vectors.size() > 0);

		new_state = next_state(current_state, event, event_buffer);
                if (setting.max_pset != -1) {
                  // current_state is the state from which we just
                  // transitioned by selecting event.
                  g_PredInfo->PSetUpdate(current_state);
                }



		this->check_property(current_state);
		num_of_transitions++;

		state_stack.push(new_state);
		assert(new_state->prev == current_state);
		current_state = new_state;
		choice = -1;
	}

	verbose(2, state_stack.toString2());

	if (!current_state->prog_state->disabled.empty()
			&& current_state->sleepset.empty()) {
		//chao: 6/12/2012, do not handle exceptions here -- leave to the caller
		throw DeadlockException();
	}

}

void Scheduler::monitor_first_run() {
	int thread_counter = 0;

	int step_counter = 0;
	InspectEvent event;
	State * init_state, *new_state, *current_state;
	TransitionSet::iterator tit;
	int replay_depth = 0; // used only for replay trace

	cout << " === run " << run_counter << " ===\n";

	this->exec_test_target(setting.target.c_str());

	init_state = this->get_initial_state();

	state_stack.push(init_state);

	//chao: 6/12/2012, do not catch exceptions here -- leave them to the caller

	//-------------------------------------------------------------
	//Replay the given trace (prefix) if it is available
	//-------------------------------------------------------------
	current_state = state_stack.top();
	for (unsigned int i = 0; i < error_trace.size(); i++) {
		InspectEvent& anticipated_ev = error_trace[i];

		event = current_state->prog_state->enabled.get_transition(
				anticipated_ev.thread_id);
		if (event == InspectEvent::dummyEvent || event.type == UNKNOWN)
			event = event_buffer.get_event(anticipated_ev.thread_id);

		//replay failed?
		assert(event != InspectEvent::dummyEvent && event.type != UNKNOWN);
		assert(event.type == anticipated_ev.type);

#ifdef FLOW_DEP
                if (!setting.update_at_end) {
#endif
                  update_backtrack_info(current_state);
                  assert(current_state->clock_vectors.size() > 0);
#ifdef FLOW_DEP
                }
#endif

		new_state = next_state(current_state, event, event_buffer);
                if (setting.max_pset != -1) {
                  // current_state is the state from which we just
                  // transitioned by selecting event.
                  g_PredInfo->PSetUpdate(current_state);
                }

		this->check_property(current_state);
		num_of_transitions++;
		if (event.type == THREAD_PRE_CREATE)
			i += 2;

		state_stack.push(new_state);
		assert(new_state->prev == current_state);
		current_state = new_state;
	}

	//-------------------------------------------------------------
	//Start the free run (with arbitrary schedule)
	//-------------------------------------------------------------
	current_state = state_stack.top();
	while (!current_state->is_enabled_empty()) {
#ifdef FLOW_DEP
                if (!setting.update_at_end) {
#endif
                  update_backtrack_info(current_state);
#ifdef FLOW_DEP
                }
#endif

		if (setting.max_spins != -1)
			event = current_state->get_transition_no_spin();
		else
			event = current_state->get_transition();
		//event = current_state->prog_state->enabled.get_transition();
//		cout<<event.toString()<<endl;
		assert(current_state->clock_vectors.size() > 0);

		new_state = next_state(current_state, event, event_buffer);
                if (setting.max_pset != -1) {
                  // current_state is the state from which we just transitioned
                  // by selected event.
                  g_PredInfo->PSetUpdate(current_state);
                }
		if (0
				&& yices_path_computer_singleton::getInstance()->verbose
						>= yices_path_computer_singleton::getInstance()->v_2) {
			InspectEventMapIterator it;

			for (it = new_state->prog_state->enabled.begin();
					it != new_state->prog_state->enabled.end(); it++) {
				cout
						<< "--------------------------------------------[enabled]: "
						<< it->second.toString() << endl;

			}

		}

		this->check_property(current_state);
		num_of_transitions++;

		state_stack.push(new_state);
		assert(new_state->prev == current_state);
		current_state = new_state;

		if (setting.max_events != -1
				&& setting.max_events < state_stack.depth()) {
			// when the max number of events is reached
			return;
		}
	}


	verbose(2, state_stack.toString2());

        // Predecessor information needs to be cleaned up after each run

        if (setting.max_pset != -1) {
          g_PredInfo->AfterEachRun(state_stack.prefixDepth());
        }

	if (!current_state->prog_state->disabled.empty()
			& current_state->sleepset.empty()) {
		//chao: 6/12/2012, do not handle exceptions here -- leave to the caller
		throw DeadlockException();
	}

#ifdef FLOW_DEP
#ifdef MK_DEBUG
        cerr << "[MK DEBUG] In monitor_first_run(): scheduler finished\n";
        cerr << "[MK DEBUG] Final state:\n" << current_state->toString();
#endif

        if (setting.update_at_end) {
          update_backtrack_at_end(current_state);
        }
#endif

}

#ifdef FLOW_DEP
void Scheduler::update_backtrack_at_end(State *curr_state) {
	  verbose(2, state_stack.toString2());
          // When --flow-dep is not enabled, the outer-loop will only run once
          // (ie we will only iterate once over the execution)
          bool sliceUpdated = true;

          // Save the initial state so we can run over all the states multiple
          // times
          State *init_state = curr_state;
          while (sliceUpdated) {
            DEBUG_MSG("[MK DEBUG] Iterating over all states\n");
            sliceUpdated = false;
            while (curr_state != NULL) {
              bool ret = update_backtrack_info(curr_state);
              // ret being true implies --flow-dep is enabled
              assert(!ret || setting.flow_dep);
              if (ret) {
                // only update ret when it is being true; we need to re-run
                // over all the states even if it is true only at one state
                sliceUpdated = true;
              }
              curr_state = curr_state->prev;
            }
            // retore the initial state of curr_state
            curr_state = init_state;
          }
}
#endif

#ifdef FLOW_DEP
void Scheduler::get_flowdep_info() {
  assert(false && "get_flowdep_info unimplimented!");
}
#endif // ifdef FLOW_DEP

bool Scheduler::examine_state(State * old_state, State * new_state) {
	return false;
}

void Scheduler::backtrack_checking() {
	State * state = NULL, *current_state = NULL, *new_state = NULL;
	InspectEvent event, event2;
	int depth, i;

#ifdef FLOW_DEP
        // set to true if the program prematurly finishes. Currently, this only
        // is done when the state immediatly following the replay of the prefix
        // has no enabled transitions. This prevents the backtrack set from
        // being updated on an execution which has not completed (this leads to
        // complications when peaking into the critical section, it is assumed
        // that each lock call has an unlock).
        //
        // I feel there is still a chance a similar situation could happen
        // during the middle of a run which should be taken care of as well.
        //
        // This is sound because the prefix has already been tested (in the
        // previous run) so there is no need to re-update the backtrack set on
        // it.
        //bool prematureEnd = false;
#endif

	cout << " === run " << run_counter << " ===\n";

	event_buffer.reset();

	thread_table.reset();
#ifdef MK_DEBUG
        size_t prefixDepth;
        prefixDepth = state_stack.prefixDepth();
#endif

	state = state_stack.top();
	while (state != NULL && state->backtrack.empty()) {
		state_stack.pop();
		delete state;
		state = state_stack.top();
	}
	depth = state_stack.depth();
        
#ifdef MK_DEBUG
        cerr << "depth == " << depth << '\n';
        cerr << "prefix depth == " << prefixDepth << '\n';
        cerr << "curr_predecessors_size() == " 
             << g_PredInfo->curr_predecessors_size() << '\n';
        assert(depth == prefixDepth);
#endif
        // The last item in curr_predecessors is the actual node with a
        // backtrack point
        if (setting.max_pset != -1) {
          assert(depth == g_PredInfo->curr_predecessors_size());
        }

	this->exec_test_target(setting.target.c_str());
	event = event_buffer.get_the_first_event();
	event_buffer.update_output_folders();
	exec_transition(event);

	// cout << state_stack.toString() << endl;

	for (i = 1; i < depth - 1; i++) {
		state = state_stack[i];
		//     verbose(0, event.toString());
		if (state->sel_event.type == THREAD_CREATE) {
			event = state->sel_event;

			InspectEvent pre, post, first;

			pre = event_buffer.get_event(event.thread_id);
                        assert(pre.valid() && "invalid pre thread create event");
			exec_transition(pre);

			post = event_buffer.get_event(event.thread_id);
                        assert(post.valid() && "invalid post thread create event");
			exec_transition(post);

			first = event_buffer.get_event(event.child_id);
                        assert(first.valid() && "invalid child thread event");
			exec_transition(first);

			string thread_nm;
			thread_table.add_thread(post.child_id, thread_nm, post.thread_arg);
		} else {
			event = event_buffer.get_event(state->sel_event.thread_id);
			assert(event.valid());
			if (event != state->sel_event) {
				cout << "event:      " << event.toString() << endl;
				cout << "sel_event:  " << state->sel_event.toString() << endl;
				assert(event == state->sel_event);
			}

			exec_transition(event);
		}
	}
	assert(state_stack[depth - 1] == state_stack.top());

	state = state_stack.top();

	TransitionSet::iterator it;
	for (it = state->prog_state->enabled.begin();
			it != state->prog_state->enabled.end(); it++) {
		event = it->second;
		event2 = event_buffer.get_event(event.thread_id);
		assert(event == event2);
	}

	for (it = state->prog_state->disabled.begin();
			it != state->prog_state->disabled.end(); it++) {
		event = it->second;
		event2 = event_buffer.get_event(event.thread_id);
		assert(event == event2);
	}

	event = state->backtrack.get_transition();
	if (!event.valid()) {
		cout << event.toString() << endl;
		assert(event.valid());
	}
	state->backtrack.remove(event);

	current_state = next_state(state, event, event_buffer);
        if (setting.max_pset != -1) {
          // state is the state from which we just transitioned by selecting
          // event.
          g_PredInfo->PSetUpdate(state);
        }
	this->check_property(state);

	num_of_transitions++;
	//   important!! we need to be careful here,
	//   to put backtrack transition into
	state->sleepset.remove(event);
	state_stack.push(current_state);

#if 0
        if (!current_state->has_executable_transition()) {
          prematureEnd = true;
        }
#endif

        //assert(current_state->has_executable_transition() && "prefix is at end of program");

	//cout << "<< \n";
	while (current_state->has_executable_transition()) {
#ifdef FLOW_DEP
                if (!setting.update_at_end) {
#endif
		  update_backtrack_info(current_state);
#ifdef FLOW_DEP
                }
#endif

		if (setting.max_spins != -1)
			event = current_state->get_transition_no_spin();
		else
			event = current_state->get_transition();
		//     verbose(0, event.toString());

		new_state = next_state(current_state, event, event_buffer);
                if (setting.max_pset != -1) {
                  // current_State is the state from which we just transitioned by selecting
                  // event.
                  g_PredInfo->PSetUpdate(current_state);
                }
                if (setting.max_pset != -1) {
                  // current_state is the state from which we just transitioned
                  // by selecting event.
                  g_PredInfo->PSetUpdate(current_state);
                }

		this->check_property(current_state);

		num_of_transitions++;

		state_stack.push(new_state);
		current_state = new_state;

		if (setting.max_events != -1
				&& setting.max_events < state_stack.depth()) {
			// when the max number of events is reached
			return;
		}

	}

	//cout << state_stack.toString() << endl;

	verbose(2, state_stack.toString2());

	if (!current_state->prog_state->disabled.empty()
			& current_state->sleepset.empty()) {
		//chao: 6/12/2012, do not handle exceptions here -- leave to the caller
		throw DeadlockException();
	}

	if (!current_state->has_been_end()) {
		kill(sut_pid, SIGTERM);
		//cout << "Kill  " << sut_pid << flush << endl;
		num_killed++;
	}

#ifdef FLOW_DEP
#ifdef MK_DEBUG
        cerr << "[MK DEBUG] Scheduler finished in backtrack_checking()\n";
        cerr << "[MK DEBUG] Final state:\n" << current_state->toString();
#endif

        if (setting.update_at_end) {
          update_backtrack_at_end(current_state);
        }
#endif

}

void Scheduler::get_mutex_protected_events(State * state1, State * state2,
		int tid, vector<InspectEvent> & events) {
	State * state = NULL;

	assert(state1 != NULL);
	assert(state2 != NULL);

	events.clear();
	state = state1->next;
	while (state != state2) {
		if (state->sel_event.thread_id == tid)
			events.push_back(state->sel_event);
		state = state->next;
	}

	assert(state == state2);
}

bool Scheduler::is_mutex_exclusive_locksets(Lockset * lockset1,
		Lockset * lockset2) {
	if (lockset1 == NULL || lockset2 == NULL)
		return false;
	return lockset1->mutual_exclusive(*lockset2);
}

#ifdef FLOW_DEP
bool Scheduler::update_backtrack_info(State * state) {
#else
void Scheduler::update_backtrack_info(State * state) {
#endif
	if (setting.max_pcb != -1) {
		PCB_update_backtrack_info(state);
        }
	else {
#ifdef FLOW_DEP
                bool ret = DPOR_update_backtrack_info(state);
                //DEBUG_MSG("DPOR_update_backtrack_info(state) returned: " 
                    //<< (ret ? "true" : "false") << '\n');
                // ret being true implies that flow_dep is enabled
                assert(!ret || (setting.flow_dep));
                return ret;
#else
                DPOR_update_backtrack_info(state);
#endif
        }

#ifdef FLOW_DEP
        return false; // reached when PCB_update_backtrack_info() is called
#endif
}

void Scheduler::PCB_update_backtrack_info(State * s) {
	if (s == NULL || s->prev == NULL)
		return;
	if (setting.max_pcb != -1 && s->prev->pcb_count >= setting.max_pcb)
		return;

	State *state = s->prev;

	EventCategory ec = get_event_category(state->sel_event.type);
	if (ec == EC_THREAD || ec == EC_SYMMETRY || ec == EC_LOCAL
			|| ec == EC_PROPERTY) {
		return;
	}

	TransitionSet::iterator it;
	for (it = state->prog_state->enabled.begin();
			it != state->prog_state->enabled.end(); it++) {
		InspectEvent ev = it->second;
		if (ev.invalid())
			continue;
		if (state->done.has_member(ev))
			continue;
		if (state->sleepset.has_member(ev))
			continue;

		state->backtrack.insert(ev);
	}
}

#ifdef FLOW_DEP
bool Scheduler::DPOR_update_backtrack_info(State * state) {
#else
void Scheduler::DPOR_update_backtrack_info(State * state) {
#endif
#ifdef FLOW_DEP
        bool sliceUpdated = false;
#endif
	ProgramState *ps = state->prog_state;
	TransitionSet::iterator it;
	for (it = ps->enabled.begin(); it != ps->enabled.end(); it++) {
		InspectEvent event = it->second;
#ifdef FLOW_DEP
		if (DPOR_update_backtrack_info(state, event)) {
                  // this should only happen when flow-dep is enabled
                  assert(setting.flow_dep 
                      && "slice updated with flowdep disabled");
                  sliceUpdated = true;
                }
#else
		DPOR_update_backtrack_info(state, event);
#endif
	}

#ifdef FLOW_DEP
        return sliceUpdated;
#endif
}

// Looks backwards from the passed state (state) in the current execution to
// see if there are any selected events conflicting with the passed event (see
// last_conflicting_state())
#ifdef FLOW_DEP
bool Scheduler::DPOR_update_backtrack_info(State * state, InspectEvent &event) {
#else
void Scheduler::DPOR_update_backtrack_info(State * state, InspectEvent &event) {
#endif
	State * old_state;
        State * curr_state = state;

  
HERE: 
        old_state = last_conflicting_state(curr_state, event);
        if (old_state == NULL) {
#ifdef FLOW_DEP
          return false;
#else
          return;
#endif
        }


#ifdef FLOW_DEP
        bool sliceUpdated = false;
        DEBUG_MSG("\tConflicting Events:\n\tcurr event: " << event.toString()
            << "\n\told conflicting event: " 
            << old_state->sel_event.toString() << '\n');

        // The selected event of old_state conflicts with the passed event thus
        // we should check the transitive dependencies from the conflicting
        // event as well (ie, add whichever event is not on the slice)
        //
        // We need to include events even if they are vector clock/lockset
        // pruned since it may be that these events will influence the disabled
        // events in the future.
        //
        // TODO: Double check that we actually need to add it to the slice; is
        // it enough to just check one level of dependencies? (I think yes)
        //
        // We need to check if the event is already on the slice so we only
        // need to re-iterate over all the states if we add a _new_ item to the
        // slice set
        if (get_event_category(event.type) == EC_OBJECT) {
          // Only applies to memory accessing events
          if (setting.flow_dep) {
            if (sliceIDs.count(event.inst_id) 
                || sliceIDs.count(old_state->sel_event.inst_id)) {
#if 0
              // atleast one event is on the slice
              if (!sliceIDs.count(old_state->sel_event.inst_id)) {
                // count == 0 so unvisited
                sliceUpdated = true;
                sliceIDs.insert(old_state->sel_event.inst_id);
              }
              else if (!sliceIDs.count(event.inst_id)) {
                // count == 0 so unvisited
                sliceUpdated = true;
                sliceIDs.insert(event.inst_id);
              }
              else {
                // only other possibility is that they are both on the slice
                // already. In this case, we don't continue attempting to
                // update the backtrack set but we don't need to set
                // sliceUpdated to true since it could be that we've reached a
                // fix point
                assert(sliceIDs.count(event.inst_id) 
                    && sliceIDs.count(old_state->sel_event.inst_id));
              }
#endif
              if (slice_insert(event, old_state->sel_event)) {
                // slice_insert returns true if the slice was updated
                assert(setting.flow_dep && "slice updated w/o --flow-dep");
                sliceUpdated = true;
              }
            }
            else {
              // neither event is on the slice
              assert(sliceUpdated == false);
              DEBUG_MSG("\tNeither event on slice\n");
              return sliceUpdated;
            }
          } // if (settings.flow_dep)
        } // get_event_category(
#endif


	if (verboseLevel >= 5)
	  cout << "    --    conflicting( e" << old_state->sel_event.eid 
	       << " , e" << event.eid << " )" << endl;

	Lockset * lockset1, *lockset2;
	lockset1 = old_state->get_lockset(old_state->sel_event.thread_id);
	lockset2 = state->get_lockset(event.thread_id);
	if (is_mutex_exclusive_locksets(lockset1, lockset2)) {
          DEBUG_MSG("[MK DEBUG] Lockset Pruned\n");
          curr_state = old_state;
#ifdef FLOW_DEP
          return sliceUpdated;
#else
          return;
#endif
	}

	if (verboseLevel >= 5)
	  cout << "    --    not_mutex_exclusive( e" << old_state->sel_event.eid 
	       << " , e" << event.eid << " )" << endl;


	ClockVector * vec1, *vec2;
	vec1 = old_state->get_clock_vector(old_state->sel_event.thread_id);
	vec2 = state->get_clock_vector(event.thread_id);
	if (verboseLevel >= 5) 
	  cout << "    --    vec1: " << vec1->toString() << endl
	       << "    --    vec2: " << vec2->toString() << endl;
	if (vec1->is_concurrent_with(*vec2) == false) {
                DEBUG_MSG("[MK DEBUG] Vector Clock Pruned\n");
                curr_state = old_state;
#ifdef FLOW_DEP
                return sliceUpdated;
#else
		return;
#endif
        }

#ifdef FLOW_DEP
        if (setting.cs_peak) {
          // Check if the critical section bound by each lock contains
          // dependent events
          if (event.type == MUTEX_LOCK 
              && old_state->sel_event.type == MUTEX_LOCK) {
            if (!cs_dep(curr_state, event, old_state, old_state->sel_event, 
                  sliceUpdated)) {
              DEBUG_MSG("[MK DEBUG] CS Peak pruned:\n\t" << event.toString() 
                    << "\n\t" << old_state->sel_event.toString() << '\n');
              assert(sliceUpdated == false);
              return sliceUpdated;
            }
          }
          // TODO: This could probably be applied to RWLocks too
        }
#endif

	if (verboseLevel >= 5) 
	  cout << "    --    MHP_cloclvector( e" << old_state->sel_event.eid 
	       << " , e" << event.eid << " )" << endl;

	InspectEvent alt_event;
	alt_event = old_state->prog_state->enabled.get_transition(event.thread_id);

	if (alt_event.invalid() == true) {
		// chao: 1/7/2013 -- this is when POR fails (couldn't reduce)
		//  , and we make "backtrack == enabled"

                DEBUG_MSG("POR failed to find reduction");
		TransitionSet::iterator tit;
		for (tit = old_state->prog_state->enabled.begin();
				tit != old_state->prog_state->enabled.end(); tit++) {
			InspectEvent tmp_ev = tit->second;
			if (tmp_ev.invalid())
				continue;
			if (old_state->done.has_member(tmp_ev))
				continue;
			if (old_state->sleepset.has_member(tmp_ev))
				continue;
			old_state->backtrack.insert(tmp_ev);
			//cout << endl << "DBG chao1: adding to backtrack: " << tmp_ev.toString() << endl;
		}
		// chao: 1/7/2013  -- I think a return should be added here
#ifdef FLOW_DEP
                return sliceUpdated;
#else
                return;
#endif
	}

	//chao, for debugging purpose only
	if (verboseLevel >= 5) 
	  cout << __FUNCTION__ << "() reduction: " << "e"
	       << old_state->sel_event.eid << " -->-  e" << event.eid << endl;

	//Reduction -- adding "alt_event" to "old_state->backtrack"
	if (old_state->done.has_member(alt_event)) {
                curr_state = old_state;
#ifdef FLOW_DEP
                return sliceUpdated;
#else
                return;
#endif
        }
	if (old_state->sleepset.has_member(alt_event)) {
                curr_state = old_state;
#ifdef FLOW_DEP
                return sliceUpdated;
#else
                return;
#endif
        }
	if (old_state->backtrack.has_member(alt_event)) {
                curr_state = old_state;
#ifdef FLOW_DEP
                return sliceUpdated;
#else
                return;
#endif
        }

	if (setting.symmetry_flag) {
		if (thread_table.is_symmetric_threads(alt_event.thread_id,
				old_state->sel_event.thread_id)) {
			InspectEvent symm1, symm2;
			symm1 = old_state->symmetry.get_transition(alt_event.thread_id);
			symm2 = old_state->symmetry.get_transition(
					old_state->sel_event.thread_id);
			if (symm1.valid() && symm2.valid() && symm1.obj_id == symm2.obj_id)  {
                                curr_state = old_state;
#ifdef FLOW_DEP
                                return sliceUpdated;
#else
                                return;
#endif
                        }
		}
	}

	if (setting.max_spins != -1) {
		if (old_state->prev != NULL) {
			InspectEvent& prior_ev = old_state->prev->sel_event;
			if (prior_ev.type == OBJ_READ && alt_event.type == OBJ_READ) {
				if (prior_ev.thread_id == alt_event.thread_id) {
					if (prior_ev.inst_id == alt_event.inst_id) {
						// alt_event and prior_ev come from the same INSTRUCTION
						// (!!!busy waiting!!!)
						curr_state = old_state; 
#ifdef FLOW_DEP
                                                return sliceUpdated;
#else
                                                return;
#endif
					}
				}
			}
		}
	}

	//-----------------------------------------------------------------
	// Chao: on 10/15/2013 
	// 
	// After thinking about it for some time, I feel that this is
	//      the only place where we should use "goto HERE" -- in
	//      all the other places, we should really use "return"
	//-----------------------------------------------------------------
        if (setting.max_pset != -1) {
          // check if we will generate new predecessors by flipping the order
          // of the conflicting events
#ifdef MK_DEBUG
          //cout << "[DEBUG] [MK] State stack depth: " << state_stack.depth() << '\n';
          //cout << "[DEBUG] [MK] State stack: " << state_stack.toString() << '\n';

#endif
          if (g_PredInfo->PSetVisited(event, old_state->sel_event, old_state)) {
#ifdef MK_DEBUG
              cerr << "[DEBUG] [MK] PSet Pruned\n";
#endif
              g_PredInfo->pruned_++; // keep track of statistics
              curr_state = old_state;
              goto HERE;
          }
#ifdef MK_DEBUG
          else {
            cerr << "[DEBUG] [MK] PSet not visited\n";
          }
#endif
        }

        //assert(alt_event.type != THREAD_END && "adding thread end to backtrack");
	old_state->backtrack.insert(alt_event);
	//cout << endl << "DBG chao2: adding to backtrack: " << alt_event.toString() << endl;
	assert(alt_event.valid());

#ifdef FLOW_DEP
        return sliceUpdated;
#else
        return;
#endif

}

State* Scheduler::last_conflicting_state(State *state, InspectEvent &event) {
        // Note: the passed in state* is the current state
	State * old_state = state->prev;

	while (old_state != NULL) {
		if (dependent(old_state->sel_event, event)) {
#ifdef FLOW_DEP
                        if (!setting.flow_dep) {
                          // only perform the reduction if one of the settings
                          // are enabled
                          break;
                        }

                        // perform {,control-}flow dependence pruning here, as
                        // opposed to inside the previous call to dependent
                        // (inside DPOR update backtrack info)  because we need
                        // to know where in the current execution the two
                        // events are. We refine simple-dependence so we only
                        // check those memory accessing transitions which are
                        // already simple dependent.
                        EventCategory cat;
                        cat = get_event_category(event.type);

                        // the call to dependent above has already filtered out
                        // memory transitions that don't satisfy the following:
                        // 1. Same object
                        // 2. Differen threads
                        // 3. Atleast one is a write.
                        //
                        // It has also already filtered out events to different
                        // categories
                        //
                        // Thread join is also classifed as EC_OBJECT
                        if (cat != EC_OBJECT || event.type == THREAD_JOIN) {
                          // our reduction only applied to memory accessing
                          // transitions
                          break;
                        }
                        
                        // our reduction could be applied to these transitions
                        // (memory accesses that are already simple dependent)
                        bool dep;
                        dep = flow_dependent(old_state, event, state);
                        if (dep) {
                          DEBUG_MSG("[MK DEBUG] flow dependent events found\n");
                          break;
                        }

#ifdef MK_DEBUG
                        cerr << "[MK DEBUG] Flow Dependence Pruned!!\n";
#endif
                        
                        // We did not find a dependence, so do nothing. This
                        // continues up state->prev looking for a dependent
                        // state
#else 
			break;
#endif

		}
		old_state = old_state->prev;
	}
	return old_state;
}

bool Scheduler::dependent(InspectEvent &e1, InspectEvent &e2) {
	bool retval = false;

	retval = e1.dependent(e2);

	return retval;
}

#ifdef FLOW_DEP
bool Scheduler::flow_dependent(State *old_state, InspectEvent &event, State *curr_state) {
  // The checks in last_conflicting_state() should have ensured that the
  // conflicting events are either:
  //
  // Read-Write
  // Write-Write

  InspectEvent old_event = old_state->sel_event;

  // Should be memory accessing events
  assert(get_event_category(event.type) == EC_OBJECT);
  assert(get_event_category(old_event.type) == EC_OBJECT);

  // Atleast one should be a write
  assert(event.type == OBJ_WRITE || old_event.type == OBJ_WRITE);

  // To the same object
  assert(event.obj_id == old_event.obj_id);


  if (is_read_write_pair(event, old_event)) {
#if 0
    InspectEvent read_event;
    read_event = get_read(event, old_event);

    if (data_dep_query(read_event)) {
      // The memory read is dependent on some kind of sink we are interested in
      return true;
    }
    // read not dependent on any sink we are interested in
    flow_dep_readwrite_pruned++;
    return false;
#endif
    // This analysis has been superceded by the slice based analysis. Simply
    // always return true (assume it always fails)
    return true;
  }
  else {
    // implicitly a write-write pair
    
    // Get the state in which the passed event occurred in 
    //State *ev_state = event_to_state(old_state, event);
    //assert(ev_state != NULL && "event_to_state returned NULL");

    // check if both writes are the last write to the object by each thread
    if (is_last_write(old_state, old_state->sel_event) 
        && is_last_write(curr_state, event)) {
#ifdef MK_DEBUG
      cerr << "[MK DEBUG] write-write; threads are last to access var\n";
#endif
      // TODO: Will there ever be a case where there is a conflicting read
      // non-co-enabled with one thread but not the other? 
      if (exists_non_coenabled_conf_read(old_state, old_state->sel_event)
          || exists_non_coenabled_conf_read(curr_state, event)) {
        // last writes and a non-co-enabled read in future so definitely
        // dependent
#ifdef MK_DEBUG
    cerr << "[MK DEBUG] Found read conflicting with write-write\n";
#endif
        return true;
      }
    }

    // Not the last writes or no co-enabled read so not dependent
    flow_dep_writewrite_pruned++;
#ifdef MK_DEBUG
    cerr << "[MK DEBUG] Write-Write Pruned!!\n"
         << "\t" << event.toString() << '\n'
         << "\t" << old_event.toString() << '\n';
#endif
    return false;
  }

  assert(false && "unreachable code encountered");
  return false;

}


State *Scheduler::event_to_state(State *old_state, InspectEvent ev) const {
#ifdef MK_DEBUG
  cerr << "[MK DEBUG] event_to_state searching for event:\n\t" << ev.toString() << '\n';
  cerr << "From state:\n" << old_state->toString() << '\n';
#endif

  assert(old_state != NULL);
  while (old_state != NULL) {
    if (old_state->sel_event == ev) {
      return old_state;
    }
    old_state = old_state->next;
  }

  return NULL;
}


bool Scheduler::exists_non_coenabled_conf_read(State *s, InspectEvent &e) {
  assert(s != NULL);

  assert(s->is_enabled(e));

  int obj_id;
  int tid;

  State *old_state = s;
  obj_id = e.obj_id;
  tid = e.thread_id;


  // don't compare the passed state to itself
  s = s->next;

  // look forward from the passed state to find a read of interest
  for (; s != NULL; s = s->next) {
    InspectEvent e;
    e = s->sel_event;

    if (e.type != OBJ_READ) {
      // not a read
      continue;
    }

    if (e.thread_id == tid) {
      // same thread
      continue;
    }

    if (e.obj_id != obj_id) {
      // different objects
      continue;
    }

    // at this point, we've found an event that is a read from another thread.
    // Next, check if it may be co-enabled
    // 
    // Check locksets
    Lockset *ls1;
    Lockset *ls2;

    ls1 = old_state->get_lockset(tid);
    ls2 = s->get_lockset(e.thread_id);
    if (is_mutex_exclusive_locksets(ls1, ls2)) {
      // the locksets share a lock so they cannot be co-enabled
      return true;
    }

    // Check vector clocks
    ClockVector *vec1; 
    ClockVector *vec2;

    vec1 = old_state->get_clock_vector(tid);
    vec2 = s->get_clock_vector(e.thread_id);

    // at this point, we've found that they can be co-enabled so continue along
    // with the while loop
    if (vec1->is_concurrent_with(*vec2) == false) {
      // the threads, at this point, may not run concurrently so they cannot be
      // co-enabled
      return true;
    }
  }

  // no read found
  return false;
}

InspectEvent get_last_write(const int tid, const int oid, State *st) {
  assert(st != NULL && "null State passed");

  // state is just any state on the string of states so scroll it to the end
  for (; st->next != NULL; st = st->next) { }

#ifdef MK_DEBUG
  cerr << "[MK DEBUG] get_last_write: last state: " << st->toString() << '\n';
#endif

  // look backwards from the last state in the current execution to find a
  // write to the same object by the same thread ID
  for (; st != NULL; st = st->prev) {
    InspectEvent sel_ev;
    sel_ev = st->sel_event;

    if (sel_ev.thread_id == tid && sel_ev.obj_id == oid && sel_ev.type == OBJ_WRITE) {
#ifdef MK_DEBUG
      cerr << "[MK DEBUG] get_last_write returing: " << sel_ev.toString() << '\n';
#endif
      return sel_ev;
    }
  }

  assert(0 && "last write not found");
  return InspectEvent();
}

#if 0
Old and useless
bool Scheduler::are_last_writes(State *s1, InspectEvent &ev1) const {
  assert(s1 != NULL && "NULL state passed");

  // the event conflicting with ev1
  InspectEvent ev2 = s1->sel_event;
#ifdef MK_DEBUG
  cerr << "[MK DEBUG] are_last_writes checking:\n"
       << "ev1: " << ev1.toString() << "\n\t" << ev2.toString() << '\n';
#endif

  InspectEvent last_write_ev1 = get_last_write(ev1.thread_id, ev1.obj_id, s1);

  InspectEvent last_write_ev2 = get_last_write(ev2.thread_id, ev2.obj_id, s1);

  if (last_write_ev1 == ev1 && last_write_ev2 == ev2) {
    return true;
  }
  return false;
}

bool Scheduler::are_last_writes(const State *s1, const State *s2) const {
  // true if both are the last write
  return is_last_write(s1) && is_last_write(s2);
}
#endif

bool Scheduler::is_last_write(State *s, InspectEvent &e) const {
  assert(s != NULL);

  assert(s->is_enabled(e));

  int obj_id;
  int tid;

  obj_id = e.obj_id;
  tid = e.thread_id;

  // don't compare the passed state to itself
  s = s->next;

  // look foward from the state attempting to find another write by the same
  // thread to the same object
  for (;s != NULL; s = s->next) {
    InspectEvent e;
    e = s->sel_event;

    if (e.thread_id != tid) {
      // not the same thread
      continue;
    }

    if (e.type != OBJ_WRITE) {
      // not a write
      continue;
    }

    if (e.obj_id != obj_id) {
      // not the same object
      continue;
    }

    assert(e.thread_id == tid);
    assert(e.type == OBJ_WRITE);
    assert(e.obj_id == obj_id);

    // not the last write
    return false;
  }

  // No write found later in the current execution
  return true;
}

bool Scheduler::data_dep_query(const InspectEvent e) {
  // Control of OPT and LLVM Library path are set in preprocessor defines in
  // this file
  int ret;

  // LLVM instruction ID
  int iid;
  iid = e.inst_id;

#ifdef MK_DEBUG
  cerr << "[MK DEBUG] data_dep_query: inst id == " << iid << '\n';
#endif

  int opt_status;

  // see if we've already queried opt for this instruction
  auto hit = opt_results.find(iid);
  if (hit != opt_results.end()) {
    // found old opt result
    opt_status = hit->second;
  }
  else {
    string command;
    command = generate_opt_cmd(iid);

#ifdef MK_DEBUG
    cerr << "[MK DEBUG] OPT command: " << command << '\n';
#endif

    ret = system(command.c_str());

    if (ret == -1) {
      perror("Unable to create child opt process");
      std::exit(EXIT_FAILURE);
    }

    // Interpret exit status of opt
    if (!WIFEXITED(ret)) {
      cerr << "Error: opt did not exit normally\n";
      std::exit(EXIT_FAILURE);
    }

    opt_status = WEXITSTATUS(ret);

    // Remember the status from opt incase we need to query this instruction
    // again
    opt_results[iid] = opt_status;
  }

#ifdef MK_DEBUG
    cerr << "[MK DEBUG] opt return status: " << opt_status << '\n';
#endif

  switch (opt_status) {
    // see DepQueryReturn.hh for possible values
    case E_NO_DEP_FOUND:       // no conflicting dependence encountered
#ifdef MK_DEBUG
      cerr << "[MK DEBUG] Read-Write Dependence Pruned!!\n'";
#endif
      return false;
      break;
    case E_DEP_FOUND:           // found some kind of dependence conflict
#ifdef MK_DEBUG
      cerr << "[MK DEBUG] opt found read depedence\n'";
#endif
      return true;
      break;
    case E_COMMAND_LINE:
      cerr << "Error: LLVM command line error\n";
      std::exit(EXIT_FAILURE);
      break;
    case E_SOURCE_NOT_FOUND:    // passed source instruction ID not found in module
      cerr << "Error: Source instruction ID not found by opt\n";
      std::exit(EXIT_FAILURE);
      break;
    case E_SH_FAILURE:          // returned if shell is unable to launch process in system() call
      cerr << "Error: shell returned error from opt invocation\n";
      std::exit(EXIT_FAILURE);
      break;
    default:
      cerr << "Error: Unknown return code from opt\n";
      std::exit(EXIT_FAILURE);
      break;
  }

  assert(false && "unreachable code encountered");
  return true;
}


std::string Scheduler::generate_opt_cmd(const int inst_id) const {
  assert(0 && "unimplimented");
#if 0
  std::string command;

  // "<opt>"
  command += opt_path;

  // "<opt> -load"
  command += " -load";

  // "<opt> -load "
  command += ' ';

  // "<opt> -load <Dependence.so>
  command += dependence_lib_path;

  // "<opt> -load <Dependence.so> "
  command += ' ';

  // "<opt> -load <Dependence.so> <dep_query_flag>"
  command += depquery_flag;

  // "<opt> -load <Dependence.so> <dep_query_flag> "
  command += ' ';

  // "<opt> -load <Dependence.so> <dep_query_flag> <input_file>"
  command += setting.bc_filename;

  // "<opt> -load <Dependence.so> <dep_query_flag> <input_file> -source "
  command += " -source ";

  // "<opt> -load <Dependence.so> <dep_query_flag> <input_file> -source <inst_id>"
  command += std::to_string(inst_id);

  if (setting.use_may_escape) {
    // "<opt> -load <Dependence.so> <dep_query_flag> <input_file> -source <inst_id> -may-escape"
    command += " -may-escape";
    // "<opt> -load <Dependence.so> <dep_query_flag> <input_file> -source <inst_id> -may-escape -basicaa"
    command += " -basicaa";
  }

  // "<opt> -load <Dependence.so> <dep_query_flag> <input_file> -source <inst_id> -may-escape -basicaa >/dev/null"
  command += " >/dev/null";

  return command;
#endif
}


bool Scheduler::is_read_write_pair(const InspectEvent &e1, const InspectEvent &e2) const {
  if (e1.type == OBJ_READ && e2.type == OBJ_WRITE) {
    return true;
  }
  else if (e1.type == OBJ_WRITE && e2.type == OBJ_READ) {
    return true;
  }
  return false;
}

InspectEvent Scheduler::get_read(const InspectEvent e1, const InspectEvent e2) const {
  // atleast one should be a read
  assert(e1.type == OBJ_READ || e2.type == OBJ_READ);

  if (e1.type == OBJ_READ)
    return e1;
  else if (e2.type == OBJ_READ)
    return e2;

  assert(false && "unreachable code encountered");
  return e1;
}

void Scheduler::processSliceDB() {
  string filename = setting.slicedb_filename;

  ifstream in(filename);

  if (!in.good()) {
    cerr << "[ERROR] Error opening slice DB: " << filename << '\n';
    std::exit(EXIT_FAILURE);
  }

  for (string line; getline(in, line); ) {
    std::istringstream iss(line);
    uint64_t cur;
    if (!(iss >> cur)) {
      cerr << "[ERROR] malformed sliceDB line: " << line << '\n';
      std::exit(EXIT_FAILURE);
    }
    else {
      sliceIDs.insert(cur);
    }
  }

  DEBUG_MSG("[MK DEBUG] Slice IDs processed: " << sliceIDs.size() << '\n');

  in.close();
}

bool Scheduler::cs_dep(State *s1, InspectEvent &ev1, State *s2, 
    InspectEvent &ev2, bool &sliceUpdated) {
  assert(ev1.type == MUTEX_LOCK);
  assert(ev2.type == MUTEX_LOCK);
  assert(s1 && "NULL state passed");
  assert(s2 && "NULL state passed");
  assert(s1->is_enabled(ev1));
  assert(s2->is_enabled(ev2));

  vector<State *> cs1 = get_cs(s1, ev1);
  vector<State *> cs2 = get_cs(s2, ev2);

  if (cs1.size() == 0 || cs2.size() == 0) {
    // bail out early: a critical section of size 0 means that the lock was not
    // protecting anything (this might sound outrageous but it sometimes
    // happens with cond signal mutexes). We can say that they will never be
    // depednent.
    return false;
  }


  DEBUG_MSG("[MK DEBUG] cs_dep(): ");
  DEBUG_MSG("\tCS 1:\n" << state_vec_to_string(cs1));
  DEBUG_MSG("\tCS 2:\n" << state_vec_to_string(cs2));

  // compare each item of cs1 to an item in cs2 and try to find a dependency
  for (std::vector<State *>::iterator i = cs1.begin(), ie = cs1.end(); 
      i != ie; ++i) {
    InspectEvent i_event = (*i)->sel_event;
    // over approximate the impact of locks/conds in critical sections by
    // assuming they could all cause deadlocks, i.e. bail out earlier for
    // anything other than memory accessing events
    if (get_event_category(i_event.type) != EC_OBJECT) {
      DEBUG_MSG("[MK DEBUG] encountered non memory accessing event in CS: "
          << i_event.toString() << '\n');
      return true;
    }
    for (std::vector<State *>::iterator j = cs2.begin(), je = cs2.end(); 
        j != je; ++j) {
      InspectEvent j_event = (*j)->sel_event;
      if (get_event_category(j_event.type) != EC_OBJECT) {
        DEBUG_MSG("[MK DEBUG] encountered non memory accessing event in CS: "
            << j_event.toString() << '\n');
        return true;
      }

      if (i_event.dependent(j_event)) {
        DEBUG_MSG("\tPeak CS: found dependent events:\n\t" 
            << i_event.toString() << "\n\t" << j_event.toString() << '\n');
        if (setting.flow_dep) {
          // prune non-flow-dep events
          if (on_slice(i_event) || on_slice(j_event)) {
            if (slice_insert(i_event, j_event)) {
              // slice_insert return true if the slice is updated
              sliceUpdated = true;
            }
            // dependent and atleast one is on the slice
            return true;
          }
          else {
            DEBUG_MSG("\tneither on slice\n");
            assert(!on_slice(i_event) && !on_slice(j_event));
            // dependent but neither is on the slice so keep checking
            continue;
          }
        } // if (setting.flow_dep)
        else {
          // we've found two dependent events in the slice so our work is done
          return true;
        }
      } // i `dependent` j
    } // for (j)
  } // for (i)

  return false;
}

vector<State *> Scheduler::get_cs(State *st, InspectEvent &ev) {
  assert(st && "NULL state passed");
  assert(st->is_enabled(ev));
  assert(ev.type == MUTEX_LOCK);
  vector<State *> cs;


  DEBUG_MSG("[MK DEBUG] get_cs()\n\tbase event: " << ev.toString() << '\n');

  // We need to extract the critical section for the thread. It is possible
  // that the event (ev) is simply enabled in the state (st). However, the
  // thread must have eventually executed the mutex lock call (the programs we
  // are testing are finite) so find that state.
  st = find_selected_state(st, ev);
  assert(st != NULL && "event never selected in execution");

#if 0
  if (st == NULL) {
    if (!setting.race_only_flag) {
      // race only flag allows the program to exit while threads are still
      // holding locks.
      assert(st != NULL && "event never selected in execution");
    }
    else {
      // There is a possibility that the enabled event is never executed when we
      // are in race-only mode: when this option is enabled, inspect does not
      // care if, when the program exits, there are disabled threads still in the
      // state. This can lead to a situation where a thread is blocking on a lock
      // call so it is never executed. In this case, we cannot see what is
      // inside the critical section of a thread.
      return cs; // default initialized (size == 0)
    }
  }
#endif

  // Don't compare the state with itself
  st = st->next;
  // find matching unlock call to the passed event
  while (1) {
    assert(st != NULL && "reached end of state chain before finding unlock call");
    InspectEvent &curr = st->sel_event;
    DEBUG_MSG("\tComparing to: " << curr.toString() << '\n');

    if (curr.type == MUTEX_LOCK && curr.mutex_id == ev.mutex_id
        && curr.thread_id == ev.thread_id) {
      cerr << "[ERROR] recursive mutex lock call found in critical section\n";
      cerr << "\tev1: " << ev.toString() << "\n\t" 
           << "ev2: " << curr.toString() << endl;
      assert(0 && "see errors above");
    }

    if (is_lock_unlock_pair(ev, curr)) {
      break;
    }

    if (curr.thread_id == ev.thread_id) {
      cs.push_back(st);
    }


    st = st->next;
  }

  return cs;
}

bool Scheduler::is_lock_unlock_pair(InspectEvent &ev1, InspectEvent &ev2) 
    const {
  assert(ev1.type == MUTEX_LOCK);

  if (ev2.type == MUTEX_UNLOCK && ev2.thread_id == ev1.thread_id 
      && ev2.mutex_id == ev1.mutex_id) {
    return true;
  }
  return false;
}

bool Scheduler::on_slice(const InspectEvent &e) const {
  if (sliceIDs.count(e.inst_id))
    return true;
  return false;
}

bool Scheduler::slice_insert(InspectEvent &e1, InspectEvent &e2) {
  bool ret;
  ret = false;
  if (!sliceIDs.count(e1.inst_id)) {
    // not already on slice
    DEBUG_MSG("Adding to slice: " << e1.toString() << '\n');
    sliceIDs.insert(e1.inst_id);
    ret = true;
  }
  if (!sliceIDs.count(e2.inst_id)) {
    // not already on slice
    DEBUG_MSG("Adding to slice: " << e2.toString() << '\n');
    sliceIDs.insert(e2.inst_id);
    ret = true;
  }
  assert(sliceIDs.count(e1.inst_id) && sliceIDs.count(e2.inst_id)
      && "item not inserted in slice_insert");

  return ret;
}


State *Scheduler::find_selected_state(State *st, InspectEvent &ev)
    const {
  for ( ; st != NULL; st = st->next) {
    if (st->sel_event == ev) {
      return st;
    }
  }
  return NULL;
}


string Scheduler::state_vec_to_string(const vector<State *> &sv) const {
  stringstream ss;
  for (vector<State *>::const_iterator i = sv.begin(), e = sv.end(); 
      i != e; ++ i) {
    ss << (*i)->sel_event.toString() << '\n';
  }
  return ss.str();
}

#endif
