# webserv
An HTTP/1.1 server written in C++

## Description
Initially, this server was written as part of the 42 curriculum that enforces several constraints, such as the inability of checking errno after reads/writes, C++98 standard and other weird stuff
I might gradually update and modernize, but if you encounter something weird, it's probably the reason.

## Usage
The Makefile has a default run and valgrind run.
I have also included a test bash script for easy testing
(make fast run) or (make debug vrun) builds and runs the server with default config
./test executes the pre-configured siege stress test and optionally takes an argument for connection slots

Examples:
	make fast run, then open a browser and go to 127.0.0.1:8080
	make debug vrun, then ./test in another terminal
	make fast run, then ./test 255 in another terminal

### Configuration
```nginx
server {
	listen 8081;
	host 127.0.0.1;							## Only localhost is recognized as a loopback
	client_max_body_size 10M;				## Can define K, M or G for KiB, MiB or GiB respectively
											## Defaults to MAX_FILE_SIZE (generally 256 TiB)
	root /path_to_root;						## Defaults to empty
	error_pages /path_to_error_folder;		## All files inside directory are prefix matched against error codes

	location /custom_index/ {
		root /path_to_root;					## Defaults to server root
		allowed_methods GET POST;			## Defaults to GET
		upload_store /path_to_upload_store;	## Defaults to root + /
		index index_file_name;				## Defaults to index.html
		autoindex on;						## Default to off
		cgi {								## Defined per location block
			.extension = /absolute_path_to_interpreter;
			.py = /bin/python3;
		}
	}

	location /old {
		allowed_methods GET;
		return 301 /new/;
	}
}
```
## Glossary
### SIMD (Single Input Multiple Data)
	Refers to instructions that process multiple data at once. 
	For example, to clear an 8 byte string, instead of setting each byte to 0, you access it as a 64 bit integer and set that to 0 once

### OOB (Out of bounds)
	Refers to an access that goes beyond the bounds of something
	For example, I have a 6 byte string, but I access it as an 8 byte integer, and it goes 2 bytes beyond its bounds

### Padding
	Something to protect against OOB, you deliberately allocate more than you need so you can guarantee that a SIMD access is safe
	For example, instead of allocating a 6 byte string, you allocate 6 + 8, so that way you never spill for an 8 byte access. This sounds wasteful until you get into Arenas

### POD (Plain Old Data)
	Refers to classes that don't have user defined constructors, destructors or assignment and are trivially initializable. This makes them suitable for unions and to be interpretable as raw memory

### Clobberable Padding
	Essentially, padding where you don't care that it gets overwritten. Read access padding can essentially be free if you just guarantee that the memory address being spilled to exists within your program.
	You can even write to it, as long as you save the values being written to and restore them after your execution. Clobberable padding is padding that exists explicitly for padding's sake, so you don't care about overwriting

### Arenas
	In simple terms, a big allocation to contain other smaller allocations inside it. This is amazing for several reasons:
	1) You can pad per arena instead of per allocation
	2) You guarantee that the memory region you got is contiguous
	3) Less memory fragmentation, more predictable runtime, better performance

## Padding
POST/PRE refer to the location of the padding. [PRE] [DATA] [POST]

1) Prepending QUERY_STRING= in cgi setup depends on the buffer being pre-clobberable-padded with 8 bytes. QUERY_STRING= is 13 bytes
(PRE 8 Clobberable padding)
2) Most find algorithms depend on the buffer being padded with at least 4 bytes to insert a sentinel like \r\n\r\n
(POST 8 bytes OOB padding because it is restored)
3) Match algorithms require 24 bytes OOB padding
(POST 24 bytes OOB padding)

Buffer has 8 clobberable bytes before data and 8 after it. The three size counters provide another 24 readable bytes after data, for 32 bytes of physical trailing storage. Sentinel writes use only the first 8 bytes and restore them; the counters must never be clobbered

## Parsing Invariants
* Comments are stripped from parsing
* The config file read is allocated with at least 64 bytes padding

