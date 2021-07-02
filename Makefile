ifeq ($(OS),Windows_NT)
	libs := -I"$(OPENSSL_ROOT_DIR)\include" -L"$(OPENSSL_ROOT_DIR)\lib" -llibcrypto -lws2_32
else
	libs := -lcrypto -pthread
endif

SRC_FILES := $(wildcard src/*.c)

nonceMiner: $(filter-out src/nonceMiner_minimal.c, $(SRC_FILES)) | bin
	gcc $^ -O3 -Wall -o bin/nonceMiner $(libs)

nonceMiner_minimal: $(filter-out src/nonceMiner.c, $(SRC_FILES)) | bin
	gcc $^ -O3 -Wall -o bin/nonceMiner_minimal $(libs)

nonceMiner_minimal_debug: $(filter-out src/nonceMiner.c, $(SRC_FILES)) | bin
	gcc $^ -g -Wall -o bin/nonceMiner_minimal_debug $(libs)

bin:
	mkdir bin