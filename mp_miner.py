import socket, urllib.request, time, datetime   # Python 3 included
import multiprocessing as mp                    # Also Python 3 included
from ctypes import c_char                       # Also Python 3 included
import nonceMiner

MINER_VERSION = "v1.1"
AUTO_RESTART_TIME = 30*3600 # 30 minutes

serverip = 'https://raw.githubusercontent.com/revoxhere/duino-coin/gh-pages/serverip.txt' 
pool_location = urllib.request.urlopen(serverip).read().decode().splitlines()
pool_ip = pool_location[0]
pool_port = int(pool_location[1])

def process_mineDUCO(hashcount, job_request_bytes):
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
            soc.send(('%i,%i,nonceMiner_%s'%(result, local_hashrate, MINER_VERSION)).encode('utf-8'))
            soc.recv(1024) # Receive feedback, don't bother decoding
            
            with hashcount.get_lock():
                hashcount.value += result
        soc.close()
        return
    except:
        soc.close()
        return

def ping_server():
    soc = socket.socket()
    soc.settimeout(2.0)
    try:
        soc.connect((pool_ip, pool_port))
        soc.recv(3)
        soc.close()
        return True
    except:
        soc.close()
        return False

if __name__ == '__main__':
    mp.freeze_support() # Required for conversion to exe

    print('Initializing nonceMiner mp_miner...')
    USERNAME = input('Enter username: ')
    N_PROCESSES = int(input('Enter # of processes: '))
    time.sleep(0.1)
    print('Starting processes...')

    hashcount = mp.Value('i', 0, lock=True)
    job_request_bytes = mp.Array(c_char, b'JOB,'+USERNAME.encode('utf-8'), lock=False)
    p_list = []
    for i in range(N_PROCESSES):
        p = mp.Process(target=process_mineDUCO, args=(hashcount, job_request_bytes))
        p.start()
        p_list.append(p)
        time.sleep(4/N_PROCESSES)
    try:
        with hashcount.get_lock(): hashcount.value = 0
        past_time = time.time()
        while True:
            time.sleep(2)

            with hashcount.get_lock():
                hash_in_2s = hashcount.value
                hashcount.value = 0
            current_time = time.time()
            hashrate = hash_in_2s/1000000/(current_time-past_time)
            past_time = current_time
            
            print(datetime.datetime.now().strftime("%H:%M:%S"), end='  ')
            if(ping_server()):
                for i in range(len(p_list)):
                    if(not p_list[i].is_alive()):
                        p_list[i].join()
                        p_list[i] = mp.Process(target=process_mineDUCO, args=(hashcount, job_request_bytes))
                        p_list[i].start()
                        time.sleep(4/N_PROCESSES)
                print('Hash rate: %.2f MH/s' % hashrate)
            else:
                print('Hash rate: %.2f MH/s, server ping timeout' % hashrate)
    except:
        time.sleep(0.1)
        print('Terminating processes...')
        time.sleep(4)
        for i in range(len(p_list)):
            p_list[i].terminate()