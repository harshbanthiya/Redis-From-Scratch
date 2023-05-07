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
./server
```

```sh
./client asdf
(err) 1 Unknown cmd
./client get asdf
(nil)
./client set k v
(nil)
./client get k
(str) v
./client keys
(arr) len=1
(str) k
(arr) end
./client del k
(int) 1
./client del k
(int) 0
./client keys
(arr) len=0
(arr) end
```


## TO DO

General clean and separation of code into specific headers and keeping about 5-6 functions only per file.

Implementing AVL tree based sorted set data structure for achieving - the default Redis ability to sort your data in order, and to support the unique feature of querying ordered data by rank.
