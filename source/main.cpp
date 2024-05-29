#include <fstream>
#include <vector>
#include <assert.h>
#include "zstd/zstd.h"

void LoadBuffer(std::vector<char>& buffer, const std::string& dict_file)
{
	std::ifstream in_file = std::ifstream(dict_file, std::ios::in | std::ios::binary | std::ios::ate);
	std::streamsize size = in_file.tellg();
	in_file.seekg(0, std::ios::beg);
	buffer.resize(size);
	in_file.read(buffer.data(), size);
}


template<typename T, int byte_num>
void ReadUValue(std::ifstream& in_file, T& value)
{
	in_file.read((char*)&value, sizeof(T));
}

void ReadByte(std::ifstream& in_file, uint8_t& value)
{
	in_file.read((char*)&value, sizeof(uint8_t));
}

struct FSHAHash
{
	uint8_t Hash[20];
};

struct FShaderMapEntry
{
	uint32_t ShaderIndicesOffset;
	uint32_t NumShaders;
	uint32_t FirstPreloadIndex;
	uint32_t NumPreloadEntries;

	void Serialize(std::ifstream& in_file)
	{
		ReadUValue<uint32_t, 4>(in_file, ShaderIndicesOffset);
		ReadUValue<uint32_t, 4>(in_file, NumShaders);
		ReadUValue<uint32_t, 4>(in_file, FirstPreloadIndex);
		ReadUValue<uint32_t, 4>(in_file, NumPreloadEntries);
	}
};

struct FShaderCodeEntry
{
	uint64_t Offset;
	uint32_t Size;
	uint32_t UncompressedSize;
	uint8_t Frequency;

	void Serialize(std::ifstream& in_file)
	{
		ReadUValue<uint64_t, 8>(in_file, Offset);
		ReadUValue<uint32_t, 4>(in_file, Size);
		ReadUValue<uint32_t, 4>(in_file, UncompressedSize);
		ReadByte(in_file, Frequency);
	}
};

struct FFileCachePreloadEntry
{
	uint64_t Offset;
	uint64_t Size;
	void Serialize(std::ifstream& in_file)
	{
		ReadUValue<uint64_t, 8>(in_file, Offset);
		ReadUValue<uint64_t, 8>(in_file, Size);
	}
};

template<typename T>
void ReadArray(std::ifstream& in_file, std::vector<T>& out_array)
{
	uint32_t array_size;
	in_file.read((char*)&array_size, sizeof(uint32_t));
	out_array.resize(array_size);
	for (uint32_t idx = 0; idx < array_size; idx++)
	{
		in_file.read((char*)(((T*)out_array.data()) + idx), sizeof(T));
	}
}

template<typename T>
void SerializeArray(std::ifstream& in_file, std::vector<T>& out_array)
{
	uint32_t array_size;
	in_file.read((char*)&array_size, sizeof(uint32_t));
	out_array.resize(array_size);
	for (uint32_t idx = 0; idx < array_size; idx++)
	{
		out_array[idx].Serialize(in_file);
	}
}

struct CShaderArchive
{
	uint32_t GShaderCodeArchiveVersion;
	std::vector<FSHAHash>ShaderMapHashes;
	std::vector<FSHAHash>ShaderHashes;
	std::vector<FShaderMapEntry>ShaderMapEntries;
	std::vector<FShaderCodeEntry>ShaderEntries;
	std::vector<FFileCachePreloadEntry>PreloadEntries;
	std::vector<uint32_t>ShaderIndices;

	void Serialize(std::ifstream& in_file)
	{
		in_file.read((char*)(&GShaderCodeArchiveVersion), sizeof(int));
		ReadArray(in_file, ShaderMapHashes);
		ReadArray(in_file, ShaderHashes);
		SerializeArray(in_file, ShaderMapEntries);
		SerializeArray(in_file, ShaderEntries);
		SerializeArray(in_file, PreloadEntries);
		ReadArray(in_file, ShaderIndices);
	}
};

int main()
{
	ZSTD_DCtx* Context = ZSTD_createDCtx();
	
	std::vector<char> dict_buffer;
	LoadBuffer(dict_buffer, std::string("H:/TanGram/tangram/source/resource/MSDKPushMsg.json"));
	ZSTD_DDict* Zstd_ddict = ZSTD_createDDict(dict_buffer.data(), dict_buffer.size());

	std::ifstream shader_data = std::ifstream(std::string("H:/TanGram/tangram/source/resource/ShaderArchive-SpeedGame-GLSL_ES3_1_ANDROID.ushaderbytecode"), std::ios::in | std::ios::binary);
	CShaderArchive shader_archive;
	shader_archive.Serialize(shader_data);
	std::streamsize shader_offset = shader_data.tellg();
	for (int idx = 0; idx < shader_archive.ShaderEntries.size(); idx+=15)
	{
		FShaderCodeEntry& shader_entry = shader_archive.ShaderEntries[idx];
		
		shader_data.seekg(shader_offset + shader_entry.Offset, std::ios::beg);

		// read zstd shader data
		std::vector<char> shader_code_data;
		shader_code_data.resize(shader_entry.Size);
		shader_data.read((char*)(shader_code_data.data()), shader_entry.Size);

		// check file
		unsigned long long const rSize = ZSTD_getFrameContentSize(shader_code_data.data(), shader_code_data.size());
		assert(rSize != ZSTD_CONTENTSIZE_ERROR);
		assert(rSize != ZSTD_CONTENTSIZE_UNKNOWN);

		// check file
		unsigned const expectedDictID = ZSTD_getDictID_fromDDict(Zstd_ddict);
		unsigned const actualDictID = ZSTD_getDictID_fromFrame(shader_code_data.data(), shader_code_data.size());
		assert(actualDictID == expectedDictID, "DictID mismatch: expected %u got %u", expectedDictID, actualDictID);

		std::vector<char> shader_code_decompressed_data;
		shader_code_decompressed_data.resize(shader_entry.UncompressedSize);
		size_t Code = ZSTD_decompress_usingDDict(Context, shader_code_decompressed_data.data(), shader_code_decompressed_data.size(), shader_code_data.data(), shader_code_data.size(), Zstd_ddict);
	}



	//std::vector<char> decompressed_shader;
	//decompressed_shader.resize(shader_data.size() * 15);
	//
	//ZSTD_DCtx* Context = ZSTD_createDCtx();
	//size_t Code = ZSTD_decompress_usingDDict(Context, decompressed_shader.data(), decompressed_shader.size(), shader_data.data(), shader_data.size(), Zstd_ddict);
	ZSTD_freeDCtx(Context);
	return 0;
}