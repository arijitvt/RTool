#include "inspect_clap.hh"
#include "inspect_pthread.hh"
#include "object_table.hh"
#include "inspect_llvm.hh"
#include "exit_tracker.hh"
#include <iostream>
#include <fstream>
#include <execinfo.h>
#include <cerrno>

#include <sstream>
#include <string>

bool skip_clap = false;  //set by 'inspect_thread_start()'
bool target_trace = false;  //set by command-line argument of 'inspect'
bool lin_check = false;  //set by command-line argument of 'inspect'
bool lin_serial = false;  //set by command-line argument of 'inspect'
pthread_mutex_t lin_serial_LOCK = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t CAS_LOCK = PTHREAD_MUTEX_INITIALIZER;

static std::map<long, int> thread_to_idx;
static std::map<long, int> mutex_to_idx;
static std::map<long, int> cond_to_idx;
static std::map<long, int> obj_to_idx;
static std::map<long, int> barrier_to_idx;
static ExitTracker exit_tracker;

#ifdef FLOW_DEP
#if 0
// Exeuction information for the current run
static ExecInfo curr_exec_info;
// To look up a thread's socket
extern SocketIndex g_socket_index;
#endif
#endif

extern ObjectIndex g_object_index;
extern ThreadIndex g_thread_index;

using namespace std;

ofstream myfile;

//----------------------------------------------------------------------------
// Start of the implementation of a scoped lock
//----------------------------------------------------------------------------
pthread_mutex_t _ClapLock_m = PTHREAD_MUTEX_INITIALIZER;
// to ensure that clap_* functions are sequentialized
class _ClapLock {
public:
	_ClapLock() {
		pthread_mutex_lock(&_ClapLock_m);
	}
	~_ClapLock() {
		pthread_mutex_unlock(&_ClapLock_m);
	}
};
#define CLAP_SCOPED_LOCK  _ClapLock _ClapLock_dummy;
//----------------------------------------------------------------------------
// End of the implementation of a scoped lock
//----------------------------------------------------------------------------

void clap_main_end_atexit(void);
void clap_thread_end_atexit(void* arg);

//----------------------------------------------------------------------------

// returns true if the thread was found and removed. Returns false if it was
// not found, and nothing was modified
static bool thread_remove(pthread_t *thread_ptr) {
  if (skip_clap)
    return true;

  long thread = *((long*) thread_ptr);
  bool found;
  {
    CLAP_SCOPED_LOCK
    auto iter = thread_to_idx.find(thread);
    if (iter == thread_to_idx.end()) {
      found = false;
    }
    else {
      found = true;
      thread_to_idx.erase(iter);
      assert(thread_to_idx.find(thread) == thread_to_idx.end());
    }
  }
  return found;
}

// returns true if the calling thread is the main thread (thread_to_idx 0)
static bool is_main_thread() {
  long thread = (long) pthread_self();
  int tid;
  {
    CLAP_SCOPED_LOCK
      ;
    auto iter = thread_to_idx.find(thread);
    assert(iter != thread_to_idx.end() && "thread not found");
    tid = iter->second;
  }

  if (tid == 0)
    return true;
  return false;
}


static bool thread_find_or_add(pthread_t *thread_ptr) {
	if (skip_clap)
		return true;
	long thread = *((long*) thread_ptr);

	bool found = true;
	{
		CLAP_SCOPED_LOCK
                  ;
		if (thread_to_idx.find(thread) == thread_to_idx.end()) {
			found = false;
                        thread_to_idx.insert(make_pair(thread, (int) thread_to_idx.size()));
		}
	}
	if (found == false) {
		//printf("%s(%0x)\n", __FUNCTION__, thread_ptr);
		inspect_thread_start(" ");
		if (!skip_clap) {
			if (thread_to_idx.size() == 1) {
                                // mark main thread as started
                                exit_tracker.started((long) pthread_self());
				atexit(clap_main_end_atexit);
			}
			if (lin_serial) {
				clap_mutex_init(&lin_serial_LOCK, 0);
			}
		}
//	cout<<thread_to_idx[thread]<<endl;
	}
	return found;
}



static bool thread_find_or_add_self() {
	if (skip_clap)
		return true;
	pthread_t self = pthread_self();
	return thread_find_or_add(&self);
}

static bool mutex_find_or_add(pthread_mutex_t *mutex_ptr) {
	if (skip_clap)
		return true;
	bool found = true;
	long mutex = (long) mutex_ptr;
	{
		CLAP_SCOPED_LOCK
		;
		if (mutex_to_idx.find(mutex) == mutex_to_idx.end()) {
			found = false;
			mutex_to_idx[mutex] = (int) mutex_to_idx.size();
		}
	}
	if (found == false) {
		//printf("%s(%0x)\n",  __FUNCTION__, (long)mutex_ptr);
		inspect_mutex_init(mutex_ptr, 0);
	}
	return found;
}

