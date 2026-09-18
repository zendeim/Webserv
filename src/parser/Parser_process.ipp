#pragma once
#include "Parser.hpp"

// Guarantee that it doesnt overflow u16

ATTR(static_inl)
Span16 s_store_location_span(Location& location, char*& wptr, const Span& source) {
	Span16 result = {(u16)(wptr - (char*)&location.uri), (u16)source.size};
	MEMCPY(wptr, source.ptr, source.size);
	wptr += source.size;
	*wptr++ = '\0';
	return result;
}

ATTR(static_inl)
Span16 s_store_upload_span(Location& location, char*& wptr, const Span& source) {
	Span16 result = {(u16)(wptr - (char*)&location.uri), (u16)source.size};
	MEMCPY(wptr, source.ptr, source.size);
	wptr += source.size;
	if (wptr[-1] != '/') {
		*wptr++ = '/';
		result.size++;
	}
	*wptr++ = '\0';
	return result;
}

ATTR(static_inl)
void s_store_cgi(char*& wptr, const Parser::ParsedCgi& cgiBlock, Location& location) {
	location.cgiBlock.index = (u16)(wptr - (char*)&location.uri);
	location.cgiBlock.size = (u16)cgiBlock.size;
	for (usize index = 0; index < cgiBlock.definitions.count; index += 4) {
		Parser::Token* definition = cgiBlock.definitions.ptr + index;
		const Span& extension = definition[0].value;
		const Span& interpreter = definition[2].value;
		const u16 lengths[2] = {(u16)extension.size, (u16)interpreter.size};
		MEMCPY_INLINE(wptr, lengths, sizeof(lengths));
		wptr += sizeof(lengths);
		MEMCPY(wptr, extension.ptr, extension.size);
		wptr += extension.size;
		MEMCPY(wptr, interpreter.ptr, interpreter.size);
		wptr += interpreter.size;
	}
	*wptr++ = '\0';
}

ATTR(static_inl)
void s_store_location(char*& wptr, const Parser::ParsedLocation& ploc, Location& loc) {
	loc.uri = s_store_location_span(loc, wptr, ploc.uri);
	loc.root = s_store_location_span(loc, wptr, ploc.root);
	loc.index = s_store_location_span(loc, wptr, ploc.index);
	loc.uploadStore = s_store_upload_span(loc, wptr, ploc.uploadStore);
	s_store_cgi(wptr, ploc.cgiBlock, loc);
	loc.redirectTarget = s_store_location_span(loc, wptr, ploc.redirectTarget);
	loc.redirectStatus = ploc.redirectStatus;
	loc.methods = ploc.methods;
	loc.autoindex = ploc.autoindex;
}

ATTR(static_inl, pure)
usize s_location_size(const Parser::ParsedLocation& loc) {
	usize packSize = 16 + loc.uri.size + loc.root.size + loc.index.size;
	packSize += loc.uploadStore.size + loc.cgiBlock.size + loc.redirectTarget.size;
	return packSize;
}

PARSER_INL
(ArrayView<Location>) store_locations(ArrayView<ParsedLocation>& ploc) {
	usize allocationSize = ploc.count * sizeof(Location);
	for (usize index = 0; index < ploc.count; index++)
		allocationSize += s_location_size(ploc[index]);
	if (allocationSize > MAX_SERVER_BLOCK_SIZE)	// Review: This assumes adversarial input for server controlled config
		PERR_EXIT(1, "Error: Stored locations exceed the maximum server block size");
	const u32 allocation = beta->alloc(allocationSize, 0, alignof(Location));
	if (allocation == UINT32_MAX)
		_exit(1);
	ArrayView<Location> locations = {(Location*)beta->mptr(allocation), ploc.count};
	char* wptr = (char*)(locations.ptr + locations.count);
	for (usize locationIndex = 0; locationIndex < locations.count; locationIndex++)
		s_store_location(wptr, ploc[locationIndex], locations[locationIndex]);
	return locations;
}

PARSER_INL
(ArrayView<Location>) process_locations(ArrayView<ParsedLocation>& ploc, VirtualServer& server) {
	Span& serverRoot = server.serverRoot;

	if (server.host.size == 0)
		server.host = beta->copy_span(Span::create("localhost"));
	if (serverRoot.size == 0)
		serverRoot = beta->copy_span(Span::create(""));
	while (serverRoot.size != 0 && serverRoot.ptr[serverRoot.size - 1] == '/')
		serverRoot.size--;
	serverRoot.ptr[serverRoot.size] = '\0';

	Span defaultIndex = beta->copy_span(Span::create("index.html"));
	for (usize index = 0; index < ploc.count; index++) {
		ParsedLocation& src = ploc[index];
		if (src.root.size == 0)
			src.root = serverRoot;
		while (src.root.size != 0 && src.root.ptr[src.root.size - 1] == '/')
			src.root.size--;
		if (src.uploadStore.size == 0)
			src.uploadStore = src.root;
		if (src.index.size == 0)
			src.index = defaultIndex;
		else if (*src.index.ptr == '/') {
			src.index.ptr++;
			src.index.size--;
		}
		if (src.methods == 0)
			src.methods = Options::GET;
	}
	return store_locations(ploc);
}

ATTR(static_inl)
void s_build_error_page_path(char* out, const Span& root, const Span& path) {
	ASSERT(path.size != 0, "Error page path is empty");
	ASSERT(root.size == 0 || root.ptr[root.size - 1] != '/', "Root has a trailing slash");
	usize length = path.ptr[0] == '/' ? 0 : root.size;
	MEMCPY(out, root.ptr, length);
	if (length != 0)
		out[length++] = '/';
	MEMCPY(out + length, path.ptr, path.size);
	length += path.size;
	out[length] = '\0';
}

PARSER_INL
(void) cache_error_pages(VirtualServer& server, const Span& folder) {
	char pathBuffer[4 * MAX_PATH_SIZE];
	Buffer64 entries = {};
	Bitmap configured = {};

	for (usize index = 0; index < Status::errorPageCount; index++)
		server.errorPages[index] = Status::get_status_page((Status::Code)(Status::i400 + index));
	if (folder.ptr == NULL)
		return;
	s_build_error_page_path(pathBuffer, server.serverRoot, folder);
	const int directoryFd = open(pathBuffer, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (directoryFd == -1)
		PERR_EXIT(1, "Error: Failed to open error pages folder");
	usize folderLength = STRLEN(pathBuffer);
	if (pathBuffer[folderLength - 1] != '/')
		pathBuffer[folderLength++] = '/';

	char* name;
	while ((name = entries.readdir(directoryFd)) != NULL) {
		const Status::Code status = Status::str_to_status(name);
		if (!Status::is_error(status))
			continue;
		const usize index = Status::get_page_index(status);
		if (configured.bitread(index))
			close(directoryFd), PERR_EXIT(1, "Error: Duplicate error page");
		configured.bitset(index);
		MEMCPY(pathBuffer + folderLength, name, STRLEN(name) + 1);
		if (fn::read_whole_file(*beta, pathBuffer, server.errorPages[index], 0, 0, HTTP_MAX_ERROR_PAGE_SIZE))
			close(directoryFd), PERR_EXIT(1, "Error: Failed to read error pages folder");
	}
	fn::close_noerr(directoryFd);
	if (errno != 0)
		PERR_EXIT(1, "Error: Failed to read error pages folder");
}
