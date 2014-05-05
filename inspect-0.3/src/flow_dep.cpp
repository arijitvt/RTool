#include "flow_dep.hpp"
#include "flow_dep.pb.h"

// Including "flow_dep.pb.h" in flow_dep.hpp introduces some complicated
// dependencies since flow_dep.hpp is included in inspect_clap.hpp. As a
// result, these helper functions are not exposed in the class definition of
// ExecInfo and LoadStoreInfo
static flow_dep::ExecInfo_Proto ei_to_proto(const ExecInfo ei);
static flow_dep::LoadStoreInfo_Proto lsi_to_proto(const LoadStoreInfo lsi);

static ExecInfo ei_from_proto(const flow_dep::ExecInfo_Proto ei);
static LoadStoreInfo lsi_from_proto(const flow_dep::LoadStoreInfo_Proto proto);


ExecInfo::ExecInfo() {
  // do nothing
}

ExecInfo::ExecInfo(const std::string data) {
  flow_dep::ExecInfo_Proto proto;
  proto.ParseFromString(data);

  assert(proto.IsInitialized());

  this->swap(ei_from_proto(proto));
}

std::string ExecInfo::serialize_to_string() const {
  // Create a protobuf object for the entire ExecInfo
  flow_dep::ExecInfo_Proto eip = ei_to_proto(*this);

  assert(eip.info_size() == this->size());

  std::string ret;
  if (!eip.SerializeToString(&ret)) {
    std::cerr << "Error: Failed to write ExecInfo";
    exit(EXIT_FAILURE);
  }

#ifdef MK_DEBUG
  std::cerr << "[MK DEBUG] Serialized ExecInfo to string of size " 
            << ret.size() << '\n';
#endif
  return ret;
}

static ExecInfo ei_from_proto(const flow_dep::ExecInfo_Proto proto) {
  ExecInfo ei;

  for (auto i = 0; i != proto.info_size(); ++i) {
    ei.push_back(lsi_from_proto(proto.info(i)));
  }

  assert(ei.size() == proto.info_size());
  return ei;
}
static LoadStoreInfo lsi_from_proto(const flow_dep::LoadStoreInfo_Proto proto) {
  // get tid
  // get val
  // get is_load
  LoadStoreInfo lsi(proto.val(), proto.tid(), proto.is_load());

  assert(proto.tid() == lsi.get_tid());
  assert(proto.val() == lsi.get_val());
  assert(proto.is_load() == lsi.is_load());

  return lsi;

}

static flow_dep::ExecInfo_Proto ei_to_proto(ExecInfo ei) {
  flow_dep::ExecInfo_Proto proto;

  // Fill out a protobuf of each LoadStoreInfo in infos
  for (ExecInfo::size_type_t i = 0, e = ei.size(); i != e; ++i) {
    const LoadStoreInfo *lsi;
    lsi = ei.at(i);
    assert(lsi != NULL);

    flow_dep::LoadStoreInfo_Proto *lsi_proto = proto.add_info();
    *lsi_proto = lsi_to_proto(*lsi);
  }

#ifdef MK_DEBUG
  std::cerr << "[MK_DEBUG] Writing ExecInfo to proto\n";
  std::cerr << "\tinfos.size() == " << ei.size() << '\n';
  std::cerr << "\tproto.infos_size() == " << proto.info_size() << '\n';
#endif

  assert(proto.info_size() == ei.size());
  return proto;
}

static flow_dep::LoadStoreInfo_Proto lsi_to_proto(const LoadStoreInfo lsi) {
  flow_dep::LoadStoreInfo_Proto proto;

  // add tid
  proto.set_tid(lsi.get_tid());
  // add val
  proto.set_val(lsi.get_val());
  // add is_load
  proto.set_is_load(lsi.is_load());

  assert(proto.IsInitialized());
  assert(proto.tid() == lsi.get_tid());
  assert(proto.val() == lsi.get_val());
  assert(proto.is_load() == lsi.is_load());
  return proto;
}

const LoadStoreInfo *ExecInfo::at(ExecInfo::size_type_t index) const {
  if (index >= infos.size())
    return NULL;

  return &infos[index];
}

std::string ExecInfo::to_string() {
  std::stringstream ss;

  for (auto i = infos.begin(), e = infos.end(); i != e; ++i) {
    LoadStoreInfo &cur = *i;
    if (cur.is_load()) {
      ss << "Load Inst: ";
    }
    else {
      ss << "Store Inst: ";
    }

    ss << "TID: " << cur.get_tid() << " value: " << cur.get_val() << '\n';
  }

  return ss.str();
}

void ExecInfo::swap(ExecInfo other) {
  // Exchange contents of vectors
  this->infos.swap(other.infos);
}