static void mutex_find_n_remove(pthread_mutex_t *mutex_ptr) {
  if (skip_clap)
    return;
  long mutex = (long) mutex_ptr;
  {
    CLAP_SCOPED_LOCK
      ;
    std::map<long,int>::iterator it = mutex_to_idx.find(mutex);
    if (it != mutex_to_idx.end()) {
      //printf("%s(%0x)\n",  __FUNCTION__, (long)mutex_ptr);
      mutex_to_idx.erase(it);
    }
  }
}

static bool cond_find_or_add(pthread_cond_t *cond_ptr) {
	if (skip_clap)
		return true;
	bool found = true;
	long cond = (long) cond_ptr;
	{
		CLAP_SCOPED_LOCK
                  ;
		if (cond_to_idx.find(cond) == cond_to_idx.end()) {
			found = false;
			cond_to_idx[cond] = (int) cond_to_idx.size();
		}
	}
	if (found == false) {
		//printf("%s(%0x)\n", __FUNCTION__, cond_ptr);
		inspect_cond_init(cond_ptr, 0);
	}
	return found;
}

static void cond_find_n_remove(pthread_cond_t *cond_ptr) {
  if (skip_clap)
    return;
  long cond = (long) cond_ptr;
  {
    CLAP_SCOPED_LOCK
      ;
    std::map<long,int>::iterator it = cond_to_idx.find(cond);
    if ( it != cond_to_idx.end()) {
      //printf("%s(%0x)\n", __FUNCTION__, cond_ptr);
      cond_to_idx.erase( it );
    }
  }
}

static bool obj_find_or_add(int *obj_ptr) {
	if (skip_clap)
		return true;
	bool found = true;
	long obj = (long) obj_ptr;
	{
		CLAP_SCOPED_LOCK
                  ;
		if (obj_to_idx.find(obj) == obj_to_idx.end()) {
			found = false;
			obj_to_idx[obj] = (int) obj_to_idx.size();
		}
	}
	if (found == false) {
		//printf("%s(%0x)\n", __FUNCTION__, obj_ptr);
		inspect_obj_reg(obj_ptr);
	}
	return found;
}


static bool barrier_find_or_add(pthread_barrier_t *barrier_ptr) {
	if (skip_clap)
		return true;
	bool found = true;
	long barrier = (long) barrier_ptr;
	{
		CLAP_SCOPED_LOCK
                  ;
		if (barrier_to_idx.find(barrier) == barrier_to_idx.end()) {
			found = false;
			barrier_to_idx[barrier] = (int) barrier_to_idx.size();
		}
	}
	if (found == false) {
		printf("%s(%0ld)\n",  __FUNCTION__, (long)barrier_ptr);
          assert(0);
	}
	return found;
}

static void barrier_find_n_remove(pthread_barrier_t *barrier_ptr) {
  if (skip_clap)
    return;
  long barrier = (long) barrier_ptr;
  {
    CLAP_SCOPED_LOCK
      ;
    std::map<long,int>::iterator it = barrier_to_idx.find(barrier);
    if (it != barrier_to_idx.end()) {
      //printf("%s(%0x)\n",  __FUNCTION__, (long)barrier_ptr);
      barrier_to_idx.erase(it);
    }
  }
}


int get_unique_inst_id(int inst_id) {
	return inst_id;

#if 0
	// retrieve the BOUNDED call stack  !!! doesn't work !!!
	static std::map<vector<void*>, int> callstack_to_idx;
	vector<void*> calls;
	{
		void *array[100];
		size_t size = backtrace(array, 10);
		if (0) {
			printf(" get_call_stack() --> %d\n", (int) size);
			char **strings = backtrace_symbols(array, size);
			printf("Obtained %zd stack frames.\n", size);
			for (unsigned int i = 0; i < size; i++) {
				printf("%s\n", strings[i]);
			}
			free(strings);
		}

		for (size_t i = 0; i < size; i++) {
			calls.push_back(array[i]);
		}
	}
	// make (inst_id) a fake call
	calls.push_back((void*) inst_id);

	int idx = 0;
	{
		CLAP_SCOPED_LOCK
		;
		if (callstack_to_idx.find(calls) == callstack_to_idx.end()) {
			callstack_to_idx[calls] = (int) callstack_to_idx.size();
		}
		idx = callstack_to_idx[calls];
	}
	return idx;
#endif
}

//----------------------------------------------------------------------------
//  PThreads API Routines
//----------------------------------------------------------------------------

void clap_thread_start() {

	thread_find_or_add_self();
}

void clap_thread_end() {
	thread_find_or_add_self();
	if (skip_clap)
		return;

	inspect_thread_end();
}

