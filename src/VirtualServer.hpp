#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>

#include "core.hpp"
#include "pure_functions.hpp"
#include "webserv.hpp"
#include "Span.hpp"
#include "Status.hpp"
#include "Array.hpp"
#include "Environment.hpp"

struct Location {
	Span16	uri;
	Span16	root;
	Span16	index;
	Span16	uploadStore;
	Span16	cgiBlock;
	Span16	redirectTarget;
	Status::Code redirectStatus;
	u8		methods;
	bool	autoindex;

	ATTR(inl, pure) Span extract(const Span16& span) const {
		Span result = {(char*)this + span.index, span.size};
		ASSERT(result.ptr[result.size] == '\0', "Location span is not null terminated");
		return result;
	}

	ATTR(inl, pure) Span get_uri() const { return extract(uri); }
	ATTR(inl, pure) Span get_root() const { return extract(root); }
	ATTR(inl, pure) Span get_index() const { return extract(index); }
	ATTR(inl, pure) Span get_upload_store() const { return extract(uploadStore); }
	ATTR(inl, pure) Span get_cgi_block() const { return extract(cgiBlock); }
	ATTR(inl, pure) Span get_redirect_target() const { return extract(redirectTarget); }
};

struct VirtualServer {
	Span serverRoot;
	Span errorPages[Status::errorPageCount];
	Span host;
	ArrayView<Location>	locations;
	usize port;
	usize maxBodySize;
	int listenFd;

	void reset() {
		MEMSET_INLINE(this, 0, sizeof(*this));
		port = SIZE_MAX;
		maxBodySize = SIZE_MAX;
		listenFd = -1;
	}

	int clear() {
		listenFd = fn::close_noerr(listenFd);
		return 1;
	}

	void init() {
		if (listenFd != -1)
			clear();
		ASSERT(port >= 1 && port <= 65535, "Invalid virtual server port");

		listenFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
		if (listenFd == -1)
			PERR_EXIT(clear(), "Error: Failed to create listening socket");

		int reuse = 1;
		if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
			PERR_EXIT(clear(), "Error: Failed to configure listening socket");

		sockaddr_in address = {};
		address.sin_family = AF_INET;
		address.sin_port = htons((u16)port);
		if (host == "localhost")
			address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		else if (inet_pton(AF_INET, host.ptr, &address.sin_addr) != 1)
			PERR_EXIT(clear(), "Error: Failed to resolve virtual server host");
		if (bind(listenFd, (sockaddr*)&address, sizeof(address)) == -1)
			PERR_EXIT(clear(), "Error: Failed to bind listening socket");
		if (listen(listenFd, SOMAXCONN) == -1)
			PERR_EXIT(clear(), "Error: Failed to listen on socket");
	}
};
