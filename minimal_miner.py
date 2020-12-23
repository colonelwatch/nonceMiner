#!/usr/bin/env python3
# Minimal version of Duino-Coin PC Miner, useful for developing own apps. Created by revox 2020
# Edited by Kenny Peng to call nonceMiner extension
import socket, urllib.request # Only python3 included libraries
import nonceMiner
soc = socket.socket()

username = 'travelmode'

# This sections grabs pool adress and port from Duino-Coin GitHub file
serverip = 'https://raw.githubusercontent.com/revoxhere/duino-coin/gh-pages/serverip.txt' # Serverip file
with urllib.request.urlopen(serverip) as content:
    content = content.read().decode().splitlines() #Read content and split into lines
pool_address = content[0] #Line 1 = pool address
pool_port = content[1] #Line 2 = pool port

# This section connects and logs user to the server
soc.connect((str(pool_address), int(pool_port))) # Connect to the server
server_version = soc.recv(3).decode() # Get server version
print('Server is on version', server_version)

# Mining section
while True:
    soc.send(bytes('JOB,'+str(username), encoding='utf8')) # Send job request
    job = soc.recv(1024) # Receive work byte-string from pool
    prefix_bytes = job[0:40] # Prefix is first 40 bytes
    target_bytes = job[41:81] # Target is 40 bytes after comma
    difficulty = int(job[82:]) # Difficulty is all bytes after second comma in utf-8

    result = nonceMiner.c_mine_DUCO_S1(prefix_bytes, target_bytes, difficulty)

    soc.send(bytes(str(result), encoding='utf8')) # Send result of hashing algorithm to pool
    feedback = soc.recv(1024).decode() # Get feedback about the result
    if feedback == 'GOOD': # If result was good
        print('Accepted share', result, 'Difficulty', difficulty)
    elif feedback == 'BAD': # If result was bad
        print('Rejected share', result, 'Difficulty', difficulty)