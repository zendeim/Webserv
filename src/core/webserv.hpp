#pragma once
#include "Array.hpp"
#include "Span.hpp"
#include "Status.hpp"

// Server configuration
#define HTTP_METADATA_SIZE (64)
#define HTTP_BUFFERSIZE (8192 - HTTP_METADATA_SIZE / 2)
#define MAX_VIRTUAL_SERVERS (64)	// TODO: This isn't really configurable yet
#define HTTP_TIMEOUT 60

#define MAX_SERVER_BLOCK_SIZE UINT16_MAX
#define MAX_LOCATION_BLOCK_SIZE INT16_MAX
#define MAX_LOCATION_COUNT INT16_MAX

#define CONFIG_POOL_SIZE (MAX_SERVER_BLOCK_SIZE * MAX_VIRTUAL_SERVERS)

// Server parameters
#define HTTP_INDEX_NAME_LENGTH 52
#define HTTP_DIRENT_MAX_SIZE (3 * 255 + HTTP_INDEX_NAME_LENGTH * 6 + 100)
#define HTTP_MAX_ERROR_PAGE_SIZE (HTTP_BUFFERSIZE - 512)

// Kernel configurations
#define MAX_FILE_SIZE (1ul << 48)	// 256 TiB
#define MAX_PATH_SIZE (4096ul)
#ifdef PIPE_BUF
	#if PIPE_BUF > 4096
		#define ATOMIC_IOSIZE 4096
	#else
		#define ATOMIC_IOSIZE PIPE_BUF
	#endif
#else
	#ifdef _POSIX_PIPE_BUF
		#define ATOMIC_IOSIZE _POSIX_PIPE_BUF
	#else
		#define ATOMIC_IOSIZE 512
	#endif
#endif

// These are exclusive states
enum class Mode : u8 {
	PARSE_FIRST = 0,	// Changes to PARSE after first line
	PARSE,				// Calls setup when finished

	GET,				// Changes to FLUSH upon bodysize == 0
	AUTOINDEX,			// Changes to FLUSH upon entry == NULL

	POST_FIXED,			// Reads from client, changes to FLUSH upon bodySize == 0
	POST_CHUNKED,		// Reads from client, changes to POST_FIXED on chunk termination

	CGI,				// Reads from CGI, writes to client, changes to FLUSH upon EOF
	CGI_FIXED,			// Reads from client, changes to CGI upon bodySize == 0
	CGI_CHUNKED,		// Reads from client, changes to CGI_FIXED on chunk termination
	CGI_PARSED,

	FLUSH				// Changes to FIRST_PARSE if keepalive is on. else terminates
};

namespace Options {
	enum e_http_options : u16 {
		GET = 1 << 0,
		POST = 1 << 1,
		DELETE = 1 << 2,
		CGI = 1 << 3,
		SSE = 1 << 4,
		CHUNKED_LENGTH = 1 << 5,
		FIXED_LENGTH = 1 << 6,
		HOST = 1 << 7,
		KEEP_ALIVE = 1 << 8
	};
}

#define FIELD_TABLE {"status", "location", "transfer-encoding", \
	"content-length", "content-type", "host", "connection", \
	"accept", "cookie"}

enum class Field : u8 {
	UNKNOWN = 0,
	STATUS,
	LOCATION,
	TRANSFER_ENCODING,
	CONTENT_LENGTH,
	CONTENT_TYPE,
	HOST,
	CONNECTION,
	ACCEPT,
	COOKIES,
	COUNT
};

#define MIME_TABLE {"html", "htm", "css", "json", "js", "png", "jpg", "jpeg", "gif", "txt"}

#define MIME_STRINGS {"\x18" "application/octet-stream", "\x09" "text/html", "\x09" "text/html",\
	"\x08" "text/css", "\x10" "application/json", "\x16" "application/javascript",\
	"\x09" "image/png", "\x0A" "image/jpeg", "\x0A" "image/jpeg", \
	"\x09" "image/gif", "\x0A" "text/plain"}

enum class Mime : u8 {
	OCTET_STREAM = 0,
	HTML,
	HTM,
	CSS,
	JSON,
	JS,
	PNG,
	JPG,
	JPEG,
	GIF,
	TXT,
	COUNT
};
