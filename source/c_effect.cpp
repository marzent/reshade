//
//  c_effect.cpp
//  Aetherium.Boot
//
//  Created by Marc-Aurel Zent on 21.06.23.
//

#include <algorithm>
#include "c_effect.h"

struct type *build_type(const reshadefx::type &type_cpp) {
    struct type *type_c = (struct type *)malloc(sizeof(struct type));
    if (!type_c) return NULL;

    type_c->base = static_cast<datatype>(type_cpp.base);
    type_c->rows = type_cpp.rows;
    type_c->cols = type_cpp.cols;
    type_c->qualifiers = type_cpp.qualifiers;
    type_c->array_length = type_cpp.array_length;
    type_c->definition = type_cpp.definition;
    
    return type_c;
}

void free_type(struct type *type_c) {
    if (type_c) free(type_c);
}

struct constant *build_constant(const reshadefx::constant &constant_cpp) {
    struct constant *constant_c = (struct constant *)malloc(sizeof(struct constant));
    if (!constant_c) return NULL;
    
    memcpy(constant_c->as_int, constant_cpp.as_int, sizeof(int) * 16);
    constant_c->string_data = strdup(constant_cpp.string_data.c_str());
    if (!constant_c->string_data) {
        free(constant_c);
        return NULL;
    }
    constant_c->array_data = (struct constant **)malloc(sizeof(struct constant *) * constant_cpp.array_data.size());
    if (!constant_c->array_data) {
        free(constant_c->string_data);
        free(constant_c);
        return NULL;
    }
    for (int i = 0; i < constant_cpp.array_data.size(); i++)
        constant_c->array_data[i] =  build_constant(constant_cpp.array_data[i]);
    constant_c->array_size = constant_cpp.array_data.size();
    return constant_c;
}

void free_constant(struct constant *constant_c) {
    if (!constant_c) return;
    free(constant_c->string_data);
    for (int i = 0; i < constant_c->array_size; i++)
        free_constant(constant_c->array_data[i]);
    free(constant_c->array_data);
    free(constant_c);
}

struct operation *build_operation(const reshadefx::expression::operation &operation_cpp) {
    struct operation *operation_c = (struct operation *)malloc(sizeof(struct operation));
    if (!operation_c) return NULL;
    
    operation_c->op = static_cast<operation_type>(operation_cpp.op);
    operation_c->from = build_type(operation_cpp.from);
    operation_c->to = build_type(operation_cpp.to);
    operation_c->index = operation_cpp.index;
    memcpy(operation_c->swizzle, operation_cpp.swizzle, sizeof(signed char) * 4);
    return operation_c;
}

void free_operation(struct operation *operation_c) {
    if (!operation_c) return;
    free_type(operation_c->from);
    free_type(operation_c->to);
    free(operation_c);
}

struct expression *build_expression(const reshadefx::expression &expression_cpp) {
    struct expression *expression_c = (struct expression *)malloc(sizeof(struct expression));
    if (!expression_c) return NULL;
    
    expression_c->base = expression_cpp.base;
    expression_c->expression_type = build_type(expression_cpp.type);
    expression_c->constant_value = build_constant(expression_cpp.constant);
    expression_c->is_lvalue = expression_cpp.is_lvalue;
    expression_c->is_constant = expression_cpp.is_constant;
    expression_c->operations_chain = (struct operation **)malloc(sizeof(struct operation *) * expression_cpp.chain.size());
    if (!expression_c->operations_chain) {
        free_type(expression_c->expression_type);
        free_constant(expression_c->constant_value);
        free(expression_c);
        return NULL;
    }
    for (int i = 0; i < expression_cpp.chain.size(); i++)
        expression_c->operations_chain[i] = build_operation(expression_cpp.chain[i]);
    expression_c->operations_chain_size = expression_cpp.chain.size();
    return expression_c;
}

void free_expression(struct expression *expression_c) {
    if (!expression_c) return;
    free_type(expression_c->expression_type);
    free_constant(expression_c->constant_value);
    for (int i = 0; i < expression_c->operations_chain_size; i++)
        free_operation(expression_c->operations_chain[i]);
    free(expression_c->operations_chain);
    free(expression_c);
}

struct struct_member_info_ *build_struct_member_info(const reshadefx::struct_member_info &struct_member_info_cpp) {
    struct struct_member_info_ *struct_member_info_c = (struct struct_member_info_ *)malloc(sizeof(struct struct_member_info_));
    if (!struct_member_info_c) return NULL;
    
