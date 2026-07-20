#include "pch.h"

#include "pshpack1.h"
// an entry in the file table
struct FileEntry
{
	char name[16];
	uint32_t size;
};

// table of files
struct FileTable
{
	uint32_t origSize;
	uint32_t totalSize; // size of all compressed data
	uint32_t entryCount;
	FileEntry entries[0]; // entries[entryCount]
						  // byte compressed[totalSize];
};
#include "poppack.h"

extern "C" FileTable g_fileTable;

// loaded file table
static std::unordered_map<std::string_view, std::span<const byte>> s_fileMap;
// all decompressed file data
static byte* s_fileData;
// size of all data
static size_t s_fileDataSize;

void InitFileTable()
{
	auto& tab = g_fileTable;

	if (tab.origSize < 1 || tab.totalSize < 1 || tab.entryCount < 1)
	{
		return;
	}

	// allocate space
	s_fileData = new byte[tab.origSize];
	auto src = std::span((byte*)&tab.entries[tab.entryCount], tab.totalSize);
	auto dest = std::span(s_fileData, tab.origSize);

	// decompress all files
	uint64_t memLimit = 2 * 1024 * 1024 * 1024;
	size_t inPos = 0;
	size_t outPos = 0;
	auto result =
		lzma_stream_buffer_decode(&memLimit, 0, nullptr, src.data(), &inPos, src.size(), dest.data(), &outPos, dest.size());
	if (result != LZMA_OK)
	{
		ErrorMessage(result, "failed to decompress files: error %d", result);
	}

	// map them
	size_t offset = 0;
	std::for_each_n(tab.entries, tab.entryCount, [&](const auto& entry) {
		s_fileMap[entry.name] = std::span(s_fileData + offset, entry.size);
		offset += entry.size;
	});
}

std::span<const byte> GetFile(const char* name)
{
	auto found = s_fileMap.find(name);
	if (found != s_fileMap.end())
	{
		return found->second;
	}

	return {};
}
