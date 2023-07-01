//
//  c_effect.h
//  ReShade
//
//  Created by Marc-Aurel Zent on 21.06.23.
//

#ifndef c_effect_h
#define c_effect_h

#include <stdint.h>
#include <stdlib.h>

#if defined __cplusplus
#include "effect_module.hpp"
struct module *build_module(const reshadefx::module &module_cpp);
#endif
void free_module(struct module *module_c);


typedef enum {
    t_void,
    t_bool,
    t_min16int,
    t_int,
    t_min16uint,
    t_uint,
    t_min16float,
    t_float,
    t_string,
    t_struct,
    t_sampler,
    t_storage,
    t_texture,
    t_function,
} datatype;

typedef enum {
    q_extern = 1 << 0,
    q_static = 1 << 1,
    q_uniform = 1 << 2,
    q_volatile = 1 << 3,
    q_precise = 1 << 4,
    q_groupshared = 1 << 14,
    q_in = 1 << 5,
    q_out = 1 << 6,
    q_inout = q_in | q_out,
    q_const = 1 << 8,
    q_linear = 1 << 10,
    q_noperspective = 1 << 11,
    q_centroid = 1 << 12,
    q_nointerpolation = 1 << 13,
} qualifier;

struct type {
    datatype base;
    unsigned int rows;
    unsigned int cols;
    unsigned int qualifiers;
    int array_length;
    uint32_t definition;
    char* description;
};

struct constant {
    union {
        float as_float[16];
        int32_t as_int[16];
        uint32_t as_uint[16];
    };
    char* string_data;
    struct constant** array_data;
    size_t array_size;
};

typedef enum {
    op_cast,
    op_member,
    op_dynamic_index,
    op_constant_index,
    op_swizzle,
} operation_type;

struct operation {
    operation_type op;
    struct type *from, *to;
    uint32_t index;
    signed char swizzle[4];
};

struct expression {
    uint32_t base;
    struct type *expression_type;
    struct constant *constant_value;
    bool is_lvalue;
    bool is_constant;
    struct operation **operations_chain;
    size_t operations_chain_size;
};

typedef enum
{
    texture_format_unknown,

    texture_format_r8,
    texture_format_r16,
    texture_format_r16f,
    texture_format_r32f,
    texture_format_rg8,
    texture_format_rg16,
    texture_format_rg16f,
    texture_format_rg32f,
    texture_format_rgba8,
    texture_format_rgba16,
    texture_format_rgba16f,
    texture_format_rgba32f,
    texture_format_rgb10a2,
} texture_format;

typedef enum
{
    filter_mode_min_mag_mip_point = 0,
    filter_mode_min_mag_point_mip_linear = 0x1,
    filter_mode_min_point_mag_linear_mip_point = 0x4,
    filter_mode_min_point_mag_mip_linear = 0x5,
    filter_mode_min_linear_mag_mip_point = 0x10,
    filter_mode_min_linear_mag_point_mip_linear = 0x11,
    filter_mode_min_mag_linear_mip_point = 0x14,
    filter_mode_min_mag_mip_linear = 0x15
} filter_mode;

typedef enum
{
    texture_address_mode_wrap = 1,
    texture_address_mode_mirror,
    texture_address_mode_clamp,
    texture_address_mode_border,
} texture_address_mode;

typedef enum
{
    pass_blend_op_add = 1,
    pass_blend_op_subtract,
    pass_blend_op_rev_subtract,
    pass_blend_op_min,
    pass_blend_op_max,
} pass_blend_op;

typedef enum
{
    pass_blend_func_zero = 0,
    pass_blend_func_one = 1,
    pass_blend_func_src_color,
    pass_blend_func_src_alpha,
    pass_blend_func_inv_src_color,
    pass_blend_func_inv_src_alpha,
    pass_blend_func_dst_color,
    pass_blend_func_dst_alpha,
    pass_blend_func_inv_dst_color,
    pass_blend_func_inv_dst_alpha,
} pass_blend_func;

typedef enum
{
    pass_stencil_op_zero,
    pass_stencil_op_keep,
    pass_stencil_op_invert,
    pass_stencil_op_replace,
    pass_stencil_op_incr,
    pass_stencil_op_incr_sat,
    pass_stencil_op_decr,
    pass_stencil_op_decr_sat,
} pass_stencil_op;

typedef enum
{
    pass_stencil_func_never,
    pass_stencil_func_equal,
    pass_stencil_func_not_equal,
    pass_stencil_func_less,
    pass_stencil_func_less_equal,
    pass_stencil_func_greater,
    pass_stencil_func_greater_equal,
    pass_stencil_func_always,
} pass_stencil_func;

