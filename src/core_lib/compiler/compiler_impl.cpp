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

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
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
	structure->print();
#endif
}

void* TslCompiler_Impl::get_scanner() {
    return m_scanner;
}

bool TslCompiler_Impl::compile(const char* source_code, ShaderUnit* su) {
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
    su_pvt->m_module = std::make_unique<llvm::Module>("shader", m_llvm_context);
    auto module = su_pvt->m_module.get();
	if(!module)
		return false;

    {
        // get the function pointer through execution engine
        su_pvt->m_execution_engine = std::unique_ptr<llvm::ExecutionEngine>(llvm::EngineBuilder(std::move(su_pvt->m_module)).create());

        // Open a new module.
        module->setDataLayout(su_pvt->m_execution_engine->getTargetMachine()->createDataLayout());

        // Create a new pass manager attached to it.
        su_pvt->m_fpm = std::make_unique<llvm::legacy::FunctionPassManager>(module);

        // Do simple "peephole" optimizations and bit-twiddling optzns.
        su_pvt->m_fpm->add(llvm::createInstructionCombiningPass());
        // Re-associate expressions.
        su_pvt->m_fpm->add(llvm::createReassociatePass());
        // Eliminate Common SubExpressions.
        su_pvt->m_fpm->add(llvm::createGVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        su_pvt->m_fpm->add(llvm::createCFGSimplificationPass());

        su_pvt->m_fpm->doInitialization();
    }

	// if there is a legit shader defined, generate LLVM IR
	if(m_ast_root){
		llvm::IRBuilder<> builder(m_llvm_context);

		LLVM_Compile_Context compile_context;
		compile_context.context = &m_llvm_context;
		compile_context.module = module;
		compile_context.builder = &builder;

        m_global_module.declare_closure_tree_types(m_llvm_context, &compile_context.m_structure_type_maps);
		m_global_module.declare_global_function(compile_context);
        for (auto& closure : m_closures_in_shader) {
            // declare the function first.
            auto function = m_global_module.declare_closure_function(closure, compile_context);
            compile_context.m_closures_maps[closure] = function;
        }

		// generate all data structures first
		for( auto& structure : m_structures )
			structure->codegen(compile_context);

        // code gen for all functions
        for( auto& function : m_functions )
            function->codegen(compile_context);

		// generate code for the shader in this module
		llvm::Function* function = m_ast_root->codegen(compile_context);

        // it should be safe to assume llvm function has to be generated, otherwise, the shader is invalid.
        if (!function)
            return false;

        // optimization pass, this is pretty cool because I don't have to implement those sophisticated optimization algorithms.
        // su_pvt->m_fpm->run(*function);

        // make sure the function is valid
        //if( !llvm::verifyFunction(*function, &llvm::errs()) )
		//	return false;

#ifdef DEBUG_OUTPUT
		module->print(llvm::errs(), nullptr);
#endif

        // make sure to link the global closure model
        su_pvt->m_execution_engine->addModule(CloneModule(*m_global_module.get_closure_module()));

        // resolve the function pointer
        su_pvt->m_function_pointer = su_pvt->m_execution_engine->getFunctionAddress(m_ast_root->get_function_name());
	}

	return true;
}

TSL_NAMESPACE_END