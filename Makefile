include_args := -I "./include"
# library_args :=

ifeq ($(findstring -D NO_OPENCL, $(CFLAGS)), -D NO_OPENCL)
	using_opencl := 0
else
	using_opencl := 1
endif

ifeq ($(OS), Windows_NT)
	ifeq ($(using_opencl), 1)
		library_args += -lOpenCL -Wl,-L "C:\Windows\System32" -D CL_TARGET_OPENCL_VERSION=120
	endif
	include_args += -I "$(OPENSSL_ROOT_DIR)\include"
	library_args += -llibcrypto -L "$(OPENSSL_ROOT_DIR)\lib" -lws2_32
    CPF := copy /y
	PSEP := \\
else
	ifeq ($(using_opencl), 1)
		library_args += -lOpenCL -D CL_TARGET_OPENCL_VERSION=120
	endif
	library_args += -lcrypto -pthread
	CPF := cp -f
	PSEP := /
endif

SRC_FILES := $(wildcard src/*.c) $(wildcard src/**/*.c)
CFLAGS := -O3 -Wall

ifeq ($(using_opencl), 1)
	folders := bin bin/OpenCL
	# If NO_OPENCL is present, kernel_copy_command is an empty command
	kernel_copy_command := $(CPF) src$(PSEP)worker$(PSEP)kernel$(PSEP)*.cl bin$(PSEP)OpenCL
else
	folders := bin
	SRC_FILES := $(filter-out $(wildcard src/worker/**), $(SRC_FILES))
endif

nonceMiner: $(SRC_FILES) | $(folders)
	gcc $^ $(include_args) $(library_args) -o bin/$@ $(CFLAGS)
	$(kernel_copy_command)

nonceMiner_minimal: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/nonceMiner_minimal.c | bin
	gcc $^ $(include_args) $(library_args) -o bin/$@ $(CFLAGS)

nonceMiner_minimal_xxhash: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/nonceMiner_minimal_xxhash.c | bin
	gcc $^ $(include_args) $(library_args) -o bin/$@ $(CFLAGS)

benchmark: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/benchmark.c | $(folders)
	gcc $^ $(include_args) $(library_args) -lm -o bin/$@ $(CFLAGS)
	$(kernel_copy_command)

bin/OpenCL:
	mkdir bin$(PSEP)OpenCL

bin:
	mkdir bin