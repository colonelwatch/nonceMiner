ifeq (NO_OPENCL, $(findstring NO_OPENCL, $(CFLAGS)))
	USING_OPENCL := 0
else
	USING_OPENCL := 1
endif

ifeq ($(OS),Windows_NT)
    opencl_libs := -lOpenCL -I".\include" -Wl,-L"C:\Windows\System32" -D CL_TARGET_OPENCL_VERSION=120 
	openssl_libs := -I"$(OPENSSL_ROOT_DIR)\include" -L"$(OPENSSL_ROOT_DIR)\lib" -llibcrypto
	ifeq (1, $(USING_OPENCL))
		libs := $(openssl_libs) $(opencl_libs) -lws2_32
	else
		libs := $(openssl_libs) -lws2_32
	endif
	CPF := copy /y
	PSEP := \\
else
	opencl_libs := -lOpenCL -D CL_TARGET_OPENCL_VERSION=120
	openssl_libs := -lcrypto
	ifeq (1, $(USING_OPENCL))
		libs := $(openssl_libs) $(opencl_libs) -pthread
	else
		libs := $(openssl_libs) -pthread
	endif
	CPF := cp -f
	PSEP := /
endif

ifeq (1, $(USING_OPENCL))
	FOLDERS := bin bin/OpenCL
	KERNEL_COPY_COMMAND := $(CPF) src$(PSEP)worker$(PSEP)kernel$(PSEP)*.cl bin$(PSEP)OpenCL
else
	FOLDERS := bin
endif

CFLAGS := -O3 -Wall
SRC_FILES := $(wildcard src/*.c) $(wildcard src/**/*.c)

nonceMiner: $(SRC_FILES) | bin bin/OpenCL
	gcc $^ $(CFLAGS) -o bin/$@ $(libs)
	$(KERNEL_COPY_COMMAND)

nonceMiner_minimal: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/nonceMiner_minimal.c | bin
	gcc $^ $(CFLAGS) -o bin/$@ $(libs)

nonceMiner_minimal_xxhash: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/nonceMiner_minimal_xxhash.c | bin
	gcc $^ $(CFLAGS) -o bin/$@ $(libs)

benchmark: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/benchmark.c | bin bin/OpenCL
	gcc $^ $(CFLAGS) -o bin/$@ $(openssl_libs) $(opencl_libs) -lm
	$(KERNEL_COPY_COMMAND)

bin/OpenCL:
	mkdir bin$(PSEP)OpenCL

bin:
	mkdir bin