void clap_main_end_atexit(void) {
	if (skip_clap)
		return;

        // it seems there is a chance that the main thread calls this function
        // twice (perhaps atexit() is firing)
        if (!exit_tracker.has_exited((long) pthread_self())) {
          exit_tracker.exited((long) pthread_self());
          inspect_thread_end();
        }

#if 0
#ifdef MK_DEBUG
        cout << "[MK DEBUG] Main exiting\n";
        cout << "\t thread_id == " << g_thread_index.get_my_thread_id() << '\n';
#ifdef FLOW_DEP
        cout << "[MK DEBUG] dumping load/store execution information\n";
        cout << curr_exec_info.to_string();

        // Send the scheduler the collected supplemental information for
        // flow-dependence checking. This needs to be done before
        // inspect_thread_end() is called 
        Socket *sock;

        sock = g_socket_index.get_my_socket();
#ifdef MK_DEBUG
        cout << "[MK DEBUG] main thread sending to scheduler\n";
        cout << "\tsockfd == " << sock->sock_fd << '\n';
#endif
        sock->send_string(curr_exec_info.serialize_to_string());
#endif

        // Cleanup the socket. This was previously done in inspect_thread_end()
        g_socket_index.remove(g_thread_index.get_my_thread_id());
#endif
#endif

}

void clap_thread_end_atexit(void* arg) {
	if (skip_clap)
		return;

#ifdef MK_DEBUG
        cerr << "Marking thread as exited: " << pthread_self() << '\n';
#endif
        assert(!exit_tracker.has_exited((long) pthread_self()) && "thread exited twice");
        exit_tracker.exited((long) pthread_self());
	inspect_thread_end();
}

void* THREAD_ROUTINE_CLAP(void* arg) {
	thread_find_or_add_self();

	void **args = (void**) arg;
	void *(*start_routine)(void*) = (void*(*)(void*)) args[0];
	void *start_routine_arg = args[1];

        // mark the thread as starting. The thread is marked as exited in the
        // cleanup handler (clap_thread_end_atexit)
        exit_tracker.started((long) pthread_self());
#ifdef MK_DEBUG
        cerr << "Set thread as started: " << (long) pthread_self() << '\n';
#endif

	void* res;
	{
		/// register cleanup routine (for normall or cancelled exit)
		pthread_cleanup_push(clap_thread_end_atexit, (void*)0);
			res = start_routine(start_routine_arg);
			/// 1 means leave the cleanup routine in the system
			pthread_cleanup_pop(1);
	}
	return res;
}

int CLAP_API clap_thread_create(pthread_t * thread, const pthread_attr_t * attr,
		void *(*start_routine)(void*), void * arg) {
	thread_find_or_add_self();
	if (skip_clap)
		return pthread_create(thread, attr, start_routine, arg);
        assert(!exit_tracker.has_exited((long) pthread_self()) && "pthread_create() after thread exited");

	void **args = (void**) malloc(sizeof(void*) * 4);
	args[0] = (void*) start_routine;
	args[1] = arg;
	int res = inspect_thread_create(thread, attr, THREAD_ROUTINE_CLAP,
			(void*) args);

	return res;
}

int CLAP_API clap_thread_join(pthread_t thread, void **value_ptr) {
	thread_find_or_add_self();

	if (skip_clap)
		return pthread_join(thread, value_ptr);

        assert(!exit_tracker.has_exited((long) pthread_self()) && "pthread_join() after thread exited");

        int ret = inspect_thread_join(thread, value_ptr);
	//return inspect_thread_join(thread, value_ptr);
        // remove the child from the CLAP thread index. The child thread must
        // still be in the thread_to_idx map
        bool childFound = thread_remove(&thread);
        assert(childFound && "child thread not found in TID map");

        return ret;
}

void CLAP_API clap_thread_exit(void* ret) {
#ifdef MK_DEBUG
        cerr << "calling pthread_exit()\n";
#endif
        // the main thread does not have a pthread_cleanup handler so it will
        // not call clap_main_end_atexit when pthread_exit() is called. That
        // function is registered with atexit() which is not called when
        // pthread_exit is called
        if (is_main_thread()) {
#ifdef MK_DEBUG
          cerr << "main thread exiting\n";
#endif
          clap_main_end_atexit();
        }
        return pthread_exit(ret);
}

int CLAP_API clap_mutex_init(pthread_mutex_t * mutex,
		const pthread_mutexattr_t * attr) {
	thread_find_or_add_self();
	if (skip_clap)
		return pthread_mutex_init(mutex, attr);
        assert(!exit_tracker.has_exited((long) pthread_self()) && "mutex_init() after thread exited");

	mutex_find_or_add(mutex);
	return 0;
}

int CLAP_API clap_mutex_destroy(pthread_mutex_t * mutex) {
	thread_find_or_add_self();
	if (skip_clap || exit_tracker.has_exited((long) pthread_self()))
		return pthread_mutex_destroy(mutex);

	mutex_find_or_add(mutex);
	int res = inspect_mutex_destroy(mutex);
        mutex_find_n_remove(mutex);
        return res;
}

