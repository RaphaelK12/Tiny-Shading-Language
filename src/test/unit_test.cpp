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

TEST(Gramma, Empty_Shader) {
    validate_shader(R"(
        shader func(){
        }
    )");
}

TEST(Gramma, Standard_Shader) {
    validate_shader(R"(
        shader function_name()
        {
            int k = 0;
            float gg = 0.0;
            // basic variable assign
            test = 1;
            
            // field in data structure
            data_structure.field = 2;
            
            // multi-line code
            data_structure.field2.afd = 3

            ;
            
            // array
            data[23] = 34;
            data.data[213421] = 32;
            data[324].dafa. sdf[21] = 3;
            
            // multi-assign
            SADFAF = ASDFASF = 234;
            
            gg = testagain = 2 + 3;
        }
    )");
}