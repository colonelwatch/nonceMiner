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

def mineDUCO(hashcount, accepted, rejected):
    soc = socket.socket()
    soc.connect((pool_ip, pool_port))
    soc.recv(3) # Receive version, but don't bother decoding

    try:
        while True:
            soc.send(job_request_bytes)
            job = soc.recv(1024).decode().split(',') # Get work from pool
            prefix_bytes = job[0].encode('ascii')
            target_bytes = job[1].encode('ascii')
            difficulty = int(job[2])

            result = nonceMiner.c_mine_DUCO_S1(prefix_bytes, target_bytes, difficulty)

            soc.send(str(result).encode('utf-8')) # Send result of hashing algorithm to pool
            feedback = soc.recv(1024).decode() # Get feedback about the result

            if feedback == 'GOOD':
                with accepted.get_lock():
                    accepted.value += 1
            elif feedback == 'BAD':
                with rejected.get_lock():
                    rejected.value += 1
            with hashcount.get_lock():
                hashcount.value += result
    except:
        soc.close()
        return

if __name__ == '__main__':
    hashcount = mp.Value('i', 0, lock=True)
    accepted = mp.Value('i', 0, lock=True)
    rejected = mp.Value('i', 0, lock=True)
    p_list = []
    for i in range(N_PROCESSES):
        p = mp.Process(target=mineDUCO, args=tuple([hashcount, accepted, rejected]))
        p.start()
        p_list.append(p)
        time.sleep(1/N_PROCESSES)
    try:
        past_time = time.time()
        while True:
            time.sleep(2)
            current_time = time.time()

            with hashcount.get_lock():
                hash_in_2s = hashcount.value
                hashcount.value = 0
            with accepted.get_lock():
                accepted_in_2s = accepted.value
                accepted.value = 0
            with rejected.get_lock():
                rejected_in_2s = rejected.value
                rejected.value = 0
            
            print('Hash rate: %.2f' % (hash_in_2s/1000000/(current_time-past_time)), 'MH/s')
            print('Accepted shares in 2s:', accepted_in_2s, '\tRejected shares in 2s:', rejected_in_2s)
            print()
            past_time = current_time

            for i in range(len(p_list)):
                if(not p_list[i].is_alive()):
                    p_list[i].join()
                    p_list[i] = mp.Process(target=mineDUCO, args=tuple([hashcount, accepted, rejected]))
                    p_list[i].start()
    except:
        print('Terminating processes...')
        time.sleep(2)
        for i in range(len(p_list)):
            p_list[i].terminate()