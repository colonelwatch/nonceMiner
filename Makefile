ifeq ($(OS),Windows_NT)
	libs := -I"$(OPENSSL_ROOT_DIR)\include" -L"$(OPENSSL_ROOT_DIR)\lib" -llibcrypto -lws2_32
else
	libs := -lcrypto -pthread
endif

nonceMiner: src/nonceMiner.c src/mine_DUCO_S1.h src/mine_DUCO_S1.c | bin
	gcc src/nonceMiner.c src/mine_DUCO_S1.c -O3 -Wall -o bin/nonceMiner $(libs)

nonceMiner_minimal: src/nonceMiner_minimal.c src/mine_DUCO_S1.h src/mine_DUCO_S1.c | bin
	gcc src/nonceMiner_minimal.c src/mine_DUCO_S1.c -O3 -Wall -o bin/nonceMiner_minimal $(libs)

nonceMiner_minimal_debug: src/nonceMiner_minimal.c src/mine_DUCO_S1.h src/mine_DUCO_S1.c | bin
	gcc src/nonceMiner_minimal.c src/mine_DUCO_S1.c -g -Wall -o bin/nonceMiner_minimal_debug $(libs)

bin:
	mkdir bin