#include <fstream>
#include <iostream>
#include <vector>
#include <assert.h>
#include <algorithm>
#include "zstd/zstd.h"
#include "zstd/zdict.h"
#include "tangram/tangram.h"

#include "tangram/glslang_headers.h"

#define TEXT(text) TEXT_I(text)
#define TEXT_I(...) #__VA_ARGS__

#define TEXT_CAT(a, b) TEXT_CAT_I(a, b)
#define TEXT_CAT_I(a, b) a ## b

#define ABSOLUTE_PATH(x)  TEXT_CAT(TEXT(TANGRAM_DIR), x);

static char cluster_shading_code[4753] = "\
if (MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceData.w != 0.0)\n\
{\
	uvec2 _734 = uvec2(gl_FragCoord.xy) >> (uvec2(MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceDataEx.x) & uvec2(31u));\
	uint _744 = (((min(uint(max(0.0, log2((1.0 / gl_FragCoord.w * MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceData.x) + MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceData.y) * MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceData.z)), (MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryData.w - 1u)) * MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryData.y) + _734.y) * MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryData.x) + _734.x;\
	uint _745 = _744 * 2u;\
	uvec4 _747 = texelFetch(ps1, int(_745));\
	uint _748 = _747.x;\
	uint _751 = _744 * MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryData.z;\
	highp vec3 _755;\
	_755 = vec3(0.0);\
	highp vec3 _756;\
	for (uint _758 = 0u; _758 < _748; _755 = _756, _758++)\
	{\
		uint _767 = texelFetch(ps2, int(_751 + _758)).x * 3u;\
		if ((Primitive_LightingChannelMask & floatBitsToUint(texelFetch(ps0, int(_767)).w)) != 0u)\
		{\
			highp vec4 _778 = texelFetch(ps0, int(_767 + 1u));\
			highp vec4 _780 = texelFetch(ps0, int(_767 + 2u));\
			highp vec3 _782 = texelFetch(ps0, int(_767)).xyz - _f;\
			highp float _783 = dot(_782, _782);\
			highp float _788 = _778.w;\
			highp float _790 = _783 * (_788 * _788);\
			highp float _793 = clamp(1.0 - (_790 * _790), 0.0, 1.0);\
			highp vec3 _798 = _782 * _788;\
			highp float _799 = _780.w;\
			_756 = _755 + ((_778.xyz * (((_799 == 0.0) ? ((_793 * _793) * (1.0 / (_783 + 1.0))) : pow(clamp(1.0 - dot(_798, _798), 0.0, 1.0), _799)) * clamp(dot(_b, _782 * inversesqrt(_783)), 0.0, 1.0))) * _c);\
		}\
		else\
		{\
			_756 = _755;\n\
		}\
	}\
	highp vec3 _923;\
	if (MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceDataEx.y != 0u)\n\
	{\
		uvec4 _817 = texelFetch(ps1, int(_745 + 1u));\
		uint _818 = _817.x;\
		uint _821 = _751 + MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceDataEx.z;\
		highp vec3 _823;\
		_823 = _755;\
		highp vec3 _824;\
		for (uint _826 = 0u; _826 < _818; _823 = _824, _826++)\
		{\
			uvec4 _833 = texelFetch(ps2, int(_821 + _826));\n\
			uint _834 = _833.x;\
			if((Primitive_LightingChannelMask & floatBitsToUint(MobileBasePass.MobileBasePass_ClusteredShading_ViewLightSpotAngles[_834].z)) != 0u)\n\
			{\
				highp vec3 _855 = MobileBasePass.MobileBasePass_ClusteredShading_ViewLightPositionAndInvRadius[_834].xyz - _f;\
				highp float _856 = dot(_855, _855);\
				highp vec3 _858 = _855 * inversesqrt(_856);\
				highp float _863 = _856 * (MobileBasePass.MobileBasePass_ClusteredShading_ViewLightPositionAndInvRadius[_834].w * MobileBasePass.MobileBasePass_ClusteredShading_ViewLightPositionAndInvRadius[_834].w);\n\
				highp float _866 = clamp(1.0 - (_863 * _863), 0.0, 1.0);\
				highp vec3 _871 = _855 * MobileBasePass.MobileBasePass_ClusteredShading_ViewLightPositionAndInvRadius[_834].w;\
				highp vec2 _894;\
				if (MobileBasePass.MobileBasePass_ClusteredShading_SpotLightTangentAndOvalScale[_834].w != 0.0)\
				{\
					highp float _887 = abs(dot(_858, MobileBasePass.MobileBasePass_ClusteredShading_SpotLightTangentAndOvalScale[_834].xyz));\n\
					highp vec2 _893 = MobileBasePass.MobileBasePass_ClusteredShading_ViewLightSpotAngles[_834].xy;\
					_893.x = MobileBasePass.MobileBasePass_ClusteredShading_ViewLightSpotAngles[_834].x * (((_887 * _887) * MobileBasePass.MobileBasePass_ClusteredShading_SpotLightTangentAndOvalScale[_834].w) + 1.0);\
					_894 = _893;\
				}\
				else\
				{\
					_894 = MobileBasePass.MobileBasePass_ClusteredShading_ViewLightSpotAngles[_834].xy;\n\
				}\
				highp float _908 = clamp(dot(vec4(_858, -_894.x), vec4(MobileBasePass.MobileBasePass_ClusteredShading_ViewLightDirectionAndSpecularScale[_834].xyz, 1.0)) * _894.y, 0.0, 1.0);\
				highp float _909 = _908 * _908;\n\
				_824 = _823 + ((mix(MobileBasePass.MobileBasePass_ClusteredShading_ViewLightOuterConeColor[_834].xyz, MobileBasePass.MobileBasePass_ClusteredShading_ViewLightColorAndFalloffExponent[_834].xyz, vec3(_909)) * (((((MobileBasePass.MobileBasePass_ClusteredShading_ViewLightColorAndFalloffExponent[_834].w == 0.0) ? ((_866 * _866) * (1.0 / (_856 + 1.0))) : pow(clamp(1.0 - dot(_871, _871), 0.0, 1.0), MobileBasePass.MobileBasePass_ClusteredShading_ViewLightColorAndFalloffExponent[_834].w)) * _909) * 0.318359375) * clamp(dot(_b, _858), 0.0, 1.0))) * (_c + (_d * abs(MobileBasePass.MobileBasePass_ClusteredShading_ViewLightDirectionAndSpecularScale[_834].w))));\
			}\
			else\
			{\
				_824 = _823;\n\
			}\
		}\
		_923 = _823;\
	}\
	else\
	{\
		_923 = _755;\n\
	}\
	_a = _923;\
}\
else\
{\
	_a = vec3(0.0);\
} ";

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

