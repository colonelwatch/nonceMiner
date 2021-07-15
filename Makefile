ifeq ($(OS),Windows_NT)
	openssl_libs := -I"$(OPENSSL_ROOT_DIR)\include" -L"$(OPENSSL_ROOT_DIR)\lib" -llibcrypto
	libs := $(openssl_libs) -lws2_32
else
	openssl_libs := -lcrypto
	libs := $(openssl_libs) -pthread
endif

CFLAGS := -O3 -Wall
SRC_FILES := $(wildcard src/*.c)

nonceMiner: $(SRC_FILES) | bin
	gcc $^ $(CFLAGS) -o bin/$@ $(libs)

nonceMiner_minimal: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/nonceMiner_minimal.c | bin
	gcc $^ $(CFLAGS) -o bin/$@ $(libs)

nonceMiner_minimal_xxhash: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/nonceMiner_minimal_xxhash.c | bin
	gcc $^ $(CFLAGS) -o bin/$@ $(libs)

benchmark: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/benchmark.c | bin
	gcc $^ $(CFLAGS) -o bin/$@ $(openssl_libs) -lm

bin:
	mkdir bin