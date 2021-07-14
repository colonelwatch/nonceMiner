ifeq ($(OS),Windows_NT)
	libs := -I"$(OPENSSL_ROOT_DIR)\include" -L"$(OPENSSL_ROOT_DIR)\lib" -llibcrypto -lws2_32
else
	libs := -lcrypto -pthread
endif

CFLAGS := -O3 -Wall
SRC_FILES := $(wildcard src/*.c)

nonceMiner: $(SRC_FILES) | bin
	gcc $^ $(CFLAGS) -o bin/$@ $(libs)

nonceMiner_minimal: $(filter-out src/nonceMiner.c, $(SRC_FILES)) test/nonceMiner_minimal.c | bin
	gcc $^ $(CFLAGS) -o bin/$@ $(libs)

bin:
	mkdir bin