# Redis-From-Scratch
Implementing Redis from Scratch in C to gain a deeper understanding of network programming

https://harsh-banthiya.vercel.app/redis-from-scratch
https://harsh-banthiya.vercel.app/redis-from-scratch-part-ii

## Usage

**To run server and client**
```sh
make server ; make client
```

**Open two terminal windows and run both programs**
```sh
./client get k
server says: [2]
./client set k v
server says: [0]
./client get k
server says: [0] v
./client del k
server says: [0]
./client get k
server says: [2]
./client aaa bbb
server says: [1] Unknown cmd

```
```sh
./server
```
