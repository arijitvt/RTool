#include "predecessor.hh"

#include <sstream>

PredecessorInfo::PredecessorInfo(unsigned depth) {
  depth_ = depth;
  pruned_ = 0;
}

void PredecessorInfo::PSetUpdate(State *state) {
#ifdef MK_DEBUG
  //cout << "[DEBUG] [MK] PSetUpdate() on state:\n" << state->toString() << '\n';
#endif
  Predecessor pred;
  PredecessorTuple ptuple;

  // Get the immediate predecessor (if any) of state
  pred = GetPredecessor(state);

  // Exapand the immediate predecessor into a predecessor tuple.
  if (pred.valid_) {
#ifdef MK_DEBUG
    cerr << "--------------------------------------------------\n"
         << "[DEBUG] [MK] Predecessor found:\n" 
         << pred.toString() 
         << "\n--------------------------------------------------\n";
#endif
    ptuple = GetPredecessorTuple(pred);
  }

  // If we found a predecessor then keep track of it by adding it to the set
  if (ptuple.valid()) {
#ifdef MK_DEBUG
    cerr << "--------------------------------------------------\n"
         << "[DEBUG] [MK] Predecessor tuple found:\n" 
         << ptuple.toString()
         << "\n--------------------------------------------------\n";
#endif
    assert(ptuple.size() == depth_);
    pred_tuple_set_.insert(ptuple);
  }

  // Always update current predecessors even if the next predecessor is
  // invalid. This gives curr_predecessors_ a one-to-one mapping with the
  // search stack allowing easy "time travel" to see what the predecessors were
  // from a certain search node.
  //
  // This is updated after the ptuple is created so that when iterating over
  // the previous predecessors seen in GetPredecessorTuple() the newly created
  // predecessor is not considered as a dependent predecessor of itself
  
  curr_predecessors_.push_back(pred);
}

bool PredecessorInfo::PSetVisited(InspectEvent &first, InspectEvent &second, 
    const State *secondState) {
  assert(secondState);
  assert(first.valid() && "invalid event passed");
  assert(second.valid() && "invalid event passed");
  assert(first != second);

#ifdef MK_DEBUG
  cerr << "[DEBUG] [MK] PSetVisited, first event:\n" << first.toString()
       << "\nsecond event\n" << second.toString() << '\n';
#endif

  // Create a predecessor based on the two events as if first was executed
  // before second.
  Predecessor pred;
  pred.Init(first, second);
  assert(pred.valid_);

#ifdef MK_DEBUG
  cerr << "[DEBUG] [MK] PSetVisited checking pred:\n" << pred.toString() 
       << '\n';
#endif


  PredecessorTuple ptuple;
  size_t idx;
  idx = GetStateIndex(secondState);

  // Look along the current execution's predecessors starting at the search
  // node where first was executed to see if we can build the predecessor arc
  // set larger
  ptuple = GetPredecessorTuple(pred, idx);

#ifdef MK_DEBUG
  cerr << "[DEBUG] [MK] PsetVisited() checking tuple:\n" << ptuple.toString() 
       << '\n';
#endif

  if (ptuple.valid()) {
    // Check if this tuple is already in our set
    // != end() means in set so return true (meaning we have seen this before)
    if (pred_tuple_set_.find(ptuple) == pred_tuple_set_.end()) {
      // not found, return false, this predecessor tuple has not been visited
      return false;
    }
    else {
      assert(ptuple.valid());
      // the tuple is already in the set so it is visited
      return true;
    }
  }

  // if no tuple is found then it could not have been previously visited
  return false;
}


PredecessorInfo::Predecessor PredecessorInfo::GetPredecessor(State *state) {
  Predecessor ret;
  ret.valid_ = false;

  assert(state && "GetPredecessor() passed NULL");
  assert(state->sel_event.valid() && "GetPredecessor() sel_event invalid");

  // Loook at the current execution trace (the selected events of each state)
  // and attempt to find conflicting accesses
  State *prev_state;

  prev_state = state->prev;

  InspectEvent curr_event;
  assert(state->sel_event.valid() && "invalid sel_event found");
  curr_event = state->sel_event;

  // Skip unlock operations. I do not think the scheduler will schedule them so
  // using them as predecessors will not help. I believe tryping to re-order
  // lock-lock or lock-unlock calls will always be lockset pruned.
  if (curr_event.type == MUTEX_UNLOCK) {
    return ret;
  }

  for (; prev_state != NULL; prev_state = prev_state->prev) {
    assert(prev_state->sel_event.valid() && "invalid sel_event found");

    InspectEvent prev_event;
    prev_event = prev_state->sel_event;

    // As mentioned before, skip unlock calls even though lock-unlock pairs
    // will be dependent
    if (prev_event.type == MUTEX_UNLOCK) {
      continue;
    }

    if (PredecessorTrump(prev_event, curr_event)) {
      // trump found, no need to continue looking
      break;
    }
    else if (curr_event.dependent(prev_event)) {
      // Immediate predecessor found, no need to continue looking
      ret.Init(prev_event, curr_event);
      break;
    }
  }

  // ret.valid may be false
  return ret;
}

