/*
    This file is a part of Tiny-Shading-Language or TSL, an open-source cross
    platform programming shading language.

    Copyright (c) 2020-2020 by Jiayin Cao - All rights reserved.

    TSL is a free software written for educational purpose. Anyone can distribute
    or modify it under the the terms of the GNU General Public License Version 3 as
    published by the Free Software Foundation. However, there is NO warranty that
    all components are functional in a perfect manner. Without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
 */

#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "compiler_impl.h"
#include "ast.h"
#include "shader_unit_pvt.h"
#include "shading_context.h"
#include "global_module.h"
#include "llvm_util.h"

// a temporary ugly solution for debugging for now
// #define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#include <iostream>
#endif

// Following are some externally defined interface and data structure generated by flex and bison.
struct yy_buffer_state;
typedef yy_buffer_state* YY_BUFFER_STATE;
int yylex_init(void**);
int yyparse(class Tsl_Namespace::TslCompiler_Impl*);
int yylex_destroy(void*);
YY_BUFFER_STATE yy_scan_string(const char* yystr, void* yyscanner);
void makeVerbose(int verbose);

TSL_NAMESPACE_BEGIN

static llvm::Type* llvm_type_from_arg_type(const ShaderArgumentTypeEnum type, LLVM_Compile_Context& context) {
    llvm::Type* llvm_type = nullptr;
    switch (type) {
    case ShaderArgumentTypeEnum::TSL_TYPE_CLOSURE:
        llvm_type = get_int_32_ptr_ty(context);
        break;
    case ShaderArgumentTypeEnum::TSL_TYPE_INT:
        llvm_type = get_int_32_ty(context);
        break;
    case ShaderArgumentTypeEnum::TSL_TYPE_FLOAT:
        llvm_type = get_float_ty(context);
        break;
    case ShaderArgumentTypeEnum::TSL_TYPE_BOOL:
        llvm_type = get_int_1_ty(context);
        break;
    case ShaderArgumentTypeEnum::TSL_TYPE_DOUBLE:
        llvm_type = get_double_ty(context);
        break;
    case ShaderArgumentTypeEnum::TSL_TYPE_FLOAT3:
        llvm_type = context.m_structure_type_maps["float3"].m_llvm_type;
        break;
    case ShaderArgumentTypeEnum::TSL_TYPE_FLOAT4:
        llvm_type = context.m_structure_type_maps["float4"].m_llvm_type;
        break;
    default:
        // not supported yet
        break;
    }
    return llvm_type;
}

TslCompiler_Impl::TslCompiler_Impl( GlobalModule& global_module ):m_global_module(global_module){
    reset();
}

TslCompiler_Impl::~TslCompiler_Impl() {
}

void TslCompiler_Impl::reset() {
    m_scanner = nullptr;
    m_ast_root = nullptr;

    m_closures_in_shader.clear();
}

void TslCompiler_Impl::push_function(AstNode_FunctionPrototype* node, const bool is_shader) {
    if (is_shader)
        m_ast_root = node;
    else
        m_functions.push_back(node);

#ifdef DEBUG_OUTPUT    
    // node->print();
#endif
}

void TslCompiler_Impl::push_structure_declaration(AstNode_StructDeclaration* structure) {
	m_structures.push_back(structure);

#ifdef DEBUG_OUTPUT
	// structure->print();
#endif
}

void* TslCompiler_Impl::get_scanner() {
    return m_scanner;
}