void MannualCodeBlockGenTest()
{
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

	glslang::InitializeProcess();

	int success_num = 0;

	for (int idx = 0; idx < shader_archive.ShaderEntries.size(); idx++)
	{
		FShaderCodeEntry& shader_entry = shader_archive.ShaderEntries[idx];
		shader_data.seekg(shader_offset + shader_entry.Offset, std::ios::beg);

		std::vector<char> shader_code_data;
		shader_code_data.resize(shader_entry.Size);
		shader_data.read((char*)(shader_code_data.data()), shader_entry.Size);

		if (shader_entry.Frequency != 3)
		{
			continue;
		}

		std::vector<char> shader_code_decompressed_data;
		shader_code_decompressed_data.resize(shader_entry.UncompressedSize);
		size_t Code = ZSTD_decompress_usingDDict(Context, shader_code_decompressed_data.data(), shader_code_decompressed_data.size(), shader_code_data.data(), shader_code_data.size(), Zstd_ddict);

		size_t src_code_size = shader_code_decompressed_data.size();
		std::vector<char> dst_code;
		dst_code.resize(src_code_size);
		size_t replaced_size = shader_code_replace(shader_code_decompressed_data.data(), shader_code_decompressed_data.size(), dst_code.data());

		bool b_replaced = false;
		if (replaced_size != src_code_size)
		{
			char magic_num[11] = { 'i','n', 't', ' ', 'c', 'l',  '=', '4' , '2',';','\0' };
			size_t new_shader_size = replace_code_block(dst_code.data(), replaced_size, magic_num, 10, cluster_shading_code, 4752);

			int sharp_pos = 0;
			for (int idx = 0; idx < new_shader_size; idx++)
			{
				if (dst_code[idx] == '#')
				{
					if (dst_code[idx + 1] == 'v')
					{
						sharp_pos = idx;
						break;
					}
				}
			}

			int code_main_size = new_shader_size - sharp_pos;
			if (code_main_size < 0)
			{
				assert(false);
			}

			glslang::TShader shader(EShLangFragment);
			{
				const int defaultVersion = 100;
				const EProfile defaultProfile = ENoProfile;
				const bool forceVersionProfile = false;
				const bool isForwardCompatible = false;

				EShMessages messages = EShMsgCascadingErrors;
				messages = static_cast<EShMessages>(messages | EShMsgAST);
				messages = static_cast<EShMessages>(messages | EShMsgHlslLegalization);

				const char* debug_strings = shader_code_decompressed_data.data() + sharp_pos;

				char* shader_strings = dst_code.data() + sharp_pos;


				int reesrve_index = code_main_size - 1;
				for (; reesrve_index > 0; reesrve_index--)
				{
					char pre_char = shader_strings[reesrve_index - 1];
					char cur_char = shader_strings[reesrve_index];
					if (pre_char != '\0' && cur_char == '\0')
					{
						shader_strings[reesrve_index] = char(- 1);
					}
				}

				const int shader_lengths = static_cast<int>(reesrve_index + 1);
				shader.setStringsWithLengths(&shader_strings, &shader_lengths, 1);
				shader.setEntryPoint("main");
				bool success = shader.parse(GetDefaultResources(), defaultVersion, isForwardCompatible, messages);

				const char* info_log = shader.getInfoLog();

				if (success)
				{
					success_num++;
				}
				assert(success == true);
			}
		}
	}

	glslang::FinalizeProcess();
}

