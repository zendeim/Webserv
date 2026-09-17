#pragma once
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#include "core.hpp"
#include "Clock.hpp"
#include "Buffer.hpp"
#include "VirtualServer.hpp"
#include "Environment.hpp"
#include "Epoll.hpp"

#include "pure_functions.hpp"

#define CONNECTION_INL(retType) inline retType Connection::

struct Connection {
	struct Request {
		Span target, query, host, cookies, interpreter;
		Span contentTypeHeader, contentSize;
		Location* location;
		Span relativeTarget, targetName, targetExt;
		Span uri, cgi, root;
		u8 padding[40];

		ATTR(inl) void clear() {
			MEMSET_INLINE(this, 0, sizeof(*this));
		}
	};
/* ========== Attributes ============================================= */
	union {
		struct { HTTP_Buffer recvBuffer, sendBuffer; };
		struct { HTTP_PBuffer parseBuffer; Request req; };
	};

	union {
		u8 metadata[64];
		struct {
			VirtualServer* cfg;
			u16 options;
			Mime contentType;
			Mode mode;
			u32 startTime;
			u8 epollState;
			i32 clientFd, readFd, writeFd;
			pid_t processId;
			usize bodySize;
			usize chunkSize;
		};
	};
/* =================================================================== */
	// Common
	isize init(int fd, VirtualServer* serverConfig);
	isize clear();
	void activate_streaming(Mode nextMode);
	void activate_parsing();

	// Dispatching
	isize dispatch(Epoll& epoll);
	isize parse_first(Epoll& epoll);
	isize parse(Epoll& epoll);

	// Parsing
	Status::Code parse_line(Span line);
	Status::Code parse_first_line(Span line);
	Status::Code parse_validate(char* str, char* end);
	Status::Code match_location();
	Span check_cgi();

	// Response
	void build_error_header(Status::Code code);
	void build_header(Status::Code code);
	Status::Code build_cgi_header(Status::Code code);
	Status::Code parse_cgi_line(Buffer64& tmpBuffer);

	// Streaming
	isize cgi(Epoll& epoll);
	isize cgi_fixed(Epoll& epoll);
	isize cgi_parsed(Epoll& epoll);
	isize cgi_chunked(Epoll& epoll);
	isize switch_to_cgi(Epoll& epoll);
	isize download_file_fixed(Epoll& epoll);
	isize download_file_chunked(Epoll& epoll);
	isize upload_file(Epoll& epoll);
	isize upload_directory(Epoll& epoll);
	isize flush(Epoll& epoll);
	isize write_to_client(Epoll& epoll);
	isize read_from_client(Epoll& epoll);
	isize read_chunked(Epoll& epoll);

	// Setup
	isize setup_dispatch(Epoll& epoll);
	isize del_setup(Epoll& epoll);

	isize get_setup(Epoll& epoll);
	isize get_autoindex_setup(Epoll& epoll);
	isize get_directory_setup(Epoll& epoll);
	isize get_redirect_setup(Epoll& epoll);

	isize post_setup(Epoll& epoll);
	isize cgi_setup(Epoll& epoll);
	char* append_env(Buffer64& buffer, char* argv[3]);
	isize flush_setup(Epoll& epoll);
	isize flush_setup_close(Epoll& epoll, Status::Code code);
	isize parse_setup(Epoll& epoll);
	isize redirect_setup(Epoll& epoll, Status::Code code);
};

STATIC_ASSERT(sizeof(Connection) == 16384);

#include "Connection_common.ipp"
#include "Connection_dispatch.ipp"
#include "Connection_parse.ipp"
#include "Connection_parse_validate.ipp"
#include "Connection_response.ipp"

#include "Connection_stream.ipp"
#include "Connection_stream_cgi.ipp"
#include "Connection_stream_get.ipp"

#include "Connection_setup.ipp"
#include "Connection_setup_cgi.ipp"
#include "Connection_setup_get.ipp"
