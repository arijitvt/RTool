#ifndef __INSPECT_SCHEDULER__SETTING_HH__
#define __INSPECT_SCHEDULER__SETTING_HH__

#include <string>

using std::string;

class SchedulerSetting
{
public:
  SchedulerSetting();

public:
  bool standalone_flag;
  bool yices_flag;
  bool lazy_flag;
  bool stateful_flag;
  bool symmetry_flag;

  bool deadlock_only_flag;
  bool race_only_flag;
  bool report_race_flag;

  //-------------------------------- chao -------------------------------------
  bool target_trace;
  int  lin_check;
  string replay_file;
  int  max_pcb;
  int  max_pset;
  int  max_runs;
  int  max_seconds;
  int  max_events;        // max number of events allowed in each run
  int  max_spins;         // max times allowed to spin on the same event
  //-------------------------------- chao -------------------------------------

  // defaults to false. If this is true, sleep set is not used to prune
  // transitions. The option is --disable-sleepset
  bool disable_sleepset;

#ifdef FLOW_DEP
  // Note: flow_dep and control_dep are mutually exclusive
  // Defaults are both disabled
  // set flow dependence checking is on or off
  bool flow_dep;

  // Update backtrack info at the end of the run. This is assumed to be true
  // when either flow_dep or control_dep is true
  bool update_at_end;

  // Requires backtrack information to be updated at the end (update_at_end) of
  // the execution. Peeks inside of the critical section of two conflicting
  // lock calls to see if they contain any dependent memory accesses. If there
  // are no dependent memory accesses then the locks are considered
  // independent.
  //
  // Just examining memory access events could cause deadlocks to be missed. As
  // such, if there are any non-memory accessing events (mutex lock, cond wait
  // etc.) then the locks are conservativley determined to be conflicting. A
  // possible optimization would be to check if the set of contained lock calls
  // could cause a deadlock.
  //
  // Defaults to false
  bool cs_peak;

  // Location of the slice database file of clap instruction IDs on the slice
  // (gerenated by AssertionSlice pass)
  string slicedb_filename;
#endif


  int  max_threads;
  int  max_errors;
  int  timeout_val;
  string  socket_file;

  string  target;
  int target_argc;
  char ** target_argv; 
};

extern SchedulerSetting  setting;

void set_stateful_flag();
void clear_stateful_flag();

void set_lazy_flag();
void clear_lazy_flag();

void set_standalone_flag();
void clear_standalone_flag();

void set_symmetry_flag();
void clear_symmetry_flag();

void set_max_pcb(int pcb);
void set_max_pset(int pset);

#endif


