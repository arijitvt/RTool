# Environment variables to control compile time options:
# FLOW_DEP=1 : compile in flow dependence code

CC  = gcc
CXX = g++

YICES_PATH = $(shell cd ../../yices-1.0.39; pwd)
SMTDP_PATH = $(shell cd ../../smt_dp ; pwd)

AC_FLAGS = -DRSS_EXTENSION=1 -I$(YICES_PATH)/include -I$(SMTDP_PATH) -DDAIKON

LD_FLAGS_YICES1 = -Wl,-rpath,$(YICES_PATH)/lib
LD_FLAGS_YICES2 = -Wl,-rpath-link,$(YICES_PATH)/lib
LD_FLAGS_YICES3 = -Wl,-L,$(YICES_PATH)/lib
LD_FLAGS_YICES = $(LD_FLAGS_YICES1) $(LD_FLAGS_YICES2) $(LD_FLAGS_YICES3) -lyices

LD_FLAGS_SMTDP1 = -Wl,-rpath,$(SMTDP_PATH)/lib
LD_FLAGS_SMTDP2 = -Wl,-rpath-link,$(SMTDP_PATH)/lib
LD_FLAGS_SMTDP3 = -Wl,-L,$(SMTDP_PATH)/lib
LD_FLAGS_SMTDP = $(LD_FLAGS_SMTDP1) $(LD_FLAGS_SMTDP2) $(LD_FLAGS_SMTDP3) -lsmtdp

LD_FLAGS = $(LD_FLAGS_SMTDP) $(LD_FLAGS_YICES)

override CCFLAGS += -std=c++0x -Wall  -g  -pthread -D__USE_XOPEN2K -fpermissive -Wunused-variable -Wno-deprecated $(AC_FLAGS)


LIB_SRC_NAME  =  inspect_util   inspect_pthread   \
                 inspect_event  object_table      \
                 inspect_clap   lin_checker       \
		 exit_tracker

VRF_SRC_NAME  =  event_buffer                     \
                 event_relation_graph             \
                 scheduler       state            \
                 lockset         state_stack      \
                 scheduler_object_table           \
                 inspect_main                     \
                 transition_set                   \
                 thread_info                      \
                 clock_vector                     \
                 program_state                    \
                 scheduler_setting                \
                 next_state                       \
                 inspect_ucg_graph                \
                 yices_path_computer_singleton    \
                 inspect_trace                    \
		 predecessor			  \

# Add in flow dependence files
ifeq ($(FLOW_DEP),1)
# check for protocol buffer variable
ifndef PROTOBUF_INSTALL
  $(error Please define PROTOBUF_INSTALL environment variable)
endif
# add preprocessor define to compile in flow_dep code
override CCFLAGS += -DFLOW_DEP
# Object file for flow dependence
FLOW_DEP_OBJ = flow_dep.o
FLOW_DEP_DLL = flow_dep.os
FLOW_DEP_PROTO_OBJ = flow_dep.pb.o
FLOW_DEP_PROTO_DLL = flow_dep.pb.os
$(info ******* FLOW DEP ENABLED, FLOW_DEP_OBJ = $(FLOW_DEP_OBJ))
# add protocol buffer descriptions to be compiled
PROTOC = $(PROTOBUF_INSTALL)/bin/protoc

# add protobuf library paths
LD_FLAGS_PROTO = -L$(PROTOBUF_INSTALL)/lib -Wl,-rpath,$(PROTOBUF_INSTALL)/lib
LD_FLAGS_PROTO += -lprotobuf
LD_FLAGS += $(LD_FLAGS_PROTO)
else 
# Make FLOW_DEP_OBJ equal to nothing so nothing is compiled in the regular inspect
# build. The dependencies to flow_dep.o only build if FLOW_DEP is one and with
# FLOW_DEP_OBJ as nothing, flow_dep.o is not needed for the inspect binary or
# library file.
FLOW_DEP_OBJ = 
FLOW_DEP_DLL = 
FLOW_DEP_PROTO_OBJ = 
FLOW_DEP_PROTO_DLL = 
endif

ALL_SRC_NAME  = $(LIB_SRC_NAME) $(VRF_SRC_NAME)


INSPECTOR = inspect

LIBDIR =  ../lib
INSPECTOR_LIB = $(LIBDIR)/lib$(INSPECTOR).a
INSPECTOR_DLL = $(LIBDIR)/lib$(INSPECTOR).so

LIB_OBJS = $(foreach file, $(LIB_SRC_NAME), $(file).o)
DLL_OBJS = $(foreach file, $(LIB_SRC_NAME), $(file).os)
VRF_OBJS = $(foreach file, $(VRF_SRC_NAME), $(file).o)

ALL_SRC = $(foreach src_name, $(ALL_SRC_NAME), $(src_name).cc )


all:   $(INSPECTOR) $(INSPECTOR_LIB) $(INSPECTOR_DLL)