bool TslCompiler_Impl::compile(const char* source_code, ShaderUnitTemplate* su) {
#ifdef DEBUG_OUTPUT
    std::cout << source_code << std::endl;
#endif

    // not verbose for now, this should be properly exported through compiler option later.
    makeVerbose(false);

    // initialize flex scanner
    m_scanner = nullptr;
    yylex_init(&m_scanner);

    // flex and bison parsing
    yy_scan_string(source_code, m_scanner);
    const int parsing_result = yyparse(this);

    // destroy scanner information
    yylex_destroy(m_scanner);

    if( parsing_result != 0 )
		return false;

    auto su_pvt = su->get_shader_unit_data();

    // shader_unit_pvt holds the life time of this module, whenever it is needed by execution engine
    // another module is cloned from this one.
    su_pvt->m_module = std::make_unique<llvm::Module>(su->get_name(), m_llvm_context);
    auto module = su_pvt->m_module.get();
	if(!module)
		return false;

	// if there is a legit shader defined, generate LLVM IR
	if(m_ast_root){
		llvm::IRBuilder<> builder(m_llvm_context);

		LLVM_Compile_Context compile_context;
		compile_context.context = &m_llvm_context;
		compile_context.module = module;
		compile_context.builder = &builder;

        m_global_module.declare_closure_tree_types(m_llvm_context, &compile_context.m_structure_type_maps);
		m_global_module.declare_global_module(compile_context);
        for (auto& closure : m_closures_in_shader) {
            // declare the function first.
            auto function = m_global_module.declare_closure_function(closure, compile_context);

            if (!function) {
                // emit error here, unregistered closure touched
                return false;
            }
            compile_context.m_closures_maps[closure] = function;
        }

		// generate all data structures first
		for( auto& structure : m_structures )
			structure->codegen(compile_context);

        // code gen for all functions
        for( auto& function : m_functions )
            function->codegen(compile_context);

		// generate code for the shader in this module
		su_pvt->m_llvm_function = m_ast_root->codegen(compile_context);
        su_pvt->m_root_function_name = m_ast_root->get_function_name();

        // there is usually just one global module as dependent in all shader unit.
        su_pvt->m_dependencies.insert(m_global_module.get_closure_module());

        // parse shader parameters, this is for groupping shader units
        m_ast_root->parse_shader_parameters(su_pvt->m_shader_params);

        // keep track of the ast root of this shader unit
        su_pvt->m_ast_root = m_ast_root;

        // it should be safe to assume llvm function has to be generated, otherwise, the shader is invalid.
        if (!su_pvt->m_llvm_function)
            return false;
    }

	return true;
}

