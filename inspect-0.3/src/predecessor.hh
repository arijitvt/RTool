/**
 * Function definitions for HaPSet and its extension.
 *
 * Author: Markus Kusano
 */
#pragma once

#include <set>
#include <vector>
#include <string>
#include <climits>

#include "inspect_event.hh"
#include "state.hh"

// This class encapsulates all of the data structures and algorithms for
// keeping track of predecessor information
class PredecessorInfo {
  public:
    // Must be constructed with a depth
    PredecessorInfo(unsigned depth);

    /** 
     * Update the predecessor information assuming that the passed state has
     * been reached. The sel_event of the passed state should be set and valid.
     * This will iterate up the chain of states to find the immediate
     * predecessor (if it exists) of the selected event.
     */
    void PSetUpdate(State *state);

    /**
     * Debugging function:
     * Dumps curr_predecessors_ to stderr
     */
     void PrintCurrPredecessors();

     /**
      * Debugging function
      * Dumps pred_tuple_set_ to stderr
      *
      * NOTE: There is a strange issue where this function thinks it should be
      * const.
      */
     //void PrintPredTupleSet();

     /**
      * Returns true if the pair of events has already been covered by a
      * previously executed predecessor tuple (ie returns true if by executing
      * first before second they will create a predecessor tuple that has not
      * been seen before).
      *
      * secondState is the state in the current execution where second is
      * enabled.
      *
      * The DPOR algorithm is attempting to re-order first before second. In
      * the context of DPOR, second is the conflicting event found in the
      * current exeuction (ie second conflicts with first). secondState is the
      * state in which second is the selected action. This is the last
      * conflicting node found in the execution.
      */
    bool PSetVisited(InspectEvent &first, InspectEvent &second, 
         const State *secondState);

    /**
     * Cleanup function to be called at the end of each run. This sets up the
     * internal data structures t
     *
     * You need to pass the size of the prefix to be executed on the next run
     * (see StateStack::PrefixDepth()).
     */
    void AfterEachRun(size_t prefixSize);

    /**
     * Returns the size of curr_predecessors_
     */
    size_t curr_predecessors_size();

    /**
     * Number of runs pruned 
     * This is modified by the scheduler after calling PSetVisited()
     */
    unsigned pruned_;

    /** 
     * The minimal amount of information (ie a subset of InspectEvent) in order
     * to capture a predecessor event.
     *  1. LLVM inst ID
     *  2. thread ID
     *  3. calling context
     *
     * Currently, there can be collisions with the LLVM inst ID since it is
     * done
     * on the file level. This would occur if the same thread on the same line
     * number in a different file has an instrumented function call.
     *
     * This class derrives from std::tuple. For reference, the items are:
     *  0: Thread ID
     *  1: Context
     *  2: Inst ID
     *
     * You should not use std::get<> to obtain these values; there are member
     * functions exposed so you wont have to remember which item is which.
     *
     * TODO: The calling context is not implemented and is always -1
     */
    class PredecessorEvent : public std::tuple<int, int, int> {
      public:
        PredecessorEvent();
        void SetThreadId(int tid);
        void SetContext(int context);
        void SetInstId(int inst_id);

        int GetThreadId() const;
        int GetContext() const;
        int GetInstId() const;
    };

    // class representing an immediate predecessor pair. Implicitly, this is a
    // pair of PredecessorEvents which consist of:
    // 1. LLVM inst ID
    // 2. thread ID
    // 3. calling context
    class Predecessor {
      public:
        Predecessor();

        /**
         * Setup the internal data members as if past is the immediate
         * predecessor of future
         */
        void Init(InspectEvent &past, InspectEvent &future);

        // past is the immediate predecessor of future. These are currently
        // only stored to determine if two predecessors are dependent by using
        // InspectEvent::Dependent
        InspectEvent past_event_;
        InspectEvent future_event_;

        // The pair of predecessor events
        PredecessorEvent past_;
        PredecessorEvent future_;

        /** 
         * Whether or not the predecessor is valid. Used when returning
         * predecessors from functions where potentially one could not be found.
         */
        bool valid_;

        bool operator<(const Predecessor &rhs) const;
        bool operator==(const Predecessor &rhs) const;

        /**
         * Create a human readable string with the past and future events with
         * no newline at the end.
         */
        std::string toString();
    };

    // A tuple of Predecessors. The size of vals should never exceed the
    // max_pset setting parameter.
    class PredecessorTuple {
      public:
        // The actual values in the tuple
        std::vector<Predecessor> vals_;
        bool valid() { return vals_.size() != 0; }

        size_t size() { return vals_.size(); }

        bool operator<(const PredecessorTuple &rhs) const;

        /**
         * Create a human readable string containing all the predecessors in
         * the tuple
         */
        std::string toString();
    };


  private:
    // Helper functions

    /**
     * Returns a properly filled out Predecessor of the passed State's
     * sel_event if an immediate predecessor is found. Returns a predecessor
     * with its valid flag set to false if no immediate predecessor is found.
     */
    Predecessor GetPredecessor(State *state);

    /**
     * Returns the expanded predecessor tuple based on the passed predecessor.
     * The tuple will be the size of depth_ if it can be expanded to be that
     * large. If it can be expanded to depth_, then the valid flag of the tuple
     * will be true, otherwise it will be false.
     *
     * startIndex will start looking back at predecessors at the given point in
     * curr_predecessors_ if desired. This will allow to search back for
     * predecessors from a search node at a given point. If startIndex is not
     * passed then the search will start at the current point in time (ie at
     * curr_predecessors_.size())
     */
    PredecessorTuple GetPredecessorTuple(Predecessor &pred,
        size_t startIndex = UINT_MAX);

    /**
     * Returns true if `past` trumps `future`. `past` is an event that is
     * assumed to have occured before `future`. This returns true if `past` is
     * a write by the same thread and to the same object as `past`.
     */ 
    bool PredecessorTrump(InspectEvent &future, InspectEvent &past);

    /**
     * Returns true if the two passed predecessors are dependent.
     *
     * Predecessors are dependent on each other if they are from the same class
     * of operations (eg memory read/write, mutex operation) and atleast one of
     * the thread IDs match in the pair. 
     *
     * For example, if two predecessors both are mutex operations and one comes
     * from threads (1,2) and the other (3,2) then the predecessors would be
     * dependent.
     */
    bool PredecessorDependent(const Predecessor &p1, const Predecessor &p2) const;

    /**
     * Returns the index of the state.
     *
     * This is the index of the state in the current string of states that have
     * been executed. 
     */
    size_t GetStateIndex(const State *state);

    /**
     * A vector of the current predecessors found during the execution.
     */
    std::vector<Predecessor> curr_predecessors_;

    /**
     * The set of predecessor tuples that have been found in all executions
     */
    std::set<PredecessorTuple> pred_tuple_set_;

    /**
     * The value of the command line argument max-pset
     */
    unsigned depth_;
};
