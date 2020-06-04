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

#include "test_common.h"

// this is a very simple real practical use case that demonstrating how tsl can fit in a ray tracing renderer.
TEST(ShaderGroup, BasicShaderGroup) {
    // global tsl shading system
    ShadingSystem shading_system;

    // make a shading context for shader compiling, since there is only one thread involved in this unit test, it is good enough.
    auto shading_context = shading_system.make_shading_context();

    // registered closure id
    const auto closure_id = ClosureTypeLambert::RegisterClosure("Lambert", shading_system);

    // the root shader node, this usually matches to the output node in material system
    const auto root_shader_unit = shading_context->compile_shader_unit_template("root_shader", R"(
        shader output_node( in closure in_bxdf , out closure out_bxdf ){
            out_bxdf = in_bxdf * 0.5f;
        }
    )");
    EXPECT_NE(nullptr, root_shader_unit);

    // a bxdf node
    const auto bxdf_shader_unit = shading_context->compile_shader_unit_template("bxdf_shader", R"(
        shader lambert_node( out closure out_bxdf ){
            out_bxdf = make_closure<Lambert>( 111, 4.0f );
        }
    )");
    EXPECT_NE(nullptr, bxdf_shader_unit);

    // make a shader group
    auto shader_group = shading_context->begin_shader_group_template("first shader");
    EXPECT_NE(nullptr, shader_group);

    // add the two shader units in this group
    auto ret = shader_group->add_shader_unit("root_shader", root_shader_unit, true);
    EXPECT_EQ(true, ret);
    ret = shader_group->add_shader_unit("bxdf_shader_test", bxdf_shader_unit);
    EXPECT_EQ(true, ret);

    // setup connections between shader units
    shader_group->connect_shader_units("bxdf_shader_test", "out_bxdf", "root_shader", "in_bxdf");

    // expose the shader interface
    ArgDescriptor arg;
    arg.m_name = "out_bxdf";
    arg.m_type = TSL_TYPE_CLOSURE;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "out_bxdf", arg);

    // resolve the shader group
    ret = shading_context->end_shader_group_template(shader_group);
    EXPECT_EQ(true, ret);

    auto shader_instance = shader_group->make_shader_instance();
    ret = shading_context->resolve_shader_instance(shader_instance.get());
    EXPECT_EQ(true, ret);

    // get the function pointer
    auto raw_function = (void(*)(ClosureTreeNodeBase**))shader_instance->get_function();
    EXPECT_NE(nullptr, raw_function);

    // execute the shader
    ClosureTreeNodeBase* closure = nullptr;
    raw_function(&closure);
    EXPECT_EQ(CLOSURE_MUL, closure->m_id);

    ClosureTreeNodeMul* mul_closure = (ClosureTreeNodeMul*)closure;
    EXPECT_EQ(0.5f, mul_closure->m_weight);
    EXPECT_EQ(closure_id, mul_closure->m_closure->m_id);

    closure = mul_closure->m_closure;
    ClosureTypeLambert* lambert_param = (ClosureTypeLambert*)closure->m_params;
    EXPECT_EQ(111, lambert_param->base_color);
    EXPECT_EQ(4.0f, lambert_param->normal);
}

