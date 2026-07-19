#include "pch.h"

// an entry in the file table
struct FileEntry
{
	char name[16];
	uint32_t offset;
	uint32_t size;
	uint32_t origSize;
};

// table of files
struct FileTable
{
	uint32_t entryCount;
	FileEntry entries[0];
};

extern "C" FileTable g_fileTable;

// loaded file table
static std::unordered_map<std::string_view, std::span<const byte>> s_fileMap;
// all decompressed file data
static byte* s_fileData;
// size of all data
static size_t s_fileDataSize;

void InitFileTable()
{
	// calculate room needed
	std::for_each_n(g_fileTable.entries, g_fileTable.entryCount, [&](const auto& entry) {
		s_fileDataSize += entry.origSize;
	});

	// allocate space
	s_fileData = new byte[s_fileDataSize];

	// decompress all files
	size_t offset = 0;
	auto ctx = FL2_createDCtx();
	std::for_each_n(g_fileTable.entries, g_fileTable.entryCount, [&](const auto& entry) {
		auto src = std::span<const byte>((const byte*)&g_fileTable + entry.offset, entry.size);
		auto dest = std::span<byte>(s_fileData + offset, entry.origSize);
		s_fileMap[entry.name] = dest;

		FL2_decompressDCtx(ctx, dest.data(), dest.size(), src.data(), src.size());

		offset += entry.origSize;
	});

	FL2_freeDCtx(ctx);
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
