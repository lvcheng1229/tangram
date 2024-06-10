#include <vector>
#include <assert.h>
#include <algorithm>
#include <string>

#include "tangram.h"


#define NOT_FOUND_RETURN(x)\
	if (x == std::string::npos || x > clu_shading_end_pos)\
	{\
		return size;\
	}\

struct SVariable
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

void replace_string(std::string& subject, const std::string& search, const std::string& replace)
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
	std::string point_light_var_str = find_token(src_code, src_code.length(), point_light_var_pos, " ", ";", temp);
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
	return true;
}


size_t shader_code_replace(const void* src_code_str, int size, char* dst_code)
{
	std::vector<SVariableMap> code_var_map;

	int sharp_pos = 0;
	for (int idx = 0; idx < size; idx++)
	{
		if (((char*)src_code_str)[idx] == '#')
		{
			sharp_pos = idx;
			break;
		}
	}

	int code_main_size = size - sharp_pos;
	if (code_main_size < 0)
	{
		return size;
	}

	std::string temp_str(((const char*)src_code_str) + sharp_pos);
	temp_str.resize(code_main_size);

	const std::string& src_code = temp_str;

	size_t clu_shading_begin_pos = src_code.find("if (MobileBasePass.MobileBasePass_ClusteredShading_CulledLightNecessaryDepthSliceData.w != 0.0)");
	if (clu_shading_begin_pos == std::string::npos)
	{
		return size;
	}

	size_t find_token_result;
	size_t point_light_var_pos = src_code.rfind("highp vec3", clu_shading_begin_pos);
	std::string point_light_var_str = find_token(src_code, src_code.length(), point_light_var_pos, " ", ";", find_token_result);
	if (find_token_result == std::string::npos)
	{
		return size;
	}

	code_var_map.push_back(SVariableMap{ point_light_var_str ,std::string("_a") });

	size_t clu_shading_end_pos = src_code.rfind(point_light_var_str);
	clu_shading_end_pos = src_code.find("}", clu_shading_end_pos) + 1;

	size_t tex_lod_pos = src_code.find("textureLod", clu_shading_begin_pos);
	size_t tex_gather_pos = src_code.find("textureGatherOffset", clu_shading_begin_pos);

	if (tex_lod_pos < clu_shading_end_pos || tex_gather_pos < tex_gather_pos)
	{
		return size;
	}

	size_t spot_light_oval_pos = src_code.find("SpotLightTangentAndOvalScale", clu_shading_begin_pos);
	NOT_FOUND_RETURN(spot_light_oval_pos);

	size_t point_nol_var_pos = src_code.find("clamp(dot", clu_shading_begin_pos);
	NOT_FOUND_RETURN(point_nol_var_pos);

	std::string point_nol_var_str = find_token(src_code, clu_shading_end_pos, point_nol_var_pos, "(", ",", find_token_result);
	NOT_FOUND_RETURN(find_token_result);
	code_var_map.push_back(SVariableMap{ point_nol_var_str ,std::string("_b") });

	size_t diff_color_var_pos = src_code.find("inversesqrt(", point_nol_var_pos);
	NOT_FOUND_RETURN(diff_color_var_pos);

	std::string diff_color_var_str = find_token(src_code, clu_shading_end_pos, diff_color_var_pos, "* ", ");", find_token_result);
	NOT_FOUND_RETURN(find_token_result);
	code_var_map.push_back(SVariableMap{ diff_color_var_str.substr(1,diff_color_var_str.length() - 1) ,std::string("_c") });

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
	//code_var_map.push_back(SVariableMap{ svw_var_str ,std::string("1.0/gl_FragCoord.w") });

	size_t world_pos_var_pos = src_code.find("MobileBasePass_ClusteredShading_ViewLightPositionAndInvRadius", clu_shading_begin_pos);
	NOT_FOUND_RETURN(world_pos_var_pos);

	std::string world_pos_var_str = find_token(src_code, clu_shading_end_pos, world_pos_var_pos, " ", ";", find_token_result);
	NOT_FOUND_RETURN(find_token_result);
	code_var_map.push_back(SVariableMap{ world_pos_var_str ,std::string("_f") });

	for (int idx = 0; idx < code_var_map.size(); idx++)
	{
		SVariableMap& var_map = code_var_map[idx];
		if (var_map.src_string[0] != char('_'))
		{
			return size;
		}

		size_t var_pos = temp_str.find(var_map.src_string);
		if (var_pos > clu_shading_begin_pos)
		{
			return size;
		}
	}

	size_t  sub_svw_var_pos = temp_str.find(svw_var_str, clu_shading_begin_pos);
	if (sub_svw_var_pos != std::string::npos)
	{
		std::string frag_coord_w("1.0/gl_FragCoord.w");
		temp_str.replace(temp_str.begin() + sub_svw_var_pos, temp_str.begin() + sub_svw_var_pos + svw_var_str.size(), frag_coord_w.data(), frag_coord_w.size());
	}



	for (int idx = 0; idx < code_var_map.size(); idx++)
	{
		SVariableMap& var_map = code_var_map[idx];
		replace_string(temp_str, var_map.src_string, var_map.dst_string);
	}

	int new_begin_pos = 0;
	int new_end_pos = 0;
	bool find_result = find_clu_begin_end_pos(temp_str, new_begin_pos, new_end_pos);
	if (find_result == false)
	{
		return size;
	}

	const char magic_num[10] = { 'i','n', 't', ' ', 'c', 'l',  '=', '4' , '2',';' };
	temp_str.replace(temp_str.begin() + new_begin_pos, temp_str.begin() + new_end_pos, magic_num, 10);
	memcpy(dst_code, src_code_str, sharp_pos);
	memcpy(dst_code + sharp_pos, temp_str.data(), temp_str.size());
	return temp_str.size() + sharp_pos;
}

size_t replace_code_block(char* src_code, int size, char* replace_from, int replace_from_size, char* replace_to, int replace_to_size)
{
	int sharp_pos = 0;
	for (int idx = 0; idx < size; idx++)
	{
		if (((const char*)src_code)[idx] == '#')
		{
			if (((const char*)src_code)[idx + 1] == 'v')
			{
				sharp_pos = idx;
				break;
			}
		}
	}

	int code_main_size = size - sharp_pos;
	if (code_main_size < 0)
	{
		return size;
	}

	std::string replace_from_str(((const char*)replace_from), replace_from_size);
	const char* find_pos = std::strstr(((const char*)src_code + sharp_pos), (const char*)replace_from);

	if (find_pos != nullptr)
	{
		int find_pos_index = (find_pos - src_code);
		std::string code_str(((const char*)src_code) + find_pos_index, size - find_pos_index);
		code_str.replace(0, replace_from_str.length(), (char*)replace_to, replace_to_size);
		memcpy(src_code + find_pos_index, code_str.data(), code_str.size());
		size = size - replace_from_size + replace_to_size;
#if 0
		std::string debug_str(((const char*)src_code) + sharp_pos, size - sharp_pos);
#endif
	}
	else
	{
		return size;
	}

	return size;
}