    struct_member_info_c->type = build_type(struct_member_info_cpp.type);
    struct_member_info_c->name = strdup(struct_member_info_cpp.name.c_str());
    if (!struct_member_info_c->name) {
        free(struct_member_info_c);
        return NULL;
    }
    struct_member_info_c->semantic = strdup(struct_member_info_cpp.semantic.c_str());
    if (!struct_member_info_c->semantic) {
        free(struct_member_info_c->name);
        free(struct_member_info_c);
        return NULL;
    }
    struct_member_info_c->definition = struct_member_info_cpp.definition;
    return struct_member_info_c;
}

void free_struct_member_info(struct struct_member_info_ *struct_member_info_c) {
    if (!struct_member_info_c) return;
    free_type(struct_member_info_c->type);
    free(struct_member_info_c->name);
    free(struct_member_info_c->semantic);
    free(struct_member_info_c);
}

struct struct_info *build_struct_info(const reshadefx::struct_info &struct_info_cpp) {
    struct struct_info *struct_info_c = (struct struct_info *)malloc(sizeof(struct struct_info));
    if (!struct_info_c) return NULL;
    
    struct_info_c->name = strdup(struct_info_cpp.name.c_str());
    if (!struct_info_c->name) {
        free(struct_info_c);
        return NULL;
    }
    struct_info_c->unique_name = strdup(struct_info_cpp.unique_name.c_str());
    if (!struct_info_c->unique_name) {
        free(struct_info_c->name);
        free(struct_info_c);
        return NULL;
    }
    struct_info_c->member_list = (struct struct_member_info_ **)malloc(sizeof(struct struct_member_info_ *) * struct_info_cpp.member_list.size());
    if (!struct_info_c->member_list) {
        free(struct_info_c->unique_name);
        free(struct_info_c->name);
        free(struct_info_c);
        return NULL;
    }
    for (int i = 0; i < struct_info_cpp.member_list.size(); i++)
        struct_info_c->member_list[i] = build_struct_member_info(struct_info_cpp.member_list[i]);
        
    struct_info_c->member_list_size = struct_info_cpp.member_list.size();
    struct_info_c->definition = struct_info_cpp.definition;
    return struct_info_c;
}

void free_struct_info(struct struct_info *struct_info_c) {
    if (!struct_info_c) return;
    free(struct_info_c->name);
    free(struct_info_c->unique_name);
    for (int i = 0; i < struct_info_c->member_list_size; i++)
        free_struct_member_info(struct_info_c->member_list[i]);
    free(struct_info_c->member_list);
    free(struct_info_c);
}

struct annotation *build_annotation(const reshadefx::annotation &annotation_cpp) {
    struct annotation *annotation_c = (struct annotation *)malloc(sizeof(struct annotation));
    if (!annotation_c) return NULL;
    
    annotation_c->type = build_type(annotation_cpp.type);
    annotation_c->name = strdup(annotation_cpp.name.c_str());
    if (!annotation_c->name) {
        free(annotation_c);
        return NULL;
    }
    annotation_c->value = build_constant(annotation_cpp.value);
    return annotation_c;
}

void free_annotation(struct annotation *annotation_c) {
    if (!annotation_c) return;
    free_type(annotation_c->type);
    free(annotation_c->name);
    free_constant(annotation_c->value);
    free(annotation_c);
}

struct texture_info *build_texture_info(const reshadefx::texture_info &texture_info_cpp) {
    struct texture_info *texture_info_c = (struct texture_info *)malloc(sizeof(struct texture_info));
    if (!texture_info_c) return NULL;
    
    texture_info_c->id = texture_info_cpp.id;
    texture_info_c->binding = texture_info_cpp.binding;
    texture_info_c->name = strdup(texture_info_cpp.name.c_str());
    texture_info_c->semantic = strdup(texture_info_cpp.semantic.c_str());
    texture_info_c->unique_name = strdup(texture_info_cpp.unique_name.c_str());
    if (!texture_info_c->name || !texture_info_c->semantic || !texture_info_c->unique_name) {
        if (!texture_info_c->name) free(texture_info_c->name);
        if (!texture_info_c->semantic) free(texture_info_c->semantic);
        if (!texture_info_c->unique_name) free(texture_info_c->unique_name);
        free(texture_info_c);
        return NULL;
    }
    texture_info_c->annotations = (struct annotation **)malloc(sizeof(struct annotation *) * texture_info_cpp.annotations.size());
    if (!texture_info_c->annotations) {
        free(texture_info_c->name);
        free(texture_info_c->semantic);
        free(texture_info_c->unique_name);
        free(texture_info_c);
        return NULL;
    }
    for (int i = 0; i < texture_info_cpp.annotations.size(); i++)
        texture_info_c->annotations[i] =  build_annotation(texture_info_cpp.annotations[i]);
    texture_info_c->annotations_size = texture_info_cpp.annotations.size();
    texture_info_c->width = texture_info_cpp.width;
    texture_info_c->height = texture_info_cpp.height;
    texture_info_c->levels = texture_info_cpp.levels;
    texture_info_c->format = static_cast<texture_format>(texture_info_cpp.format);
    texture_info_c->render_target = texture_info_cpp.render_target;
    texture_info_c->storage_access = texture_info_cpp.storage_access;
    return texture_info_c;
}

