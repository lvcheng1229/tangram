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

/*struct SVariable
{
	size_t begin_pos;
	size_t end_pos;
	char before_token;
	char latter_token;
	std::string token_string;
};

struct SVariableMap
{
	std::string src_string;
	std::string dst_string;
};

const static std::string clu_code = "\
highp vec3 _a;\
if (MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceData.w != 0.0)\
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
			_756 = _755;\
		}\
	}\
	highp vec3 _923;\
	if (MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceDataEx.y != 0u)\
	{\
		uvec4 _817 = texelFetch(ps1, int(_745 + 1u));\
		uint _818 = _817.x;\
		uint _821 = _751 + MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceDataEx.z;\
		highp vec3 _823;\
		_823 = _755;\
		highp vec3 _824;\
		for (uint _826 = 0u; _826 < _818; _823 = _824, _826++)\
		{\
			uvec4 _833 = texelFetch(ps2, int(_821 + _826));\
			uint _834 = _833.x;\
			if ((Primitive_LightingChannelMask & floatBitsToUint(MobileBasePass.MobileBasePass_ClusteredShading_ViewLightSpotAngles[_834].z)) != 0u)\
			{\
				highp vec3 _855 = MobileBasePass.MobileBasePass_ClusteredShading_ViewLightPositionAndInvRadius[_834].xyz - _f;\
				highp float _856 = dot(_855, _855);\
				highp vec3 _858 = _855 * inversesqrt(_856);\
				highp float _863 = _856 * (MobileBasePass.MobileBasePass_ClusteredShading_ViewLightPositionAndInvRadius[_834].w * MobileBasePass.MobileBasePass_ClusteredShading_ViewLightPositionAndInvRadius[_834].w);\
				highp float _866 = clamp(1.0 - (_863 * _863), 0.0, 1.0);\
				highp vec3 _871 = _855 * MobileBasePass.MobileBasePass_ClusteredShading_ViewLightPositionAndInvRadius[_834].w;\
				highp vec2 _894;\
				if (MobileBasePass.MobileBasePass_ClusteredShading_SpotLightTangentAndOvalScale[_834].w != 0.0)\
				{\
					highp float _887 = abs(dot(_858, MobileBasePass.MobileBasePass_ClusteredShading_SpotLightTangentAndOvalScale[_834].xyz));\
					highp vec2 _893 = MobileBasePass.MobileBasePass_ClusteredShading_ViewLightSpotAngles[_834].xy;\
					_893.x = MobileBasePass.MobileBasePass_ClusteredShading_ViewLightSpotAngles[_834].x * (((_887 * _887) * MobileBasePass.MobileBasePass_ClusteredShading_SpotLightTangentAndOvalScale[_834].w) + 1.0);\
					_894 = _893;\
				}\
				else\
				{\
					_894 = MobileBasePass.MobileBasePass_ClusteredShading_ViewLightSpotAngles[_834].xy;\
				}\
				highp float _908 = clamp(dot(vec4(_858, -_894.x), vec4(MobileBasePass.MobileBasePass_ClusteredShading_ViewLightDirectionAndSpecularScale[_834].xyz, 1.0)) * _894.y, 0.0, 1.0);\
				highp float _909 = _908 * _908;\
				_824 = _823 + ((mix(MobileBasePass.MobileBasePass_ClusteredShading_ViewLightOuterConeColor[_834].xyz, MobileBasePass.MobileBasePass_ClusteredShading_ViewLightColorAndFalloffExponent[_834].xyz, vec3(_909)) * (((((MobileBasePass.MobileBasePass_ClusteredShading_ViewLightColorAndFalloffExponent[_834].w == 0.0) ? ((_866 * _866) * (1.0 / (_856 + 1.0))) : pow(clamp(1.0 - dot(_871, _871), 0.0, 1.0), MobileBasePass.MobileBasePass_ClusteredShading_ViewLightColorAndFalloffExponent[_834].w)) * _909) * 0.318359375) * clamp(dot(_b, _858), 0.0, 1.0))) * (_103 + (_d * abs(MobileBasePass.MobileBasePass_ClusteredShading_ViewLightDirectionAndSpecularScale[_834].w))));\
			}\
			else\
			{\
				_824 = _823;\
			}\
		}\
		_923 = _823;\
	}\
	else\
	{\
		_923 = _755;\
	}\
	_a = _923;\
}\
else\
{\
	_a = vec3(0.0);\
}";

#define NOT_FOUND_RETURN(x)\
	if (x == std::string::npos || x > clu_shading_end_pos)\
	{\
		dst_code = src_code;\
		return;\
	}\

std::string find_token(const std::string& src_str, int clu_end_pos, int offset, const std::string& before_token, const std::string& latter_token, size_t& ret_pos)
{
	size_t var_end_pos = src_str.find(latter_token, offset);
	ret_pos = var_end_pos;
	if (ret_pos == std::string::npos || ret_pos > clu_end_pos)
	{
		return std::string(" ");
	}

	size_t var_begin_pos = src_str.rfind(before_token, var_end_pos);
	ret_pos = var_begin_pos;
	if (ret_pos == std::string::npos || ret_pos > clu_end_pos)
	{
		return std::string(" ");
	}

	var_begin_pos = var_begin_pos + 1;
	std::string var_str = src_str.substr(var_begin_pos, var_end_pos - var_begin_pos);
	return var_str;
}

void replace_string(std::string& subject, const std::string& search,const std::string& replace) 
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos) 
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}

bool find_clu_begin_end_pos(const std::string& src_code, int& begine_pos, int& end_pos)
{
	size_t clu_shading_begin_pos = src_code.find("if (MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceData.w != 0.0)");

	size_t temp = 0;

	size_t point_light_var_pos = src_code.rfind("highp vec3", clu_shading_begin_pos);
	std::string point_light_var_str = find_token(src_code, point_light_var_str.length(), point_light_var_pos, " ", ";", temp);
	if (point_light_var_pos == std::string::npos)
	{
		return false;
	}

	size_t clu_shading_end_pos = src_code.rfind(point_light_var_str + " = vec3(0.0);");
	if (clu_shading_end_pos == std::string::npos)
	{
		return false;
	}

	clu_shading_end_pos = src_code.find("}", clu_shading_end_pos);
	if (clu_shading_end_pos == std::string::npos)
	{
		return false;
	}

	begine_pos = clu_shading_begin_pos;
	end_pos = clu_shading_end_pos + 1;
}

void cluster_shading_replace(char* src_code_str, int size, std::string& dst_code,bool& breplaced)
{
	std::vector<SVariableMap> code_var_map;
	
	std::string temp_str(src_code_str);
	temp_str.resize(size);

	const std::string& src_code = temp_str;

	size_t clu_shading_begin_pos = src_code.find("if (MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceData.w != 0.0)");
	if (clu_shading_begin_pos == std::string::npos)
	{
		dst_code = src_code;
		return;
	}

	size_t find_token_result;
	size_t point_light_var_pos = src_code.rfind("highp vec3", clu_shading_begin_pos);
	std::string point_light_var_str = find_token(src_code, src_code.length(), point_light_var_pos, " ", ";", find_token_result);
	if (find_token_result == std::string::npos)
	{
		dst_code = src_code;
		return;
	}

	code_var_map.push_back(SVariableMap{ point_light_var_str ,std::string("_a")});

	size_t clu_shading_end_pos = src_code.rfind(point_light_var_str);
	clu_shading_end_pos = src_code.find("}", clu_shading_end_pos) + 1;

	size_t tex_lod_pos = src_code.find("textureLod", clu_shading_begin_pos);
	size_t tex_gather_pos = src_code.find("textureGatherOffset", clu_shading_begin_pos);

	if (tex_lod_pos < clu_shading_end_pos || tex_gather_pos < tex_gather_pos)
	{
		dst_code = src_code;
		return;
	}


	size_t point_nol_var_pos = src_code.find("clamp(dot", clu_shading_begin_pos);
	NOT_FOUND_RETURN(point_nol_var_pos);

	std::string point_nol_var_str = find_token(src_code, clu_shading_end_pos,point_nol_var_pos, "(", ",", find_token_result);
	NOT_FOUND_RETURN(find_token_result);
	code_var_map.push_back(SVariableMap{ point_nol_var_str ,std::string("_b") });

	size_t diff_color_var_pos = src_code.find("inversesqrt(", point_nol_var_pos);
	NOT_FOUND_RETURN(diff_color_var_pos);

	std::string diff_color_var_str = find_token(src_code, clu_shading_end_pos, diff_color_var_pos, "* ", ");", find_token_result);
	NOT_FOUND_RETURN(find_token_result);
	code_var_map.push_back(SVariableMap{ diff_color_var_str ,std::string("_c") });

	size_t clu_end_pos = src_code.rfind(point_light_var_str);
	NOT_FOUND_RETURN(clu_end_pos);

	size_t light_color_var_pos = src_code.rfind(point_light_var_str, clu_end_pos);
	NOT_FOUND_RETURN(light_color_var_pos);

	light_color_var_pos = src_code.rfind("abs(", light_color_var_pos);
	NOT_FOUND_RETURN(light_color_var_pos);

	light_color_var_pos = src_code.rfind("+", light_color_var_pos);
	NOT_FOUND_RETURN(light_color_var_pos);

	std::string light_color_var_str = find_token(src_code, clu_shading_end_pos, light_color_var_pos, "(", " *", find_token_result);
	NOT_FOUND_RETURN(find_token_result);
	code_var_map.push_back(SVariableMap{ light_color_var_str ,std::string("_d") });

	size_t svw_var_pos = src_code.find("log2((", clu_shading_begin_pos);
	NOT_FOUND_RETURN(svw_var_pos);

	std::string svw_var_str = find_token(src_code, clu_shading_end_pos, svw_var_pos, "(", " * MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceData.x", find_token_result);
	NOT_FOUND_RETURN(find_token_result);
	code_var_map.push_back(SVariableMap{ svw_var_str ,std::string("1.0/gl_FragCoord.w") });

	size_t world_pos_var_pos = src_code.find("MobileBasePass_ClusteredShading_ViewLightPositionAndInvRadius", clu_shading_begin_pos);
	NOT_FOUND_RETURN(world_pos_var_pos);

	std::string world_pos_var_str = find_token(src_code, clu_shading_end_pos, world_pos_var_pos, " ", ";", find_token_result);
	NOT_FOUND_RETURN(find_token_result);
	code_var_map.push_back(SVariableMap{ world_pos_var_str ,std::string("_f") });

	dst_code = src_code;
	for (int idx = 0; idx < code_var_map.size(); idx++)
	{
		SVariableMap& var_map = code_var_map[idx];
		replace_string(dst_code, var_map.src_string, var_map.dst_string);
	}

	int new_begin_pos = 0;
	int new_end_pos = 0;
	bool find_result = find_clu_begin_end_pos(dst_code, new_begin_pos, new_end_pos);
	if (find_result == false)
	{
		dst_code = src_code;
		return;
	}
	dst_code.replace(dst_code.begin() + new_begin_pos, dst_code.begin() + new_end_pos, clu_code.begin(), clu_code.end());
	breplaced = true;
}*/

int main()
{
	{
		std::vector<char> dict_buffer;
		std::string shader_path = ABSOLUTE_PATH("/source/resource/ast_test.frag");
		LoadBuffer(dict_buffer, shader_path);
		int size_code = dict_buffer.size();
		ast_to_glsl((const char* const*)dict_buffer.data(), &size_code);
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
		ast_to_glsl((const char* const*)shader_code_decompressed_data.data(), &size_code);

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