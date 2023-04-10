

# OpenCL compile and link flags.                                                                                                                                                            
AOCL_COMPILE_CONFIG := $(shell aocl compile-config )
AOCL_LINK_LIBS := $(shell aocl ldlibs )
AOCL_LINK_FLAGS := $(shell aocl ldflags )

# Linking with defences enabled                                                                                                                                                             
AOCL_LINK_FLAGS += -z noexecstack
AOCL_LINK_FLAGS += -Wl,-z,relro,-z,now
AOCL_LINK_FLAGS += -Wl,-Bsymbolic
AOCL_LINK_FLAGS += -pie -fopenmp
AOCL_LINK_CONFIG := $(AOCL_LINK_FLAGS) $(AOCL_LINK_LIBS)

SRC = main.cpp ../common/src/AOCLUtils/opencl.cpp ../common/src/AOCLUtils/options.cpp
CXXFLAGS += -O3 -fstack-protector -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -fPIE -fPIC -I../common/inc/

all:
	g++ $(CXXFLAGS) $(AOCL_COMPILE_CONFIG) $(SRC) $(AOCL_LINK_CONFIG) -o main -lrt -lpthread

sim:
	EMULATOR=1 CL_CONTEXT_EMULATOR_DEVICE_ALTERA=1 CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=1 ./main -emulator