PredecessorInfo::PredecessorTuple PredecessorInfo::GetPredecessorTuple(
    PredecessorInfo::Predecessor &pred, size_t startIndex) {
  assert(pred.valid_ && "invalid predecessor passed");

  PredecessorTuple ret;
  // First item in the tuple is always the passed predecessor
  ret.vals_.push_back(pred); 

  auto it = curr_predecessors_.rbegin();

  // startIndex allows for searching through arbitrary positions in
  // curr_predecessors_
  if (startIndex != UINT_MAX) {
    // Example: If size of current_predecessors is 20 and startIndex is 17 then
    // we want to advance the iterator by 3 (20 - 17 = 3).
#ifdef MK_DEBUG
    cerr << "[DEBUG] [MK] GetPredecessorTuple(), startIndex == " << startIndex
         << "\n\tcurr_predecessors_.size() == " << curr_predecessors_.size() 
         << '\n';
#endif
    assert(startIndex <= curr_predecessors_.size());
    if (startIndex != curr_predecessors_.size()) {
      std::advance(it, curr_predecessors_.size() - startIndex);
    }
  }

  for (; it != curr_predecessors_.rend(); ++it) {
    if (ret.size() == depth_) {
      break;
    }

    // current_predecessors_ are the predecessors for the each node on the
    // search stack. This means it can contain invalid pairs if no
    // predeccessors are found for the selected event at a state.
    if (!it->valid_)
      continue;

    if (PredecessorDependent(pred, *it)) {
      ret.vals_.push_back(*it);
    }
  }

  // Only return tuples that are the size of the depth. This prevents smaller
  // tuples from subsuming larger ones.
  if (ret.size() != depth_) {
    ret.vals_.clear();
  }

  assert((ret.size() == 0 || ret.size() == depth_) && "invalid tuple");
  return ret;
}

void PredecessorInfo::Predecessor::Init(InspectEvent &past, InspectEvent &future) {
  // Copy events
  past_event_ = past;
  future_event_ = future;

  // Get minimal information for predecessors
  past_.SetThreadId(past.thread_id);
  past_.SetInstId(past.inst_id);

  // TODO: Initialize the context of past here when it is implemented

  future_.SetThreadId(future.thread_id);
  future_.SetInstId(future.inst_id);

  // TODO: Initialize the context of futurehere when it is implemented

  valid_ = true;
}

bool PredecessorInfo::PredecessorTrump(InspectEvent &future, InspectEvent &past) {
  assert(future.valid() && "PredecessorTrump passed invalid event");
  assert(past.valid() && "PredecessorTrump passed invalid event");

  EventCategory future_cat;
  EventCategory past_cat;

  future_cat = future.event_category();
  past_cat = past.event_category();

  // Events from different categories cannot trump each other
  if (future_cat != past_cat)
    return false;

  // Assumption: Only memory access operations can trump each other
  assert(future_cat == past_cat);
  if (future_cat != EC_OBJECT)
    return false;

  assert(future_cat == EC_OBJECT && past_cat == EC_OBJECT);

  if (future.obj_id == past.obj_id) { // same object
    if (future.thread_id == past.thread_id) { // same thread
      if (past.type == OBJ_WRITE) { // past is a write
        return true;
      }
    }
  }

  return false;
}


bool PredecessorInfo::PredecessorDependent(const Predecessor &p1, 
    const Predecessor &p2) const {
  assert(p1.valid_ && "invalid predecessor passed");
  assert(p2.valid_ && "invalid predecessor passed");

  if (p1 == p2) {
    // a predecessor cannot be dependent on itself
    return false;
  }

  // Find atleast one thread ID match. This comapres each past_/future_ in the
  // predecessors with everything else: 4 combinations.
  if (p1.past_.GetThreadId() == p2.past_.GetThreadId()) {
    return true;
  }
  if (p1.past_.GetThreadId() == p2.future_.GetThreadId()) {
    return true;
  }
  if (p1.future_.GetThreadId() == p2.past_.GetThreadId()) {
    return true;
  }
  if (p1.future_.GetThreadId() == p2.future_.GetThreadId()) {
    return true;
  }

  return false;
}

PredecessorInfo::Predecessor::Predecessor() {
  valid_ = false;
}

PredecessorInfo::PredecessorEvent::PredecessorEvent() { 
  SetThreadId(-1);
  SetContext(-1);
  SetInstId(-1);
}

void PredecessorInfo::PredecessorEvent::SetThreadId(int tid) {
  std::get<0>(*this) = tid;
}

void PredecessorInfo::PredecessorEvent::SetContext(int context) {
  std::get<1>(*this) = context;
}

