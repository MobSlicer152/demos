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

struct MapEntry
{
	uint64_t hash = 0;
	std::span<const byte> data;

	MapEntry() = default;
	MapEntry(const char* name, std::span<const byte> data) : data(data)
	{
		hash = FNV(std::span<const byte>((const byte*)name, strnlen(name, sizeof(FileEntry::name))));
	}
};

// loaded file table
static MapEntry* s_fileMap;
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
	s_fileMap = new MapEntry[tab.entryCount];

	// decompress all files
	uint64_t memLimit = 2ull * 1024 * 1024 * 1024;
	size_t inPos = 0;
	size_t outPos = 0;
	auto result = ZSTD_decompress(dest.data(), dest.size(), src.data(), src.size());
	if (ZSTD_isError(result))
	{
		ErrorMessage(result, "failed to decompress files: %s", ZSTD_getErrorName(result));
	}

	// map them
	size_t offset = 0;
	for (uint32_t i = 0; i < tab.entryCount; i++)
	{
		auto& entry = tab.entries[i];
		s_fileMap[i] = MapEntry(entry.name, std::span(s_fileData + offset, entry.size));
		offset += entry.size;
	};
}

std::span<const byte> GetFile(const char* name)
{
	auto hash = FNV(std::span<const byte>((const byte*)name, strnlen(name, 16)));
	for (uint32_t i = 0; i < g_fileTable.entryCount; i++)
	{
		if (s_fileMap[i].hash == hash)
		{
			return s_fileMap[i].data;
		}
	}

	return {};
}