typedef enum
{
    primitive_topology_point_list = 1,
    primitive_topology_line_list,
    primitive_topology_line_strip,
    primitive_topology_triangle_list,
    primitive_topology_triangle_strip,
} primitive_topology;

struct struct_member_info_
{
    struct type* type;
    char* name;
    char* semantic;
    uint32_t definition;
};

struct struct_info
{
    char *name;
    char *unique_name;
    struct struct_member_info_ **member_list;
    size_t member_list_size;
    uint32_t definition;
};

struct annotation
{
    struct type *type;
    char *name;
    struct constant *value;
};

struct texture_info
{
    uint32_t id;
    uint32_t binding;
    char *name;
    char *semantic;
    char *unique_name;
    struct annotation **annotations;
    size_t annotations_size;
    uint32_t width;
    uint32_t height;
    uint16_t levels;
    texture_format format;
    bool render_target;
    bool storage_access;
};

struct sampler_info
{
    uint32_t id;
    uint32_t binding;
    uint32_t texture_binding;
    char *name;
    char *unique_name;
    char *texture_name;
    struct annotation **annotations;
    size_t annotations_size;
    filter_mode filter;
    texture_address_mode address_u;
    texture_address_mode address_v;
    texture_address_mode address_w;
    float min_lod;
    float max_lod;
    float lod_bias;
    uint8_t srgb;
};

struct storage_info
{
    uint32_t id;
    uint32_t binding;
    char *name;
    char *unique_name;
    char *texture_name;
    texture_format format;
    uint16_t level;
};

struct uniform_info
{
    char *name;
    struct type *type;
    uint32_t size;
    uint32_t offset;
    struct annotation **annotations;
    size_t annotations_size;
    bool has_initializer_value;
    struct constant* initializer_value;
};

typedef enum
{
    shader_type_vs,
    shader_type_ps,
    shader_type_cs,
} shader_type;

struct entry_point
{
    char *name;
    shader_type type;
};

struct function_info
{
    uint32_t definition;
    char *name;
    char *unique_name;
    struct type *return_type;
    char *return_semantic;
    struct struct_member_info_ **parameter_list;
    size_t parameter_list_size;
    uint32_t *referenced_samplers;
    size_t referenced_samplers_size;
    uint32_t *referenced_storages;
    size_t referenced_storages_size;
};

struct pass_info
{
    char *name;
    char *render_target_names[8];
    char *vs_entry_point;
    char *ps_entry_point;
    char *cs_entry_point;
    uint8_t generate_mipmaps;
    uint8_t clear_render_targets;
    uint8_t srgb_write_enable;
    uint8_t blend_enable[8];
    uint8_t stencil_enable;
    uint8_t color_write_mask[8];
    uint8_t stencil_read_mask;
    uint8_t stencil_write_mask;
    pass_blend_op blend_op[8];
    pass_blend_op blend_op_alpha[8];
    pass_blend_func src_blend[8];
    pass_blend_func dest_blend[8];
    pass_blend_func src_blend_alpha[8];
    pass_blend_func dest_blend_alpha[8];
    pass_stencil_func stencil_comparison_func;
    uint32_t stencil_reference_value;
    pass_stencil_op stencil_op_pass;
    pass_stencil_op stencil_op_fail;
    pass_stencil_op stencil_op_depth_fail;
    uint32_t num_vertices;
    primitive_topology topology;
    uint32_t viewport_width;
    uint32_t viewport_height;
    uint32_t viewport_dispatch_z;
    struct sampler_info **samplers;
    size_t samplers_size;
    struct storage_info **storages;
    size_t storages_size;
};

struct technique_info
{
    char *name;
    struct pass_info **passes;
    size_t passes_size;
    struct annotation **annotations;
    size_t annotations_size;
};

struct module
{
    char *code;
    size_t code_size;

    struct entry_point **entry_points;
    size_t entry_points_size;
    struct texture_info **textures;
    size_t textures_size;
    struct sampler_info **samplers;
    size_t samplers_size;
    struct storage_info **storages;
    size_t storages_size;
    struct uniform_info **uniforms, **spec_constants;
    size_t uniforms_size, spec_constants_size;
    struct technique_info **techniques;
    size_t techniques_size;

    uint32_t total_uniform_size;
    uint32_t num_texture_bindings;
    uint32_t num_sampler_bindings;
    uint32_t num_storage_bindings;
};

#endif /* c_effect_h */