void PredecessorInfo::PredecessorEvent::SetInstId(int instId) {
  std::get<2>(*this) = instId;
}

int PredecessorInfo::PredecessorEvent::GetThreadId() const {
  return std::get<0>(*this);
}

int PredecessorInfo::PredecessorEvent::GetContext() const {
  return std::get<1>(*this);
}

int PredecessorInfo::PredecessorEvent::GetInstId() const {
  return std::get<2>(*this);
}

bool PredecessorInfo::Predecessor::operator<(const Predecessor &rhs) const {
  if (past_ != rhs.past_) {
    return past_ < rhs.past_;
  }
  else if (future_ != rhs.future_) {
    return future_ < rhs.future_;
  }

  // at this point, past_ == rhs.past_ && future_ == rhs.future_ meaning the
  // two Predecessors are equivalent.
  return false;
}

bool PredecessorInfo::Predecessor::operator==(const Predecessor &rhs) const {
  return past_ == rhs.past_ && future_ == rhs.future_;
}

bool PredecessorInfo::PredecessorTuple::operator<(const PredecessorTuple &rhs) const {
  // It does not make sense to call this function with two tuples that are
  // different sizes
  assert(vals_.size() == rhs.vals_.size());
  //if (vals_.size() != rhs.vals_.size())
    //return false;

  // Assumption: vals_.size() == rhs.vals_.size()
  for (size_t i = 0; i < vals_.size(); ++i) {
    if (vals_[i] == rhs.vals_[i])
      continue;
    return vals_[i] < rhs.vals_[i];
  }

  // Reaching this point means that each item in vals_ is equivalent to every
  // corresponding item in rhs.vals_. This means the two tuples are equivalent
  return false;
}

std::string PredecessorInfo::Predecessor::toString() {
  std::stringstream ss;
  ss << "past event" << past_event_.toString() << "\nfuture event" 
     << future_event_.toString();

  return ss.str();
}

std::string PredecessorInfo::PredecessorTuple::toString() {
  std::stringstream ss;
  for (size_t i = 0; i != vals_.size(); ++i) {
      ss << vals_[i].toString();
    // dont insert a newline after the last item is added
    if (i != vals_.size() - 1) {
      ss << '\n';
    }
  }

  return ss.str();
}

void PredecessorInfo::PrintCurrPredecessors() {
  for (size_t i = 0; i != curr_predecessors_.size(); ++i) {
    cerr << "Index: " << i;
    cerr << "\nFuture event: " << curr_predecessors_[i].future_event_.toString();
    cerr << "\nPast event: " << curr_predecessors_[i].past_event_.toString();
    // dont insert a newline after the last item is added
    if (i != curr_predecessors_.size() - 1) {
      cerr << '\n';
    }
  }
}

void PredecessorInfo::AfterEachRun(size_t prefixSize) {
#ifdef MK_DEBUG
  cerr << "[DEBUG] [MK] Cleaning up predecessor info\n";
  cerr << "\tprefix size =" << prefixSize << '\n';
  cerr << "\tcurr_predecessors_.size() = " << curr_predecessors_.size() << '\n';
#endif

  if (prefixSize == curr_predecessors_.size()) {
    assert(prefixSize == 0);
  }
  assert(prefixSize <= curr_predecessors_.size());

  // curr_predecessors should mirror the state stack. Delete all the
  // predecessors found after the prefix.
  //

  // if the size of the prefix is zero then delete everything. I beleive in
  // reality we dont need to do anything since a prefix of size zero indicates
  // that the search is over.
  if (prefixSize == 0) {
    curr_predecessors_.clear();
  }


  // markus: I beleive we need to use prefixSize - 1 because begin() is an
  // iterator to the first item, if we advance it prefixSize - 1 times then we
  // are at the item after the prefix.
  else {
    assert(prefixSize != 0);
    curr_predecessors_.erase(curr_predecessors_.begin() + (prefixSize - 1), 
        curr_predecessors_.end() - 1);
  }

#ifdef MK_DEBUG
  cerr << "[DEBUG] [MK] After cleanup, size == " << curr_predecessors_.size()
       << '\n';
#endif
}

size_t PredecessorInfo::GetStateIndex(const State *state) {
  // TODO: This seems fairly clumsy of a way to get simple information, but it
  // is easy. This information could be stored in the State itself (perhaps
  // giving the state an index on creation).

  size_t ret;
  ret = 0;
  for (;state != NULL; state = state->prev) {
    ++ret;
  }
  return ret;
}


size_t PredecessorInfo::curr_predecessors_size() {
  return curr_predecessors_.size();
}

#if 0
void PredecessorInfo::PrintPredTupleSet() {
  cerr << "pred_tuple_set_.size(): " << pred_tuple_set_.size() << '\n';
  for (std::set<PredecessorTuple>::iterator i = pred_tuple_set_.begin(); i != pred_tuple_set_.end(); ++i) {
    cerr << i->toString() << '\n';
  }
}
#endif