void free_texture_info(struct texture_info *texture_info_c) {
    if (!texture_info_c) return;
    free(texture_info_c->name);
    free(texture_info_c->semantic);
    free(texture_info_c->unique_name);
    for (int i = 0; i < texture_info_c->annotations_size; i++)
        free_annotation(texture_info_c->annotations[i]);
    free(texture_info_c->annotations);
    free(texture_info_c);
}

struct sampler_info *build_sampler_info(const reshadefx::sampler_info &sampler_info_cpp) {
    struct sampler_info *sampler_info_c = (struct sampler_info *)malloc(sizeof(struct sampler_info));
    if (!sampler_info_c) return NULL;
    
    sampler_info_c->id = sampler_info_cpp.id;
    sampler_info_c->binding = sampler_info_cpp.binding;
    sampler_info_c->texture_binding = sampler_info_cpp.texture_binding;
    sampler_info_c->name = strdup(sampler_info_cpp.name.c_str());
    sampler_info_c->texture_name = strdup(sampler_info_cpp.texture_name.c_str());
    sampler_info_c->unique_name = strdup(sampler_info_cpp.unique_name.c_str());
    if (!sampler_info_c->name || !sampler_info_c->texture_name || !sampler_info_c->unique_name) {
        if (!sampler_info_c->name) free(sampler_info_c->name);
        if (!sampler_info_c->texture_name) free(sampler_info_c->texture_name);
        if (!sampler_info_c->unique_name) free(sampler_info_c->unique_name);
        free(sampler_info_c);
        return NULL;
    }
    sampler_info_c->annotations = (struct annotation **)malloc(sizeof(struct annotation *) * sampler_info_cpp.annotations.size());
    if (!sampler_info_c->annotations) {
        free(sampler_info_c->name);
        free(sampler_info_c->texture_name);
        free(sampler_info_c->unique_name);
        free(sampler_info_c);
        return NULL;
    }
    for (int i = 0; i < sampler_info_cpp.annotations.size(); i++)
        sampler_info_c->annotations[i] =  build_annotation(sampler_info_cpp.annotations[i]);
    sampler_info_c->annotations_size = sampler_info_cpp.annotations.size();
    sampler_info_c->filter = static_cast<filter_mode>(sampler_info_cpp.filter);
    sampler_info_c->address_u = static_cast<texture_address_mode>(sampler_info_cpp.address_u);
    sampler_info_c->address_v = static_cast<texture_address_mode>(sampler_info_cpp.address_v);
    sampler_info_c->address_w = static_cast<texture_address_mode>(sampler_info_cpp.address_w);
    sampler_info_c->min_lod = sampler_info_cpp.min_lod;
    sampler_info_c->max_lod = sampler_info_cpp.max_lod;
    sampler_info_c->lod_bias = sampler_info_cpp.lod_bias;
    sampler_info_c->srgb = sampler_info_cpp.srgb;
    return sampler_info_c;
}

void free_sampler_info(struct sampler_info *sampler_info_c) {
    if (!sampler_info_c) return;
    free(sampler_info_c->name);
    free(sampler_info_c->texture_name);
    free(sampler_info_c->unique_name);
    for (int i = 0; i < sampler_info_c->annotations_size; i++)
        free_annotation(sampler_info_c->annotations[i]);
    free(sampler_info_c->annotations);
    free(sampler_info_c);
}

struct storage_info *build_storage_info(const reshadefx::storage_info &storage_info_cpp) {
    struct storage_info *storage_info_c = (struct storage_info *)malloc(sizeof(struct storage_info));
    if (!storage_info_c) return NULL;
    