int CLAP_API clap_mutex_lock(pthread_mutex_t *mutex) {
	thread_find_or_add_self();
	if (skip_clap)
		return pthread_mutex_lock(mutex);
        assert(!exit_tracker.has_exited((long) pthread_self()) && "mutex_lock() after thread exited");

	mutex_find_or_add(mutex);
	return inspect_mutex_lock(mutex);
}

int CLAP_API clap_mutex_unlock(pthread_mutex_t *mutex) {
	thread_find_or_add_self();
	if (skip_clap)
		return pthread_mutex_unlock(mutex);
        assert(!exit_tracker.has_exited((long) pthread_self()) && "mutex_unlock() after thread exited");

	mutex_find_or_add(mutex);
	return inspect_mutex_unlock(mutex);
}

int CLAP_API clap_cond_init(pthread_cond_t * cond,
		const pthread_condattr_t * attr) {
	thread_find_or_add_self();
	if (skip_clap)
		return pthread_cond_init(cond, attr);
        assert(!exit_tracker.has_exited((long) pthread_self()) && "cond_init() after thread exited");

	cond_find_or_add(cond);
	return 0;
}

int CLAP_API clap_cond_destroy(pthread_cond_t * cond) {
	thread_find_or_add_self();
	if (skip_clap)
		return pthread_cond_destroy(cond);
        assert(!exit_tracker.has_exited((long) pthread_self()) && "cond_destroy() after thread exited");

	cond_find_or_add(cond);
	int res = inspect_cond_destroy(cond);
        cond_find_n_remove(cond);
        return res;

}

int CLAP_API clap_cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex) {
	thread_find_or_add_self();
	if (skip_clap)
		return pthread_cond_wait(cond, mutex);
        assert(!exit_tracker.has_exited((long) pthread_self()) && "cond_wait() after thread exited");

	mutex_find_or_add(mutex);
	cond_find_or_add(cond);
	return inspect_cond_wait(cond, mutex);
}

int CLAP_API clap_cond_broadcast(pthread_cond_t *cond) {
	thread_find_or_add_self();
	if (skip_clap)
		return pthread_cond_broadcast(cond);
        assert(!exit_tracker.has_exited((long) pthread_self()) && "cond_broadcast() after thread exited");

	cond_find_or_add(cond);
	return inspect_cond_broadcast(cond);
}

int CLAP_API clap_cond_signal(pthread_cond_t *cond) {
	thread_find_or_add_self();
	if (skip_clap)
		return pthread_cond_signal(cond);
        assert(!exit_tracker.has_exited((long) pthread_self()) && "cond_signal() after thread exited");

	cond_find_or_add(cond);
	return inspect_cond_signal(cond);
}

void CLAP_API clap_obj_read(int *obj_addr, int inst_id) {
	thread_find_or_add_self();
	if (skip_clap || exit_tracker.has_exited((long) pthread_self()))
		return;

	obj_find_or_add(obj_addr);
	int uid = get_unique_inst_id(inst_id);
	inspect_obj_read(obj_addr, uid);
}

void CLAP_API clap_obj_write(int *obj_addr, int value, int inst_id) {
	thread_find_or_add_self();
	if (skip_clap || exit_tracker.has_exited((long) pthread_self()))
		return;

	obj_find_or_add(obj_addr);
	int uid = get_unique_inst_id(inst_id);
	inspect_obj_write(obj_addr, uid);
}

void CLAP_API clap_obj_cmpxchg(int *obj_addr, int oldVal, int value, int inst_id) {
	thread_find_or_add_self();
	if (skip_clap || exit_tracker.has_exited((long) pthread_self()))
		return;

	obj_find_or_add(obj_addr);
	int uid = get_unique_inst_id(inst_id);
	inspect_obj_write(obj_addr, uid);
}

void CLAP_API clap_obj_rmw(int *obj_addr, int value, int inst_id) {
	thread_find_or_add_self();
	if (skip_clap || exit_tracker.has_exited((long) pthread_self()))
		return;

	obj_find_or_add(obj_addr);
	int uid = get_unique_inst_id(inst_id);
	inspect_obj_write(obj_addr, uid);
}

InspectEvent * CLAP_API clap_call_begin(char *fun_name, char *obj_ptr) {
	thread_find_or_add_self();
	if (skip_clap)
		return NULL;
        assert(!exit_tracker.has_exited((long) pthread_self()) && "call_begin() after thread exited");

	obj_find_or_add((int*) obj_ptr);
	return inspect_obj_call(fun_name, obj_ptr);
}

InspectEvent * CLAP_API clap_call_end(char *fun_name, char *obj_ptr,
		char* retval, int return_value) {
	thread_find_or_add_self();
	if (skip_clap)
		return NULL;
        assert(!exit_tracker.has_exited((long) pthread_self()) && "call_end() after thread exited");

	obj_find_or_add((int*) obj_ptr);
//	if (return_value) {
//					printf("return value: %d\n", retval);
//				}
	return inspect_obj_resp(fun_name, obj_ptr, retval, return_value);
}

