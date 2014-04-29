#include "transition_set.hh"
#include <cassert>
#include <algorithm>
#include "yices_path_computer_singleton.hh"

using namespace std;
using namespace __gnu_cxx;

TransitionSet::TransitionSet()
{
  events.clear();
}


TransitionSet::TransitionSet(TransitionSet & another)
{
  events = another.events;
}


TransitionSet::~TransitionSet()
{ }


TransitionSet & TransitionSet::operator= (TransitionSet & another)
{
  events = another.events;
  return  *this;
}


bool TransitionSet::has_member(InspectEvent &event)
{
  int tid;
  InspectEventMapIterator it;
  InspectEvent  event2;

  tid = event.thread_id;
  it = events.find(tid);

  if (it == events.end()) return false;  
  return  it->second == event;
}


bool TransitionSet::has_member(int tid)
{
  iterator it;
  it = events.find(tid);
  return (it != events.end());
}


void TransitionSet::remove(int tid)
{
  InspectEventMapIterator it;

  it = events.find(tid);
  if (it != events.end())  events.erase(it);
}


void TransitionSet::remove(InspectEvent &event)
{
  if (! has_member(event) ) return;
  remove(event.thread_id);
}


void TransitionSet::insert(InspectEvent &event)
{
  InspectEventMapIterator it;

  it = events.find(event.thread_id);

//   if (it != events.end() )
//     assert(it->second.invalid() );

  events[event.thread_id] = event;
}



InspectEvent& TransitionSet::get_transition(int tid)
{
  InspectEventMapIterator it;
  it = events.find(tid);
  if (it == events.end()) return InspectEvent::dummyEvent;
  return it->second;
}


InspectEvent& TransitionSet::get_transition()
{
  InspectEventMapIterator it;

  if (events.empty()) return InspectEvent::dummyEvent;
  it = events.begin();
  return it->second;
}



void TransitionSet::remove_depedent_events(InspectEvent &event)
{
  iterator  it;
  InspectEvent tmpev;
  vector<InspectEvent>  tmp_vec;
  vector<InspectEvent>::iterator  vit;

  tmp_vec.clear();
  for (it = events.begin(); it != events.end(); it++){
    if (event.dependent(it->second))
      tmp_vec.push_back(event);
  }

  for (vit = tmp_vec.begin(); vit != tmp_vec.end(); vit++){
    tmpev = *vit;
    events[tmpev.thread_id].clear();
  }
}



InspectEvent TransitionSet::get_race_event(InspectEvent &event)
{
  iterator it;
  InspectEvent tmpev;

  for (it = events.begin(); it != events.end(); it++){
    tmpev = it->second;
    if (tmpev.thread_id == event.thread_id) continue;	       
    if (tmpev.obj_id != event.obj_id ) continue;
    if (tmpev.type == OBJ_WRITE && event.type == OBJ_WRITE ||
        tmpev.type == OBJ_WRITE && event.type == OBJ_READ ||
        tmpev.type == OBJ_READ && event.type == OBJ_WRITE) {
      return tmpev;
    }
  }
  
  return InspectEvent::dummyEvent;  
}


bool TransitionSet::has_rwlock_rdlock(int rwlock_id)
{
  iterator it;
  
  for (it = events.begin(); it != events.end(); it++){
    if(it->second.type == RWLOCK_RDLOCK && it->second.rwlock_id == rwlock_id) 
      return true;
  }
  return false;
}



bool TransitionSet::has_rwlock_wrlock(int rwlock_id)
{
  iterator it;
  
  for (it = events.begin(); it != events.end(); it++){
    if (it->second.type == RWLOCK_WRLOCK && it->second.rwlock_id == rwlock_id)
      return true;
  }
  return false;
}


bool TransitionSet::operator==(TransitionSet &another)
{
  iterator it;
  int tid;
  InspectEvent e1, e2;
  
  if (this->size() != another.size()) return false;

  for(it = events.begin(); it != events.end(); it++){
    tid = it->first;
    e1 = it->second;
    e2 = another.get_transition(tid);
    if (e1 != e2) return false;
  }
  
  return true;
}


bool TransitionSet::operator!=(TransitionSet &another)
{
  return !(*this == another);
}