void TestSingleAstToGL()
{
	init_ast_to_glsl();
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

	finish_ast_to_glsl();
}

void TestGlobalASTToGL()
{
	init_ast_to_glsl();

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

	glslang::InitializeProcess();

	int success_num = 0;

	for (int idx = 0; idx < shader_archive.ShaderEntries.size(); idx++)
	{
		FShaderCodeEntry& shader_entry = shader_archive.ShaderEntries[idx];
		shader_data.seekg(shader_offset + shader_entry.Offset, std::ios::beg);

		std::vector<char> shader_code_data;
		shader_code_data.resize(shader_entry.Size);
		shader_data.read((char*)(shader_code_data.data()), shader_entry.Size);

		if (shader_entry.Frequency != 3)
		{
			continue;
		}

		std::vector<char> shader_code_decompressed_data;
		shader_code_decompressed_data.resize(shader_entry.UncompressedSize);
		size_t Code = ZSTD_decompress_usingDDict(Context, shader_code_decompressed_data.data(), shader_code_decompressed_data.size(), shader_code_data.data(), shader_code_data.size(), Zstd_ddict);

		{
			int code_size = Code;
			std::vector<char> out_buffer;
			out_buffer.resize(Code);
			int out_size;
			bool result = ast_to_glsl((const char* const*)(shader_code_decompressed_data.data()), &code_size, out_buffer.data(), out_size);
		}
	}

	finish_ast_to_glsl();
};

int main()
{
	//{
	//	MannualCodeBlockGenTest();
	//	return 0;
	//}
	//
	//{
	//	TestSingleAstToGL();
	//	return 0;
	//}

	{
		TestGlobalASTToGL();
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