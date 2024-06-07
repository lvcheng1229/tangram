#include <fstream>
#include <iostream>
#include <vector>
#include <assert.h>
#include <algorithm>
#include "zstd/zstd.h"
#include "zstd/zdict.h"
#include "tangram/tangram.h"

#define TEXT(text) TEXT_I(text)
#define TEXT_I(...) #__VA_ARGS__

#define TEXT_CAT(a, b) TEXT_CAT_I(a, b)
#define TEXT_CAT_I(a, b) a ## b

#define ABSOLUTE_PATH(x)  TEXT_CAT(TEXT(TANGRAM_DIR), x);

void LoadBuffer(std::vector<char>& buffer, const std::string& dict_file)
{
	std::ifstream in_file = std::ifstream(dict_file, std::ios::in | std::ios::binary | std::ios::ate);
	std::streamsize size = in_file.tellg();
	in_file.seekg(0, std::ios::beg);
	buffer.resize(size);
	in_file.read(buffer.data(), size);
}

void OutBuffer(const std::vector<char>& buffer, const std::string& out_file_path)
{
	std::ofstream out_file = std::ofstream(out_file_path, std::ios::out | std::ios::binary);
	out_file.write(buffer.data(), buffer.size());
	out_file.close();
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
	{
		std::vector<char> dict_buffer;
		std::string shader_path = ABSOLUTE_PATH("/source/resource/ast_test.frag");
		LoadBuffer(dict_buffer, shader_path);
		int size_code = dict_buffer.size();

		std::vector<char> out_buffer;
		out_buffer.resize(size_code);
		int out_size;
		ast_to_glsl((const char* const*)dict_buffer.data(), &size_code, out_buffer.data(), out_size);
		out_buffer.resize(out_size);
		
		std::string out_path = ABSOLUTE_PATH("/source/resource/ast_test_o.frag");
		OutBuffer(out_buffer, out_path);
		return 0;
	}
	ZSTD_DCtx* Context = ZSTD_createDCtx();
	
	std::vector<char> dict_buffer;
	std::string dict_path = ABSOLUTE_PATH("/source/resource/MSDKPushMsg.json");
	LoadBuffer(dict_buffer, dict_path);
	ZSTD_DDict* Zstd_ddict = ZSTD_createDDict(dict_buffer.data(), dict_buffer.size());

	std::string shader_path = ABSOLUTE_PATH("/source/resource/ShaderArchive-SpeedGame-GLSL_ES3_1_ANDROID.ushaderbytecode");
	std::ifstream shader_data = std::ifstream(shader_path, std::ios::in | std::ios::binary);
	CShaderArchive shader_archive;
	shader_archive.Serialize(shader_data);
	std::streamsize shader_offset = shader_data.tellg();


	std::streamsize total_size = shader_data.tellg();
	std::vector<size_t> uncompressed_samples_size;
	uncompressed_samples_size.resize(shader_archive.ShaderEntries.size());

	size_t uncompressed_size_22 = 0;
	for (int idx = 0; idx < shader_archive.ShaderEntries.size(); idx++)
	{
		FShaderCodeEntry& shader_entry = shader_archive.ShaderEntries[idx];
		uncompressed_size_22 += shader_entry.UncompressedSize;
	}

	std::vector<char> total_replaced_shader_code;
	total_replaced_shader_code.resize(uncompressed_size_22);

	//decompress shader code
	size_t global_offset = 0;
	int total_replaced_num = 0;
	for (int idx = 0; idx < shader_archive.ShaderEntries.size(); idx++)
	{
		FShaderCodeEntry& shader_entry = shader_archive.ShaderEntries[idx];
		uncompressed_samples_size[idx] = shader_entry.UncompressedSize;

		shader_data.seekg(shader_offset + shader_entry.Offset, std::ios::beg);

		std::vector<char> shader_code_data;
		shader_code_data.resize(shader_entry.Size);
		shader_data.read((char*)(shader_code_data.data()), shader_entry.Size);

		uncompressed_size_22 += shader_entry.UncompressedSize;

		if (shader_entry.Frequency != 3)
		{
			memcpy(total_replaced_shader_code.data() + global_offset, shader_code_data.data(), shader_code_data.size());
			shader_entry.Offset = global_offset;
			global_offset += shader_code_data.size();
			continue;
		}


		unsigned long long const rSize = ZSTD_getFrameContentSize(shader_code_data.data(), shader_code_data.size());
		assert(rSize != ZSTD_CONTENTSIZE_ERROR);
		assert(rSize != ZSTD_CONTENTSIZE_UNKNOWN);

		unsigned const expectedDictID = ZSTD_getDictID_fromDDict(Zstd_ddict);
		unsigned const actualDictID = ZSTD_getDictID_fromFrame(shader_code_data.data(), shader_code_data.size());
		assert(actualDictID == expectedDictID);

		std::vector<char> shader_code_decompressed_data;
		shader_code_decompressed_data.resize(shader_entry.UncompressedSize);
		size_t Code = ZSTD_decompress_usingDDict(Context, shader_code_decompressed_data.data(), shader_code_decompressed_data.size(), shader_code_data.data(), shader_code_data.size(), Zstd_ddict);

		int size_code = shader_code_decompressed_data.size();
		//ast_to_glsl((const char* const*)shader_code_decompressed_data.data(), &size_code);

		//{
		//	size_t src_code_size = shader_code_decompressed_data.size();
		//	std::vector<char> dst_code;
		//	dst_code.resize(src_code_size);
		//	size_t replaced_size = shader_code_replace(shader_code_decompressed_data.data(), shader_code_decompressed_data.size(), dst_code.data());
		//
		//	bool b_replaced = false;
		//	if (replaced_size != src_code_size)
		//	{
		//		total_replaced_num++;
		//		b_replaced = true;
		//	}
		//
		//	if (b_replaced == false)
		//	{
		//		memcpy(total_replaced_shader_code.data() + global_offset, shader_code_decompressed_data.data(), shader_code_decompressed_data.size());
		//		shader_entry.Offset = global_offset;
		//		shader_entry.UncompressedSize = shader_code_decompressed_data.size();
		//		global_offset += shader_code_decompressed_data.size();
		//	}
		//	else
		//	{
		//		int size_int = replaced_size;
		//		ast_to_glsl((const char* const*)shader_code_decompressed_data.data(), &size_int);
		//		memcpy(total_replaced_shader_code.data() + global_offset, shader_code_decompressed_data.data());
		//		memcpy(total_replaced_shader_code.data() + global_offset + sharp_pos, dst_code.data(), replaced_size);
		//		shader_entry.Offset = global_offset;
		//		shader_entry.UncompressedSize = sharp_pos + replaced_size;
		//		global_offset += (sharp_pos + replaced_size);
		//	}
		//
		//
		//}
	}
	ZSTD_freeDCtx(Context);

	constexpr int new_dict_buffer_size = 110 * 1024;

	ZDICT_params_t params;
	params.compressionLevel = 10;
	params.notificationLevel = 0;
	params.dictID = 0;

	//std::vector<char>custom_dict_buffer;
	//custom_dict_buffer.resize(new_dict_buffer_size);
	//size_t custom_dict_size = ZDICT_finalizeDictionary(custom_dict_buffer.data(), new_dict_buffer_size, clu_code.data(), clu_code.size(), total_replaced_shader_code.data(), uncompressed_samples_size.data(), uncompressed_samples_size.size(), params);
	//if (ZSTD_isError(custom_dict_size))
	//{
	//	assert(false);
	//}

	std::vector<char>new_dict_buffer;
	new_dict_buffer.resize(new_dict_buffer_size);
	size_t new_dict_size = ZDICT_trainFromBuffer(new_dict_buffer.data(), new_dict_buffer.size(), total_replaced_shader_code.data(), uncompressed_samples_size.data(), uncompressed_samples_size.size());
	if (ZSTD_isError(new_dict_size))
	{
		assert(false);
	}


	assert(new_dict_size > 0 && new_dict_size <= new_dict_buffer_size);
	//assert(custom_dict_size > 0 && custom_dict_size <= new_dict_buffer_size);

	// save dict
	std::vector<char>merged_dict_buffer;
	{
		//merged_dict_buffer.resize(new_dict_size + custom_dict_size);
		//memcpy(custom_dict_buffer.data(), custom_dict_buffer.data(), custom_dict_size);
		//memcpy(merged_dict_buffer.data() + custom_dict_size, new_dict_buffer.data(), new_dict_size);

		std::string new_dict_path = ABSOLUTE_PATH("/source/resource/new_MSDKPushMsg.json");
		std::ofstream new_dict_file = std::ofstream(new_dict_path, std::ios::out | std::ios::binary);
		//new_dict_file.write(custom_dict_buffer.data(), custom_dict_size);
		new_dict_file.write(new_dict_buffer.data(), new_dict_size);
	}

	// compress use new dict
	{
		auto Zstd_cdict = ZSTD_createCDict(merged_dict_buffer.data(), merged_dict_buffer.size(), 10);
		ZSTD_CCtx* zstd_compress_context = ZSTD_createCCtx();

		std::vector<char>new_compressed_shader;
		new_compressed_shader.resize(total_size);

		std::vector<char>total_compressed_buffer;
		size_t new_compressed_shader_offset = 0;
		for (int idx = 0; idx < shader_archive.ShaderEntries.size(); idx ++)
		{
			FShaderCodeEntry& shader_entry = shader_archive.ShaderEntries[idx];
			int uncompressed_size = shader_entry.UncompressedSize;

			size_t compressed_size = ZSTD_compressBound(uncompressed_size);
			std::vector<char>new_compressed_buffer;
			new_compressed_buffer.resize(compressed_size);

			size_t new_compressed_size = ZSTD_compress_usingCDict(zstd_compress_context, new_compressed_buffer.data(), compressed_size, total_replaced_shader_code.data() + shader_entry.Offset, shader_entry.UncompressedSize, Zstd_cdict);
			memcpy(new_compressed_shader.data(), new_compressed_buffer.data(), new_compressed_size);
			shader_entry.Offset = new_compressed_shader_offset;
			new_compressed_shader_offset += new_compressed_size;
		}
		std::cout << new_compressed_shader_offset;
	}

	return 0;
}