### Limits
* Each server block is at maximum MAX_SERVER_BLOCK_SIZE (64KB)
* Each location block is at maximum MAX_LOCATION_BLOCK_SIZE (32KB)
* Number of locations is at maximum MAX_LOCATION_COUNT (32767 locations)
* Each error page cached is at maximum MAX_ERROR_PAGE_SIZE (HTTP_BUFFERSIZE - 512B)

### Location Invariants
* All stored location strings are null terminated and 0 <= length <= MAX_PATH_SIZE
* 0 length strings still point to empty data
* There are no duplicates of any kind
* CGI blocks are length-prefixed binary records. Each record stores two u16 lengths, extension bytes without a terminator, and interpreter bytes with a terminator. The interpreter length includes its terminator
* A configured redirect status is a supported 3xx status
* There is at least one allowed method
* A server root span always exists
* A server root and a location root never end with a "/"
* An upload store always ends with a "/"
* An index never starts with a "/"
* A URI always starts with a "/"

## Conventions
### Sizes
Will always take a maximum size of LONG_MAX, even for unsigned types. This is done to avoid overflows and always have error sentinels. LONG_MAX is a ridiculously large number anyhow, any real constraint should realistically be much smaller

### Classes
All classes are POD types
* init() prepares the class to be used
* reset() resets the class to a starting position, but still reusable
* clear() effectively destroys the class (deallocates, closes all fds, etc), requiring an init again

When reset() and clear() would effectively mean same thing, clear() is used

### Style
* Class names are UpperCase, functions are snake_case, variables are camelCase
* Brace styles are K&R, one liners don't get braces, and end braces are always solo, because
if {
	do..whatever
} else is ugly as shit. Notable exception being do while

* Pointers and references attach close to their type
Example: char* str, char& str, char*& str

* Simple getters and methods can become one-liners to avoid using too much vertical space

## Architecture
A single connection uses 16kb of space, of which 64 bytes is used by metadata, and the rest by buffers
A connection pool holds 4096 connections, totalling 64MB + ~1KB of metadata

Each virtual server takes up roughly 784 bytes of memory, plus a fixed storage space for its configs (a budget of 64KB)
This means that for 64 virtual servers, it uses 64 * (784 + 64KB) =  50KB + 4MB = ~4MB. Configured error pages also consume this shared budget.

Additionally, default error pages and status strings are cached in memory, occupying roughly 10kb of space

There are two arenas:
	* Alpha	(64MB): Memory space occupied by the connection pool;
	* Beta	(4MB): Fixed storage space budget for virtual server configs and error pages

For temporary things that aren't going to be used by the program later like tokens and unprocessed configs, arena alpha is used. This ensures no additional allocations or frees are necessary, since whatever was going to be used by connections is considered garbage memory before initialization.

## Dechunking
HTTP chunking is a horrible protocol. Variable sized payloads with ASCII parsing is a recipe for disaster and inefficiency. It also introduces an annoying difficulty when using fixed buffers : Ideally you need two persistent buffers, one for raw input, and another for processed output. But using more memory for all connection types when this is only an issue for chunked encoding didn't sit right with me, and inplace dechunking is slow because you have to deal with aliasing and MEMMOVEs. 

My solution to this was to read into a temporary stack buffer, dechunk and save the processed input into recvBuffer, mark the processed output with scanPos, and append the unprocessed tail. The unprocessed tail is generally going to be very small like 16 bytes, or contain the next request. The extra memory used lives entirely on the stack, so it ends up being a very clean one buffer, one read, one copy, one write

## Epoll
Previously all reads/writes went through epoll, but this made for a horrible mess of state transitions and poor producer/consumer configurations.
For example, a connection dispatch call could read from client, not be able to write to CGI, which makes most further calls pointless because there is no new data.
By constraining epoll to client FD only, we ensure continuity of the data stream, aka no clogging of the pipes. Because we also always constrain IO sizes to ATOMIC_IOSIZE, all writes and reads are atomic and no short writes can occur, ensuring that errors we receive are fatal errors. This is only a problem when:
- CGI sleeps and doesn't consume input;
- CGI sleeps and doesn't produce output;
- The delta between data produced by the client and consumed by CGI is greater than 64kb (i.e. poorly written cgi implementation);

Given that these conditions are deliberately obtuse, treating them as fatal errors is acceptable