    storage_info_c->id = storage_info_cpp.id;
    storage_info_c->binding = storage_info_cpp.binding;
    storage_info_c->name = strdup(storage_info_cpp.name.c_str());
    storage_info_c->texture_name = strdup(storage_info_cpp.texture_name.c_str());
    storage_info_c->unique_name = strdup(storage_info_cpp.unique_name.c_str());
    if (!storage_info_c->name || !storage_info_c->texture_name || !storage_info_c->unique_name) {
        if (!storage_info_c->name) free(storage_info_c->name);
        if (!storage_info_c->texture_name) free(storage_info_c->texture_name);
        if (!storage_info_c->unique_name) free(storage_info_c->unique_name);
        free(storage_info_c);
        return NULL;
    }
    storage_info_c->format = static_cast<texture_format>(storage_info_cpp.format);
    storage_info_c->level = storage_info_cpp.level;
    return storage_info_c;
}

void free_storage_info(struct storage_info *storage_info_c) {
    if (!storage_info_c) return;
    free(storage_info_c->name);
    free(storage_info_c->unique_name);
    free(storage_info_c->texture_name);
    free(storage_info_c);
}

struct uniform_info *build_uniform_info(const reshadefx::uniform_info &uniform_info_cpp) {
    struct uniform_info *uniform_info_c = (struct uniform_info *)malloc(sizeof(struct uniform_info));
    if (!uniform_info_c) return NULL;
    
    uniform_info_c->name = strdup(uniform_info_cpp.name.c_str());
    if (!uniform_info_c->name) {
        free(uniform_info_c);
        return NULL;
    }
    uniform_info_c->type = build_type(uniform_info_cpp.type);
    uniform_info_c->size = uniform_info_cpp.size;
    uniform_info_c->offset = uniform_info_cpp.offset;
    uniform_info_c->annotations = (struct annotation **)malloc(sizeof(struct annotation *) * uniform_info_cpp.annotations.size());
    if (!uniform_info_c->annotations) {
        free(uniform_info_c->name);
        free(uniform_info_c);
        return NULL;
    }
    for (int i = 0; i < uniform_info_cpp.annotations.size(); i++)
        uniform_info_c->annotations[i] =  build_annotation(uniform_info_cpp.annotations[i]);
    uniform_info_c->annotations_size = uniform_info_cpp.annotations.size();
    uniform_info_c->has_initializer_value = uniform_info_cpp.has_initializer_value;
    uniform_info_c->initializer_value = build_constant(uniform_info_cpp.initializer_value);
    return uniform_info_c;
}

void free_uniform_info(struct uniform_info *uniform_info_c) {
    if (!uniform_info_c) return;
    free(uniform_info_c->name);
    for (int i = 0; i < uniform_info_c->annotations_size; i++)
        free_annotation(uniform_info_c->annotations[i]);
    free(uniform_info_c->annotations);
    free_constant(uniform_info_c->initializer_value);
    free(uniform_info_c);
}

struct entry_point *build_entry_point(const reshadefx::entry_point &entry_point_cpp) {
    struct entry_point *entry_point_c = (struct entry_point *)malloc(sizeof(struct entry_point));
    if (!entry_point_c) return NULL;
    
    entry_point_c->name = strdup(entry_point_cpp.name.c_str());
    if (!entry_point_c->name) {
        free(entry_point_c);
        return NULL;
    }
    entry_point_c->type = static_cast<shader_type>(entry_point_cpp.type);
    return entry_point_c;
}

void free_entry_point(struct entry_point *entry_point_c) {
    if (!entry_point_c) return;
    free(entry_point_c->name);
    free(entry_point_c);
}

struct function_info *build_function_info(const reshadefx::function_info &function_info_cpp) {
    struct function_info *function_info_c = (struct function_info *)malloc(sizeof(struct function_info));
    if (!function_info_c) return NULL;
    
