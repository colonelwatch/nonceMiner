cdef extern from 'mine_DUCO_S1.h':
    # Assuming a HASH_SIZE of 20
    int mine_DUCO_S1(
        const char target_hash[2*20],
        const unsigned char hash_prefix[2*20],
        int difficulty)

def c_mine_DUCO_S1(prefix : bytes, target : bytes, difficulty : int) -> int:
    return mine_DUCO_S1(target, prefix, difficulty)