// This unit test tries to verify that a shader unit can exist in a shader group more than once.
// It could even have different default values if needed.
TEST(ShaderGroup, DuplicateShaderUnits) {
    // global tsl shading system
    ShadingSystem shading_system;

    // make a shading context for shader compiling, since there is only one thread involved in this unit test, it is good enough.
    auto shading_context = shading_system.make_shading_context();

    // registered closure id
    const auto closure_id = ClosureTypeLambert::RegisterClosure("Lambert", shading_system);

    // the root shader node, this usually matches to the output node in material system
    const auto root_shader_unit = shading_context->compile_shader_unit_template("root_shader", R"(
        shader output_node( closure in_bxdf0 , closure in_bxdf1, out closure out_bxdf ){
            out_bxdf = ( in_bxdf0 + in_bxdf1 ) * 0.5f;
        }
    )");
    EXPECT_NE(nullptr, root_shader_unit);

    // a bxdf node
    const auto bxdf_shader_unit = shading_context->compile_shader_unit_template("bxdf_shader", R"(
        shader lambert_node( float test , out closure out_bxdf ){
            out_bxdf = make_closure<Lambert>( 111, test );
        }
    )");
    EXPECT_NE(nullptr, bxdf_shader_unit);

    // make a shader group
    auto shader_group = shading_context->begin_shader_group_template("first shader");
    EXPECT_NE(nullptr, shader_group);

    // add the two shader units in this group
    auto ret = shader_group->add_shader_unit("root_shader", root_shader_unit, true);
    EXPECT_EQ(true, ret);
    ret = shader_group->add_shader_unit("bxdf_shader0", bxdf_shader_unit);
    EXPECT_EQ(true, ret);
    ret = shader_group->add_shader_unit("bxdf_shader1", bxdf_shader_unit);
    EXPECT_EQ(true, ret);

    // setup connections between shader units
    shader_group->connect_shader_units("bxdf_shader0", "out_bxdf", "root_shader", "in_bxdf0");
    shader_group->connect_shader_units("bxdf_shader1", "out_bxdf", "root_shader", "in_bxdf1");

    ShaderUnitInputDefaultValue dv;
    dv.m_type = ShaderArgumentTypeEnum::TSL_TYPE_FLOAT;
    dv.m_val.m_float = 2.0f;
    shader_group->init_shader_input("bxdf_shader0", "test", dv);

    dv.m_type = ShaderArgumentTypeEnum::TSL_TYPE_FLOAT;
    dv.m_val.m_float = 12.0f;
    shader_group->init_shader_input("bxdf_shader1", "test", dv);

    // expose the shader interface
    ArgDescriptor arg;
    arg.m_name = "out_bxdf";
    arg.m_type = TSL_TYPE_CLOSURE;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "out_bxdf", arg);

    // resolve the shader group
    ret = shading_context->end_shader_group_template(shader_group);
    EXPECT_EQ(true, ret);

    auto shader_instance = shader_group->make_shader_instance();
    ret = shading_context->resolve_shader_instance(shader_instance.get());
    EXPECT_EQ(true, ret);

    // get the function pointer
    auto raw_function = (void(*)(ClosureTreeNodeBase**))shader_instance->get_function();
    EXPECT_NE(nullptr, raw_function);

    // execute the shader
    ClosureTreeNodeBase* closure = nullptr;
    raw_function(&closure);
    EXPECT_EQ(CLOSURE_MUL, closure->m_id);

    auto mul_closure = (ClosureTreeNodeMul*)closure;
    EXPECT_EQ(0.5f, mul_closure->m_weight);
    EXPECT_EQ(CLOSURE_ADD, mul_closure->m_closure->m_id);

    auto add_closure = (ClosureTreeNodeAdd*)mul_closure->m_closure;
    EXPECT_EQ(closure_id, add_closure->m_closure0->m_id);
    EXPECT_EQ(closure_id, add_closure->m_closure1->m_id);

    auto lambert_param = (ClosureTypeLambert*)add_closure->m_closure0->m_params;
    EXPECT_EQ(111, lambert_param->base_color);
    EXPECT_EQ(2.0f, lambert_param->normal);

    lambert_param = (ClosureTypeLambert*)add_closure->m_closure1->m_params;
    EXPECT_EQ(111, lambert_param->base_color);
    EXPECT_EQ(12.0f, lambert_param->normal);
}