$(INSPECTOR): $(LIB_OBJS) $(VRF_OBJS) $(FLOW_DEP_OBJ) $(FLOW_DEP_PROTO_OBJ)
	$(CXX) $(CCFLAGS) -o $(INSPECTOR) $(VRF_OBJS) $(FLOW_DEP_OBJ) $(FLOW_DEP_PROTO_OBJ) $(LIB_OBJS) $(LD_FLAGS) -lpthread -lrt 

$(INSPECTOR_LIB): $(LIB_OBJS) $(FLOW_DEP_OBJ)
	ar rcs $@ $(LIB_OBJS) $(FLOW_DEP_OBJ) $(FLOW_DEP_PROTO_OBJ)
	ranlib $@

$(INSPECTOR_DLL): $(DLL_OBJS) $(FLOW_DEP_DLL) $(FLOW_DEP_PROTO_DLL)
	$(CC) -shared -o $@  $(DLL_OBJS) $(FLOW_DEP_DLL) $(FLOW_DEP_PROTO_DLL)

# Only build flowdep.o and proto files if FLOW_DEP is enabled
$(FLOW_DEP_OBJ): flow_dep.cpp flow_dep.proto flow_dep.pb.h flow_dep.pb.cc
# only build flow_dep.o if FLOW_DEP is enabled
ifeq ($(FLOW_DEP),1)
	$(info ****** Building $(FLOW_DEP_OBJ))
	$(CXX) $(CCFLAGS) -c $< -o $@
	# TODO: Do we have to build .d files?
else
	$(info ****** NOT Building $(FLOW_DEP_OBJ))
	# do nothing
endif

$(FLOW_DEP_DLL): flow_dep.cpp flow_dep.proto flow_dep.pb.h flow_dep.pb.cc
ifeq ($(FLOW_DEP),1)
	$(info ****** Building $(FLOW_DEP_OBJ))
	$(CXX) $(CCFLAGS) -fPIC -c $< -o $@
	# TODO: Do we have to build .d files?
else
	$(info ****** NOT Building $(FLOW_DEP_OBJ))
	# do nothing
endif

$(FLOW_DEP_PROTO_OBJ): flow_dep.pb.cc flow_dep.proto flow_dep.pb.h
# only build flow_dep.o if FLOW_DEP is enabled
ifeq ($(FLOW_DEP),1)
	$(info ****** Building $(FLOW_DEP_OBJ))
	$(CXX) $(CCFLAGS) -c $< -o $@ 
	# TODO: Do we have to build .d files?
else
	$(info ****** NOT Building $(FLOW_DEP_OBJ))
	# do nothing
endif

$(FLOW_DEP_PROTO_DLL): flow_dep.pb.cc flow_dep.proto flow_dep.pb.h
ifeq ($(FLOW_DEP),1)
	$(info ****** Building $(FLOW_DEP_OBJ))
	$(CXX) $(CCFLAGS) -fPIC -c $< -o $@ 
	# TODO: Do we have to build .d files?
else
	$(info ****** NOT Building $(FLOW_DEP_OBJ))
	# do nothing
endif






flow_dep.pb.h: flow_dep.proto
# only build protofiles if FLOW_DEP is enabled
ifeq ($(FLOW_DEP),1)
	$(PROTOC) -I=. --cpp_out=. $^ && exit
	$(info ****** BUILDING flow_dep.pb.h)
else
 	#$(error not building flow_dep.pb.h)
  	# do nothing
endif

flow_dep.pb.cc: flow_dep.proto
# only build protofiles if FLOW_DEP is enabled
ifeq ($(FLOW_DEP),1)
  	#$(info ******* BUILDING flow_dep.pb.cc)
	$(PROTOC) -I=. --cpp_out=. $^
else
 	#$(error not building flow_dep.pb.cc)
  	# do nothing
endif

clean:  
	@echo "Cleaning object files ...."
	@rm -f *.o *.os *.d
	@rm -f *.pb.h *.pb.cc
	@rm -f $(INSPECTOR_LIB) 
	@rm -f $(INSPECTOR_DLL) 
	@rm -f $(INSPECTOR)


-include $(OBJS:.o=.d)

%.o : %.cc
	$(CXX) $(CCFLAGS) -c  $^  -o $@
	$(CXX) $(CCFLAGS) -MM $^ > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//'  < $*.d.tmp | fmt -1 | \
	 sed -e 's/^ *//' -e 's/$$/:/'  >> $*.d
	@rm -f $*.d.tmp

%.os : %.cc
	$(CXX) $(CCFLAGS) -fPIC -c $^ -o $@
	$(CXX) $(CCFLAGS) -MM $^ > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//'  < $*.d.tmp | fmt -1 | \
	 sed -e 's/^ *//' -e 's/$$/:/'  >> $*.d
	@rm -f $*.d.tmp