    function_info_c->definition = function_info_cpp.definition;
    function_info_c->name = strdup(function_info_cpp.name.c_str());
    function_info_c->return_semantic = strdup(function_info_cpp.return_semantic.c_str());
    function_info_c->unique_name = strdup(function_info_cpp.unique_name.c_str());
    if (!function_info_c->name || !function_info_c->return_semantic || !function_info_c->unique_name) {
        if (!function_info_c->name) free(function_info_c->name);
        if (!function_info_c->return_semantic) free(function_info_c->return_semantic);
        if (!function_info_c->unique_name) free(function_info_c->unique_name);
        free(function_info_c);
        return NULL;
    }
    function_info_c->return_type = build_type(function_info_cpp.return_type);
    function_info_c->parameter_list = (struct struct_member_info_ **)malloc(sizeof(struct struct_member_info_ *) * function_info_cpp.parameter_list.size());
    if (!function_info_c->parameter_list) {
        free(function_info_c->unique_name);
        free(function_info_c->name);
        free(function_info_c->return_semantic);
        free(function_info_c);
        return NULL;
    }
    for (int i = 0; i < function_info_cpp.parameter_list.size(); i++)
        function_info_c->parameter_list[i] =  build_struct_member_info(function_info_cpp.parameter_list[i]);
    function_info_c->parameter_list_size = function_info_cpp.parameter_list.size();
    function_info_c->referenced_samplers = (uint32_t *)malloc(sizeof(uint32_t) * function_info_cpp.referenced_samplers.size());
    if (!function_info_c->referenced_samplers) {
        free(function_info_c->unique_name);
        free(function_info_c->name);
        free(function_info_c->return_semantic);
        for (int i = 0; i < function_info_c->parameter_list_size; i++)
            free_struct_member_info(function_info_c->parameter_list[i]);
        free(function_info_c->parameter_list);
        free(function_info_c);
        return NULL;
    }
    std::copy(function_info_cpp.referenced_samplers.begin(), function_info_cpp.referenced_samplers.end(), function_info_c->referenced_samplers);
    function_info_c->referenced_samplers_size = function_info_cpp.referenced_samplers.size();
    function_info_c->referenced_storages = (uint32_t *)malloc(sizeof(uint32_t) * function_info_cpp.referenced_storages.size());
    if (!function_info_c->referenced_storages) {
        free(function_info_c->unique_name);
        free(function_info_c->name);
        free(function_info_c->return_semantic);
        for (int i = 0; i < function_info_c->parameter_list_size; i++)
            free_struct_member_info(function_info_c->parameter_list[i]);
        free(function_info_c->parameter_list);
        free(function_info_c->referenced_samplers);
        free(function_info_c);
        return NULL;
    }
    std::copy(function_info_cpp.referenced_storages.begin(), function_info_cpp.referenced_storages.end(), function_info_c->referenced_storages);
    function_info_c->referenced_storages_size = function_info_cpp.referenced_storages.size();
    return function_info_c;
}

void free_function_info(struct function_info *function_info_c) {
    if (!function_info_c) return;
    free(function_info_c->unique_name);
    free(function_info_c->name);
    free(function_info_c->return_semantic);
    for (int i = 0; i < function_info_c->parameter_list_size; i++)
        free_struct_member_info(function_info_c->parameter_list[i]);
    free(function_info_c->parameter_list);
    free(function_info_c->referenced_samplers);
    free(function_info_c->referenced_storages);
    free(function_info_c);
}

struct pass_info *build_pass_info(const reshadefx::pass_info &pass_info_cpp) {
    struct pass_info *pass_info_c = (struct pass_info *)malloc(sizeof(struct pass_info));
    if (!pass_info_c) return NULL;
    