void CLAP_API clap___assert_fail(const char *__assertion, const char *__file,
		unsigned int __line, const char *__function) {
	inspect_assert();
	__assert_fail(__assertion, __file, __line, __function);
}

//---------------------------------------------------------------------------
//
// enum TypeId {
//---------------------------------------------------------------------------
//#define NLZ_FLAG  

#ifdef NLZ_FLAG


// 9/17/2013: Used by Naling Zhang to implement TSO/PSO memory simulators
// 
// This is a thread-specific storeage -- each thread has its own copy
// of this variable named 'nlz_save_MEM_value'
__thread long nlz_save_MEM_value;
#endif

/*
  a =  load  addr;  // instruction from Program-under-test

  is converted into

  clap_load_pre( 4, addr, order, typ, dummy );  
  a =  load  addr;  // instruction from Program-under-test
  clap_load_post( 4, addr, order, typ, dummy );
*/

void CLAP_API clap_load_pre(int num_param, ...) {
  assert( num_param == 4 );
  va_list ap;
  va_start(ap, num_param);
  int *addr = va_arg(ap, int*);
  llvm_AtomicOrdering_t ord = va_arg(ap, llvm_AtomicOrdering_t);
  int typ = va_arg(ap, int);
  int inst_id = va_arg(ap,int);
  va_end(ap);

  // Silence unused variable warnings
  (void) ord;
  (void) typ;
  (void) addr;

#ifdef NLZ_FLAG 
  // save MEMORY[addr] to the temp variable
  if (typ == sizeof(int))
    nlz_save_MEM_value = * ((int*)addr);
  else if (typ == sizeof(long))
    nlz_save_MEM_value = * ((long*)addr);    
  // Naling: in "inspect_obj_read(addr)", overwrite MEMORY[addr]
  // for example, 
  //   *addr = TSO_BUFFER (addr);
#endif

  clap_obj_read(addr, inst_id);
}

void CLAP_API clap_load_post(int num_param, ...) {
  assert( num_param == 4 );
  va_list ap;
  va_start(ap, num_param);
  int *addr;
  addr = va_arg(ap, int*); // get arg 1
  llvm_AtomicOrdering_t ord = va_arg(ap, llvm_AtomicOrdering_t);
  int typ = va_arg(ap, int); // get arg 2
  int inst_id = va_arg(ap,int); // get arg 3
  va_end(ap);

  // Silence unused variable warnings
  (void) ord;
  (void) typ;
  (void) addr;
  (void) inst_id;

  // do nothing

#ifdef NLZ_FLAG
  // restore MEMORY[addr] from the temp variable, since MEMORY[addr]
  // may have been overwritten inside "inspect_obj_read()"
  if (typ == sizeof(int))
    *((int*)addr) = (int)nlz_save_MEM_value;
  else if (typ == sizeof(long))
    *((long*)addr) = (long)nlz_save_MEM_value;
#endif

}

/*
  store val  addr;   // instruction from Program-under-test

  is converted into

  clap_store_pre( 5, addr, val, ord, typ, id );  
  store val  addr;  // instruction from Program-under-test
  clap_store_post( 5, addr, val, ord, typ, id );
*/
void CLAP_API clap_store_pre(int num_param, ...) {
  assert( num_param == 5 );
  va_list ap;
  va_start(ap, num_param);
  int *addr = va_arg(ap, int*);
  int value = va_arg(ap, int);
  llvm_AtomicOrdering_t ord = va_arg(ap, llvm_AtomicOrdering_t);
  int typ = va_arg(ap, int);
  int inst_id = va_arg(ap,int);
  va_end(ap);

  // Silence unused variable warnings
  (void) ord;
  (void) typ;
  (void) addr;
  (void) inst_id;
	
#ifdef NLZ_FLAG
  // save MEMORY[addr] to the temp variable
  if (typ == sizeof(int))
    nlz_save_MEM_value = * ((int*)addr);
  else if (typ == sizeof(long))
    nlz_save_MEM_value = * ((long*)addr);    
  // Naling: inside "inspect_obj_write(addr)", you may want to
  // store 'value' into a TSO_BUFFER instead of MEMORY[addr]
  //
  // for example, 
  //
  //   TSO_BUFFER( addr ) = value;
#endif

  clap_obj_write(addr, value, inst_id);
}

