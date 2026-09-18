#pragma once
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "core.hpp"
#include "Array.hpp"
#include "Arena.hpp"
#include "Span.hpp"
#include "Bitmap.hpp"
#include "Buffer.hpp"
#include "VirtualServer.hpp"
#include "pure_functions.hpp"

#define PARSER_INL(retType) retType inline Parser::

struct Parser {
	struct Token {
		enum Type {
			OPEN_BRACKET,
			CLOSE_BRACKET,
			SEMICOLON,
			WORD
		}	type;
		Span value;
	};

	struct Directive {
		Span name;
		ArrayView<Span> args;
	};

	struct ParsedCgi {
		ArrayView<Token> definitions;
		usize size;
	};

	struct ParsedLocation {
		Span uri;
		Span root;
		Span index;
		Span uploadStore;
		ParsedCgi cgiBlock;
		Span redirectTarget;
		Status::Code redirectStatus;
		u8 methods;
		bool autoindex;
		bool autoindexSet;
	};

	Arena* alpha;
	Arena* beta;
	Span file;
	usize serverCount;

	void init(const char* filePath, VirtualServer (&servers)[MAX_VIRTUAL_SERVERS], Arena& srcAlpha, Arena& srcBeta) {
		alpha = &srcAlpha;
		beta = &srcBeta;
		file = {};
		serverCount = 0;
		if (fn::read_whole_file(*alpha, filePath, file, 63, 16))
			_exit(1);
		ArrayView<Token> tokArray = tokenize();
		for (usize serverIndex = 0; serverIndex < serverCount; serverIndex++) {
			tokArray.ptr++;
			parse_server(tokArray, servers[serverIndex]);
		}
		alpha->clear();
	}

	ArrayView<Token> tokenize();
	void cache_error_pages(VirtualServer& server, const Span& folder);
	ParsedLocation parse_location(ArrayView<Token>& tokArray);
	void parse_server(ArrayView<Token>& tokArray, VirtualServer& server);

	ArrayView<Location> store_locations(ArrayView<ParsedLocation>& ploc);
	ArrayView<Location> process_locations(ArrayView<ParsedLocation>& ploc, VirtualServer& server);

	ParsedCgi parse_cgi(ArrayView<Token>& tokArray);
	void parse_location_directive(ParsedLocation& location, Directive& dir);
	void parse_server_directive(VirtualServer& server, Directive& dir, Span& errorPageFolder);
	static Directive s_build_directive(Arena& arena, ArrayView<Token>& tokArray);
};

#include "Parser_common.ipp"
#include "Parser_locations.ipp"
#include "Parser_server.ipp"
#include "Parser_tokenize.ipp"
#include "Parser_process.ipp"
