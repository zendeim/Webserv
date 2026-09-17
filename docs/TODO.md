1. Make multi-level bitarray (learn how to do specializations)

2. Change all bitfields to a bitmap

3. Make it so all functions that use ATTR use it correctly (status especially)

4. Use constexpr where correct

5. Revamp status

6. Improve matching function

7. Create a safe open wrapper

8. Change CGI pipe open method to handle CLOEXEC and NONBLOCKING directly;

9. Use sendfile for get requests and post/cgi unchunked

10. Change the read from client in chunked requests to instead read into a tmp buffer then copy to a permanent buffer
	READ_CHUNK_HEADER
		↓
	FORWARD_BUFFERED_BODY
		↓
	chunkRemaining large?
	yes ↓          no
		SPLICE      keep buffering
		↓
	READ_CHUNK_CRLF
		↓
	READ_CHUNK_HEADER

11. Change the design of the LUT functions that alter static memory

12. Make const variables gold colored white

13. Museum of insanity concept (like for old status)

14. A profiler should take variance into account

A test should ideally be XY where X is variance and Y is range magnitude

Cold and Hot, something that clears the cache, and something that requires load (to simulate real scenarios where the algorithm isn't being run solo)

Investigate Clang Vectors

15. Have Matlab like syntax in terminal, ex: 4:2:64 gives 4 8 12 .. 64