void CLAP_API clap_store_post(int num_param, ...) {
 assert( num_param == 5 );
  va_list ap;
  va_start(ap, num_param);
  int *addr = va_arg(ap, int*);
  int value = va_arg(ap, int);
  llvm_AtomicOrdering_t ord = va_arg(ap, llvm_AtomicOrdering_t);
  int typ = va_arg(ap, int);
  int inst_id = va_arg(ap,int);
  va_end(ap);

  // Silence unused variable warnings
  (void) ord;
  (void) typ;
  (void) addr;
  (void) inst_id;

  // do nothing

#if 0
#ifdef FLOW_DEP
  if (!skip_clap) {
    // See assumption in clap_load_post
    long long val;

    if (typ == sizeof(char)) {
      val = *((char *) addr);
    }
    else if (typ == sizeof(short)) {
      val = *((char *) addr);
    }
    else if (typ == sizeof(int)) {
      val = *((int *) addr);
    }
    else if (typ == sizeof(long long)) {
      val = *((long long *) addr);
    }
    else {
      assert(0 && "unknown size encountered");
    }

    pthread_t self = pthread_self();

    // Thread should already have been created
    assert(thread_to_idx.find((long) self) != thread_to_idx.end());
    int tid = thread_to_idx[(long) self];
    curr_exec_info.push_back(LoadStoreInfo(val, tid, false));
  }
#endif
#endif

#ifdef NLZ_FLAG
  // restore MEMORY[addr] from the temp variable, which means
  // MEMORY[addr] has never been written -- because the value has been
  // written to the TSO_BUFFER inside "inspect_obj_write()"

  /*
  if (typ == sizeof(int))
    *((int*)addr) = (int)nlz_save_MEM_value;
  else if (typ == sizeof(long))
    *((long*)addr) = (long)nlz_save_MEM_value;
  */
#endif

}


/*
  cmpxchg  cmp newVal  addr;   // instruction from Program-under-test

  is converted into

  clap_cmpxchg_pre( 6, addr, cmp, newVal, ord, typ, id );  
  cmpxchg cmp newVal addr;  // instruction from Program-under-test
  clap_cmpxchg_post( 6, addr, cmp, newVal, ord, typ, id );
*/

void CLAP_API clap_cmpxchg_pre(int num_param, ...) {
  assert(num_param == 6);
  va_list ap;
  va_start(ap, num_param);
  int *addr = va_arg(ap, int*);
  int old_val = va_arg(ap,int);
  int value = va_arg(ap,int);
  llvm_AtomicOrdering_t ord = va_arg(ap, llvm_AtomicOrdering_t);
  int typ = va_arg(ap, int);
  int inst_id = va_arg(ap,int);
  va_end(ap);

#ifdef NLZ_FLAG
  // save MEMORY[addr] to the temp variable
  if (typ == sizeof(int))
    nlz_save_MEM_value = * ((int*)addr);
  else if (typ == sizeof(long))
    nlz_save_MEM_value = * ((long*)addr);    

  // Naling: inside "inspect_obj_write(addr)", you may want to
  // store 'value' into a TSO_BUFFER instead of MEMORY[addr]
  //
  // for example, 
  //
  //   TSO_BUFFER( addr ) = value;
#endif

  clap_obj_cmpxchg(addr, old_val, value, inst_id);
}

void CLAP_API clap_cmpxchg_post(int num_param, ...) {
  assert(num_param == 6);
  va_list ap;
  va_start(ap, num_param);
  int *addr = va_arg(ap, int*);
  int old_val = va_arg(ap,int);
  int value = va_arg(ap,int);
  llvm_AtomicOrdering_t ord = va_arg(ap, llvm_AtomicOrdering_t);
  int typ = va_arg(ap, int);
  int inst_id = va_arg(ap,int);
  va_end(ap);

  // do nothing

#ifdef NLZ_FLAG
  // restore MEMORY[addr] from the temp variable, which means
  // MEMORY[addr] has never been written -- because the value has been
  // written to the TSO_BUFFER inside "inspect_obj_write()"

 /*
  if (typ == sizeof(int))
    *((int*)addr) = (int)nlz_save_MEM_value;
  else if (typ == sizeof(long))
    *((long*)addr) = (long)nlz_save_MEM_value;
  */
#endif

  usleep(1000);
}

/*
  rmw  newVal  addr;   // instruction from Program-under-test

  is converted into

  clap_cmpxchg_pre( 5, addr, newVal, ord, typ, id );  
  rmw  newVal addr;  // instruction from Program-under-test
  clap_cmpxchg_post( 5, addr, newVal, ord, typ, id );
*/
void CLAP_API clap_atomicrmw_pre(int num_param, ...) {
  assert(num_param == 5);
  va_list ap;
  va_start(ap, num_param);
  int *addr = va_arg(ap, int*);
  int value = va_arg(ap,int);
  llvm_AtomicOrdering_t ord = va_arg(ap, llvm_AtomicOrdering_t);
  int typ = va_arg(ap, int);
  int inst_id = va_arg(ap,int);
  va_end(ap);
	
#ifdef NLZ_FLAG
  // save MEMORY[addr] to the temp variable
  if (typ == sizeof(int))
    nlz_save_MEM_value = * ((int*)addr);
  else if (typ == sizeof(long))
    nlz_save_MEM_value = * ((long*)addr);    

  // Naling: inside "inspect_obj_write(addr)", you may want to
  // store 'value' into a TSO_BUFFER instead of MEMORY[addr]
  //
  // for example, 
  //
  //   TSO_BUFFER( addr ) = value;
#endif

  clap_obj_rmw(addr, value, inst_id);
}

