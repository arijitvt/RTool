#ifndef HOOK_H
#define HOOK_H

extern void do_nothing();
extern void write_assert_failure_trace();


#define hook_assert(expr) \
	(expr)? (do_nothing(),0) :(write_assert_failure_trace(),-1)


//extern int write_assert_failure_trace(int cond);

#endif
