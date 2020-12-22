import socket, urllib.request, time, sys # Python 3 included
import multiprocessing as mp        # Also Python 3 included
import nonceMiner

USERNAME = sys.argv[1]
N_PROCESSES = int(sys.argv[2])

serverip = 'https://raw.githubusercontent.com/revoxhere/duino-coin/gh-pages/serverip.txt' 
pool_location = urllib.request.urlopen(serverip).read().decode().splitlines()
pool_ip = pool_location[0]
pool_port = int(pool_location[1])

job_request_bytes = ('JOB,'+USERNAME).encode('utf-8')

def mineDUCO(hashcount):
    soc = socket.socket()
    try:
        soc.connect((pool_ip, pool_port))
        soc.recv(3) # Receive version, but don't bother decoding
        while True:
            soc.send(job_request_bytes)
            job = soc.recv(1024) # Receive work byte-string from pool
            prefix_bytes = job[0:40] # Prefix is first 40 bytes
            target_bytes = job[41:81] # Target is 40 bytes after comma
            difficulty = int(job[82:]) # Difficulty is all bytes after second comma in utf-8

            result = nonceMiner.c_mine_DUCO_S1(prefix_bytes, target_bytes, difficulty)

            soc.send(str(result).encode('utf-8')) # Send result of hashing algorithm to pool
            soc.recv(1024) # Receive feedback, don't bother decoding
            
            with hashcount.get_lock():
                hashcount.value += result
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
    hashcount = mp.Value('i', 0, lock=True)
    p_list = []
    for i in range(N_PROCESSES):
        p = mp.Process(target=mineDUCO, args=tuple([hashcount]))
        p.start()
        p_list.append(p)
        time.sleep(1/N_PROCESSES)
    try:
        past_time = time.time()
        while True:
            time.sleep(2)

            with hashcount.get_lock():
                hash_in_2s = hashcount.value
                hashcount.value = 0
            current_time = time.time()
            hashrate = hash_in_2s/1000000/(current_time-past_time)
            past_time = current_time
            
            if(ping_server()):
                for i in range(len(p_list)):
                    if(not p_list[i].is_alive()):
                        p_list[i].join()
                        p_list[i] = mp.Process(target=mineDUCO, args=tuple([hashcount]))
                        p_list[i].start()
                print('Hash rate: %.2f MH/s' % hashrate)
            else:
                print('Hash rate: %.2f MH/s, server ping timeout' % hashrate)
    except:
        print('Terminating processes...')
        time.sleep(2)
        for i in range(len(p_list)):
            p_list[i].terminate()