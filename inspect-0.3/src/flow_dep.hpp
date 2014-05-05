// Author: Markus Kusano
//
// Data structures used to keep track of information required to perform
// {control-,}flow dependence checks. These data structures are held in memory
// in the program under test and are sent to the scheduler after the current
// exeuction has completed.
//
// This should probably only be included in a file if the preprocessor
// definition FLOW_DEP is defined.
#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <cstdint>
#include "inspect_util.hh"

class ExecInfo;

// Serialize and deserialize ExecInfo over a Socket
Socket &operator<<(Socket &sock, ExecInfo &info);
Socket &operator>>(Socket &sock, ExecInfo &info);

// Information about a load/store
class LoadStoreInfo {
  public:
    typedef std::vector<LoadStoreInfo> vector_t;
    LoadStoreInfo(int64_t val, int32_t tid, bool isLoad) 
      : thread_id(tid), value(val), load(isLoad) { }

    // Build a LoadStoreInfo from a string. This string is assumed to have been
    // created using serialize_to_string() (see below).
    LoadStoreInfo(const std::string data);

    int32_t get_tid() const { return thread_id; }
    int64_t get_val() const { return value; }
    bool is_load() const { return load; }

    // TODO: Probably need to add associated control statements with this
    // load/store as well

  private:
    // All types are explicitly sized in order to portably serialize the data.
    // Thread performing load/store
    int32_t thread_id;
    // Value read/written from memory
    // Note: The fact that this is an int64 means that we cannot handle
    // integers larger than 64 bits in the program under test.
    int64_t value;
    // true if this is information about a load inst. Otherwise false.
    bool load;
    LoadStoreInfo();

};

// Hold information about a current execution. This is essentially a
// time-ordered vector (item 0 ocurred before item 1...) of information about
// loads and stores that has happened in the current execution.
class ExecInfo {
  public:
    typedef LoadStoreInfo::vector_t::size_type size_type_t;
    ExecInfo();
    // Build exec info from string. This is assumed to be a string created from
    // serialize_to_string() (see below)
    ExecInfo(const std::string data);
    void push_back(LoadStoreInfo lsi) { infos.push_back(lsi); }

    // Swap two ExecInfos
    void swap(ExecInfo other);

    // returns a human readable string of the object. Contains a newline at the
    // end
    std::string to_string();

    // Serialize to a binary string using protobuf. You should not convert this
    // to something like UTF-8 and attempt to read it; it is binary data using
    // std::string as a convienient container.
    std::string serialize_to_string() const;

    friend Socket &operator<<(Socket &sock, ExecInfo &info);

    size_type_t size() const { return infos.size(); } 

    // Return a pointer to the LoadStoreInfo at the given index. Returns NULL
    // if index is out-of-bounds
    const LoadStoreInfo *at(size_type_t index) const;

  private:
    LoadStoreInfo::vector_t infos;
};
