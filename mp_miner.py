import socket
import urllib.request
import time
import datetime
import threading
import multiprocessing as mp
from ctypes import c_char

import nonceMiner

MINER_VERSION = "v1.2.1"
AUTO_RESTART_TIME = 30*60 # 30 minutes

serverip = 'https://raw.githubusercontent.com/revoxhere/duino-coin/gh-pages/serverip.txt' 
pool_location = urllib.request.urlopen(serverip).read().decode().splitlines()
pool_ip = pool_location[0]
pool_port = int(pool_location[1])

def process_mineDUCO(hashcount, accepted, rejected, job_request_bytes):
    soc = socket.socket()
    soc.settimeout(16.0)
    try:
        soc.connect((pool_ip, pool_port))
        soc.recv(3) # Receive version, but don't bother decoding
        start_time = time.time()
        compute_start_time = time.time()
        while time.time() < start_time + AUTO_RESTART_TIME:
            soc.send(job_request_bytes)
            job = soc.recv(1024) # Receive work byte-string from pool
            prefix_bytes = job[0:40] # Prefix is first 40 bytes
            target_bytes = job[41:81] # Target is 40 bytes after comma
            difficulty = int(job[82:]) # Difficulty is all bytes after second comma in utf-8

            result = nonceMiner.c_mine_DUCO_S1(prefix_bytes, target_bytes, difficulty)

            compute_end_time = time.time()
            local_hashrate = int(result/(compute_end_time-compute_start_time))
            compute_start_time = compute_end_time

            # Send result of hashing algorithm to pool along with metadata
            soc.send(f'{result},{local_hashrate},nonceMiner_{MINER_VERSION}'.encode('utf-8'))
            feedback_bytes = soc.recv(1024) # Receive feedback, don't bother decoding
            
            # Update counters according to feedback
            with hashcount.get_lock():
                hashcount.value += result
            if feedback_bytes == b'GOOD' or feedback_bytes == b'BLOCK':
                with accepted.get_lock():
                    accepted.value += 1
            else:
                with rejected.get_lock():
                    rejected.value += 1
        soc.close()
        return
    except (OSError, KeyboardInterrupt): # Handle socket errors and KeyboardInterrupt the same, with a restart
        soc.close()
        return
    except: # TODO: Gracefully report other encountered exceptions before restarting as well
        soc.close()
        return

server_is_online = True
# Attempts to find the server and updates server_is_online flag on success or fail
def thread_monitor_server():
    global server_is_online
    while True:
        soc = socket.socket()
        soc.settimeout(2.0)
        try:
            soc.connect((pool_ip, pool_port))
            soc.recv(3)
            soc.close()
            server_is_online = True
        except OSError:
            soc.close()
            server_is_online = False
        time.sleep(2)

# Checks for completed threads (either by exception or by exceeding auto-restart time) and restarts them
def thread_autorestarter(p_list, hashcount, accepted, rejected, job_request_bytes):
    while True:
        # Will only restart processes when the server is alive so it doesn't get overwhelmed
        if server_is_online:
            for i in range(len(p_list)):
                if not p_list[i].is_alive():
                    p_list[i].join()
                    p_list[i] = mp.Process(target=process_mineDUCO, args=(hashcount, accepted, rejected, job_request_bytes))
                    p_list[i].start()
                    time.sleep(4/N_PROCESSES)
        time.sleep(2)

if __name__ == '__main__':
    mp.freeze_support() # Required for conversion to exe

    print('Initializing nonceMiner mp_miner...')
    USERNAME = input('Enter username: ')
    N_PROCESSES = int(input('Enter # of processes: '))
    time.sleep(0.1)
    print('Starting processes...')

    hashcount = mp.Value('i', 0, lock=True)
    accepted = mp.Value('i', 0, lock=True)
    rejected = mp.Value('i', 0, lock=True)
    job_request_bytes = mp.Array(c_char, b'JOB,'+USERNAME.encode('utf-8'), lock=False)

    # Starts processes and prepares list for thread_autorestarter to handle
    p_list = []
    for i in range(N_PROCESSES):
        p = mp.Process(target=process_mineDUCO, args=(hashcount, accepted, rejected, job_request_bytes))
        p.start()
        p_list.append(p)
        time.sleep(4/N_PROCESSES)
    
    try:
        # Reset counters now that all processes are contributing
        with hashcount.get_lock():
            hashcount.value = 0
        with accepted.get_lock():
            accepted.value = 0
        with rejected.get_lock():
            rejected.value = 0
        past_time = time.time()

        # Starts threads that manage auxiliary functions
        threading.Thread(target=thread_monitor_server, daemon=True).start()
        threading.Thread(target=thread_autorestarter, args=(p_list, hashcount, accepted, rejected, job_request_bytes), daemon=True).start()

        # Main process runs the terminal output and calculates hashrate
        while True:
            time.sleep(2)

            # Gets counter values and resets them
            with hashcount.get_lock():
                hash_in_2s = hashcount.value
                hashcount.value = 0
            with accepted.get_lock():
                accepted_in_2s = accepted.value
                accepted.value = 0
            with rejected.get_lock():
                rejected_in_2s = rejected.value
                rejected.value = 0
            
            # Run-time of loop NOT guaranteed to be 2 seconds, so we keep time manually
            current_time = time.time()
            hashrate = hash_in_2s/1000000/(current_time-past_time)
            past_time = current_time
            
            print(datetime.datetime.now().strftime("%H:%M:%S"), end='  ')
            print(f'Hash rate: {hashrate:.2f} MH/s, Accepted: {accepted_in_2s}, Rejected: {rejected_in_2s}', end='')
            if server_is_online:
                print()
            else:
                print(', server ping timeout')
    except KeyboardInterrupt:
        time.sleep(0.1)
        print('Terminating processes...')
        time.sleep(4)
        for i in range(len(p_list)):
            p_list[i].terminate()