TEST(ShaderGroup, ShaderGroupWithoutClosure) {
    // global tsl shading system
    ShadingSystem shading_system;

    // make a shading context for shader compiling, since there is only one thread involved in this unit test, it is good enough.
    auto shading_context = shading_system.make_shading_context();

    // the root shader node, this usually matches to the output node in material system
    const auto root_shader_unit = shading_context->compile_shader_unit_template("root_shader", R"(
        shader output_node( float in_bxdf , out float out_bxdf ){
            out_bxdf = in_bxdf * 1231.0f;
        }
    )");
    EXPECT_NE(nullptr, root_shader_unit);

    // a bxdf node
    const auto bxdf_shader_unit = shading_context->compile_shader_unit_template("bxdf_shader", R"(
        shader lambert_node( float in_bxdf , out float out_bxdf , out float dummy ){
            out_bxdf = in_bxdf;
            // dummy = 1.0f;
        }
    )");
    EXPECT_NE(nullptr, bxdf_shader_unit);

    // make a shader group
    auto shader_group = shading_context->begin_shader_group_template("first shader");
    EXPECT_NE(nullptr, shader_group);

    // add the two shader units in this group
    auto ret = shader_group->add_shader_unit("root_shader", root_shader_unit, true);
    EXPECT_EQ(true, ret);
    ret = shader_group->add_shader_unit("bxdf_shader", bxdf_shader_unit);
    EXPECT_EQ(true, ret);

    // setup connections between shader units
    shader_group->connect_shader_units("bxdf_shader", "out_bxdf", "root_shader", "in_bxdf");

    // expose the shader interface
    ArgDescriptor arg;
    arg.m_name = "out_bxdf";
    arg.m_type = TSL_TYPE_FLOAT;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "out_bxdf", arg);

    arg.m_name = "in_bxdf";
    arg.m_type = TSL_TYPE_FLOAT;
    arg.m_is_output = false;
    shader_group->expose_shader_argument("bxdf_shader", "in_bxdf", arg);

    // resolve the shader group
    ret = shading_context->end_shader_group_template(shader_group);
    EXPECT_EQ(true, ret);

    auto shader_instance = shader_group->make_shader_instance();
    ret = shading_context->resolve_shader_instance(shader_instance.get());
    EXPECT_EQ(true, ret);

    // get the function pointer
    auto raw_function = (void(*)(float*, float))shader_instance->get_function();
    EXPECT_NE(nullptr, raw_function);

    // execute the shader
    float closure , in_bxdf = 0.5f;
    raw_function(&closure, in_bxdf);
    EXPECT_EQ(1231.0f * 0.5f, closure);
}

TEST(ShaderGroup, ShaderGroupArgTypes) {
    // global tsl shading system
    ShadingSystem shading_system;

    // make a shading context for shader compiling, since there is only one thread involved in this unit test, it is good enough.
    auto shading_context = shading_system.make_shading_context();

    // registered closure id
    const auto closure_id = ClosureTypeLambert::RegisterClosure("Lambert", shading_system);

    // the root shader node, this usually matches to the output node in material system
    const auto root_shader_unit = shading_context->compile_shader_unit_template("root_shader", R"(
        shader output_node( out int i , out float f , out double d , out bool b , out closure c , out vector vec ){
            i = 123;
            f = 123.0f;
            d = 123.0d;
            b = true;
            c = make_closure<Lambert>( 111, 4.0f );
            vec.x = 1.0f; vec.y = 2.0f; vec.b = 3.0f;
        }
    )");
    EXPECT_NE(nullptr, root_shader_unit);

    // make a shader group
    auto shader_group = shading_context->begin_shader_group_template("first shader");
    EXPECT_NE(nullptr, shader_group);

    // add the two shader units in this group
    auto ret = shader_group->add_shader_unit("root_shader", root_shader_unit, true);
    EXPECT_EQ(true, ret);

    // expose the shader interface
    ArgDescriptor arg;
    arg.m_name = "i";
    arg.m_type = TSL_TYPE_INT;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "i", arg);

    arg.m_name = "f";
    arg.m_type = TSL_TYPE_FLOAT;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "f", arg);

    arg.m_name = "d";
    arg.m_type = TSL_TYPE_DOUBLE;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "d", arg);

    arg.m_name = "b";
    arg.m_type = TSL_TYPE_BOOL;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "b", arg);

    arg.m_name = "f3";
    arg.m_type = TSL_TYPE_FLOAT3;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "vec", arg);

    arg.m_name = "c";
    arg.m_type = TSL_TYPE_CLOSURE;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "c", arg);

    // resolve the shader group
    ret = shading_context->end_shader_group_template(shader_group);
    EXPECT_EQ(true, ret);

    auto shader_instance = shader_group->make_shader_instance();
    ret = shading_context->resolve_shader_instance(shader_instance.get());
    EXPECT_EQ(true, ret);

    // get the function pointer
    auto raw_function = (void(*)(int*, float*, double*, bool*, Tsl_Namespace::float3*, ClosureTreeNodeBase**))shader_instance->get_function();
    EXPECT_NE(nullptr, raw_function);

    // execute the shader
    float f;
    int   i;
    double d;
    bool b;
    Tsl_Namespace::float3 f3;
    ClosureTreeNodeBase* closure = nullptr;
    raw_function(&i, &f, &d, &b, &f3, &closure);
    EXPECT_EQ(123, i);
    EXPECT_EQ(123.0f, f);
    EXPECT_EQ(123.0, d);
    EXPECT_EQ(true, b);
    EXPECT_EQ(1.0f, f3.x);
    EXPECT_EQ(2.0f, f3.y);
    EXPECT_EQ(3.0f, f3.z);

    EXPECT_EQ(closure_id, closure->m_id);
    ClosureTypeLambert* lambert_param = (ClosureTypeLambert*)closure->m_params;
    EXPECT_EQ(111, lambert_param->base_color);
    EXPECT_EQ(4.0f, lambert_param->normal);
}