    for (int i = 0; i < 8; i++) {
        pass_info_c->render_target_names[i] = strdup(pass_info_cpp.render_target_names[i].c_str());
        if(!pass_info_c->render_target_names[i]) {
            for (int j = i - 1; j >= 0; j--)
                free(pass_info_c->render_target_names[j]);
            free(pass_info_c);
            return NULL;
        }
    }
    pass_info_c->name = strdup(pass_info_cpp.name.c_str());
    pass_info_c->vs_entry_point = strdup(pass_info_cpp.vs_entry_point.c_str());
    pass_info_c->ps_entry_point = strdup(pass_info_cpp.ps_entry_point.c_str());
    pass_info_c->cs_entry_point = strdup(pass_info_cpp.cs_entry_point.c_str());
    if (!pass_info_c->vs_entry_point || !pass_info_c->ps_entry_point || !pass_info_c->cs_entry_point) {
        if (!pass_info_c->name) free(pass_info_c->name);
        if (!pass_info_c->vs_entry_point) free(pass_info_c->vs_entry_point);
        if (!pass_info_c->ps_entry_point) free(pass_info_c->ps_entry_point);
        if (!pass_info_c->cs_entry_point) free(pass_info_c->cs_entry_point);
        for (int i = 0; i < 8; i++)
            free(pass_info_c->render_target_names[i]);
        free(pass_info_c);
        return NULL;
    }
    pass_info_c->generate_mipmaps = pass_info_cpp.generate_mipmaps;
    pass_info_c->clear_render_targets = pass_info_cpp.clear_render_targets;
    pass_info_c->srgb_write_enable = pass_info_cpp.srgb_write_enable;
    memcpy(pass_info_c->blend_enable, pass_info_cpp.blend_enable, sizeof(uint8_t) * 8);
    pass_info_c->stencil_enable = pass_info_cpp.stencil_enable;
    memcpy(pass_info_c->color_write_mask, pass_info_cpp.color_write_mask, sizeof(uint8_t) * 8);
    pass_info_c->stencil_read_mask = pass_info_cpp.stencil_read_mask;
    pass_info_c->stencil_write_mask = pass_info_cpp.stencil_write_mask;
    memcpy(pass_info_c->blend_op, pass_info_cpp.blend_op, sizeof(pass_blend_op) * 8);
    memcpy(pass_info_c->blend_op_alpha, pass_info_cpp.blend_op_alpha, sizeof(pass_blend_op) * 8);
    memcpy(pass_info_c->src_blend, pass_info_cpp.src_blend, sizeof(pass_blend_func) * 8);
    memcpy(pass_info_c->dest_blend, pass_info_cpp.dest_blend, sizeof(pass_blend_func) * 8);
    memcpy(pass_info_c->src_blend_alpha, pass_info_cpp.src_blend_alpha, sizeof(pass_blend_func) * 8);
    memcpy(pass_info_c->dest_blend_alpha, pass_info_cpp.dest_blend_alpha, sizeof(pass_blend_func) * 8);
    pass_info_c->stencil_comparison_func = static_cast<pass_stencil_func>(pass_info_cpp.stencil_comparison_func);
    pass_info_c->stencil_reference_value = pass_info_cpp.stencil_reference_value;
    pass_info_c->stencil_op_pass = static_cast<pass_stencil_op>(pass_info_cpp.stencil_op_pass);
    pass_info_c->stencil_op_fail = static_cast<pass_stencil_op>(pass_info_cpp.stencil_op_fail);
    pass_info_c->stencil_op_depth_fail = static_cast<pass_stencil_op>(pass_info_cpp.stencil_op_depth_fail);
    pass_info_c->num_vertices = pass_info_cpp.num_vertices;
    pass_info_c->topology = static_cast<primitive_topology>(pass_info_cpp.topology);
    pass_info_c->viewport_width = pass_info_cpp.viewport_width;
    pass_info_c->viewport_height = pass_info_cpp.viewport_height;
    pass_info_c->viewport_dispatch_z = pass_info_cpp.viewport_dispatch_z;
    pass_info_c->samplers = (struct sampler_info **)malloc(sizeof(struct sampler_info *) * pass_info_cpp.samplers.size());
    if (!pass_info_c->samplers) {
        free(pass_info_c->name);
        free(pass_info_c->vs_entry_point);
        free(pass_info_c->ps_entry_point);
        free(pass_info_c->cs_entry_point);
        for (int i = 0; i < 8; i++)
            free(pass_info_c->render_target_names[i]);
        free(pass_info_c);
        return NULL;
    }
    for (int i = 0; i < pass_info_cpp.samplers.size(); i++)
        pass_info_c->samplers[i] =  build_sampler_info(pass_info_cpp.samplers[i]);
    pass_info_c->samplers_size = pass_info_cpp.samplers.size();
    pass_info_c->storages = (struct storage_info **)malloc(sizeof(struct storage_info *) * pass_info_cpp.storages.size());
    if (!pass_info_c->storages) {
        free(pass_info_c->name);
        free(pass_info_c->vs_entry_point);
        free(pass_info_c->ps_entry_point);
        free(pass_info_c->cs_entry_point);
        for (int i = 0; i < 8; i++)
            free(pass_info_c->render_target_names[i]);
        for (int i = 0; i < pass_info_c->samplers_size; i++)
            free_sampler_info(pass_info_c->samplers[i]);
        free(pass_info_c);
        return NULL;
    }
    for (int i = 0; i < pass_info_cpp.storages.size(); i++)
        pass_info_c->storages[i] = build_storage_info(pass_info_cpp.storages[i]);
    pass_info_c->storages_size = pass_info_cpp.storages.size();
    return pass_info_c;
}

void free_pass_info(struct pass_info *pass_info_c) {
    if (!pass_info_c) return;
    free(pass_info_c->name);
    free(pass_info_c->vs_entry_point);
    free(pass_info_c->ps_entry_point);
    free(pass_info_c->cs_entry_point);
    for (int i = 0; i < 8; i++)
        free(pass_info_c->render_target_names[i]);
    for (int i = 0; i < pass_info_c->samplers_size; i++)
        free_sampler_info(pass_info_c->samplers[i]);
    for (int i = 0; i < pass_info_c->storages_size; i++)
        free_storage_info(pass_info_c->storages[i]);
    free(pass_info_c);
}

struct technique_info *build_technique_info(const reshadefx::technique_info &technique_info_cpp) {
    struct technique_info *technique_info_c = (struct technique_info *)malloc(sizeof(struct technique_info));
    if (!technique_info_c) return NULL;
    