bool TslCompiler_Impl::resolve(ShaderInstance* si) {
    if (!si)
        return false;

    const auto& shader_template = si->get_shader_template();
    auto shader_instance_data = si->get_shader_instance_data();
    auto shader_template_data = shader_template.get_shader_unit_data();

    // invalid shader unit template
    if (!shader_template_data->m_module || !shader_template_data->m_llvm_function)
        return false;

    // don't consume the module to avoid limitation of creating more shader instance
    auto cloned_module = llvm::CloneModule(*shader_template_data->m_module);

    // optimization pass, this is pretty cool because I don't have to implement those sophisticated optimization algorithms.
    if (shader_template.allow_optimization()) {
        // Create a new pass manager attached to it.
        shader_instance_data->m_fpm = std::make_unique<llvm::legacy::FunctionPassManager>(cloned_module.get());

        // Do simple "peephole" optimizations and bit-twiddling optzns.
        shader_instance_data->m_fpm->add(llvm::createInstructionCombiningPass());
        // Re-associate expressions.
        shader_instance_data->m_fpm->add(llvm::createReassociatePass());
        // Eliminate Common SubExpressions.
        shader_instance_data->m_fpm->add(llvm::createGVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        shader_instance_data->m_fpm->add(llvm::createCFGSimplificationPass());

        shader_instance_data->m_fpm->doInitialization();

        shader_instance_data->m_fpm->run(*shader_template_data->m_llvm_function);
    }

    // make sure the function is valid
    if (shader_template.allow_verification() && !llvm::verifyFunction(*shader_template_data->m_llvm_function, &llvm::errs()))
        return false;

#ifdef DEBUG_OUTPUT
    cloned_module->print(llvm::errs(), nullptr);
#endif

    // get the function pointer through execution engine
    shader_instance_data->m_execution_engine = std::unique_ptr<llvm::ExecutionEngine>(llvm::EngineBuilder(std::move(cloned_module)).create());

    // setup data layout
    // cloned_module->setDataLayout(shader_instance_data->m_execution_engine->getTargetMachine()->createDataLayout());

    // make sure to link the global closure model
    for (auto& dep_module : shader_template_data->m_dependencies) {
        auto cloned_module = llvm::CloneModule(*dep_module);
        shader_instance_data->m_execution_engine->addModule(std::move(cloned_module));
    }

    // resolve the function pointer
    shader_instance_data->m_function_pointer = shader_instance_data->m_execution_engine->getFunctionAddress(shader_template_data->m_root_function_name);

    return true;
}

bool TslCompiler_Impl::resolve(ShaderGroupTemplate* sg) {
    if (!sg)
        return false;

    auto su_pvt = sg->get_shader_unit_data();
    if (!su_pvt)
        return false;

    auto module = su_pvt->m_module.get();

    // if no root shader setup yet, return false
    if (sg->m_root_shader_unit_name == "")
        return false;

    // if we can't find the root shader, it should return false
    if (0 == sg->m_shader_units.count(sg->m_root_shader_unit_name))
        return false;

    // essentially, this is a topological sort
    std::unordered_set<const ShaderUnitTemplate*>   visited_shader_units;
    std::unordered_set<const ShaderUnitTemplate*>   current_shader_units_being_visited;

    // get the root shader
    auto root_shader = sg->m_shader_units[sg->m_root_shader_unit_name];

    // allocate the shader module for this shader group
    sg->m_shader_unit_data->m_module = std::make_unique<llvm::Module>(sg->get_name(), m_llvm_context);
    module = sg->m_shader_unit_data->m_module.get();

    llvm::IRBuilder<> builder(m_llvm_context);

    LLVM_Compile_Context compile_context;
    compile_context.context = &m_llvm_context;
    compile_context.module = sg->m_shader_unit_data->m_module.get();
    compile_context.builder = &builder;

    m_global_module.declare_global_module(compile_context);

    // dependency modules
    su_pvt->m_dependencies.insert(m_global_module.get_closure_module());

    std::unordered_map<std::string, llvm::Function*>    m_shader_unit_llvm_function;
    // pre-declare all shader interfaces
    for (auto& shader_unit : sg->m_shader_units) {
        auto local_su_pvt = shader_unit.second->get_shader_unit_data();
            
#ifdef DEBUG_OUTPUT
        local_su_pvt->m_module->print(llvm::errs(), nullptr);
#endif
            
        // parse shader unit dependencies
        shader_unit.second->parse_dependencies(su_pvt);

        // declare the root function of the shader unit
        auto function = local_su_pvt->m_ast_root->declare_shader(compile_context);

        // update shader unit root functions
        const auto& name = shader_unit.second->get_name();
        m_shader_unit_llvm_function[name] = function;
    }

    // parse argument types
    const auto& args = sg->get_shader_arguments();
    std::vector<llvm::Type*>	llvm_arg_types(args.size());
    for (auto i = 0; i < args.size(); ++i) {
        auto raw_type = llvm_type_from_arg_type(args[i].m_type, compile_context);
        if (args[i].m_is_output)
            raw_type = raw_type->getPointerTo();

        llvm_arg_types[i] = raw_type;
    }

    // declare the function prototype
    llvm::FunctionType* function_type = llvm::FunctionType::get(get_void_ty(compile_context), llvm_arg_types, false);

    // declare the function
    const auto func_name = sg->get_name() + "_shader_wrapper";
    llvm::Function* function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, func_name, module);

    // function arguments
    std::vector<llvm::Value*>   llvm_args(args.size());
    for (auto i = 0; i < args.size(); ++i)
        llvm_args[i] = function->getArg(i);

    // create a separate code block
    llvm::BasicBlock* wrapper_shader_entry = llvm::BasicBlock::Create(m_llvm_context, "entry", function);
    builder.SetInsertPoint(wrapper_shader_entry);

    // push var table
    compile_context.push_var_symbol_layer();

    // variable mapping keeps track of variables to bridge shader units
    VarMapping var_mapping;

    // generate wrapper shader source code.
    const auto ret = generate_shader_source(compile_context, sg, root_shader, visited_shader_units, current_shader_units_being_visited, var_mapping, m_shader_unit_llvm_function, llvm_args);
    if (!ret)
        return false;
        
    // pop var table
    compile_context.pop_var_symbol_layer();

    // make sure there is a terminator
    builder.CreateRetVoid();
        
    // keep record of the llvm function
    sg->m_shader_unit_data->m_llvm_function = function;
    sg->m_shader_unit_data->m_root_function_name = func_name;

    return true;
}