void CLAP_API clap_atomicrmw_post(int num_param, ...) {
  assert(num_param == 5);
  va_list ap;
  va_start(ap, num_param);
  int *addr = va_arg(ap, int*);
  int value = va_arg(ap,int);
  llvm_AtomicOrdering_t ord = va_arg(ap, llvm_AtomicOrdering_t);
  int typ = va_arg(ap, int);
  int inst_id = va_arg(ap,int);
  va_end(ap);

  // do nothing

#ifdef NLZ_FLAG
  // restore MEMORY[addr] from the temp variable, which means
  // MEMORY[addr] has never been written -- because the value has been
  // written to the TSO_BUFFER inside "inspect_obj_write()"
 
 /*
  if (typ == sizeof(int))
    *((int*)addr) = (int)nlz_save_MEM_value;
  else if (typ == sizeof(long))
    *((long*)addr) = (long)nlz_save_MEM_value;
  */
#endif

  usleep(1000);
}

// This is a function call for which we want to check linearizability?
bool is_lin_check_method(char *fname) {
  bool res = false;
  if (fname != NULL && strstr(fname, "::")) {
    // TO-DO: check if "fname" is a method of the concurrent object, for which
    // we want to check linearizability
    static bool flag_read = false;
    static set<string> function_filter;
    if (flag_read == false) {
      flag_read = true;
      char* filter_file = "linFilter/filter.txt";
      fstream fin(filter_file);
      string line;
      cout << __FUNCTION__   << " Filter: { ";
      while (getline(fin, line)) {
        function_filter.insert(line);
        cout << "           " <<  line << ", ";
      }
      cout << " }" << endl;
      fin.close();    
    }
    // is fname part of the function_filter?
    string fstr(fname);
    if (function_filter.find(fstr) != function_filter.end()) {
      res = true;
    }
  }
  return res;
}

map<int,int> lin_lock_holder;

void CLAP_API clap_call_pre(int num_param, ...) {
	thread_find_or_add_self();
	if (skip_clap) return;

        if (0){ //debugging only
          va_list ap;
          va_start(ap, num_param);
          char *fname = va_arg(ap, char*);
          char *ptr = 0;
          if (num_param >= 2) 
            ptr = va_arg(ap, char*);
          va_end(ap);
          printf("dbg: clap_call_pre ( %s , %zu)\n", fname, (size_t)ptr);
        }

	if (lin_check) {
		// we want to check linearizability ...
		va_list ap;
		va_start(ap, num_param);
		char *fname = va_arg(ap, char*);
		char *ptr = 0;
		if (num_param >= 2) {
			ptr = va_arg(ap, char*);
		}
		va_end(ap);
		////
//		printf("call_pre: %s\n", fname);
		if (fname && is_lin_check_method(fname)) {

//			if (true) {
			//forcing 'inspect' to handle the call (pre and post) atomically
//				printf("lock   %s",fname);
			if (lin_serial) {
				int tid = g_thread_index.get_my_thread_id();

//				cout<<"call tid: "<<tid<<endl;

				if(lin_lock_holder.find(tid)==lin_lock_holder.end()){

					clap_mutex_lock(&lin_serial_LOCK);
					lin_lock_holder[tid]=0;

				}
				else{
					lin_lock_holder[tid]++;
				}
			}
//				printf("lock   %s (ptr = %d)\n", fname, ptr);
                        assert( ptr );
			InspectEvent * ret = clap_call_begin(fname, ptr);
//			cout << ret->toString() << endl;
			if (lin_serial) {
//				cout << ret->toString() << endl;
				myfile << ret->toString() << endl;
			}

		}
	}
}