    technique_info_c->name = strdup(technique_info_cpp.name.c_str());
    if (!technique_info_c->name) {
        free(technique_info_c);
        return NULL;
    }
    technique_info_c->passes = (struct pass_info **)malloc(sizeof(struct pass_info *) * technique_info_cpp.passes.size());
    if (!technique_info_c->passes) {
        free(technique_info_c->name);
        free(technique_info_c);
        return NULL;
    }
    for (int i = 0; i < technique_info_cpp.passes.size(); i++)
        technique_info_c->passes[i] =  build_pass_info(technique_info_cpp.passes[i]);
    technique_info_c->passes_size = technique_info_cpp.passes.size();
    technique_info_c->annotations = (struct annotation **)malloc(sizeof(struct annotation *) * technique_info_cpp.annotations.size());
    if (!technique_info_c->annotations) {
        free(technique_info_c->name);
        for (int i = 0; i < technique_info_cpp.passes.size(); i++)
            free_pass_info(technique_info_c->passes[i]);
        free(technique_info_c);
        return NULL;
    }
    for (int i = 0; i < technique_info_cpp.annotations.size(); i++)
        technique_info_c->annotations[i] =  build_annotation(technique_info_cpp.annotations[i]);
    technique_info_c->annotations_size = technique_info_cpp.annotations.size();
    return technique_info_c;
}

void free_technique_info(struct technique_info *technique_info_c) {
    if (!technique_info_c) return;
    free(technique_info_c->name);
    for (int i = 0; i < technique_info_c->passes_size; i++)
        free_pass_info(technique_info_c->passes[i]);
    for (int i = 0; i < technique_info_c->annotations_size; i++)
        free_annotation(technique_info_c->annotations[i]);
    free(technique_info_c);
}

struct module *build_module(const reshadefx::module &module_cpp) {
    struct module *module_c = (struct module *)malloc(sizeof(struct module));
    if (!module_c) return NULL;
    