bool TslCompiler_Impl::generate_shader_source(  LLVM_Compile_Context& context, ShaderGroupTemplate* sg, ShaderUnitTemplate* su, std::unordered_set<const ShaderUnitTemplate*>& visited, std::unordered_set<const ShaderUnitTemplate*>& being_visited, 
                                                VarMapping& var_mapping, const std::unordered_map<std::string, llvm::Function*>& function_mapping , const std::vector<llvm::Value*>& args ) {
    // cycles detected, incorrect shader setup!!!
    if (being_visited.count(su))
        return false;

    // avoid generating code for this shader unit again
    if (visited.count(su))
        return true;

    // push shader unit in cache so that we can detect cycles
    being_visited.insert(su);
    visited.insert(su);

    // check shader unit it depends on
    const std::string shader_unit_name = su->get_name();
    if (sg->m_shader_unit_connections.count(shader_unit_name)) {
        const auto& dependencies = sg->m_shader_unit_connections[shader_unit_name];
        for (const auto& dep : dependencies) {
            const auto& dep_shader_unit_name = dep.second.first;

            // if an undefined shader unit is assigned, simply quit the process
            if (sg->m_shader_units.count(shader_unit_name) == 0)
                return false;

            const auto dep_shader_unit = sg->m_shader_units[dep_shader_unit_name];
            if (!generate_shader_source(context, sg, dep_shader_unit, visited, being_visited, var_mapping, function_mapping, args))
                return false;
        }
    }

    // generate the shader code for this shader unit
    std::vector<llvm::Value*> callee_args;
    for (auto& arg : su->get_shader_unit_data()->m_shader_params) {
        const auto name = arg.m_name;
        const auto type = arg.m_type;
        const auto is_input = !arg.m_is_output;

        if (is_input) {
            auto& connections = sg->m_shader_unit_connections;
            if (connections.count(su->get_name())) {
                auto& connection = connections[su->get_name()];
                if (connection.count(name)) {
                    const auto& source = connection[name];
                    auto var = var_mapping[source.first][source.second];
                    auto loaded_var = context.builder->CreateLoad(var);
                    callee_args.push_back(loaded_var);
                } else {
                    // emit an error here, something is wrong
                }
            }
            else {
                bool need_allocation = true;

                // check if this input is connected with exposed argument of the shader group first
                const auto it = sg->m_input_args.find(shader_unit_name);
                if (it != sg->m_input_args.end()) {
                    const auto& shader_mapping = it->second;
                    const auto it1 = shader_mapping.find(name);
                    if (it1 != shader_mapping.end()) {
                        // this parameter is exposed, use it directly
                        auto value = args[it1->second];
                        callee_args.push_back(value);
                        need_allocation = false;
                    }
                }

                if (need_allocation)
                {
                    llvm::Type* llvm_type = llvm_type_from_arg_type(type, context);
                    if (!llvm_type)
                        return false;
                    llvm_type = llvm_type->getPointerTo();

                    bool has_init_value = false;

                    const auto& mapping = sg->m_shader_input_defaults;
                    const auto it = mapping.find(shader_unit_name);
                    if (it != mapping.end()) {
                        const auto it1 = it->second.find(name);
                        if (it1 != it->second.end()) {
                            const auto& var = it1->second;

                            llvm::Value* llvm_value = nullptr;
                            switch (var.m_type) {
                            case ShaderArgumentTypeEnum::TSL_TYPE_INT:
                                llvm_value = get_llvm_constant_int(var.m_val.m_int, 32, context);
                                break;
                            case ShaderArgumentTypeEnum::TSL_TYPE_FLOAT:
                                llvm_value = get_llvm_constant_fp(var.m_val.m_float, context);
                                break;
                            case ShaderArgumentTypeEnum::TSL_TYPE_DOUBLE:
                                llvm_value = get_llvm_constant_fp(var.m_val.m_double, context);
                                break;
                            case ShaderArgumentTypeEnum::TSL_TYPE_BOOL:
                                llvm_value = get_llvm_constant_int((int)var.m_val.m_bool, 1, context);
                                break;
                            case ShaderArgumentTypeEnum::TSL_TYPE_FLOAT3:
                                llvm_value = get_llvm_constant_float3(var.m_val.m_float3, context);
                                break;
                            default:
                                has_init_value = false;
                                break;
                            }

                            if (llvm_value) {
                                has_init_value = true;
                                callee_args.push_back(llvm_value);
                            }
                        }
                    }

                    if (!has_init_value) {
                        // emit an error here, uninitialized input parameter and it is not connected with anything
                        return false;
                    }
                }
            }
        }
        else {
            bool need_allocation = true;

            // check if this output is connected with exposed argument of the shader group first
            const auto it = sg->m_output_args.find(shader_unit_name);
            if (it != sg->m_output_args.end()) {
                const auto& shader_mapping = it->second;
                const auto it1 = shader_mapping.find(name);
                if (it1 != shader_mapping.end()) {
                    // this parameter is exposed, use it directly
                    auto value = args[it1->second];
                    callee_args.push_back(value);
                    need_allocation = false;
                }
            }

            // if the parameter is not exposed, allocate one
            if (need_allocation) {
                llvm::Type* llvm_type = llvm_type_from_arg_type(type, context);

                auto output_var = context.builder->CreateAlloca(llvm_type, nullptr, name);
                var_mapping[su->get_name()][name] = output_var;
                callee_args.push_back(output_var);
            }
        }
    }

    // make the call
    const auto it = function_mapping.find(su->get_name());
    auto function = it == function_mapping.end() ? nullptr : it->second;
    context.builder->CreateCall(function, callee_args);

    // erase the shader unit from being visited
    being_visited.erase(su);

    return true;
}

TSL_NAMESPACE_END
