ifeq ($(OS),Windows_NT)
    opencl_libs := -lOpenCL -I".\include" -Wl,-L"C:\Windows\System32" -D CL_TARGET_OPENCL_VERSION=120 
	openssl_libs := -I"$(OPENSSL_ROOT_DIR)\include" -L"$(OPENSSL_ROOT_DIR)\lib" -llibcrypto
	libs := $(openssl_libs) $(opencl_libs) -lws2_32
	CPF := copy /y
	PSEP := \\
else
	opencl_libs := -lOpenCL -D CL_TARGET_OPENCL_VERSION=120
	openssl_libs := -lcrypto
	libs := $(openssl_libs) $(opencl_libs) -pthread
	CPF := cp -f
	PSEP := /
endif

CFLAGS := -O3 -Wall
SRC_FILES := $(wildcard src/*.c) $(wildcard src/**/*.c)

nonceMiner: $(SRC_FILES) | bin bin/OpenCL
	gcc $^ $(CFLAGS) -o bin/$@ $(libs)
	$(CPF) src$(PSEP)worker$(PSEP)kernel$(PSEP)*.cl bin$(PSEP)OpenCL

nonceMiner_minimal: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/nonceMiner_minimal.c | bin
	gcc $^ $(CFLAGS) -o bin/$@ $(libs)

nonceMiner_minimal_xxhash: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/nonceMiner_minimal_xxhash.c | bin
	gcc $^ $(CFLAGS) -o bin/$@ $(libs)

benchmark: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/benchmark.c | bin bin/OpenCL
	gcc $^ $(CFLAGS) -o bin/$@ $(openssl_libs) $(opencl_libs) -lm
	$(CPF) src$(PSEP)worker$(PSEP)kernel$(PSEP)*.cl bin$(PSEP)OpenCL

bin/OpenCL:
	mkdir bin$(PSEP)OpenCL

bin:
	mkdir bin