
#ifndef __TRANSITION_SET_H__
#define __TRANSITION_SET_H__


#include <vector>
#include <ext/hash_map>
#include <functional>
#include "inspect_event.hh"

using std::vector;
using namespace __gnu_cxx; //::hash_map;
using std::unary_function;

typedef hash_map<int, InspectEvent, __gnu_cxx::hash<int> >  InspectEventMap;
typedef InspectEventMap::iterator  InspectEventMapIterator;


/**
 *  the state of 
 *
 */

class TransitionSet
{
public:
  typedef InspectEventMapIterator  iterator;

public:
  TransitionSet();
  TransitionSet(TransitionSet&);
  ~TransitionSet();
  
  TransitionSet& operator = (TransitionSet&);
  
  inline bool empty() { return events.size() == 0; }   
  inline int size()   {  return events.size(); } 

  bool has_member(InspectEvent &event); 
  bool has_member(int tid);
  void remove(int tid);
  void remove(InspectEvent &event);
  void insert(InspectEvent &event);
  void remove_depedent_events(InspectEvent &event);
  
  InspectEvent& get_transition(int tid);
  InspectEvent& get_transition();

  iterator begin() { return events.begin(); }
  iterator end()   { return events.end(); }
 
  InspectEvent get_race_event(InspectEvent &event);
  
  bool has_rwlock_rdlock(int rwlock_id);
  bool has_rwlock_wrlock(int rwlock_id);
  
  bool operator ==(TransitionSet &another);
  bool operator !=(TransitionSet &another);

public:
  hash_map<int, InspectEvent, __gnu_cxx::hash<int> > events;
};



#endif


