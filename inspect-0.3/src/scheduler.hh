#ifndef __THREAD_SCHEDULER_H__
#define __THREAD_SCHEDULER_H__


#include <vector>
#include <set>
#include <ext/hash_set>
#include <utility>

#include "inspect_util.hh"
#include "inspect_event.hh"
#include "scheduler_object_table.hh"
#include "inspect_exception.hh"

#include "state.hh"
#include "state_stack.hh"
#include "event_buffer.hh"
#include "thread_info.hh"
#include "scheduler_setting.hh"
#include "event_relation_graph.hh"


using std::vector;
using std::set;
using std::map;
using std::pair;
using __gnu_cxx::hash_map;
using __gnu_cxx::hash;


class Scheduler : public Thread
{
public:
  Scheduler();
  ~Scheduler();

  bool init();
  void read_replay_file();

  InspectEvent receive_event( Socket * socket ); 
  InspectEvent receive_main_start();
  void approve_event(InspectEvent &event);

  void run(); 
  void stop();
  
  inline bool is_listening_socket(int fd); 
  void print_out_trace();
  pair<int, int> set_read_fds();

  void exec_test_target(const char * path);


  // The Flow dependence version of DPOR_update_backtrack_info returns true if
  // a new item was added to the slice (i.e. an event was found to be dependent
  // (or transitively dependent) with an event on the slice thus it was added
  // to the sliceIDs set). We updating backtrack information until we reach a
  // fix point (no new items added to the set). This is guaranteed to terminate
  // since there are fixed number of events.
  //
  // If --flow-dep is not enabled, these always return false
  //
  // The valyes from DPOR_update_backtrack_info() are propegated up to
  // update_backtrack_info()
#ifdef FLOW_DEP
  bool update_backtrack_info(State * state);
  bool DPOR_update_backtrack_info(State * state);
  bool DPOR_update_backtrack_info(State * state, InspectEvent &event);
#else
  void update_backtrack_info(State * state);
  void DPOR_update_backtrack_info(State * state);
  void DPOR_update_backtrack_info(State * state, InspectEvent &event);
#endif

  void PCB_update_backtrack_info(State * state);
  void PSET_update_backtrack_info(State * state);

  void PCB_update_backtrack_info(State * state, InspectEvent &event);
  void PSET_update_backtrack_info(State * state, InspectEvent &event);

  void lazy_update_backtrack_info(State * state, InspectEvent &event, bool allow_communicative=true);

  void lazy_update_locks(State*, InspectEvent&);
  void lazy_update_internal(State*, State*, InspectEvent&, InspectEvent&, InspectEvent&);

  State* last_conflicting_state(State *s, InspectEvent &e);

  bool dependent(InspectEvent &e1, InspectEvent &e2);
  bool is_mutex_exclusive_locksets(Lockset *, Lockset*);
  void report();

#ifdef FLOW_DEP
  // Map of Instruction_IDs to OPT results. Used to prevent querying opt more
  // than once for the same instruction
  std::map<int, int> opt_results;

  // Clap IDs on the slice of the call to assert()
  std::set<uint64_t> sliceIDs;

  // Helper function to generate the opt command to pass to system()
  std::string generate_opt_cmd(const int inst_id) const;

  // Returns true if the passed inspect event is on the slice (i.e. its inst ID
  // is in sliceIDs. 
  bool is_on_slice(const InspectEvent &e) const;

  // Examine the critical sections of the two mutex calls passed and determine
  // if they contain any dependent events. As an overapproximation, if the
  // critical section contains anything other than memory accessing events,
  // they are considered dependent (this could be reduced if we calculate if
  // multiple lock calls could cause a deadlock).
  //
  // ev1 is assumed to occur in s1 and similarly for ev2 and s2. This also
  // assumes that only non-recursive mutexes are used. ev1 and ev2 should be
  // mutex lock events. This also requires that the entire trace be visible
  // (or, atleast until the corresponding unlock call for each event.
  //
  // Note: this only gathers states who's selected event matches the thread ID
  // of the passed event.
  //
  // When calculating dependence, this assumes that the lock calls themselves
  // may be co-enabled (ie there is no happens-before relation between them).
  //
  // sliceUpdated is an output parameter: it will be set to true if --flow-dep
  // is enabled and a new item (from the inside of the critical section) is
  // added to the slice. Otherwise, it will be unchanged.
  bool cs_dep(State *s1, InspectEvent &ev1, State *s2, InspectEvent &ev2,
      bool &sliceUpdated);

  // returns true if the passed event in on the slice (i.e. in sliceIDs)
  bool on_slice(const InspectEvent &e) const;