void CLAP_API clap_call_post(int num_param, ...) {

         if (0) {  //debugging only
            va_list ap;
            va_start(ap, num_param);
            char *fname = va_arg(ap, char*);
            char *ptr = 0;
            if (num_param >= 2) 
              ptr = va_arg(ap, char*);
            va_end(ap);
            printf("dbg: clap_call_POST( %s , %zu)\n", fname, (size_t)ptr);
          }

	if (lin_check) {
		// we want to check linearizability ...
		va_list ap;
		va_start(ap, num_param);
		char *fname = va_arg(ap, char*);
		char *ptr = 0;
		char *tmp = 0;
		char *retval = (char*) -1;
		int return_value = 0;

		// To-DO:   please check which is the obj_ptr, and which is the return val

//		printf("call_post: %s\n", fname);
		////
		if (fname && is_lin_check_method(fname)) {

			if (num_param > 2) {
				retval = va_arg(ap, char*);
//				cout << "return value:" << ((int) retval) << endl;
//				printf("return value: %d	\n", retval);

				ptr = va_arg(ap, char*);
//				printf("ptr value: %d\n",ptr);
				return_value = 1;

			} else {
				assert(num_param == 2);
				ptr = va_arg(ap, char*);
			}
//			printf("unlock   %s (ptr = %d)\n", fname, ptr);
//			if (return_value) {
//				printf("return value: %d\n", retval);
//			}
			InspectEvent * ret = clap_call_end(fname, ptr, retval,
					return_value);
//			cout << ret->toString() << endl;
			if (lin_serial) {

//				cout << ret->toString() << endl;
				myfile << ret->toString() << endl;
				int tid = g_thread_index.get_my_thread_id();

//				cout<<"resp tid: "<<tid<<endl;
//				if(lin_lock_holder.find(tid)==lin_lock_holder.end()){
//
//					clap_mutex_lock(&lin_serial_LOCK);
//					lin_lock_holder[tid]=0;
//
//				}
//				else{
//					lin_lock_holder[tid]++;
//				}
				if(lin_lock_holder[tid]==0){
					clap_mutex_unlock(&lin_serial_LOCK);
					lin_lock_holder.erase(tid);
				}
				else{
					lin_lock_holder[tid]--;
				}


			}
		}
		va_end(ap);
	}
}




/**
 * Variable: barrier_clap;
 *
 * The static variable for clap barrier.
 **/
static map<pthread_barrier_t *, BARRIER_CLAP *> hm_barrier_clap;


int CLAP_API clap_barrier_init(pthread_barrier_t *barrier,  const pthread_barrierattr_t *attr, unsigned count)
{
  thread_find_or_add_self();
  if (skip_clap)
    return pthread_barrier_init(barrier,attr,count);
  
  barrier_find_or_add(barrier);
  
  assert( hm_barrier_clap.find(barrier) == hm_barrier_clap.end() );
  BARRIER_CLAP *barrier_clap = new BARRIER_CLAP;
  hm_barrier_clap[barrier] = barrier_clap;

  barrier_clap->counter = count;
  barrier_clap->threshold = count;
  int status = clap_mutex_init(&(barrier_clap->mutex),NULL);
  int status2 = clap_cond_init (&(barrier_clap->cond), NULL);
  return (status == 0 ? status : status2);
}

int CLAP_API clap_barrier_destroy(pthread_barrier_t *barrier)  {
  thread_find_or_add_self();
  if (skip_clap)
    return pthread_barrier_destroy(barrier);

  barrier_find_or_add(barrier);

  map<pthread_barrier_t*, BARRIER_CLAP*>::iterator it =
    hm_barrier_clap.find(barrier);
  assert(it != hm_barrier_clap.end());
  BARRIER_CLAP *barrier_clap = it->second; //hm_barrier_clap[barrier];

  clap_mutex_lock(&(barrier_clap->mutex));
  if (barrier_clap->counter != barrier_clap->threshold) {
    clap_mutex_unlock (&(barrier_clap->mutex));
    return EBUSY; // failed because not all threads reached the barrier yet.
  }
  int status = clap_mutex_destroy (&(barrier_clap->mutex));
  int status2 = clap_cond_destroy  (&(barrier_clap->cond));

  //free(barrier_clap);
  hm_barrier_clap.erase( it );
  delete barrier_clap;

  barrier_find_n_remove(barrier);

  return (status == 0 ? status : status2);
}

int CLAP_API clap_barrier_wait(pthread_barrier_t *barrier)  {
  thread_find_or_add_self();
  if (skip_clap)
    return pthread_barrier_wait(barrier);

  int status = 0;
  BARRIER_CLAP *barrier_clap = hm_barrier_clap[barrier];
  clap_mutex_lock(&(barrier_clap->mutex));
  if (--(barrier_clap->counter) == 0) {
    barrier_clap->counter = barrier_clap->threshold;
    clap_cond_broadcast(&(barrier_clap->cond));
    status = -1; // the last one
  } else {
    clap_cond_wait(&(barrier_clap->cond),&(barrier_clap->mutex));
  }
  clap_mutex_unlock (&(barrier_clap->mutex));
  return status; /* -1 for waker, or 0 */
}

/**
 * Daikon Related Function Exposing
 */

#ifdef DAIKON

int CLAP_API getThreadId(pthread_t *threadStr) {
	thread_find_or_add_self();
	for ( map<long,int>::iterator itr = thread_to_idx.begin();
			itr !=  thread_to_idx.end(); ++itr) {
		if(itr->first == *threadStr) {
			return itr->second;
		}
	}
	return -1;
}
#endif

/**
 * Daikon code ends here
 */