    module_c->code = (char *)malloc(sizeof(char) * module_cpp.code.size());
    if (!module_c->code) {
        free(module_c);
        return NULL;
    }
    memcpy(module_c->code, module_cpp.code.data(), module_cpp.code.size());
    module_c->code_size = module_cpp.code.size();
    module_c->entry_points = (struct entry_point **)malloc(sizeof(struct entry_point *) * module_cpp.entry_points.size());
    if (!module_c->entry_points) {
        free(module_c->code);
        free(module_c);
        return NULL;
    }
    for (int i = 0; i < module_cpp.entry_points.size(); i++)
        module_c->entry_points[i] = build_entry_point(module_cpp.entry_points[i]);
    module_c->entry_points_size = module_cpp.entry_points.size();
    module_c->textures = (struct texture_info **)malloc(sizeof(struct texture_info *) * module_cpp.textures.size());
    if (!module_c->textures) {
        free(module_c->code);
        for (int i = 0; i < module_c->entry_points_size; i++)
            free_entry_point(module_c->entry_points[i]);
        free(module_c);
        return NULL;
    }
    for (int i = 0; i < module_cpp.textures.size(); i++)
        module_c->textures[i] = build_texture_info(module_cpp.textures[i]);
    module_c->textures_size = module_cpp.textures.size();
    module_c->samplers = (struct sampler_info **)malloc(sizeof(struct sampler_info *) * module_cpp.samplers.size());
    if (!module_c->samplers) {
        free(module_c->code);
        for (int i = 0; i < module_c->entry_points_size; i++)
            free_entry_point(module_c->entry_points[i]);
        for (int i = 0; i < module_c->textures_size; i++)
            free_texture_info(module_c->textures[i]);
        free(module_c);
        return NULL;
    }
    for (int i = 0; i < module_cpp.samplers.size(); i++)
        module_c->samplers[i] = build_sampler_info(module_cpp.samplers[i]);
    module_c->samplers_size = module_cpp.samplers.size();
    module_c->storages = (struct storage_info **)malloc(sizeof(struct storage_info *) * module_cpp.storages.size());
    if (!module_c->storages) {
        free(module_c->code);
        for (int i = 0; i < module_c->entry_points_size; i++)
            free_entry_point(module_c->entry_points[i]);
        for (int i = 0; i < module_c->textures_size; i++)
            free_texture_info(module_c->textures[i]);
        for (int i = 0; i < module_c->samplers_size; i++)
            free_sampler_info(module_c->samplers[i]);
        free(module_c);
        return NULL;
    }
    for (int i = 0; i < module_cpp.storages.size(); i++)
        module_c->storages[i] = build_storage_info(module_cpp.storages[i]);
    module_c->storages_size = module_cpp.storages.size();
    module_c->uniforms = (struct uniform_info **)malloc(sizeof(struct uniform_info *) * module_cpp.uniforms.size());
    if (!module_c->uniforms) {
        free(module_c->code);
        for (int i = 0; i < module_c->entry_points_size; i++)
            free_entry_point(module_c->entry_points[i]);
        for (int i = 0; i < module_c->textures_size; i++)
            free_texture_info(module_c->textures[i]);
        for (int i = 0; i < module_c->samplers_size; i++)
            free_sampler_info(module_c->samplers[i]);
        for (int i = 0; i < module_c->storages_size; i++)
            free_storage_info(module_c->storages[i]);
        free(module_c);
        return NULL;
    }
    for (int i = 0; i < module_cpp.uniforms.size(); i++)
        module_c->uniforms[i] = build_uniform_info(module_cpp.uniforms[i]);
    module_c->uniforms_size = module_cpp.uniforms.size();
    module_c->spec_constants = (struct uniform_info **)malloc(sizeof(struct uniform_info *) * module_cpp.spec_constants.size());
    if (!module_c->spec_constants) {
        free(module_c->code);
        for (int i = 0; i < module_c->entry_points_size; i++)
            free_entry_point(module_c->entry_points[i]);
        for (int i = 0; i < module_c->textures_size; i++)
            free_texture_info(module_c->textures[i]);
        for (int i = 0; i < module_c->samplers_size; i++)
            free_sampler_info(module_c->samplers[i]);
        for (int i = 0; i < module_c->storages_size; i++)
            free_storage_info(module_c->storages[i]);
        for (int i = 0; i < module_c->uniforms_size; i++)
            free_uniform_info(module_c->uniforms[i]);
        free(module_c);
        return NULL;
    }
    for (int i = 0; i < module_cpp.spec_constants.size(); i++)
        module_c->spec_constants[i] = build_uniform_info(module_cpp.spec_constants[i]);
    module_c->spec_constants_size = module_cpp.spec_constants.size();
    module_c->techniques = (struct technique_info **)malloc(sizeof(struct technique_info *) * module_cpp.techniques.size());
    if (!module_c->techniques) {
        free(module_c->code);
        for (int i = 0; i < module_c->entry_points_size; i++)
            free_entry_point(module_c->entry_points[i]);
        for (int i = 0; i < module_c->textures_size; i++)
            free_texture_info(module_c->textures[i]);
        for (int i = 0; i < module_c->samplers_size; i++)
            free_sampler_info(module_c->samplers[i]);
        for (int i = 0; i < module_c->storages_size; i++)
            free_storage_info(module_c->storages[i]);
        for (int i = 0; i < module_c->uniforms_size; i++)
            free_uniform_info(module_c->uniforms[i]);
        for (int i = 0; i < module_c->spec_constants_size; i++)
            free_uniform_info(module_c->spec_constants[i]);
        free(module_c);
        return NULL;
    }
    for (int i = 0; i < module_cpp.techniques.size(); i++)
        module_c->techniques[i] = build_technique_info(module_cpp.techniques[i]);
    module_c->techniques_size = module_cpp.techniques.size();
    module_c->total_uniform_size = module_cpp.total_uniform_size;
    module_c->num_texture_bindings = module_cpp.num_texture_bindings;
    module_c->num_sampler_bindings = module_cpp.num_sampler_bindings;
    module_c->num_storage_bindings = module_cpp.num_storage_bindings;
    return module_c;
}

void free_module(struct module *module_c) {
    if (!module_c) return;
    free(module_c->code);
    for (int i = 0; i < module_c->entry_points_size; i++)
        free_entry_point(module_c->entry_points[i]);
    for (int i = 0; i < module_c->textures_size; i++)
        free_texture_info(module_c->textures[i]);
    for (int i = 0; i < module_c->samplers_size; i++)
        free_sampler_info(module_c->samplers[i]);
    for (int i = 0; i < module_c->storages_size; i++)
        free_storage_info(module_c->storages[i]);
    for (int i = 0; i < module_c->uniforms_size; i++)
        free_uniform_info(module_c->uniforms[i]);
    for (int i = 0; i < module_c->spec_constants_size; i++)
        free_uniform_info(module_c->spec_constants[i]);
    for (int i = 0; i < module_c->techniques_size; i++)
        free_technique_info(module_c->techniques[i]);
    free(module_c);
}
