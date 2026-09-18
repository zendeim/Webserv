#pragma once
#include "Parser.hpp"

ATTR(static_inl)
void s_set_methods(const ArrayView<Span>& methods, Parser::ParsedLocation& loc) {
	for (usize index = 0; index < methods.count; index++) {
		u8 method;
		if (methods[index] == "GET")
			method = Options::GET;
		else if (methods[index] == "POST")
			method = Options::POST;
		else if (methods[index] == "DELETE")
			method = Options::DELETE;
		else
			PERR_EXIT(1, "Error: Invalid method");
		if ((loc.methods & method) != 0)
			PERR_EXIT(1, "Error: Duplicate method");
		loc.methods |= method;
	}
}

PARSER_INL
(void) parse_location_directive(ParsedLocation& loc, Directive& dir) {
	usize length = 1;

	if (dir.name == "root") {
		if (dir.args.count != 1 || loc.root.size != 0)
			PERR_EXIT(1, "Error: Invalid root");
		loc.root = dir.args[0];
		length = dir.args[0].size;
	}
	else if (dir.name == "autoindex") {
		if (dir.args.count != 1 || loc.autoindexSet == true)
			PERR_EXIT(1, "Error: Invalid autoindex");
		if ((!(dir.args[0] == "on") && !(dir.args[0] == "off")))
			PERR_EXIT(1, "Error: Invalid autoindex");
		loc.autoindex = dir.args[0] == "on";
		loc.autoindexSet = true;
	}
	else if (dir.name == "allowed_methods") {
		if (dir.args.count == 0)
			PERR_EXIT(1, "Error: No allowed methods defined");
		if (loc.methods != 0)
			PERR_EXIT(1, "Error: Duplicate methods");
		s_set_methods(dir.args, loc);
	}
	else if (dir.name == "index") {
		if (dir.args.count != 1 || loc.index.size != 0)
			PERR_EXIT(1, "Error: Invalid index");
		loc.index = dir.args[0];
		if (loc.index.size == 1 && *loc.index.ptr == '/')
			PERR_EXIT(1, "Error: Invalid index");
		length = dir.args[0].size;
	}
	else if (dir.name == "upload_store") {
		if (dir.args.count != 1 || loc.uploadStore.size != 0)
			PERR_EXIT(1, "Error: Invalid upload store");
		loc.uploadStore = dir.args[0];
		length = dir.args[0].size;
	}
	else if (dir.name == "return") {
		if (dir.args.count != 2 || dir.args[0].size != 3 || loc.redirectTarget.size != 0)
			PERR_EXIT(1, "Error: Invalid redirect");
		const usize status = fn::qstrtol10(dir.args[0].ptr);
		loc.redirectStatus = Status::num_to_status(status);
		if (!Status::is_redirect(loc.redirectStatus))
			PERR_EXIT(1, "Error: Invalid redirect status");
		loc.redirectTarget = dir.args[1];
		length = dir.args[1].size;
	}
	else
		PERR_EXIT(1, "Error: Invalid location directive");
	if (length >= MAX_PATH_SIZE)
		PERR_EXIT(1, "Error: Path size is too large");
}

PARSER_INL
(Parser::ParsedCgi) parse_cgi(ArrayView<Token>& tokArray) {
	if (tokArray[0].type != Token::OPEN_BRACKET)
		PERR_EXIT(1, "Error: Invalid CGI block");

	ParsedCgi cgi = {};
	tokArray.ptr++;
	Token* definitionStart = tokArray.ptr;
	while (tokArray[0].type != Token::CLOSE_BRACKET) {
		Token* definition = tokArray.ptr;
		const Span& extension = tokArray[0].value;
		if (extension.size < 2 || extension.size >= MAX_PATH_SIZE || extension.ptr[0] != '.')
			PERR_EXIT(1, "Error: Invalid CGI extension");
		for (Token* previousToken = definitionStart; previousToken < definition; previousToken += 4) {
			const Span& previous = previousToken->value;
			if (previous.size == extension.size && MEMCMP(previous.ptr, extension.ptr, extension.size) == 0)
				PERR_EXIT(1, "Error: Duplicate CGI extension");
		}
		tokArray.ptr++;
		if (!(tokArray[0].value == "="))
			PERR_EXIT(1, "Error: Expected '=' in CGI definition");
		tokArray.ptr++;
		if (tokArray[0].type != Token::WORD)
			PERR_EXIT(1, "Error: Invalid CGI interpreter");
		Span& interpreter = tokArray[0].value;
		if (interpreter.size >= MAX_PATH_SIZE)
			PERR_EXIT(1, "Error: Path size is too large");
		interpreter.ptr[interpreter.size++] = '\0';
		tokArray.ptr++;
		if (tokArray[0].type != Token::SEMICOLON)
			PERR_EXIT(1, "Error: Expected ';' after CGI definition");
		tokArray.ptr++;
		cgi.size += sizeof(u16) * 2 + extension.size + interpreter.size;
	}

	cgi.definitions = {definitionStart, (usize)(tokArray.ptr - definitionStart)};
	tokArray.ptr++;
	return cgi;
}

PARSER_INL
(Parser::ParsedLocation) parse_location(ArrayView<Token>& tokArray) {
	ParsedLocation loc = {};
	loc.uri = tokArray[0].value;
	loc.redirectTarget = Span::create("");
	loc.redirectStatus = Status::unset;

	if (loc.uri.ptr[0] != '/')
		PERR_EXIT(1, "Error: Invalid location path");
	if (loc.uri.size >= MAX_PATH_SIZE)
		PERR_EXIT(1, "Error: Path size is too large");
	tokArray.ptr += 2;
	bool cgiDefined = false;
	while (tokArray[0].type != Token::CLOSE_BRACKET) {
		if (tokArray[0].value == "cgi") {
			if (cgiDefined)
				PERR_EXIT(1, "Error: Duplicate CGI block");
			cgiDefined = true;
			tokArray.ptr++;
			loc.cgiBlock = parse_cgi(tokArray);
		}
		else {
			Directive dir = s_build_directive(*alpha, tokArray);
			parse_location_directive(loc, dir);
		}
	}
	tokArray.ptr++;
	return loc;
}
