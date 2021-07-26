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

CFLAGS := -O3 -Wall
SRC_FILES := $(wildcard src/*.c) $(wildcard src/**/*.c)

ifeq (1, $(USING_OPENCL))
	FOLDERS := bin bin/OpenCL
	KERNEL_COPY_COMMAND := $(CPF) src$(PSEP)worker$(PSEP)kernel$(PSEP)*.cl bin$(PSEP)OpenCL
else
	FOLDERS := bin
	SRC_FILES := $(filter-out $(wildcard src/worker/**), $(SRC_FILES))
endif

nonceMiner: $(SRC_FILES) | bin bin/OpenCL
	gcc $^ -O3 -Wall -o bin/$@ $(libs) $(CFLAGS)
	$(KERNEL_COPY_COMMAND)

nonceMiner_minimal: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/nonceMiner_minimal.c | bin
	gcc $^ -O3 -Wall -o bin/$@ $(libs) $(CFLAGS)

nonceMiner_minimal_xxhash: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/nonceMiner_minimal_xxhash.c | bin
	gcc $^ -O3 -Wall -o bin/$@ $(libs) $(CFLAGS)

benchmark: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/benchmark.c | bin bin/OpenCL
	gcc $^ -O3 -Wall -o bin/$@ $(openssl_libs) $(opencl_libs) -lm $(CFLAGS)
	$(KERNEL_COPY_COMMAND)

bin/OpenCL:
	mkdir bin$(PSEP)OpenCL

bin:
	mkdir bin