  // Insert the two passed events onto sliceIDs. Returns true if atleast one of
  // the events was not already in sliceIDs (i.e. returns true if sliceIDs was
  // modified)
  bool slice_insert(InspectEvent &e1, InspectEvent &e2);

  // Returns true if the passed events are a potential lock-unlock pair. ev1
  // must be a lock call.
  bool is_lock_unlock_pair(InspectEvent &ev1, InspectEvent &ev2) const;

  // Look forward from the passed state to find the state where the passed
  // event is selected. Returns NULL if no such state is found
  State *find_selected_state(State *st, InspectEvent &ev) const;

  // Return all states in the critical section. ev is assumed to be a mutex
  // lock call occuring in the passed state st. This requires that the mutex
  // unlock call be somewhere on the current execution. 
  //
  // If a mutex unlock call does not occur in the execution then we must be in
  // race-only mode (--race-only). This makes it possible for a thread to be
  // disabled when the thread exits. A default initialized vector will be
  // returned in this case.
  vector<State *> get_cs(State *st, InspectEvent &ev);

  // Number of runs pruned due to flow-dep read-write dependency
  unsigned flow_dep_readwrite_pruned;

  // Number of runs pruned due to flow-dep write-write dependency
  unsigned flow_dep_writewrite_pruned;

  // does nothing at the moment
  void get_flowdep_info();

  // Open the slice DB (see arg --slice-db) and add its contents to sliceIDs
  void processSliceDB();

  // Query the data-dependence LLVM pass of the given event
  bool data_dep_query(const InspectEvent e);

  // Return the text form of the selected events of each state in the passed
  // vector.
  string state_vec_to_string(const vector<State *> &sv) const;

  // Traverses backwards from curr_state calling update_backtrack_info on each
  // state.
  void update_backtrack_at_end(State *curr_state);

  // currently just returns true
  bool flow_dependent(State *old_state, InspectEvent &event, State *curr_state);

  // returns true if one of the events is a read and the other is a write
  bool is_read_write_pair(const InspectEvent &e1, const InspectEvent &e2) const;

  // Returns the OBJ_READ event from the two events. Assumes that at least one
  // of the events is an OBJ_READ. If both are reads, returns e1.
  InspectEvent get_read(const InspectEvent e1, const InspectEvent e2) const;

  // Returns the state in which InspectEvent ev was selected. NULL if none was
  // found. Assumes old_state to be a state that took place before ev (ie
  // old_state's selected event ocurred before ev during the current execution.
  //
  // const friendly to ev and old_state but operator == in InspectEvent is not
  // const friendly
  State *event_to_state(State *old_state, InspectEvent ev) const;

  // Return true if there exists a read ocurring sometime after the passed
  // event occuring in the passed state that (ie the passed event is enabled in
  // the passed state)
  // 1. cannot be co-enabled with the selected event of the passed state, and
  // 2. The read is to the same object as the write
  // 3. The read is from a different thread
  //
  // Assumes that the selected event of the passed state is a write
  //
  // const friend to s and this but some of the functions used are not
  bool exists_non_coenabled_conf_read(State *s, InspectEvent &e);

  // Returns true if the selected event is the last write to the object by the
  // thread.
  bool is_last_write(State *s, InspectEvent &e) const;

  // Returns true if the selected event in the both of the passed states is the
  // last write to the object by each thread. 
  //bool are_last_writes(const State *s1, const State *s2) const;

  //bool are_last_writes(State *s1, InspectEvent &e) const;
#endif // #ifdef FLOW_DEP

  bool examine_state(State*, State*);
public:
  InspectEvent & get_event_from_buffer(int tid);
  void exec_transition(InspectEvent &event);

  void free_run();
  void monitor_first_run();
  void yices_run();
  void backtrack_checking();
//   void monitor();
  State * execute_one_thread_until(State * state, int tid, InspectEvent);
  //State * next_state(State *state, InspectEvent &event, LocalStateTable * ls_table);

public:
  void get_mutex_protected_events(State * state1, State * state2, int tid, vector<InspectEvent>& events);

  State * get_initial_state();

  InspectEvent get_latest_executed_event_of_thread(State * state, int thread_id);

  void check_property(State * state);

  bool found_enough_error() { return (num_of_errors_detected >= max_errors); }
public:
  EventBuffer event_buffer;
  EventGraph  event_graph;
  
public:
  int max_errors;
  int num_of_errors_detected;
  int num_of_transitions;
  int num_of_states;
  int num_of_truncated_branches;
  int num_killed;
  int sut_pid;  
  int run_counter;
  bool found_bug; 
  StateStack state_stack;

  vector<InspectEvent> error_trace;  //RSS
  int exit_status;                   //RSS
  long start_cpu_time;               //RSS
};




#endif



