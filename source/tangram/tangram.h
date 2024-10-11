#pragma once
size_t shader_code_replace(const void* src_code, int size, char* dst_code);
size_t replace_code_block(char* src_code, int size, char* replace_from, int replace_from_size, char* replace_to, int replace_to_size);

void init_ast_to_glsl();
void finish_ast_to_glsl();
bool ast_to_glsl(const char* const* s, const int* l, char* compiled_buffer, int& compiled_size);

void initAstToHashTree();
void finalizeAstToHashTree();
bool ast_to_hash_treel(const char* const* shaderStrings, const int* shaderLengths, int shader_id);