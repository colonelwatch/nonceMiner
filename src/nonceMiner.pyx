cdef extern from 'mine_DUCO_S1.h':
    # Assuming a HASH_SIZE of 20
    long mine_DUCO_S1(
        const char target_hash[2*20],
        const unsigned char hash_prefix[2*20],
        int difficulty)
    long mine_DUCO_S1_extend_cache(
        const char target_hexdigest[20*2],
        const unsigned char input_prefix[20*2],
        int difficulty)

def c_mine_DUCO_S1(prefix : bytes, target : bytes, difficulty : int, extend_cache : bool = True) -> int:
    if(extend_cache): return mine_DUCO_S1_extend_cache(target, prefix, difficulty)
    else: return mine_DUCO_S1(target, prefix, difficulty)