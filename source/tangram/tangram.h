size_t shader_code_replace(const void* src_code, int size, char* dst_code);
size_t replace_code_block(char* src_code, int size, char* replace_from, int replace_from_size, char* replace_to, int replace_to_size);
void ast_to_glsl(const char* const* s, const int* l);