TEST(ShaderGroup, ShaderGroupInputDefaults) {
    // global tsl shading system
    ShadingSystem shading_system;

    // make a shading context for shader compiling, since there is only one thread involved in this unit test, it is good enough.
    auto shading_context = shading_system.make_shading_context();

    // the root shader node, this usually matches to the output node in material system
    const auto root_shader_unit = shading_context->compile_shader_unit_template("root_shader", R"(
        shader output_node( int ii , float iff , double id , bool ib , vector if3, 
                            out int i , out float f , out double d , out bool b , out vector f3 ){
            i = ii;
            f = iff;
            d = id;
            b = ib;
            f3 = if3;
        }
    )");
    EXPECT_NE(nullptr, root_shader_unit);

    // make a shader group
    auto shader_group = shading_context->begin_shader_group_template("first shader");
    EXPECT_NE(nullptr, shader_group);

    // add the two shader units in this group
    auto ret = shader_group->add_shader_unit("root_shader", root_shader_unit, true);
    EXPECT_EQ(true, ret);

    // expose the shader interface
    ArgDescriptor arg;
    arg.m_name = "i";
    arg.m_type = TSL_TYPE_INT;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "i", arg);

    arg.m_name = "f";
    arg.m_type = TSL_TYPE_FLOAT;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "f", arg);

    arg.m_name = "d";
    arg.m_type = TSL_TYPE_DOUBLE;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "d", arg);

    arg.m_name = "b";
    arg.m_type = TSL_TYPE_BOOL;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "b", arg);

    arg.m_name = "f3";
    arg.m_type = TSL_TYPE_FLOAT3;
    arg.m_is_output = true;
    shader_group->expose_shader_argument("root_shader", "f3", arg);

    ShaderUnitInputDefaultValue ii;
    ii.m_type = ShaderArgumentTypeEnum::TSL_TYPE_INT;
    ii.m_val.m_int = 12;
    shader_group->init_shader_input("root_shader", "ii", ii);

    ShaderUnitInputDefaultValue iff;
    iff.m_type = ShaderArgumentTypeEnum::TSL_TYPE_FLOAT;
    iff.m_val.m_float = 13.0f;
    shader_group->init_shader_input("root_shader", "iff", iff);

    ShaderUnitInputDefaultValue id;
    id.m_type = ShaderArgumentTypeEnum::TSL_TYPE_DOUBLE;
    id.m_val.m_double = 14.0;
    shader_group->init_shader_input("root_shader", "id", id);

    ShaderUnitInputDefaultValue ib;
    ib.m_type = ShaderArgumentTypeEnum::TSL_TYPE_BOOL;
    ib.m_val.m_bool = true;
    shader_group->init_shader_input("root_shader", "ib", ib);

    ShaderUnitInputDefaultValue if3;
    if3.m_type = ShaderArgumentTypeEnum::TSL_TYPE_FLOAT3;
    if3.m_val.m_float3 = { 1.0f , 2.0f , 3.0f };
    shader_group->init_shader_input("root_shader", "if3", if3);

    // resolve the shader group
    ret = shading_context->end_shader_group_template(shader_group);
    EXPECT_EQ(true, ret);

    auto shader_instance = shader_group->make_shader_instance();
    ret = shading_context->resolve_shader_instance(shader_instance.get());
    EXPECT_EQ(true, ret);

    // get the function pointer
    auto raw_function = (void(*)(int*, float*, double*, bool*, Tsl_Namespace::float3*))shader_instance->get_function();
    EXPECT_NE(nullptr, raw_function);

    // execute the shader
    float f;
    int   i;
    double d;
    bool b;
    Tsl_Namespace::float3 f3;
    // Tsl_Namespace::float3 f3;
    raw_function(&i, &f, &d, &b, &f3);
    EXPECT_EQ(12, i);
    EXPECT_EQ(13.0f, f);
    EXPECT_EQ(14.0, d);
    EXPECT_EQ(true, b);

    EXPECT_EQ(1.0f, f3.x);
    EXPECT_EQ(2.0f, f3.y);
    EXPECT_EQ(3.0f, f3.z);
}

// This is an advanced usage of shader group.
// Shader group itself is composed of multiple shader units. However, since shader group itself is also a shader unit,
// this means that a shader group can be used in another shader group as a single shader unit.
// This perfectly matches to the groupping node feature in certain material editors like Blender.
//
//                            inner_shader 
//  -----------------------------------------------------------------
//  |                                         root_shader           |
//  |         bxdf_shader                  -----------------        |
//  |      -----------------               |  output_node  |        |
//  |      |  lambert_node |               |               |        |
//  |      |               |               |        out_bxdf---------
//  |      |        out_bxdf-------------->in_bxdf         |        |
//  |      |           dummy               -----------------        |
//  -------in_bxdf         |                                        |
//  |      -----------------                                        |
//  -----------------------------------------------------------------
//
//
//            inner_shader   
//         -----------------
//         |               |
//         |               |
//         |        out_bxdf-------           final_shader  
//         in_bxdf         |      |        -----------------
//         -----------------      |        |  resolve_node |
//                                |        |               |
//          constant_shader       |        |        out_bxdf
//         -----------------      -------->bxdf0           |
//         | constant_node |      -------->bxdf1           |
//         |               |      |        -----------------
//         |        out_bxdf-------
//         |               |
//         -----------------

TEST(ShaderGroup, ShaderGroupRecursive) {
    // global tsl shading system
    ShadingSystem shading_system;

    // make a shading context for shader compiling, since there is only one thread involved in this unit test, it is good enough.
    auto shading_context = shading_system.make_shading_context();

    ShaderGroupTemplate* shader_group0 = nullptr;
    {
        // the root shader node, this usually matches to the output node in material system
        const auto root_shader_unit = shading_context->compile_shader_unit_template("root_shader", R"(
            shader output_node( float in_bxdf , out float out_bxdf ){
                out_bxdf = in_bxdf * 1231.0f;
            }
        )");
        EXPECT_NE(nullptr, root_shader_unit);

        // a bxdf node
        const auto bxdf_shader_unit = shading_context->compile_shader_unit_template("bxdf_shader", R"(
            shader lambert_node( float in_bxdf , out float out_bxdf , out float dummy ){
                out_bxdf = in_bxdf;
                // dummy = 1.0f;
            }
        )");
        EXPECT_NE(nullptr, bxdf_shader_unit);

        // make a shader group
        shader_group0 = shading_context->begin_shader_group_template("inner_shader");
        EXPECT_NE(nullptr, shader_group0);

        // add the two shader units in this group
        auto ret = shader_group0->add_shader_unit("root_shader", root_shader_unit, true);
        EXPECT_EQ(true, ret);
        ret = shader_group0->add_shader_unit("bxdf_shader", bxdf_shader_unit);
        EXPECT_EQ(true, ret);

        // setup connections between shader units
        shader_group0->connect_shader_units("bxdf_shader", "out_bxdf", "root_shader", "in_bxdf");

        // expose the shader interface
        ArgDescriptor arg;
        arg.m_name = "out_bxdf";
        arg.m_type = TSL_TYPE_FLOAT;
        arg.m_is_output = true;
        shader_group0->expose_shader_argument("root_shader", "out_bxdf", arg);

        arg.m_name = "in_bxdf";
        arg.m_type = TSL_TYPE_FLOAT;
        arg.m_is_output = false;
        shader_group0->expose_shader_argument("bxdf_shader", "in_bxdf", arg);

        // resolve the shader group
        ret = shading_context->end_shader_group_template(shader_group0);
        EXPECT_EQ(true, ret);
    }

    // a bxdf node
    const auto constant_shader_unit = shading_context->compile_shader_unit_template("constant_shader", R"(
            shader constant_node( out float out_bxdf ){
                out_bxdf = 3.0f;
            }
        )");
    EXPECT_NE(nullptr, constant_shader_unit);

    // a bxdf node
    const auto final_shader_unit = shading_context->compile_shader_unit_template("final_shader", R"(
            shader resolve_node( float bxdf0 , float bxdf1 , out float out_bxdf ){
                out_bxdf = bxdf0 + bxdf1;
            }
        )");
    EXPECT_NE(nullptr, final_shader_unit);

    // make another shader group
    auto shader_group1 = shading_context->begin_shader_group_template("outter shader group");
    EXPECT_NE(nullptr, shader_group1);

    auto ret = shader_group1->add_shader_unit("final_shader", final_shader_unit, true);
    EXPECT_EQ(true, ret);
    ret = shader_group1->add_shader_unit("inner_shader", shader_group0);
    EXPECT_EQ(true, ret);
    ret = shader_group1->add_shader_unit("constant_shader", constant_shader_unit);
    EXPECT_EQ(true, ret);

    // setup connections between shader units
    shader_group1->connect_shader_units("inner_shader", "out_bxdf", "final_shader", "bxdf0");
    shader_group1->connect_shader_units("constant_shader", "out_bxdf", "final_shader", "bxdf1");

    // expose the shader interface
    ArgDescriptor arg;
    arg.m_name = "out_bxdf";
    arg.m_type = TSL_TYPE_FLOAT;
    arg.m_is_output = true;
    shader_group1->expose_shader_argument("final_shader", "out_bxdf", arg);

    ShaderUnitInputDefaultValue ii;
    ii.m_type = ShaderArgumentTypeEnum::TSL_TYPE_FLOAT;
    ii.m_val.m_float = 0.2f;
    shader_group1->init_shader_input("inner_shader", "in_bxdf", ii);

    // resolve the shader group
    ret = shading_context->end_shader_group_template(shader_group1);
    EXPECT_EQ(true, ret);

    auto shader_instance = shader_group1->make_shader_instance();
    ret = shading_context->resolve_shader_instance(shader_instance.get());
    EXPECT_EQ(true, ret);

    // get the function pointer
    auto raw_function = (void(*)(float*))shader_instance->get_function();
    EXPECT_NE(nullptr, raw_function);

    // execute the shader
    float closure, in_bxdf = 0.2f;
    raw_function(&closure);
    EXPECT_EQ(1231.0f * 0.2f + 3.0f, closure);
}