// Copyright (c) 2017 Valve Corporation
// Copyright (c) 2017 LunarG Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>
#include <string>

#include "test/opt/pass_fixture.h"
#include "test/opt/pass_utils.h"

namespace spvtools {
namespace opt {
namespace {

using LocalSSAElimTest = PassTest<::testing::Test>;

TEST_F(LocalSSAElimTest, ForLoop) {
  // #version 140
  //
  // in vec4 BC;
  // out float fo;
  //
  // void main()
  // {
  //     float f = 0.0;
  //     for (int i=0; i<4; i++) {
  //       f = f + BC[i];
  //     }
  //     fo = f;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BC %fo
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %f "f"
OpName %i "i"
OpName %BC "BC"
OpName %fo "fo"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BC = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %8
%22 = OpLabel
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
OpStore %f %float_0
OpStore %i %int_0
OpBranch %23
%23 = OpLabel
OpLoopMerge %24 %25 None
OpBranch %26
%26 = OpLabel
%27 = OpLoad %int %i
%28 = OpSLessThan %bool %27 %int_4
OpBranchConditional %28 %29 %24
%29 = OpLabel
%30 = OpLoad %float %f
%31 = OpLoad %int %i
%32 = OpAccessChain %_ptr_Input_float %BC %31
%33 = OpLoad %float %32
%34 = OpFAdd %float %30 %33
OpStore %f %34
OpBranch %25
%25 = OpLabel
%35 = OpLoad %int %i
%36 = OpIAdd %int %35 %int_1
OpStore %i %36
OpBranch %23
%24 = OpLabel
%37 = OpLoad %float %f
OpStore %fo %37
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %8
%22 = OpLabel
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
OpStore %f %float_0
OpStore %i %int_0
OpBranch %23
%23 = OpLabel
%39 = OpPhi %float %float_0 %22 %34 %25
%38 = OpPhi %int %int_0 %22 %36 %25
OpLoopMerge %24 %25 None
OpBranch %26
%26 = OpLabel
%28 = OpSLessThan %bool %38 %int_4
OpBranchConditional %28 %29 %24
%29 = OpLabel
%32 = OpAccessChain %_ptr_Input_float %BC %38
%33 = OpLoad %float %32
%34 = OpFAdd %float %39 %33
OpStore %f %34
OpBranch %25
%25 = OpLabel
%36 = OpIAdd %int %38 %int_1
OpStore %i %36
OpBranch %23
%24 = OpLabel
OpStore %fo %39
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<SSARewritePass>(predefs + before, predefs + after, true,
                                        true);
}

TEST_F(LocalSSAElimTest, NestedForLoop) {
  // #version 450
  //
  // layout (location=0) in mat4 BC;
  // layout (location=0) out float fo;
  //
  // void main()
  // {
  //     float f = 0.0;
  //     for (int i=0; i<4; i++)
  //       for (int j=0; j<4; j++)
  //         f = f + BC[i][j];
  //     fo = f;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BC %fo
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 450
OpName %main "main"
OpName %f "f"
OpName %i "i"
OpName %j "j"
OpName %BC "BC"
OpName %fo "fo"
OpDecorate %BC Location 0
OpDecorate %fo Location 0
%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%mat4v4float = OpTypeMatrix %v4float 4
%_ptr_Input_mat4v4float = OpTypePointer Input %mat4v4float
%BC = OpVariable %_ptr_Input_mat4v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output
)";

  const std::string before =
      R"(
; CHECK: = OpFunction
; CHECK-NEXT: [[entry:%\w+]] = OpLabel
; CHECK: [[outer_header:%\w+]] = OpLabel
; CHECK-NEXT: [[outer_f:%\w+]] = OpPhi %float %float_0 [[entry]] [[inner_f:%\w+]] [[outer_be:%\w+]]
; CHECK-NEXT: [[i:%\w+]] = OpPhi %int %int_0 [[entry]] [[i_next:%\w+]] [[outer_be]]
; CHECK-NEXT: OpSLessThan {{%\w+}} [[i]]
; CHECK: [[inner_pre_header:%\w+]] = OpLabel
; CHECK: [[inner_header:%\w+]] = OpLabel
; CHECK-NEXT: [[inner_f]] = OpPhi %float [[outer_f]] [[inner_pre_header]] [[f_next:%\w+]] [[inner_be:%\w+]]
; CHECK-NEXT: [[j:%\w+]] = OpPhi %int %int_0 [[inner_pre_header]] [[j_next:%\w+]] [[inner_be]]
; CHECK: [[inner_be]] = OpLabel
; CHECK: [[f_next]] = OpFAdd %float [[inner_f]]
; CHECK: [[j_next]] = OpIAdd %int [[j]] %int_1
; CHECK: [[outer_be]] = OpLabel
; CHECK: [[i_next]] = OpIAdd
; CHECK: OpStore %fo [[outer_f]]
%main = OpFunction %void None %9
%24 = OpLabel
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
%j = OpVariable %_ptr_Function_int Function
OpStore %f %float_0
OpStore %i %int_0
OpBranch %25
%25 = OpLabel
%26 = OpLoad %int %i
%27 = OpSLessThan %bool %26 %int_4
OpLoopMerge %28 %29 None
OpBranchConditional %27 %30 %28
%30 = OpLabel
OpStore %j %int_0
OpBranch %31
%31 = OpLabel
%32 = OpLoad %int %j
%33 = OpSLessThan %bool %32 %int_4
OpLoopMerge %50 %34 None
OpBranchConditional %33 %34 %50
%34 = OpLabel
%35 = OpLoad %float %f
%36 = OpLoad %int %i
%37 = OpLoad %int %j
%38 = OpAccessChain %_ptr_Input_float %BC %36 %37
%39 = OpLoad %float %38
%40 = OpFAdd %float %35 %39
OpStore %f %40
%41 = OpLoad %int %j
%42 = OpIAdd %int %41 %int_1
OpStore %j %42
OpBranch %31
%50 = OpLabel
OpBranch %29
%29 = OpLabel
%43 = OpLoad %int %i
%44 = OpIAdd %int %43 %int_1
OpStore %i %44
OpBranch %25
%28 = OpLabel
%45 = OpLoad %float %f
OpStore %fo %45
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndMatch<SSARewritePass>(predefs + before, true);
}

TEST_F(LocalSSAElimTest, ForLoopWithContinue) {
  // #version 140
  //
  // in vec4 BC;
  // out float fo;
  //
  // void main()
  // {
  //     float f = 0.0;
  //     for (int i=0; i<4; i++) {
  //       float t = BC[i];
  //       if (t < 0.0)
  //         continue;
  //       f = f + t;
  //     }
  //     fo = f;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BC %fo
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
)";

  const std::string names =
      R"(OpName %main "main"
OpName %f "f"
OpName %i "i"
OpName %t "t"
OpName %BC "BC"
OpName %fo "fo"
)";

  const std::string predefs2 =
      R"(%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BC = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %9
%23 = OpLabel
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
%t = OpVariable %_ptr_Function_float Function
OpStore %f %float_0
OpStore %i %int_0
OpBranch %24
%24 = OpLabel
OpLoopMerge %25 %26 None
OpBranch %27
%27 = OpLabel
%28 = OpLoad %int %i
%29 = OpSLessThan %bool %28 %int_4
OpBranchConditional %29 %30 %25
%30 = OpLabel
%31 = OpLoad %int %i
%32 = OpAccessChain %_ptr_Input_float %BC %31
%33 = OpLoad %float %32
OpStore %t %33
%34 = OpLoad %float %t
%35 = OpFOrdLessThan %bool %34 %float_0
OpSelectionMerge %36 None
OpBranchConditional %35 %37 %36
%37 = OpLabel
OpBranch %26
%36 = OpLabel
%38 = OpLoad %float %f
%39 = OpLoad %float %t
%40 = OpFAdd %float %38 %39
OpStore %f %40
OpBranch %26
%26 = OpLabel
%41 = OpLoad %int %i
%42 = OpIAdd %int %41 %int_1
OpStore %i %42
OpBranch %24
%25 = OpLabel
%43 = OpLoad %float %f
OpStore %fo %43
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %9
%23 = OpLabel
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
%t = OpVariable %_ptr_Function_float Function
OpStore %f %float_0
OpStore %i %int_0
OpBranch %24
%24 = OpLabel
%45 = OpPhi %float %float_0 %23 %47 %26
%44 = OpPhi %int %int_0 %23 %42 %26
OpLoopMerge %25 %26 None
OpBranch %27
%27 = OpLabel
%29 = OpSLessThan %bool %44 %int_4
OpBranchConditional %29 %30 %25
%30 = OpLabel
%32 = OpAccessChain %_ptr_Input_float %BC %44
%33 = OpLoad %float %32
OpStore %t %33
%35 = OpFOrdLessThan %bool %33 %float_0
OpSelectionMerge %36 None
OpBranchConditional %35 %37 %36
%37 = OpLabel
OpBranch %26
%36 = OpLabel
%40 = OpFAdd %float %45 %33
OpStore %f %40
OpBranch %26
%26 = OpLabel
%47 = OpPhi %float %45 %37 %40 %36
%42 = OpIAdd %int %44 %int_1
OpStore %i %42
OpBranch %24
%25 = OpLabel
OpStore %fo %45
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<SSARewritePass>(predefs + names + predefs2 + before,
                                        predefs + names + predefs2 + after,
                                        true, true);
}

TEST_F(LocalSSAElimTest, ForLoopWithBreak) {
  // #version 140
  //
  // in vec4 BC;
  // out float fo;
  //
  // void main()
  // {
  //     float f = 0.0;
  //     for (int i=0; i<4; i++) {
  //       float t = f + BC[i];
  //       if (t > 1.0)
  //         break;
  //       f = t;
  //     }
  //     fo = f;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BC %fo
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %f "f"
OpName %i "i"
OpName %t "t"
OpName %BC "BC"
OpName %fo "fo"
%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BC = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%float_1 = OpConstant %float 1
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %9
%24 = OpLabel
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
%t = OpVariable %_ptr_Function_float Function
OpStore %f %float_0
OpStore %i %int_0
OpBranch %25
%25 = OpLabel
OpLoopMerge %26 %27 None
OpBranch %28
%28 = OpLabel
%29 = OpLoad %int %i
%30 = OpSLessThan %bool %29 %int_4
OpBranchConditional %30 %31 %26
%31 = OpLabel
%32 = OpLoad %float %f
%33 = OpLoad %int %i
%34 = OpAccessChain %_ptr_Input_float %BC %33
%35 = OpLoad %float %34
%36 = OpFAdd %float %32 %35
OpStore %t %36
%37 = OpLoad %float %t
%38 = OpFOrdGreaterThan %bool %37 %float_1
OpSelectionMerge %39 None
OpBranchConditional %38 %40 %39
%40 = OpLabel
OpBranch %26
%39 = OpLabel
%41 = OpLoad %float %t
OpStore %f %41
OpBranch %27
%27 = OpLabel
%42 = OpLoad %int %i
%43 = OpIAdd %int %42 %int_1
OpStore %i %43
OpBranch %25
%26 = OpLabel
%44 = OpLoad %float %f
OpStore %fo %44
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %9
%24 = OpLabel
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
%t = OpVariable %_ptr_Function_float Function
OpStore %f %float_0
OpStore %i %int_0
OpBranch %25
%25 = OpLabel
%46 = OpPhi %float %float_0 %24 %36 %27
%45 = OpPhi %int %int_0 %24 %43 %27
OpLoopMerge %26 %27 None
OpBranch %28
%28 = OpLabel
%30 = OpSLessThan %bool %45 %int_4
OpBranchConditional %30 %31 %26
%31 = OpLabel
%34 = OpAccessChain %_ptr_Input_float %BC %45
%35 = OpLoad %float %34
%36 = OpFAdd %float %46 %35
OpStore %t %36
%38 = OpFOrdGreaterThan %bool %36 %float_1
OpSelectionMerge %39 None
OpBranchConditional %38 %40 %39
%40 = OpLabel
OpBranch %26
%39 = OpLabel
OpStore %f %36
OpBranch %27
%27 = OpLabel
%43 = OpIAdd %int %45 %int_1
OpStore %i %43
OpBranch %25
%26 = OpLabel
OpStore %fo %46
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<SSARewritePass>(predefs + before, predefs + after, true,
                                        true);
}

TEST_F(LocalSSAElimTest, SwapProblem) {
  // #version 140
  //
  // in float fe;
  // out float fo;
  //
  // void main()
  // {
  //     float f1 = 0.0;
  //     float f2 = 1.0;
  //     int ie = int(fe);
  //     for (int i=0; i<ie; i++) {
  //       float t = f1;
  //       f1 = f2;
  //       f2 = t;
  //     }
  //     fo = f1;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %fe %fo
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %f1 "f1"
OpName %f2 "f2"
OpName %ie "ie"
OpName %fe "fe"
OpName %i "i"
OpName %t "t"
OpName %fo "fo"
%void = OpTypeVoid
%11 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%float_1 = OpConstant %float 1
%int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
%_ptr_Input_float = OpTypePointer Input %float
%fe = OpVariable %_ptr_Input_float Input
%int_0 = OpConstant %int 0
%bool = OpTypeBool
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %11
%23 = OpLabel
%f1 = OpVariable %_ptr_Function_float Function
%f2 = OpVariable %_ptr_Function_float Function
%ie = OpVariable %_ptr_Function_int Function
%i = OpVariable %_ptr_Function_int Function
%t = OpVariable %_ptr_Function_float Function
OpStore %f1 %float_0
OpStore %f2 %float_1
%24 = OpLoad %float %fe
%25 = OpConvertFToS %int %24
OpStore %ie %25
OpStore %i %int_0
OpBranch %26
%26 = OpLabel
OpLoopMerge %27 %28 None
OpBranch %29
%29 = OpLabel
%30 = OpLoad %int %i
%31 = OpLoad %int %ie
%32 = OpSLessThan %bool %30 %31
OpBranchConditional %32 %33 %27
%33 = OpLabel
%34 = OpLoad %float %f1
OpStore %t %34
%35 = OpLoad %float %f2
OpStore %f1 %35
%36 = OpLoad %float %t
OpStore %f2 %36
OpBranch %28
%28 = OpLabel
%37 = OpLoad %int %i
%38 = OpIAdd %int %37 %int_1
OpStore %i %38
OpBranch %26
%27 = OpLabel
%39 = OpLoad %float %f1
OpStore %fo %39
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %11
%23 = OpLabel
%f1 = OpVariable %_ptr_Function_float Function
%f2 = OpVariable %_ptr_Function_float Function
%ie = OpVariable %_ptr_Function_int Function
%i = OpVariable %_ptr_Function_int Function
%t = OpVariable %_ptr_Function_float Function
OpStore %f1 %float_0
OpStore %f2 %float_1
%24 = OpLoad %float %fe
%25 = OpConvertFToS %int %24
OpStore %ie %25
OpStore %i %int_0
OpBranch %26
%26 = OpLabel
%43 = OpPhi %float %float_1 %23 %42 %28
%42 = OpPhi %float %float_0 %23 %43 %28
%40 = OpPhi %int %int_0 %23 %38 %28
OpLoopMerge %27 %28 None
OpBranch %29
%29 = OpLabel
%32 = OpSLessThan %bool %40 %25
OpBranchConditional %32 %33 %27
%33 = OpLabel
OpStore %t %42
OpStore %f1 %43
OpStore %f2 %42
OpBranch %28
%28 = OpLabel
%38 = OpIAdd %int %40 %int_1
OpStore %i %38
OpBranch %26
%27 = OpLabel
OpStore %fo %42
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<SSARewritePass>(predefs + before, predefs + after, true,
                                        true);
}

TEST_F(LocalSSAElimTest, LostCopyProblem) {
  // #version 140
  //
  // in vec4 BC;
  // out float fo;
  //
  // void main()
  // {
  //     float f = 0.0;
  //     float t;
  //     for (int i=0; i<4; i++) {
  //       t = f;
  //       f = f + BC[i];
  //       if (f > 1.0)
  //         break;
  //     }
  //     fo = t;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BC %fo
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %f "f"
OpName %i "i"
OpName %t "t"
OpName %BC "BC"
OpName %fo "fo"
%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BC = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%float_1 = OpConstant %float 1
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %9
%24 = OpLabel
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
%t = OpVariable %_ptr_Function_float Function
OpStore %f %float_0
OpStore %i %int_0
OpBranch %25
%25 = OpLabel
OpLoopMerge %26 %27 None
OpBranch %28
%28 = OpLabel
%29 = OpLoad %int %i
%30 = OpSLessThan %bool %29 %int_4
OpBranchConditional %30 %31 %26
%31 = OpLabel
%32 = OpLoad %float %f
OpStore %t %32
%33 = OpLoad %float %f
%34 = OpLoad %int %i
%35 = OpAccessChain %_ptr_Input_float %BC %34
%36 = OpLoad %float %35
%37 = OpFAdd %float %33 %36
OpStore %f %37
%38 = OpLoad %float %f
%39 = OpFOrdGreaterThan %bool %38 %float_1
OpSelectionMerge %40 None
OpBranchConditional %39 %41 %40
%41 = OpLabel
OpBranch %26
%40 = OpLabel
OpBranch %27
%27 = OpLabel
%42 = OpLoad %int %i
%43 = OpIAdd %int %42 %int_1
OpStore %i %43
OpBranch %25
%26 = OpLabel
%44 = OpLoad %float %t
OpStore %fo %44
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%49 = OpUndef %float
%main = OpFunction %void None %9
%24 = OpLabel
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
%t = OpVariable %_ptr_Function_float Function
OpStore %f %float_0
OpStore %i %int_0
OpBranch %25
%25 = OpLabel
%46 = OpPhi %float %float_0 %24 %37 %27
%45 = OpPhi %int %int_0 %24 %43 %27
%48 = OpPhi %float %49 %24 %46 %27
OpLoopMerge %26 %27 None
OpBranch %28
%28 = OpLabel
%30 = OpSLessThan %bool %45 %int_4
OpBranchConditional %30 %31 %26
%31 = OpLabel
OpStore %t %46
%35 = OpAccessChain %_ptr_Input_float %BC %45
%36 = OpLoad %float %35
%37 = OpFAdd %float %46 %36
OpStore %f %37
%39 = OpFOrdGreaterThan %bool %37 %float_1
OpSelectionMerge %40 None
OpBranchConditional %39 %41 %40
%41 = OpLabel
OpBranch %26
%40 = OpLabel
OpBranch %27
%27 = OpLabel
%43 = OpIAdd %int %45 %int_1
OpStore %i %43
OpBranch %25
%26 = OpLabel
%47 = OpPhi %float %48 %28 %46 %41
OpStore %fo %47
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<SSARewritePass>(predefs + before, predefs + after, true,
                                        true);
}

TEST_F(LocalSSAElimTest, IfThenElse) {
  // #version 140
  //
  // in vec4 BaseColor;
  // in float f;
  //
  // void main()
  // {
  //     vec4 v;
  //     if (f >= 0)
  //       v = BaseColor * 0.5;
  //     else
  //       v = BaseColor + vec4(1.0,1.0,1.0,1.0);
  //     gl_FragColor = v;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %f %BaseColor %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %f "f"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Input_float = OpTypePointer Input %float
%f = OpVariable %_ptr_Input_float Input
%float_0 = OpConstant %float 0
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%float_0_5 = OpConstant %float 0.5
%float_1 = OpConstant %float 1
%18 = OpConstantComposite %v4float %float_1 %float_1 %float_1 %float_1
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %8
%20 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%21 = OpLoad %float %f
%22 = OpFOrdGreaterThanEqual %bool %21 %float_0
OpSelectionMerge %23 None
OpBranchConditional %22 %24 %25
%24 = OpLabel
%26 = OpLoad %v4float %BaseColor
%27 = OpVectorTimesScalar %v4float %26 %float_0_5
OpStore %v %27
OpBranch %23
%25 = OpLabel
%28 = OpLoad %v4float %BaseColor
%29 = OpFAdd %v4float %28 %18
OpStore %v %29
OpBranch %23
%23 = OpLabel
%30 = OpLoad %v4float %v
OpStore %gl_FragColor %30
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %8
%20 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%21 = OpLoad %float %f
%22 = OpFOrdGreaterThanEqual %bool %21 %float_0
OpSelectionMerge %23 None
OpBranchConditional %22 %24 %25
%24 = OpLabel
%26 = OpLoad %v4float %BaseColor
%27 = OpVectorTimesScalar %v4float %26 %float_0_5
OpStore %v %27
OpBranch %23
%25 = OpLabel
%28 = OpLoad %v4float %BaseColor
%29 = OpFAdd %v4float %28 %18
OpStore %v %29
OpBranch %23
%23 = OpLabel
%31 = OpPhi %v4float %27 %24 %29 %25
OpStore %gl_FragColor %31
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<SSARewritePass>(predefs + before, predefs + after, true,
                                        true);
}

TEST_F(LocalSSAElimTest, IfThen) {
  // #version 140
  //
  // in vec4 BaseColor;
  // in float f;
  //
  // void main()
  // {
  //     vec4 v = BaseColor;
  //     if (f <= 0)
  //       v = v * 0.5;
  //     gl_FragColor = v;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %f %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %f "f"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%f = OpVariable %_ptr_Input_float Input
%float_0 = OpConstant %float 0
%bool = OpTypeBool
%float_0_5 = OpConstant %float 0.5
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %8
%18 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%19 = OpLoad %v4float %BaseColor
OpStore %v %19
%20 = OpLoad %float %f
%21 = OpFOrdLessThanEqual %bool %20 %float_0
OpSelectionMerge %22 None
OpBranchConditional %21 %23 %22
%23 = OpLabel
%24 = OpLoad %v4float %v
%25 = OpVectorTimesScalar %v4float %24 %float_0_5
OpStore %v %25
OpBranch %22
%22 = OpLabel
%26 = OpLoad %v4float %v
OpStore %gl_FragColor %26
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %8
%18 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%19 = OpLoad %v4float %BaseColor
OpStore %v %19
%20 = OpLoad %float %f
%21 = OpFOrdLessThanEqual %bool %20 %float_0
OpSelectionMerge %22 None
OpBranchConditional %21 %23 %22
%23 = OpLabel
%25 = OpVectorTimesScalar %v4float %19 %float_0_5
OpStore %v %25
OpBranch %22
%22 = OpLabel
%27 = OpPhi %v4float %19 %18 %25 %23
OpStore %gl_FragColor %27
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<SSARewritePass>(predefs + before, predefs + after, true,
                                        true);
}

TEST_F(LocalSSAElimTest, Switch) {
  // #version 140
  //
  // in vec4 BaseColor;
  // in float f;
  //
  // void main()
  // {
  //     vec4 v = BaseColor;
  //     int i = int(f);
  //     switch (i) {
  //       case 0:
  //         v = v * 0.25;
  //         break;
  //       case 1:
  //         v = v * 0.625;
  //         break;
  //       case 2:
  //         v = v * 0.75;
  //         break;
  //       default:
  //         break;
  //     }
  //     gl_FragColor = v;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %f %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %i "i"
OpName %f "f"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
%_ptr_Input_float = OpTypePointer Input %float
%f = OpVariable %_ptr_Input_float Input
%float_0_25 = OpConstant %float 0.25
%float_0_625 = OpConstant %float 0.625
%float_0_75 = OpConstant %float 0.75
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %9
%21 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%i = OpVariable %_ptr_Function_int Function
%22 = OpLoad %v4float %BaseColor
OpStore %v %22
%23 = OpLoad %float %f
%24 = OpConvertFToS %int %23
OpStore %i %24
%25 = OpLoad %int %i
OpSelectionMerge %26 None
OpSwitch %25 %27 0 %28 1 %29 2 %30
%27 = OpLabel
OpBranch %26
%28 = OpLabel
%31 = OpLoad %v4float %v
%32 = OpVectorTimesScalar %v4float %31 %float_0_25
OpStore %v %32
OpBranch %26
%29 = OpLabel
%33 = OpLoad %v4float %v
%34 = OpVectorTimesScalar %v4float %33 %float_0_625
OpStore %v %34
OpBranch %26
%30 = OpLabel
%35 = OpLoad %v4float %v
%36 = OpVectorTimesScalar %v4float %35 %float_0_75
OpStore %v %36
OpBranch %26
%26 = OpLabel
%37 = OpLoad %v4float %v
OpStore %gl_FragColor %37
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %9
%21 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%i = OpVariable %_ptr_Function_int Function
%22 = OpLoad %v4float %BaseColor
OpStore %v %22
%23 = OpLoad %float %f
%24 = OpConvertFToS %int %23
OpStore %i %24
OpSelectionMerge %26 None
OpSwitch %24 %27 0 %28 1 %29 2 %30
%27 = OpLabel
OpBranch %26
%28 = OpLabel
%32 = OpVectorTimesScalar %v4float %22 %float_0_25
OpStore %v %32
OpBranch %26
%29 = OpLabel
%34 = OpVectorTimesScalar %v4float %22 %float_0_625
OpStore %v %34
OpBranch %26
%30 = OpLabel
%36 = OpVectorTimesScalar %v4float %22 %float_0_75
OpStore %v %36
OpBranch %26
%26 = OpLabel
%38 = OpPhi %v4float %22 %27 %32 %28 %34 %29 %36 %30
OpStore %gl_FragColor %38
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<SSARewritePass>(predefs + before, predefs + after, true,
                                        true);
}

TEST_F(LocalSSAElimTest, SwitchWithFallThrough) {
  // #version 140
  //
  // in vec4 BaseColor;
  // in float f;
  //
  // void main()
  // {
  //     vec4 v = BaseColor;
  //     int i = int(f);
  //     switch (i) {
  //       case 0:
  //         v = v * 0.25;
  //         break;
  //       case 1:
  //         v = v + 0.25;
  //       case 2:
  //         v = v * 0.75;
  //         break;
  //       default:
  //         break;
  //     }
  //     gl_FragColor = v;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %f %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %i "i"
OpName %f "f"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
%_ptr_Input_float = OpTypePointer Input %float
%f = OpVariable %_ptr_Input_float Input
%float_0_25 = OpConstant %float 0.25
%float_0_75 = OpConstant %float 0.75
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %9
%20 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%i = OpVariable %_ptr_Function_int Function
%21 = OpLoad %v4float %BaseColor
OpStore %v %21
%22 = OpLoad %float %f
%23 = OpConvertFToS %int %22
OpStore %i %23
%24 = OpLoad %int %i
OpSelectionMerge %25 None
OpSwitch %24 %26 0 %27 1 %28 2 %29
%26 = OpLabel
OpBranch %25
%27 = OpLabel
%30 = OpLoad %v4float %v
%31 = OpVectorTimesScalar %v4float %30 %float_0_25
OpStore %v %31
OpBranch %25
%28 = OpLabel
%32 = OpLoad %v4float %v
%33 = OpCompositeConstruct %v4float %float_0_25 %float_0_25 %float_0_25 %float_0_25
%34 = OpFAdd %v4float %32 %33
OpStore %v %34
OpBranch %29
%29 = OpLabel
%35 = OpLoad %v4float %v
%36 = OpVectorTimesScalar %v4float %35 %float_0_75
OpStore %v %36
OpBranch %25
%25 = OpLabel
%37 = OpLoad %v4float %v
OpStore %gl_FragColor %37
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %9
%20 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%i = OpVariable %_ptr_Function_int Function
%21 = OpLoad %v4float %BaseColor
OpStore %v %21
%22 = OpLoad %float %f
%23 = OpConvertFToS %int %22
OpStore %i %23
OpSelectionMerge %25 None
OpSwitch %23 %26 0 %27 1 %28 2 %29
%26 = OpLabel
OpBranch %25
%27 = OpLabel
%31 = OpVectorTimesScalar %v4float %21 %float_0_25
OpStore %v %31
OpBranch %25
%28 = OpLabel
%33 = OpCompositeConstruct %v4float %float_0_25 %float_0_25 %float_0_25 %float_0_25
%34 = OpFAdd %v4float %21 %33
OpStore %v %34
OpBranch %29
%29 = OpLabel
%38 = OpPhi %v4float %21 %20 %34 %28
%36 = OpVectorTimesScalar %v4float %38 %float_0_75
OpStore %v %36
OpBranch %25
%25 = OpLabel
%39 = OpPhi %v4float %21 %26 %31 %27 %36 %29
OpStore %gl_FragColor %39
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<SSARewritePass>(predefs + before, predefs + after, true,
                                        true);
}

TEST_F(LocalSSAElimTest, DontPatchPhiInLoopHeaderThatIsNotAVar) {
  // From https://github.com/KhronosGroup/SPIRV-Tools/issues/826
  // Don't try patching the (%16 %7) value/predecessor pair in the OpPhi.
  // That OpPhi is unrelated to this optimization: we did not set that up
  // in the SSA initialization for the loop header block.
  // The pass should be a no-op on this module.

  const std::string before = R"(OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %1 "main"
%void = OpTypeVoid
%3 = OpTypeFunction %void
%float = OpTypeFloat 32
%float_1 = OpConstant %float 1
%1 = OpFunction %void None %3
%6 = OpLabel
OpBranch %7
%7 = OpLabel
%8 = OpPhi %float %float_1 %6 %9 %7
%9 = OpFAdd %float %8 %float_1
OpLoopMerge %10 %7 None
OpBranch %7
%10 = OpLabel
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<SSARewritePass>(before, before, true, true);
}

TEST_F(LocalSSAElimTest, OptInitializedVariableLikeStore) {
  // Note: SPIR-V edited to change store to v into variable initialization
  //
  // #version 450
  //
  // layout (location=0) in vec4 iColor;
  // layout (location=1) in float fi;
  // layout (location=0) out vec4 oColor;
  //
  // void main()
  // {
  //     vec4 v = vec4(0.0);
  //     if (fi < 0.0)
  //       v.x = iColor.x;
  //     oColor = v;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %fi %iColor %oColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 450
OpName %main "main"
OpName %v "v"
OpName %fi "fi"
OpName %iColor "iColor"
OpName %oColor "oColor"
OpDecorate %fi Location 1
OpDecorate %iColor Location 0
OpDecorate %oColor Location 0
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%float_0 = OpConstant %float 0
%13 = OpConstantComposite %v4float %float_0 %float_0 %float_0 %float_0
%_ptr_Input_float = OpTypePointer Input %float
%fi = OpVariable %_ptr_Input_float Input
%bool = OpTypeBool
%_ptr_Input_v4float = OpTypePointer Input %v4float
%iColor = OpVariable %_ptr_Input_v4float Input
%uint = OpTypeInt 32 0
%uint_0 = OpConstant %uint 0
%_ptr_Function_float = OpTypePointer Function %float
%_ptr_Output_v4float = OpTypePointer Output %v4float
%oColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string func_before =
      R"(%main = OpFunction %void None %8
%21 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function %13
%22 = OpLoad %float %fi
%23 = OpFOrdLessThan %bool %22 %float_0
OpSelectionMerge %24 None
OpBranchConditional %23 %25 %24
%25 = OpLabel
%26 = OpAccessChain %_ptr_Input_float %iColor %uint_0
%27 = OpLoad %float %26
%28 = OpLoad %v4float %v
%29 = OpCompositeInsert %v4float %27 %28 0
OpStore %v %29
OpBranch %24
%24 = OpLabel
%30 = OpLoad %v4float %v
OpStore %oColor %30
OpReturn
OpFunctionEnd
)";

  const std::string func_after =
      R"(%main = OpFunction %void None %8
%21 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function %13
%22 = OpLoad %float %fi
%23 = OpFOrdLessThan %bool %22 %float_0
OpSelectionMerge %24 None
OpBranchConditional %23 %25 %24
%25 = OpLabel
%26 = OpAccessChain %_ptr_Input_float %iColor %uint_0
%27 = OpLoad %float %26
%29 = OpCompositeInsert %v4float %27 %13 0
OpStore %v %29
OpBranch %24
%24 = OpLabel
%31 = OpPhi %v4float %13 %21 %29 %25
OpStore %oColor %31
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<SSARewritePass>(predefs + func_before,
                                        predefs + func_after, true, true);
}

TEST_F(LocalSSAElimTest, PointerVariable) {
  // Test that checks if a pointer variable is removed.

  const std::string before =
      R"(OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %1 "main" %2
OpExecutionMode %1 OriginUpperLeft
OpMemberDecorate %_struct_3 0 Offset 0
OpDecorate %_runtimearr__struct_3 ArrayStride 16
OpMemberDecorate %_struct_5 0 Offset 0
OpDecorate %_struct_5 BufferBlock
OpMemberDecorate %_struct_6 0 Offset 0
OpDecorate %_struct_6 BufferBlock
OpDecorate %2 Location 0
OpDecorate %7 DescriptorSet 0
OpDecorate %7 Binding 0
%void = OpTypeVoid
%10 = OpTypeFunction %void
%int = OpTypeInt 32 1
%uint = OpTypeInt 32 0
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
%_ptr_Uniform_v4float = OpTypePointer Uniform %v4float
%_struct_3 = OpTypeStruct %v4float
%_runtimearr__struct_3 = OpTypeRuntimeArray %_struct_3
%_struct_5 = OpTypeStruct %_runtimearr__struct_3
%_ptr_Uniform__struct_5 = OpTypePointer Uniform %_struct_5
%_struct_6 = OpTypeStruct %int
%_ptr_Uniform__struct_6 = OpTypePointer Uniform %_struct_6
%_ptr_Function__ptr_Uniform__struct_5 = OpTypePointer Function %_ptr_Uniform__struct_5
%_ptr_Function__ptr_Uniform__struct_6 = OpTypePointer Function %_ptr_Uniform__struct_6
%int_0 = OpConstant %int 0
%uint_0 = OpConstant %uint 0
%2 = OpVariable %_ptr_Output_v4float Output
%7 = OpVariable %_ptr_Uniform__struct_5 Uniform
%1 = OpFunction %void None %10
%23 = OpLabel
%24 = OpVariable %_ptr_Function__ptr_Uniform__struct_5 Function
OpStore %24 %7
%26 = OpLoad %_ptr_Uniform__struct_5 %24
%27 = OpAccessChain %_ptr_Uniform_v4float %26 %int_0 %uint_0 %int_0
%28 = OpLoad %v4float %27
%29 = OpCopyObject %v4float %28
OpStore %2 %28
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %1 "main" %2
OpExecutionMode %1 OriginUpperLeft
OpMemberDecorate %_struct_3 0 Offset 0
OpDecorate %_runtimearr__struct_3 ArrayStride 16
OpMemberDecorate %_struct_5 0 Offset 0
OpDecorate %_struct_5 BufferBlock
OpMemberDecorate %_struct_6 0 Offset 0
OpDecorate %_struct_6 BufferBlock
OpDecorate %2 Location 0
OpDecorate %7 DescriptorSet 0
OpDecorate %7 Binding 0
%void = OpTypeVoid
%10 = OpTypeFunction %void
%int = OpTypeInt 32 1
%uint = OpTypeInt 32 0
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
%_ptr_Uniform_v4float = OpTypePointer Uniform %v4float
%_struct_3 = OpTypeStruct %v4float
%_runtimearr__struct_3 = OpTypeRuntimeArray %_struct_3
%_struct_5 = OpTypeStruct %_runtimearr__struct_3
%_ptr_Uniform__struct_5 = OpTypePointer Uniform %_struct_5
%_struct_6 = OpTypeStruct %int
%_ptr_Uniform__struct_6 = OpTypePointer Uniform %_struct_6
%_ptr_Function__ptr_Uniform__struct_5 = OpTypePointer Function %_ptr_Uniform__struct_5
%_ptr_Function__ptr_Uniform__struct_6 = OpTypePointer Function %_ptr_Uniform__struct_6
%int_0 = OpConstant %int 0
%uint_0 = OpConstant %uint 0
%2 = OpVariable %_ptr_Output_v4float Output
%7 = OpVariable %_ptr_Uniform__struct_5 Uniform
%1 = OpFunction %void None %10
%23 = OpLabel
%24 = OpVariable %_ptr_Function__ptr_Uniform__struct_5 Function
OpStore %24 %7
%27 = OpAccessChain %_ptr_Uniform_v4float %7 %int_0 %uint_0 %int_0
%28 = OpLoad %v4float %27
%29 = OpCopyObject %v4float %28
OpStore %2 %28
OpReturn
OpFunctionEnd
)";

  // Relax logical pointers to allow pointer allocations.
  SetAssembleOptions(SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  ValidatorOptions()->relax_logical_pointer = true;
  SinglePassRunAndCheck<SSARewritePass>(before, after, true, true);
}

TEST_F(LocalSSAElimTest, VerifyInstToBlockMap) {
  // #version 140
  //
  // in vec4 BC;
  // out float fo;
  //
  // void main()
  // {
  //     float f = 0.0;
  //     for (int i=0; i<4; i++) {
  //       f = f + BC[i];
  //     }
  //     fo = f;
  // }

  const std::string text = R"(
OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BC %fo
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %f "f"
OpName %i "i"
OpName %BC "BC"
OpName %fo "fo"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BC = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output
%main = OpFunction %void None %8
%22 = OpLabel
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
OpStore %f %float_0
OpStore %i %int_0
OpBranch %23
%23 = OpLabel
OpLoopMerge %24 %25 None
OpBranch %26
%26 = OpLabel
%27 = OpLoad %int %i
%28 = OpSLessThan %bool %27 %int_4
OpBranchConditional %28 %29 %24
%29 = OpLabel
%30 = OpLoad %float %f
%31 = OpLoad %int %i
%32 = OpAccessChain %_ptr_Input_float %BC %31
%33 = OpLoad %float %32
%34 = OpFAdd %float %30 %33
OpStore %f %34
OpBranch %25
%25 = OpLabel
%35 = OpLoad %int %i
%36 = OpIAdd %int %35 %int_1
OpStore %i %36
OpBranch %23
%24 = OpLabel
%37 = OpLoad %float %f
OpStore %fo %37
OpReturn
OpFunctionEnd
)";

  std::unique_ptr<IRContext> context =
      BuildModule(SPV_ENV_UNIVERSAL_1_1, nullptr, text,
                  SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  EXPECT_NE(nullptr, context);

  // Force the instruction to block mapping to get built.
  context->get_instr_block(27u);

  auto pass = MakeUnique<SSARewritePass>();
  pass->SetMessageConsumer(nullptr);
  const auto status = pass->Run(context.get());
  EXPECT_TRUE(status == Pass::Status::SuccessWithChange);
}

TEST_F(LocalSSAElimTest, CompositeExtractProblem) {
  const std::string spv_asm = R"(
               OpCapability Tessellation
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint TessellationControl %2 "main" %16 %17 %18 %20 %22 %26 %27 %30 %31
       %void = OpTypeVoid
          %4 = OpTypeFunction %void
      %float = OpTypeFloat 32
    %v4float = OpTypeVector %float 4
       %uint = OpTypeInt 32 0
     %uint_3 = OpConstant %uint 3
    %v3float = OpTypeVector %float 3
    %v2float = OpTypeVector %float 2
 %_struct_11 = OpTypeStruct %v4float %v4float %v4float %v3float %v3float %v2float %v2float
%_arr__struct_11_uint_3 = OpTypeArray %_struct_11 %uint_3
%_ptr_Function__arr__struct_11_uint_3 = OpTypePointer Function %_arr__struct_11_uint_3
%_arr_v4float_uint_3 = OpTypeArray %v4float %uint_3
%_ptr_Input__arr_v4float_uint_3 = OpTypePointer Input %_arr_v4float_uint_3
         %16 = OpVariable %_ptr_Input__arr_v4float_uint_3 Input
         %17 = OpVariable %_ptr_Input__arr_v4float_uint_3 Input
         %18 = OpVariable %_ptr_Input__arr_v4float_uint_3 Input
%_ptr_Input_uint = OpTypePointer Input %uint
         %20 = OpVariable %_ptr_Input_uint Input
%_ptr_Output__arr_v4float_uint_3 = OpTypePointer Output %_arr_v4float_uint_3
         %22 = OpVariable %_ptr_Output__arr_v4float_uint_3 Output
%_ptr_Output_v4float = OpTypePointer Output %v4float
%_arr_v3float_uint_3 = OpTypeArray %v3float %uint_3
%_ptr_Input__arr_v3float_uint_3 = OpTypePointer Input %_arr_v3float_uint_3
         %26 = OpVariable %_ptr_Input__arr_v3float_uint_3 Input
         %27 = OpVariable %_ptr_Input__arr_v3float_uint_3 Input
%_arr_v2float_uint_3 = OpTypeArray %v2float %uint_3
%_ptr_Input__arr_v2float_uint_3 = OpTypePointer Input %_arr_v2float_uint_3
         %30 = OpVariable %_ptr_Input__arr_v2float_uint_3 Input
         %31 = OpVariable %_ptr_Input__arr_v2float_uint_3 Input
%_ptr_Function__struct_11 = OpTypePointer Function %_struct_11
          %2 = OpFunction %void None %4
         %33 = OpLabel
         %66 = OpVariable %_ptr_Function__arr__struct_11_uint_3 Function
         %34 = OpLoad %_arr_v4float_uint_3 %16
         %35 = OpLoad %_arr_v4float_uint_3 %17
         %36 = OpLoad %_arr_v4float_uint_3 %18
         %37 = OpLoad %_arr_v3float_uint_3 %26
         %38 = OpLoad %_arr_v3float_uint_3 %27
         %39 = OpLoad %_arr_v2float_uint_3 %30
         %40 = OpLoad %_arr_v2float_uint_3 %31
         %41 = OpCompositeExtract %v4float %34 0
         %42 = OpCompositeExtract %v4float %35 0
         %43 = OpCompositeExtract %v4float %36 0
         %44 = OpCompositeExtract %v3float %37 0
         %45 = OpCompositeExtract %v3float %38 0
         %46 = OpCompositeExtract %v2float %39 0
         %47 = OpCompositeExtract %v2float %40 0
         %48 = OpCompositeConstruct %_struct_11 %41 %42 %43 %44 %45 %46 %47
         %49 = OpCompositeExtract %v4float %34 1
         %50 = OpCompositeExtract %v4float %35 1
         %51 = OpCompositeExtract %v4float %36 1
         %52 = OpCompositeExtract %v3float %37 1
         %53 = OpCompositeExtract %v3float %38 1
         %54 = OpCompositeExtract %v2float %39 1
         %55 = OpCompositeExtract %v2float %40 1
         %56 = OpCompositeConstruct %_struct_11 %49 %50 %51 %52 %53 %54 %55
         %57 = OpCompositeExtract %v4float %34 2
         %58 = OpCompositeExtract %v4float %35 2
         %59 = OpCompositeExtract %v4float %36 2
         %60 = OpCompositeExtract %v3float %37 2
         %61 = OpCompositeExtract %v3float %38 2
         %62 = OpCompositeExtract %v2float %39 2
         %63 = OpCompositeExtract %v2float %40 2
         %64 = OpCompositeConstruct %_struct_11 %57 %58 %59 %60 %61 %62 %63
         %65 = OpCompositeConstruct %_arr__struct_11_uint_3 %48 %56 %64
         %67 = OpLoad %uint %20

; CHECK OpStore {{%\d+}} [[store_source:%\d+]]
               OpStore %66 %65
         %68 = OpAccessChain %_ptr_Function__struct_11 %66 %67

; This load was being removed, because %_ptr_Function__struct_11 was being
; wrongfully considered an SSA target.
; CHECK OpLoad %_struct_11 %68
         %69 = OpLoad %_struct_11 %68

; Similarly, %69 cannot be replaced with %65.
; CHECK-NOT: OpCompositeExtract %v4float [[store_source]] 0
         %70 = OpCompositeExtract %v4float %69 0

         %71 = OpAccessChain %_ptr_Output_v4float %22 %67
               OpStore %71 %70
               OpReturn
               OpFunctionEnd)";

  SinglePassRunAndMatch<SSARewritePass>(spv_asm, true);
}

// Test that the RelaxedPrecision decoration on the variable to added to the
// result of the OpPhi instruction.
TEST_F(LocalSSAElimTest, DecoratedVariable) {
  const std::string spv_asm = R"(
; CHECK: OpDecorate [[var:%\w+]] RelaxedPrecision
; CHECK: OpDecorate [[phi_id:%\w+]] RelaxedPrecision
; CHECK: [[phi_id]] = OpPhi
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %2 "main"
               OpDecorate %v RelaxedPrecision
       %void = OpTypeVoid
     %func_t = OpTypeFunction %void
       %bool = OpTypeBool
       %true = OpConstantTrue %bool
       %int  = OpTypeInt 32 0
      %int_p = OpTypePointer Function %int
      %int_1 = OpConstant %int 1
      %int_0 = OpConstant %int 0
          %2 = OpFunction %void None %func_t
         %33 = OpLabel
         %v  = OpVariable %int_p Function
               OpSelectionMerge %merge None
               OpBranchConditional %true %l1 %l2
         %l1 = OpLabel
               OpStore %v %int_1
               OpBranch %merge
         %l2 = OpLabel
               OpStore %v %int_0
               OpBranch %merge
      %merge = OpLabel
         %ld = OpLoad %int %v
               OpReturn
               OpFunctionEnd)";

  SinglePassRunAndMatch<SSARewritePass>(spv_asm, true);
}

// Test that the RelaxedPrecision decoration on the variable to added to the
// result of the OpPhi instruction.
TEST_F(LocalSSAElimTest, MultipleEdges) {
  const std::string spv_asm = R"(
  ; CHECK: OpSelectionMerge
  ; CHECK: [[header_bb:%\w+]] = OpLabel
  ; CHECK-NOT: OpLabel
  ; CHECK: OpSwitch {{%\w+}} {{%\w+}} 76 [[bb1:%\w+]] 17 [[bb2:%\w+]]
  ; CHECK-SAME: 4 [[bb2]]
  ; CHECK: [[bb2]] = OpLabel
  ; CHECK-NEXT: OpPhi [[type:%\w+]] [[val:%\w+]] [[header_bb]] %int_0 [[bb1]]
          OpCapability Shader
     %1 = OpExtInstImport "GLSL.std.450"
          OpMemoryModel Logical GLSL450
          OpEntryPoint Fragment %4 "main"
          OpExecutionMode %4 OriginUpperLeft
          OpSource ESSL 310
  %void = OpTypeVoid
     %3 = OpTypeFunction %void
   %int = OpTypeInt 32 1
  %_ptr_Function_int = OpTypePointer Function %int
  %int_0 = OpConstant %int 0
  %bool = OpTypeBool
  %true = OpConstantTrue %bool
  %false = OpConstantFalse %bool
  %int_1 = OpConstant %int 1
     %4 = OpFunction %void None %3
     %5 = OpLabel
     %8 = OpVariable %_ptr_Function_int Function
          OpBranch %10
    %10 = OpLabel
          OpLoopMerge %12 %13 None
          OpBranch %14
    %14 = OpLabel
          OpBranchConditional %true %11 %12
    %11 = OpLabel
          OpSelectionMerge %19 None
          OpBranchConditional %false %18 %19
    %18 = OpLabel
          OpSelectionMerge %22 None
          OpSwitch %int_0 %22 76 %20 17 %21 4 %21
    %20 = OpLabel
    %23 = OpLoad %int %8
          OpStore %8 %int_0
          OpBranch %21
    %21 = OpLabel
          OpBranch %22
    %22 = OpLabel
          OpBranch %19
    %19 = OpLabel
          OpBranch %13
    %13 = OpLabel
          OpBranch %10
    %12 = OpLabel
          OpReturn
          OpFunctionEnd
  )";

  SinglePassRunAndMatch<SSARewritePass>(spv_asm, true);
}

TEST_F(LocalSSAElimTest, VariablePointerTest1) {
  // Check that the load of the first variable is still used and that the load
  // of the third variable is propagated.  The first load has to remain because
  // of the store to the variable pointer.
  const std::string text = R"(
; CHECK: [[v1:%\w+]] = OpVariable
; CHECK: [[v2:%\w+]] = OpVariable
; CHECK: [[v3:%\w+]] = OpVariable
; CHECK: [[ld1:%\w+]] = OpLoad %int [[v1]]
; CHECK: OpIAdd %int [[ld1]] %int_0
               OpCapability Shader
               OpCapability VariablePointers
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %2 "main"
               OpExecutionMode %2 LocalSize 1 1 1
               OpSource GLSL 450
               OpMemberDecorate %_struct_3 0 Offset 0
               OpMemberDecorate %_struct_3 1 Offset 4
       %void = OpTypeVoid
          %5 = OpTypeFunction %void
        %int = OpTypeInt 32 1
       %bool = OpTypeBool
  %_struct_3 = OpTypeStruct %int %int
%_ptr_Function__struct_3 = OpTypePointer Function %_struct_3
%_ptr_Function_int = OpTypePointer Function %int
       %true = OpConstantTrue %bool
      %int_0 = OpConstant %int 0
      %int_1 = OpConstant %int 1
         %13 = OpConstantNull %_struct_3
          %2 = OpFunction %void None %5
         %14 = OpLabel
         %15 = OpVariable %_ptr_Function_int Function
         %16 = OpVariable %_ptr_Function_int Function
         %17 = OpVariable %_ptr_Function_int Function
               OpStore %15 %int_1
               OpStore %17 %int_0
               OpSelectionMerge %18 None
               OpBranchConditional %true %19 %20
         %19 = OpLabel
               OpBranch %18
         %20 = OpLabel
               OpBranch %18
         %18 = OpLabel
         %21 = OpPhi %_ptr_Function_int %15 %19 %16 %20
               OpStore %21 %int_0
         %22 = OpLoad %int %15
         %23 = OpLoad %int %17
         %24 = OpIAdd %int %22 %23
               OpReturn
               OpFunctionEnd
  )";
  SinglePassRunAndMatch<SSARewritePass>(text, false);
}

TEST_F(LocalSSAElimTest, VariablePointerTest2) {
  // Check that the load of the first variable is still used and that the load
  // of the third variable is propagated.  The first load has to remain because
  // of the store to the variable pointer.
  const std::string text = R"(
; CHECK: [[v1:%\w+]] = OpVariable
; CHECK: [[v2:%\w+]] = OpVariable
; CHECK: [[v3:%\w+]] = OpVariable
; CHECK: [[ld1:%\w+]] = OpLoad %int [[v1]]
; CHECK: OpIAdd %int [[ld1]] %int_0
               OpCapability Shader
               OpCapability VariablePointers
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %2 "main"
               OpExecutionMode %2 LocalSize 1 1 1
               OpSource GLSL 450
               OpMemberDecorate %_struct_3 0 Offset 0
               OpMemberDecorate %_struct_3 1 Offset 4
       %void = OpTypeVoid
          %5 = OpTypeFunction %void
        %int = OpTypeInt 32 1
       %bool = OpTypeBool
  %_struct_3 = OpTypeStruct %int %int
%_ptr_Function__struct_3 = OpTypePointer Function %_struct_3
%_ptr_Function_int = OpTypePointer Function %int
       %true = OpConstantTrue %bool
      %int_0 = OpConstant %int 0
      %int_1 = OpConstant %int 1
         %13 = OpConstantNull %_struct_3
          %2 = OpFunction %void None %5
         %14 = OpLabel
         %15 = OpVariable %_ptr_Function_int Function
         %16 = OpVariable %_ptr_Function_int Function
         %17 = OpVariable %_ptr_Function_int Function
               OpStore %15 %int_1
               OpStore %17 %int_0
               OpSelectionMerge %18 None
               OpBranchConditional %true %19 %20
         %19 = OpLabel
               OpBranch %18
         %20 = OpLabel
               OpBranch %18
         %18 = OpLabel
         %21 = OpPhi %_ptr_Function_int %15 %19 %16 %20
               OpStore %21 %int_0
         %22 = OpLoad %int %15
         %23 = OpLoad %int %17
         %24 = OpIAdd %int %22 %23
               OpReturn
               OpFunctionEnd
  )";
  SinglePassRunAndMatch<SSARewritePass>(text, false);
}

TEST_F(LocalSSAElimTest, ChainedTrivialPhis) {
  // Check that the copy object get the undef value implicitly assigned in the
  // entry block.
  const std::string text = R"(
; CHECK: [[undef:%\w+]] = OpUndef %v4float
; CHECK: OpCopyObject %v4float [[undef]]
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %2 "main"
               OpExecutionMode %2 LocalSize 1 18 6
               OpSource ESSL 310
       %void = OpTypeVoid
          %4 = OpTypeFunction %void
       %bool = OpTypeBool
      %float = OpTypeFloat 32
    %v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
          %2 = OpFunction %void None %4
          %9 = OpLabel
         %10 = OpVariable %_ptr_Function_v4float Function
               OpBranch %11
         %11 = OpLabel
               OpLoopMerge %12 %13 None
               OpBranch %14
         %14 = OpLabel
         %15 = OpUndef %bool
               OpBranchConditional %15 %16 %12
         %16 = OpLabel
         %17 = OpUndef %bool
               OpSelectionMerge %18 None
               OpBranchConditional %17 %19 %18
         %19 = OpLabel
         %20 = OpUndef %bool
               OpLoopMerge %21 %22 None
               OpBranchConditional %20 %23 %21
         %23 = OpLabel
         %24 = OpLoad %v4float %10
         %25 = OpCopyObject %v4float %24
         %26 = OpUndef %bool
               OpBranch %22
         %22 = OpLabel
               OpBranch %19
         %21 = OpLabel
               OpBranch %12
         %18 = OpLabel
               OpBranch %13
         %13 = OpLabel
               OpBranch %11
         %12 = OpLabel
         %27 = OpLoad %v4float %10
               OpReturn
               OpFunctionEnd
  )";
  SinglePassRunAndMatch<SSARewritePass>(text, false);
}

TEST_F(LocalSSAElimTest, Overflowtest1) {
  // Check that the copy object get the undef value implicitly assigned in the
  // entry block.
  const std::string text = R"(
OpCapability Geometry
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %4 "P2Mai" %12 %17
OpExecutionMode %4 OriginUpperLeft
%2 = OpTypeVoid
%3 = OpTypeFunction %2
%6 = OpTypeFloat 32
%7 = OpTypeVector %6 4
%11 = OpTypePointer Input %7
%16 = OpTypePointer Output %7
%23 = OpTypePointer Function %7
%12 = OpVariable %11 Input
%17 = OpVariable %16 Output
%4 = OpFunction %2 None %3
%2177 = OpLabel
%4194302 = OpVariable %23 Function
%4194301 = OpLoad %7 %4194302
OpStore %17 %4194301
OpReturn
OpFunctionEnd
  )";

  SetAssembleOptions(SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);

  std::vector<Message> messages = {
      {SPV_MSG_ERROR, "", 0, 0, "ID overflow. Try running compact-ids."}};
  SetMessageConsumer(GetTestMessageConsumer(messages));
  auto result = SinglePassRunToBinary<SSARewritePass>(text, true);
  EXPECT_EQ(Pass::Status::Failure, std::get<1>(result));
}

TEST_F(LocalSSAElimTest, OpConstantNull) {
  const std::string text = R"(
OpCapability Addresses
OpCapability Kernel
OpCapability Int64
OpMemoryModel Physical64 OpenCL
OpEntryPoint Kernel %4 "A"
OpSource OpenCL_C 200000
%2 = OpTypeVoid
%3 = OpTypeFunction %2
%6 = OpTypeInt 32 0
%11 = OpTypePointer CrossWorkgroup %6
%16 = OpConstantNull %11
%20 = OpConstant %6 269484031
%4 = OpFunction %2 None %3
%17 = OpLabel
%18 = OpLoad %6 %16 Aligned 536870912
%19 = OpBitwiseXor %6 %18 %20
OpStore %16 %19 Aligned 536870912
OpReturn
OpFunctionEnd
  )";

  SinglePassRunToBinary<SSARewritePass>(text, false);
}

TEST_F(LocalSSAElimTest, DebugForLoop) {
  // #version 140
  //
  // in vec4 BC;
  // out float fo;
  //
  // void main()
  // {
  //     float f = 0.0;
  //     for (int i=0; i<4; i++) {
  //       f = f + BC[i];
  //     }
  //     fo = f;
  // }

  const std::string text = R"(
; CHECK: [[f_name:%\w+]] = OpString "f"
; CHECK: [[i_name:%\w+]] = OpString "i"
; CHECK: [[dbg_f:%\w+]] = OpExtInst %void [[ext:%\d+]] DebugLocalVariable [[f_name]]
; CHECK: [[dbg_i:%\w+]] = OpExtInst %void [[ext]] DebugLocalVariable [[i_name]]

; CHECK:      OpStore %f %float_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_f]] %float_0
; CHECK-NEXT: OpStore %i %int_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_i]] %int_0

; CHECK-NOT:  DebugDeclare

; CHECK:      [[loop_head:%\w+]] = OpLabel
; CHECK:      [[phi0:%\w+]] = OpPhi %float %float_0
; CHECK:      [[phi1:%\w+]] = OpPhi %int %int_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_f]] [[phi0]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_i]] [[phi1]]
; CHECK:      OpLoopMerge [[loop_merge:%\w+]] [[loop_cont:%\w+]] None
; CHECK-NEXT: OpBranch [[loop_body:%\w+]]

; CHECK-NEXT: [[loop_body]] = OpLabel
; CHECK:      OpBranchConditional {{%\w+}} [[bb:%\w+]] [[loop_merge]]

; CHECK:      [[bb]] = OpLabel
; CHECK:      OpStore %f [[f_val:%\w+]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_f]] [[f_val]]
; CHECK-NEXT: OpBranch [[loop_cont]]

; CHECK:      [[loop_cont]] = OpLabel
; CHECK:      OpStore %i [[i_val:%\w+]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_i]] [[i_val]]
; CHECK-NEXT: OpBranch [[loop_head]]

; CHECK:      [[loop_merge]] = OpLabel

OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
%ext = OpExtInstImport "OpenCL.DebugInfo.100"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BC %fo
OpExecutionMode %main OriginUpperLeft
%file_name = OpString "test"
OpSource GLSL 140
%float_name = OpString "float"
%main_name = OpString "main"
%f_name = OpString "f"
%i_name = OpString "i"
OpName %main "main"
OpName %f "f"
OpName %i "i"
OpName %BC "BC"
OpName %fo "fo"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%int = OpTypeInt 32 1
%uint = OpTypeInt 32 0
%uint_32 = OpConstant %uint 32
%_ptr_Function_int = OpTypePointer Function %int
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BC = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output
%null_expr = OpExtInst %void %ext DebugExpression
%src = OpExtInst %void %ext DebugSource %file_name
%cu = OpExtInst %void %ext DebugCompilationUnit 1 4 %src HLSL
%dbg_tf = OpExtInst %void %ext DebugTypeBasic %float_name %uint_32 Float
%dbg_v4f = OpExtInst %void %ext DebugTypeVector %dbg_tf 4
%main_ty = OpExtInst %void %ext DebugTypeFunction FlagIsProtected|FlagIsPrivate %dbg_v4f %dbg_v4f
%dbg_main = OpExtInst %void %ext DebugFunction %main_name %main_ty %src 0 0 %cu %main_name FlagIsProtected|FlagIsPrivate 10 %main
%dbg_f = OpExtInst %void %ext DebugLocalVariable %f_name %dbg_v4f %src 0 0 %dbg_main FlagIsLocal
%dbg_i = OpExtInst %void %ext DebugLocalVariable %i_name %dbg_v4f %src 0 0 %dbg_main FlagIsLocal
%main = OpFunction %void None %8
%22 = OpLabel
%s0 = OpExtInst %void %ext DebugScope %dbg_main
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
OpStore %f %float_0
OpStore %i %int_0
%decl0 = OpExtInst %void %ext DebugDeclare %dbg_f %f %null_expr
%decl1 = OpExtInst %void %ext DebugDeclare %dbg_i %i %null_expr
OpBranch %23
%23 = OpLabel
%s1 = OpExtInst %void %ext DebugScope %dbg_main
OpLoopMerge %24 %25 None
OpBranch %26
%26 = OpLabel
%s2 = OpExtInst %void %ext DebugScope %dbg_main
%27 = OpLoad %int %i
%28 = OpSLessThan %bool %27 %int_4
OpBranchConditional %28 %29 %24
%29 = OpLabel
%s3 = OpExtInst %void %ext DebugScope %dbg_main
%30 = OpLoad %float %f
%31 = OpLoad %int %i
%32 = OpAccessChain %_ptr_Input_float %BC %31
%33 = OpLoad %float %32
%34 = OpFAdd %float %30 %33
OpStore %f %34
OpBranch %25
%25 = OpLabel
%s4 = OpExtInst %void %ext DebugScope %dbg_main
%35 = OpLoad %int %i
%36 = OpIAdd %int %35 %int_1
OpStore %i %36
OpBranch %23
%24 = OpLabel
%s5 = OpExtInst %void %ext DebugScope %dbg_main
%37 = OpLoad %float %f
OpStore %fo %37
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndMatch<SSARewritePass>(text, true);
}

TEST_F(LocalSSAElimTest, DebugValueForReferenceVariable) {
  // #version 140
  //
  // in vec4 BC;
  // out float fo;
  //
  // void main()
  // {
  //     float f = 0.0;
  //     float& x = f;
  //     for (int i=0; i<4; i++) {
  //       x = x + BC[i];
  //     }
  //     fo = f;
  // }

  const std::string text = R"(
; CHECK: [[f_name:%\w+]] = OpString "f"
; CHECK: [[i_name:%\w+]] = OpString "i"
; CHECK: [[x_name:%\w+]] = OpString "x"
; CHECK: [[dbg_f:%\w+]] = OpExtInst %void [[ext:%\d+]] DebugLocalVariable [[f_name]]
; CHECK: [[dbg_i:%\w+]] = OpExtInst %void [[ext]] DebugLocalVariable [[i_name]]
; CHECK: [[dbg_x:%\w+]] = OpExtInst %void [[ext]] DebugLocalVariable [[x_name]]

; CHECK:      OpStore %f %float_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_x]] %float_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_f]] %float_0
; CHECK-NEXT: OpStore %i %int_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_i]] %int_0

; CHECK-NOT:  DebugDeclare

; CHECK:      [[loop_head:%\w+]] = OpLabel
; CHECK:      [[phi0:%\w+]] = OpPhi %float %float_0
; CHECK:      [[phi1:%\w+]] = OpPhi %int %int_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_x]] [[phi0]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_f]] [[phi0]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_i]] [[phi1]]
; CHECK:      OpLoopMerge [[loop_merge:%\w+]] [[loop_cont:%\w+]] None
; CHECK-NEXT: OpBranch [[loop_body:%\w+]]

; CHECK:      [[loop_body]] = OpLabel
; CHECK:      OpBranchConditional {{%\w+}} [[bb:%\w+]] [[loop_merge]]

; CHECK:      [[bb]] = OpLabel
; CHECK:      OpStore %f [[f_val:%\w+]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_x]] [[f_val]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_f]] [[f_val]]
; CHECK-NEXT: OpBranch [[loop_cont]]

; CHECK:      [[loop_cont]] = OpLabel
; CHECK:      OpStore %i [[i_val:%\w+]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_i]] [[i_val]]
; CHECK-NEXT: OpBranch [[loop_head]]

; CHECK:      [[loop_merge]] = OpLabel

OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
%ext = OpExtInstImport "OpenCL.DebugInfo.100"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BC %fo
OpExecutionMode %main OriginUpperLeft
%file_name = OpString "test"
OpSource GLSL 140
%float_name = OpString "float"
%main_name = OpString "main"
%f_name = OpString "f"
%i_name = OpString "i"
%x_name = OpString "x"
OpName %main "main"
OpName %f "f"
OpName %i "i"
OpName %BC "BC"
OpName %fo "fo"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%int = OpTypeInt 32 1
%uint = OpTypeInt 32 0
%uint_32 = OpConstant %uint 32
%_ptr_Function_int = OpTypePointer Function %int
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BC = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output
%null_expr = OpExtInst %void %ext DebugExpression
%src = OpExtInst %void %ext DebugSource %file_name
%cu = OpExtInst %void %ext DebugCompilationUnit 1 4 %src HLSL
%dbg_tf = OpExtInst %void %ext DebugTypeBasic %float_name %uint_32 Float
%dbg_v4f = OpExtInst %void %ext DebugTypeVector %dbg_tf 4
%main_ty = OpExtInst %void %ext DebugTypeFunction FlagIsProtected|FlagIsPrivate %dbg_v4f %dbg_v4f
%dbg_main = OpExtInst %void %ext DebugFunction %main_name %main_ty %src 0 0 %cu %main_name FlagIsProtected|FlagIsPrivate 10 %main
%dbg_f = OpExtInst %void %ext DebugLocalVariable %f_name %dbg_v4f %src 0 0 %dbg_main FlagIsLocal
%dbg_i = OpExtInst %void %ext DebugLocalVariable %i_name %dbg_v4f %src 1 0 %dbg_main FlagIsLocal
%dbg_x = OpExtInst %void %ext DebugLocalVariable %x_name %dbg_v4f %src 2 0 %dbg_main FlagIsLocal
%main = OpFunction %void None %8
%22 = OpLabel
%s0 = OpExtInst %void %ext DebugScope %dbg_main
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
OpStore %f %float_0
OpStore %i %int_0
%decl0 = OpExtInst %void %ext DebugDeclare %dbg_f %f %null_expr
%decl1 = OpExtInst %void %ext DebugDeclare %dbg_i %i %null_expr
%decl2 = OpExtInst %void %ext DebugDeclare %dbg_x %f %null_expr
OpBranch %23
%23 = OpLabel
%s1 = OpExtInst %void %ext DebugScope %dbg_main
OpLoopMerge %24 %25 None
OpBranch %26
%26 = OpLabel
%s2 = OpExtInst %void %ext DebugScope %dbg_main
%27 = OpLoad %int %i
%28 = OpSLessThan %bool %27 %int_4
OpBranchConditional %28 %29 %24
%29 = OpLabel
%s3 = OpExtInst %void %ext DebugScope %dbg_main
%30 = OpLoad %float %f
%31 = OpLoad %int %i
%32 = OpAccessChain %_ptr_Input_float %BC %31
%33 = OpLoad %float %32
%34 = OpFAdd %float %30 %33
OpStore %f %34
OpBranch %25
%25 = OpLabel
%s4 = OpExtInst %void %ext DebugScope %dbg_main
%35 = OpLoad %int %i
%36 = OpIAdd %int %35 %int_1
OpStore %i %36
OpBranch %23
%24 = OpLabel
%s5 = OpExtInst %void %ext DebugScope %dbg_main
%37 = OpLoad %float %f
OpStore %fo %37
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndMatch<SSARewritePass>(text, true);
}

TEST_F(LocalSSAElimTest, DebugValueForReferenceVariableInBB) {
  // #version 140
  //
  // in vec4 BC;
  // out float fo;
  //
  // void main()
  // {
  //     float f = 0.0;
  //     for (int i=0; i<4; i++) {
  //       float& x = f;
  //       x = x + BC[i];
  //       {
  //         x = x + BC[i];
  //       }
  //     }
  //     fo = f;
  // }

  const std::string text = R"(
; CHECK: [[f_name:%\w+]] = OpString "f"
; CHECK: [[i_name:%\w+]] = OpString "i"
; CHECK: [[x_name:%\w+]] = OpString "x"
; CHECK: [[dbg_main:%\w+]] = OpExtInst %void [[ext:%\d+]] DebugFunction
; CHECK: [[dbg_bb:%\w+]] = OpExtInst %void [[ext]] DebugLexicalBlock
; CHECK: [[dbg_bb_child:%\w+]] = OpExtInst %void [[ext]] DebugLexicalBlock
; CHECK: [[dbg_f:%\w+]] = OpExtInst %void [[ext]] DebugLocalVariable [[f_name]]
; CHECK: [[dbg_i:%\w+]] = OpExtInst %void [[ext]] DebugLocalVariable [[i_name]]
; CHECK: [[dbg_x:%\w+]] = OpExtInst %void [[ext]] DebugLocalVariable [[x_name]]

; CHECK:      OpExtInst %void [[ext]] DebugScope [[dbg_main]]
; CHECK:      OpStore %f %float_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_f]] %float_0
; CHECK-NEXT: OpStore %i %int_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_i]] %int_0

; CHECK-NOT:  DebugDeclare

; CHECK:      [[loop_head:%\w+]] = OpLabel
; CHECK:      OpExtInst %void [[ext]] DebugScope [[dbg_main]]
; CHECK:      [[phi0:%\w+]] = OpPhi %float %float_0
; CHECK:      [[phi1:%\w+]] = OpPhi %int %int_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_f]] [[phi0]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_i]] [[phi1]]
; CHECK:      OpLoopMerge [[loop_merge:%\w+]] [[loop_cont:%\w+]] None
; CHECK-NEXT: OpBranch [[loop_body:%\w+]]

; CHECK-NEXT: [[loop_body]] = OpLabel
; CHECK:      OpBranchConditional {{%\w+}} [[bb:%\w+]] [[loop_merge]]

; CHECK:      [[bb]] = OpLabel
; CHECK:      OpExtInst %void [[ext]] DebugScope [[dbg_bb]]
; CHECK:      OpStore %f [[f_val:%\w+]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_x]] [[f_val]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_f]] [[f_val]]
; CHECK-NEXT: OpBranch [[bb_child:%\w+]]

; CHECK:      [[bb_child]] = OpLabel
; CHECK:      OpExtInst %void [[ext]] DebugScope [[dbg_bb_child]]
; CHECK:      OpStore %f [[new_f_val:%\w+]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_x]] [[new_f_val]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_f]] [[new_f_val]]
; CHECK-NEXT: OpBranch [[loop_cont]]

; CHECK:      [[loop_cont]] = OpLabel
; CHECK:      OpStore %i [[i_val:%\w+]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_i]] [[i_val]]
; CHECK-NEXT: OpBranch [[loop_head]]

; CHECK:      [[loop_merge]] = OpLabel

OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
%ext = OpExtInstImport "OpenCL.DebugInfo.100"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BC %fo
OpExecutionMode %main OriginUpperLeft
%file_name = OpString "test"
OpSource GLSL 140
%float_name = OpString "float"
%main_name = OpString "main"
%f_name = OpString "f"
%i_name = OpString "i"
%x_name = OpString "x"
OpName %main "main"
OpName %f "f"
OpName %i "i"
OpName %BC "BC"
OpName %fo "fo"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%int = OpTypeInt 32 1
%uint = OpTypeInt 32 0
%uint_32 = OpConstant %uint 32
%_ptr_Function_int = OpTypePointer Function %int
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BC = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output
%null_expr = OpExtInst %void %ext DebugExpression
%src = OpExtInst %void %ext DebugSource %file_name
%cu = OpExtInst %void %ext DebugCompilationUnit 1 4 %src HLSL
%dbg_tf = OpExtInst %void %ext DebugTypeBasic %float_name %uint_32 Float
%dbg_v4f = OpExtInst %void %ext DebugTypeVector %dbg_tf 4
%main_ty = OpExtInst %void %ext DebugTypeFunction FlagIsProtected|FlagIsPrivate %dbg_v4f %dbg_v4f
%dbg_main = OpExtInst %void %ext DebugFunction %main_name %main_ty %src 0 0 %cu %main_name FlagIsProtected|FlagIsPrivate 10 %main
%bb = OpExtInst %void %ext DebugLexicalBlock %src 0 0 %dbg_main
%bb_child = OpExtInst %void %ext DebugLexicalBlock %src 1 0 %bb
%dbg_f = OpExtInst %void %ext DebugLocalVariable %f_name %dbg_v4f %src 0 0 %dbg_main FlagIsLocal
%dbg_i = OpExtInst %void %ext DebugLocalVariable %i_name %dbg_v4f %src 1 0 %dbg_main FlagIsLocal
%dbg_x = OpExtInst %void %ext DebugLocalVariable %x_name %dbg_v4f %src 2 0 %bb FlagIsLocal
%main = OpFunction %void None %8
%22 = OpLabel
%s0 = OpExtInst %void %ext DebugScope %dbg_main
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
OpStore %f %float_0
OpStore %i %int_0
%decl0 = OpExtInst %void %ext DebugDeclare %dbg_f %f %null_expr
%decl1 = OpExtInst %void %ext DebugDeclare %dbg_i %i %null_expr
OpBranch %23
%23 = OpLabel
%s1 = OpExtInst %void %ext DebugScope %dbg_main
OpLoopMerge %24 %25 None
OpBranch %26
%26 = OpLabel
%s2 = OpExtInst %void %ext DebugScope %dbg_main
%27 = OpLoad %int %i
%28 = OpSLessThan %bool %27 %int_4
OpBranchConditional %28 %29 %24
%29 = OpLabel
%scope = OpExtInst %void %ext DebugScope %bb
%decl2 = OpExtInst %void %ext DebugDeclare %dbg_x %f %null_expr
%30 = OpLoad %float %f
%31 = OpLoad %int %i
%32 = OpAccessChain %_ptr_Input_float %BC %31
%33 = OpLoad %float %32
%34 = OpFAdd %float %30 %33
OpStore %f %34
OpBranch %38
%38 = OpLabel
%child_scope = OpExtInst %void %ext DebugScope %bb_child
%39 = OpLoad %float %f
%40 = OpFAdd %float %39 %33
OpStore %f %40
OpBranch %25
%25 = OpLabel
%s3 = OpExtInst %void %ext DebugScope %dbg_main
%35 = OpLoad %int %i
%36 = OpIAdd %int %35 %int_1
OpStore %i %36
OpBranch %23
%24 = OpLabel
%s4 = OpExtInst %void %ext DebugScope %dbg_main
%37 = OpLoad %float %f
OpStore %fo %37
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndMatch<SSARewritePass>(text, true);
}

TEST_F(LocalSSAElimTest, DebugForLoopUseDebugValueInsteadOfDebugDeclare) {
  // #version 140
  //
  // in vec4 BC;
  // out float fo;
  //
  // struct S {
  //     float f;
  //     int i;
  // };
  //
  // void main()
  // {
  //     S foo = {0.0, 0};
  //     for (; foo.i<4; foo.i++) {
  //       foo.f = foo.f + BC[foo.i];
  //     }
  //     fo = foo.f;
  // }

  const std::string text = R"(
; CHECK: [[f_name:%\w+]] = OpString "f"
; CHECK: [[empty_expr:%\w+]] = OpExtInst %void [[ext:%\d+]] DebugExpression
; CHECK: [[deref_op:%\w+]] = OpExtInst %void [[ext]] DebugOperation Deref
; CHECK: [[deref:%\w+]] = OpExtInst %void [[ext]] DebugExpression [[deref_op]]
; CHECK: [[dbg_foo:%\w+]] = OpExtInst %void [[ext]] DebugLocalVariable [[f_name]]

; CHECK:      OpStore %f %float_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_foo]] %float_0 [[empty_expr]] %uint_0
; CHECK-NEXT: OpStore %i %int_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_foo]] %int_0 [[empty_expr]] %uint_1

; CHECK:      [[loop_head:%\w+]] = OpLabel
; CHECK:      [[phi0:%\w+]] = OpPhi %float %float_0
; CHECK:      [[phi1:%\w+]] = OpPhi %int %int_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_foo]] [[phi0]] [[empty_expr]] %uint_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_foo]] [[phi1]] [[empty_expr]] %uint_1
; CHECK:      OpLoopMerge [[loop_merge:%\w+]] [[loop_cont:%\w+]] None
; CHECK-NEXT: OpBranch [[loop_body:%\w+]]

; CHECK:      [[loop_body]] = OpLabel
; CHECK:      OpBranchConditional {{%\w+}} [[bb:%\w+]] [[loop_merge]]

; CHECK:      [[bb]] = OpLabel
; CHECK:      OpStore %f [[f_val:%\w+]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_foo]] [[f_val]] [[empty_expr]] %uint_0
; CHECK-NEXT: OpBranch [[loop_cont]]

; CHECK:      [[loop_cont]] = OpLabel
; CHECK:      OpStore %i [[i_val:%\w+]]
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_foo]] [[i_val]] [[empty_expr]] %uint_1
; CHECK-NEXT: OpBranch [[loop_head]]

OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
%ext = OpExtInstImport "OpenCL.DebugInfo.100"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BC %fo
OpExecutionMode %main OriginUpperLeft
%file_name = OpString "test"
OpSource GLSL 140
%float_name = OpString "float"
%main_name = OpString "main"
%f_name = OpString "f"
OpName %main "main"
OpName %f "f"
OpName %i "i"
OpName %BC "BC"
OpName %fo "fo"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%int = OpTypeInt 32 1
%uint = OpTypeInt 32 0
%uint_32 = OpConstant %uint 32
%uint_0 = OpConstant %uint 0
%uint_1 = OpConstant %uint 1
%_ptr_Function_int = OpTypePointer Function %int
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BC = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output
%deref_op = OpExtInst %void %ext DebugOperation Deref
%deref = OpExtInst %void %ext DebugExpression %deref_op
%src = OpExtInst %void %ext DebugSource %file_name
%cu = OpExtInst %void %ext DebugCompilationUnit 1 4 %src HLSL
%dbg_tf = OpExtInst %void %ext DebugTypeBasic %float_name %uint_32 Float
%dbg_v4f = OpExtInst %void %ext DebugTypeVector %dbg_tf 4
%main_ty = OpExtInst %void %ext DebugTypeFunction FlagIsProtected|FlagIsPrivate %dbg_v4f %dbg_v4f
%dbg_main = OpExtInst %void %ext DebugFunction %main_name %main_ty %src 0 0 %cu %main_name FlagIsProtected|FlagIsPrivate 10 %main
%dbg_foo = OpExtInst %void %ext DebugLocalVariable %f_name %dbg_v4f %src 0 0 %dbg_main FlagIsLocal
%main = OpFunction %void None %8
%22 = OpLabel
%s0 = OpExtInst %void %ext DebugScope %dbg_main
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
OpStore %f %float_0
OpStore %i %int_0
%decl0 = OpExtInst %void %ext DebugValue %dbg_foo %f %deref %uint_0
%decl1 = OpExtInst %void %ext DebugValue %dbg_foo %i %deref %uint_1
OpBranch %23
%23 = OpLabel
%s1 = OpExtInst %void %ext DebugScope %dbg_main
OpLoopMerge %24 %25 None
OpBranch %26
%26 = OpLabel
%s2 = OpExtInst %void %ext DebugScope %dbg_main
%27 = OpLoad %int %i
%28 = OpSLessThan %bool %27 %int_4
OpBranchConditional %28 %29 %24
%29 = OpLabel
%s3 = OpExtInst %void %ext DebugScope %dbg_main
%30 = OpLoad %float %f
%31 = OpLoad %int %i
%32 = OpAccessChain %_ptr_Input_float %BC %31
%33 = OpLoad %float %32
%34 = OpFAdd %float %30 %33
OpStore %f %34
OpBranch %25
%25 = OpLabel
%s4 = OpExtInst %void %ext DebugScope %dbg_main
%35 = OpLoad %int %i
%36 = OpIAdd %int %35 %int_1
OpStore %i %36
OpBranch %23
%24 = OpLabel
%s5 = OpExtInst %void %ext DebugScope %dbg_main
%37 = OpLoad %float %f
OpStore %fo %37
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndMatch<SSARewritePass>(text, true);
}

TEST_F(LocalSSAElimTest, DebugValueNotUsedForDebugDeclare) {
  // #version 140
  //
  // in vec4 BC;
  // out float fo;
  //
  // void main()
  // {
  //     float f = 0.0;
  //     for (int i=0; i<4; i++) {
  //       f = f + BC[i];
  //     }
  //     fo = f;
  // }

  const std::string text = R"(
; CHECK: [[f_name:%\w+]] = OpString "f"
; CHECK: [[i_name:%\w+]] = OpString "i"
; CHECK: [[dbg_f:%\w+]] = OpExtInst %void [[ext:%\d+]] DebugLocalVariable [[f_name]]
; CHECK: [[dbg_i:%\w+]] = OpExtInst %void [[ext]] DebugLocalVariable [[i_name]]

; CHECK:      OpStore %f %float_0
; CHECK-NEXT: OpStore %i %int_0
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_f]] %f
; CHECK-NEXT: OpExtInst %void [[ext]] DebugValue [[dbg_i]] %i

; CHECK-NOT:  DebugValue
; CHECK-NOT:  DebugDeclare

OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
%ext = OpExtInstImport "OpenCL.DebugInfo.100"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BC %fo
OpExecutionMode %main OriginUpperLeft
%file_name = OpString "test"
OpSource GLSL 140
%float_name = OpString "float"
%main_name = OpString "main"
%f_name = OpString "f"
%i_name = OpString "i"
OpName %main "main"
OpName %f "f"
OpName %i "i"
OpName %BC "BC"
OpName %fo "fo"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%int = OpTypeInt 32 1
%uint = OpTypeInt 32 0
%uint_32 = OpConstant %uint 32
%_ptr_Function_int = OpTypePointer Function %int
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BC = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output
%null_expr = OpExtInst %void %ext DebugExpression
%src = OpExtInst %void %ext DebugSource %file_name
%cu = OpExtInst %void %ext DebugCompilationUnit 1 4 %src HLSL
%dbg_tf = OpExtInst %void %ext DebugTypeBasic %float_name %uint_32 Float
%dbg_v4f = OpExtInst %void %ext DebugTypeVector %dbg_tf 4
%main_ty = OpExtInst %void %ext DebugTypeFunction FlagIsProtected|FlagIsPrivate %dbg_v4f %dbg_v4f
%dbg_main = OpExtInst %void %ext DebugFunction %main_name %main_ty %src 0 0 %cu %main_name FlagIsProtected|FlagIsPrivate 10 %main
%dbg_f = OpExtInst %void %ext DebugLocalVariable %f_name %dbg_v4f %src 0 0 %dbg_main FlagIsLocal
%dbg_i = OpExtInst %void %ext DebugLocalVariable %i_name %dbg_v4f %src 1 0 %dbg_main FlagIsLocal
%main = OpFunction %void None %8
%22 = OpLabel
%s0 = OpExtInst %void %ext DebugScope %dbg_main
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
OpStore %f %float_0
OpStore %i %int_0
%decl0 = OpExtInst %void %ext DebugValue %dbg_f %f %null_expr
%decl1 = OpExtInst %void %ext DebugValue %dbg_i %i %null_expr
OpBranch %23
%23 = OpLabel
%s1 = OpExtInst %void %ext DebugScope %dbg_main
OpLoopMerge %24 %25 None
OpBranch %26
%26 = OpLabel
%s2 = OpExtInst %void %ext DebugScope %dbg_main
%27 = OpLoad %int %i
%28 = OpSLessThan %bool %27 %int_4
OpBranchConditional %28 %29 %24
%29 = OpLabel
%s3 = OpExtInst %void %ext DebugScope %dbg_main
%30 = OpLoad %float %f
%31 = OpLoad %int %i
%32 = OpAccessChain %_ptr_Input_float %BC %31
%33 = OpLoad %float %32
%34 = OpFAdd %float %30 %33
OpStore %f %34
OpBranch %25
%25 = OpLabel
%s4 = OpExtInst %void %ext DebugScope %dbg_main
%35 = OpLoad %int %i
%36 = OpIAdd %int %35 %int_1
OpStore %i %36
OpBranch %23
%24 = OpLabel
%s5 = OpExtInst %void %ext DebugScope %dbg_main
%37 = OpLoad %float %f
OpStore %fo %37
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndMatch<SSARewritePass>(text, true);
}

TEST_F(LocalSSAElimTest, DebugNestedForLoop) {
  const std::string text = R"(
; CHECK: = OpFunction
; CHECK-NEXT: [[entry:%\w+]] = OpLabel
; CHECK: OpStore %f %float_0
; CHECK-NEXT: = OpExtInst %void [[ext:%\w+]] DebugValue [[dbg_f:%\w+]] %float_0

; CHECK: [[outer_header:%\w+]] = OpLabel
; CHECK: [[outer_f:%\w+]] = OpPhi %float %float_0 [[entry]] [[inner_f:%\w+]] [[outer_be:%\w+]]
; CHECK: = OpExtInst %void [[ext]] DebugValue [[dbg_f]] [[outer_f]]

; CHECK: [[inner_pre_header:%\w+]] = OpLabel
; CHECK: [[inner_header:%\w+]] = OpLabel
; CHECK: [[inner_f]] = OpPhi %float [[outer_f]] [[inner_pre_header]] [[f_next:%\w+]] [[inner_be:%\w+]]
; CHECK: = OpExtInst %void [[ext]] DebugValue [[dbg_f]] [[inner_f]]

; CHECK: [[inner_be]] = OpLabel
; CHECK: [[f_next]] = OpFAdd %float [[inner_f]]
; CHECK-NEXT: OpStore %f [[f_next]]
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_f]] [[f_next]]

OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
%ext = OpExtInstImport "OpenCL.DebugInfo.100"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BC %fo
OpExecutionMode %main OriginUpperLeft
%file_name = OpString "test"
%float_name = OpString "float"
%main_name = OpString "main"
%f_name = OpString "f"
OpSource GLSL 450
OpName %main "main"
OpName %f "f"
OpName %i "i"
OpName %j "j"
OpName %BC "BC"
OpName %fo "fo"
OpDecorate %BC Location 0
OpDecorate %fo Location 0
%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%uint = OpTypeInt 32 0
%uint_32 = OpConstant %uint 32
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%mat4v4float = OpTypeMatrix %v4float 4
%_ptr_Input_mat4v4float = OpTypePointer Input %mat4v4float
%BC = OpVariable %_ptr_Input_mat4v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output

; Debug information
%null_expr = OpExtInst %void %ext DebugExpression
%src = OpExtInst %void %ext DebugSource %file_name
%cu = OpExtInst %void %ext DebugCompilationUnit 1 4 %src HLSL
%dbg_tf = OpExtInst %void %ext DebugTypeBasic %float_name %uint_32 Float
%main_ty = OpExtInst %void %ext DebugTypeFunction FlagIsProtected|FlagIsPrivate %void
%dbg_main = OpExtInst %void %ext DebugFunction %main_name %main_ty %src 0 0 %cu %main_name FlagIsProtected|FlagIsPrivate 10 %main
%dbg_f = OpExtInst %void %ext DebugLocalVariable %f_name %dbg_tf %src 0 0 %dbg_main FlagIsLocal

%main = OpFunction %void None %9
%24 = OpLabel
%s1 = OpExtInst %void %ext DebugScope %dbg_main
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
%j = OpVariable %_ptr_Function_int Function

; DebugDeclare
OpStore %f %float_0
%decl = OpExtInst %void %ext DebugDeclare %dbg_f %f %null_expr

OpStore %i %int_0
OpBranch %25
%25 = OpLabel
%s2 = OpExtInst %void %ext DebugScope %dbg_main
%26 = OpLoad %int %i
%27 = OpSLessThan %bool %26 %int_4
OpLoopMerge %28 %29 None
OpBranchConditional %27 %30 %28
%30 = OpLabel
%s3 = OpExtInst %void %ext DebugScope %dbg_main
OpStore %j %int_0
OpBranch %31
%31 = OpLabel
%s4 = OpExtInst %void %ext DebugScope %dbg_main
%32 = OpLoad %int %j
%33 = OpSLessThan %bool %32 %int_4
OpLoopMerge %50 %34 None
OpBranchConditional %33 %34 %50
%34 = OpLabel
%s5 = OpExtInst %void %ext DebugScope %dbg_main
%35 = OpLoad %float %f
%36 = OpLoad %int %i
%37 = OpLoad %int %j
%38 = OpAccessChain %_ptr_Input_float %BC %36 %37
%39 = OpLoad %float %38
%40 = OpFAdd %float %35 %39
OpStore %f %40
%41 = OpLoad %int %j
%42 = OpIAdd %int %41 %int_1
OpStore %j %42
OpBranch %31
%50 = OpLabel
%s6 = OpExtInst %void %ext DebugScope %dbg_main
OpBranch %29
%29 = OpLabel
%s7 = OpExtInst %void %ext DebugScope %dbg_main
%43 = OpLoad %int %i
%44 = OpIAdd %int %43 %int_1
OpStore %i %44
OpBranch %25
%28 = OpLabel
%s8 = OpExtInst %void %ext DebugScope %dbg_main
%45 = OpLoad %float %f
OpStore %fo %45
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndMatch<SSARewritePass>(text, true);
}

TEST_F(LocalSSAElimTest, DebugForLoopWithContinue) {
  const std::string text = R"(
; CHECK: = OpFunction
; CHECK-NEXT: [[entry:%\w+]] = OpLabel
; CHECK: OpStore %f %float_0
; CHECK-NEXT: = OpExtInst %void [[ext:%\w+]] DebugValue [[dbg_f:%\w+]] %float_0

; CHECK: [[outer_header:%\w+]] = OpLabel
; CHECK: [[outer_f:%\w+]] = OpPhi %float %float_0 [[entry]] [[inner_f:%\w+]] [[cont:%\w+]]
; CHECK: = OpExtInst %void [[ext]] DebugValue [[dbg_f]] [[outer_f]]

; CHECK: [[f_next:%\w+]] = OpFAdd %float [[outer_f]]
; CHECK-NEXT: OpStore %f [[f_next]]
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_f]] [[f_next]]

; CHECK: [[cont]] = OpLabel
; CHECK: [[inner_f]] = OpPhi %float [[outer_f]] {{%\d+}} [[f_next]] {{%\d+}}
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_f]] [[inner_f]]

OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
%ext = OpExtInstImport "OpenCL.DebugInfo.100"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BC %fo
OpExecutionMode %main OriginUpperLeft
%file_name = OpString "test"
%float_name = OpString "float"
%main_name = OpString "main"
%f_name = OpString "f"
OpSource GLSL 140
OpName %main "main"
OpName %f "f"
OpName %i "i"
OpName %t "t"
OpName %BC "BC"
OpName %fo "fo"
%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%uint = OpTypeInt 32 0
%uint_32 = OpConstant %uint 32
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BC = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%int_1 = OpConstant %int 1
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output

; Debug information
%null_expr = OpExtInst %void %ext DebugExpression
%src = OpExtInst %void %ext DebugSource %file_name
%cu = OpExtInst %void %ext DebugCompilationUnit 1 4 %src HLSL
%dbg_tf = OpExtInst %void %ext DebugTypeBasic %float_name %uint_32 Float
%main_ty = OpExtInst %void %ext DebugTypeFunction FlagIsProtected|FlagIsPrivate %void
%dbg_main = OpExtInst %void %ext DebugFunction %main_name %main_ty %src 0 0 %cu %main_name FlagIsProtected|FlagIsPrivate 10 %main
%dbg_f = OpExtInst %void %ext DebugLocalVariable %f_name %dbg_tf %src 0 0 %dbg_main FlagIsLocal

%main = OpFunction %void None %9
%23 = OpLabel
%s0 = OpExtInst %void %ext DebugScope %dbg_main
%f = OpVariable %_ptr_Function_float Function
%i = OpVariable %_ptr_Function_int Function
%t = OpVariable %_ptr_Function_float Function

; DebugDeclare
OpStore %f %float_0
%decl = OpExtInst %void %ext DebugDeclare %dbg_f %f %null_expr

OpStore %i %int_0
OpBranch %24
%24 = OpLabel
%s1 = OpExtInst %void %ext DebugScope %dbg_main
OpLoopMerge %25 %26 None
OpBranch %27
%27 = OpLabel
%s2 = OpExtInst %void %ext DebugScope %dbg_main
%28 = OpLoad %int %i
%29 = OpSLessThan %bool %28 %int_4
OpBranchConditional %29 %30 %25
%30 = OpLabel
%s3 = OpExtInst %void %ext DebugScope %dbg_main
%31 = OpLoad %int %i
%32 = OpAccessChain %_ptr_Input_float %BC %31
%33 = OpLoad %float %32
OpStore %t %33
%34 = OpLoad %float %t
%35 = OpFOrdLessThan %bool %34 %float_0
OpSelectionMerge %36 None
OpBranchConditional %35 %37 %36
%37 = OpLabel
%s4 = OpExtInst %void %ext DebugScope %dbg_main
OpBranch %26
%36 = OpLabel
%s5 = OpExtInst %void %ext DebugScope %dbg_main
%38 = OpLoad %float %f
%39 = OpLoad %float %t
%40 = OpFAdd %float %38 %39
OpStore %f %40
OpBranch %26
%26 = OpLabel
%s6 = OpExtInst %void %ext DebugScope %dbg_main
%41 = OpLoad %int %i
%42 = OpIAdd %int %41 %int_1
OpStore %i %42
OpBranch %24
%25 = OpLabel
%s7 = OpExtInst %void %ext DebugScope %dbg_main
%43 = OpLoad %float %f
OpStore %fo %43
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndMatch<SSARewritePass>(text, true);
}

TEST_F(LocalSSAElimTest, DebugIfElse) {
  const std::string text = R"(
OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
%ext = OpExtInstImport "OpenCL.DebugInfo.100"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %f %BaseColor %gl_FragColor
OpExecutionMode %main OriginUpperLeft
%file_name = OpString "test"
%float_name = OpString "float"
%main_name = OpString "main"
%v_name = OpString "v"
OpSource GLSL 140
OpName %main "main"
OpName %f "f"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Input_float = OpTypePointer Input %float
%f = OpVariable %_ptr_Input_float Input
%float_0 = OpConstant %float 0
%uint = OpTypeInt 32 0
%uint_32 = OpConstant %uint 32
%bool = OpTypeBool
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%float_0_5 = OpConstant %float 0.5
%float_1 = OpConstant %float 1
%18 = OpConstantComposite %v4float %float_1 %float_1 %float_1 %float_1
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output

; Debug information
%null_expr = OpExtInst %void %ext DebugExpression
%src = OpExtInst %void %ext DebugSource %file_name
%cu = OpExtInst %void %ext DebugCompilationUnit 1 4 %src HLSL
%dbg_tf = OpExtInst %void %ext DebugTypeBasic %float_name %uint_32 Float
%main_ty = OpExtInst %void %ext DebugTypeFunction FlagIsProtected|FlagIsPrivate %void
%dbg_main = OpExtInst %void %ext DebugFunction %main_name %main_ty %src 0 0 %cu %main_name FlagIsProtected|FlagIsPrivate 10 %main
%dbg_v = OpExtInst %void %ext DebugLocalVariable %v_name %dbg_tf %src 0 0 %dbg_main FlagIsLocal

%main = OpFunction %void None %8
%20 = OpLabel
%s0 = OpExtInst %void %ext DebugScope %dbg_main

; DebugDeclare
%v = OpVariable %_ptr_Function_v4float Function
%decl = OpExtInst %void %ext DebugDeclare %dbg_v %v %null_expr

%21 = OpLoad %float %f
%22 = OpFOrdGreaterThanEqual %bool %21 %float_0
OpSelectionMerge %23 None
OpBranchConditional %22 %24 %25

; CHECK: OpBranchConditional
; CHECK-NEXT: [[br0:%\w+]] = OpLabel
; CHECK: OpStore %v [[v0:%\w+]]
; CHECK-NEXT: = OpExtInst %void [[ext:%\w+]] DebugValue [[dbg_v:%\w+]] [[v0]]
%24 = OpLabel
%s1 = OpExtInst %void %ext DebugScope %dbg_main
%26 = OpLoad %v4float %BaseColor
%27 = OpVectorTimesScalar %v4float %26 %float_0_5
OpStore %v %27
OpBranch %23

; CHECK: [[br1:%\w+]] = OpLabel
; CHECK: OpStore %v [[v1:%\w+]]
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_v]] [[v1]]
%25 = OpLabel
%s2 = OpExtInst %void %ext DebugScope %dbg_main
%28 = OpLoad %v4float %BaseColor
%29 = OpFAdd %v4float %28 %18
OpStore %v %29
OpBranch %23

; CHECK: [[phi:%\w+]] = OpPhi %v4float [[v0]] [[br0]] [[v1]] [[br1]]
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_v]] [[phi]]
%23 = OpLabel
%s3 = OpExtInst %void %ext DebugScope %dbg_main
%30 = OpLoad %v4float %v
OpStore %gl_FragColor %30
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndMatch<SSARewritePass>(text, true);
}

TEST_F(LocalSSAElimTest, DebugSwitch) {
  const std::string text = R"(
OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
%ext = OpExtInstImport "OpenCL.DebugInfo.100"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %f %gl_FragColor
OpExecutionMode %main OriginUpperLeft
%file_name = OpString "test"
%float_name = OpString "float"
%main_name = OpString "main"
%v_name = OpString "v"
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %i "i"
OpName %f "f"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%uint = OpTypeInt 32 0
%uint_32 = OpConstant %uint 32
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
%_ptr_Input_float = OpTypePointer Input %float
%f = OpVariable %_ptr_Input_float Input
%float_0_25 = OpConstant %float 0.25
%float_0_75 = OpConstant %float 0.75
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output

; Debug information
%null_expr = OpExtInst %void %ext DebugExpression
%src = OpExtInst %void %ext DebugSource %file_name
%cu = OpExtInst %void %ext DebugCompilationUnit 1 4 %src HLSL
%dbg_tf = OpExtInst %void %ext DebugTypeBasic %float_name %uint_32 Float
%main_ty = OpExtInst %void %ext DebugTypeFunction FlagIsProtected|FlagIsPrivate %void
%dbg_main = OpExtInst %void %ext DebugFunction %main_name %main_ty %src 0 0 %cu %main_name FlagIsProtected|FlagIsPrivate 10 %main
%dbg_v = OpExtInst %void %ext DebugLocalVariable %v_name %dbg_tf %src 0 0 %dbg_main FlagIsLocal

%main = OpFunction %void None %9
%20 = OpLabel
%s0 = OpExtInst %void %ext DebugScope %dbg_main
%v = OpVariable %_ptr_Function_v4float Function
%i = OpVariable %_ptr_Function_int Function
%21 = OpLoad %v4float %BaseColor

; DebugDeclare
OpStore %v %21
%decl = OpExtInst %void %ext DebugDeclare %dbg_v %v %null_expr

; CHECK: %main = OpFunction %void None
; CHECK-NEXT: [[entry:%\w+]] = OpLabel
; CHECK: OpStore %v [[v0:%\w+]]
; CHECK-NEXT: = OpExtInst %void [[ext:%\w+]] DebugValue [[dbg_v:%\w+]] [[v0]]
; CHECK: OpSwitch {{%\w+}} [[case0:%\w+]] 0 [[case1:%\w+]] 1 [[case2:%\w+]] 2 [[case3:%\w+]]
; CHECK: OpStore %v [[v1:%\w+]]
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_v]] [[v1]]
; CHECK: OpStore %v [[v2:%\w+]]
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_v]] [[v2]]
; CHECK: [[phi0:%\w+]] = OpPhi %v4float [[v0]] [[entry]] [[v2]] [[case2]]
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_v]] [[phi0]]
; CHECK: OpStore %v [[v3:%\w+]]
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_v]] [[v3]]
; CHECK: [[phi1:%\w+]] = OpPhi %v4float [[v0]] [[case0]] [[v1]] [[case1]] [[v3]] [[case3]]
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_v]] [[phi1]]

%22 = OpLoad %float %f
%23 = OpConvertFToS %int %22
OpStore %i %23
%24 = OpLoad %int %i
OpSelectionMerge %25 None
OpSwitch %24 %26 0 %27 1 %28 2 %29
%26 = OpLabel
%s1 = OpExtInst %void %ext DebugScope %dbg_main
OpBranch %25
%27 = OpLabel
%s2 = OpExtInst %void %ext DebugScope %dbg_main
%30 = OpLoad %v4float %v
%31 = OpVectorTimesScalar %v4float %30 %float_0_25
OpStore %v %31
OpBranch %25
%28 = OpLabel
%s3 = OpExtInst %void %ext DebugScope %dbg_main
%32 = OpLoad %v4float %v
%33 = OpCompositeConstruct %v4float %float_0_25 %float_0_25 %float_0_25 %float_0_25
%34 = OpFAdd %v4float %32 %33
OpStore %v %34
OpBranch %29
%29 = OpLabel
%s4 = OpExtInst %void %ext DebugScope %dbg_main
%35 = OpLoad %v4float %v
%36 = OpVectorTimesScalar %v4float %35 %float_0_75
OpStore %v %36
OpBranch %25
%25 = OpLabel
%s5 = OpExtInst %void %ext DebugScope %dbg_main
%37 = OpLoad %v4float %v
OpStore %gl_FragColor %37
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndMatch<SSARewritePass>(text, true);
}

TEST_F(LocalSSAElimTest, DebugSwapProblem) {
  // #version 140
  //
  // in float fe;
  // out float fo;
  //
  // void main()
  // {
  //     float f1 = 0.0;
  //     float f2 = 1.0;
  //     int ie = int(fe);
  //     for (int i=0; i<ie; i++) {
  //       float t = f1;
  //       f1 = f2;
  //       f2 = t;
  //     }
  //     fo = f1;
  // }
  //
  // Because of the swap in the for loop, it generates the following phi
  // instructions:
  //
  // [[phi_f2]] = OpPhi %float %float_1 [[entry]] [[phi_f1]] ..
  // [[phi_f1]] = OpPhi %float %float_0 [[entry]] [[phi_f2]] ..
  //
  // Since they are used as operands by each other, we want to clearly check
  // what DebugValue we have to add for them.

  const std::string text = R"(
OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
%ext = OpExtInstImport "OpenCL.DebugInfo.100"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %fe %fo
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
%file_name = OpString "test"
%float_name = OpString "float"
%main_name = OpString "main"
%t_name = OpString "t"
OpName %main "main"
OpName %f1 "f1"
OpName %f2 "f2"
OpName %ie "ie"
OpName %fe "fe"
OpName %i "i"
OpName %t "t"
OpName %fo "fo"
%void = OpTypeVoid
%11 = OpTypeFunction %void
%float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%float_1 = OpConstant %float 1
%int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
%_ptr_Input_float = OpTypePointer Input %float
%fe = OpVariable %_ptr_Input_float Input
%int_0 = OpConstant %int 0
%bool = OpTypeBool
%int_1 = OpConstant %int 1
%uint = OpTypeInt 32 0
%uint_32 = OpConstant %uint 32
%_ptr_Output_float = OpTypePointer Output %float
%fo = OpVariable %_ptr_Output_float Output

; Debug information
%null_expr = OpExtInst %void %ext DebugExpression
%src = OpExtInst %void %ext DebugSource %file_name
%cu = OpExtInst %void %ext DebugCompilationUnit 1 4 %src HLSL
%dbg_tf = OpExtInst %void %ext DebugTypeBasic %float_name %uint_32 Float
%main_ty = OpExtInst %void %ext DebugTypeFunction FlagIsProtected|FlagIsPrivate %void
%dbg_main = OpExtInst %void %ext DebugFunction %main_name %main_ty %src 0 0 %cu %main_name FlagIsProtected|FlagIsPrivate 10 %main
%dbg_f1 = OpExtInst %void %ext DebugLocalVariable %t_name %dbg_tf %src 0 0 %dbg_main FlagIsLocal
%dbg_f2 = OpExtInst %void %ext DebugLocalVariable %t_name %dbg_tf %src 0 0 %dbg_main FlagIsLocal
%dbg_i = OpExtInst %void %ext DebugLocalVariable %t_name %dbg_tf %src 0 0 %dbg_main FlagIsLocal

%main = OpFunction %void None %11
%23 = OpLabel
%s0 = OpExtInst %void %ext DebugScope %dbg_main
%f1 = OpVariable %_ptr_Function_float Function
%f2 = OpVariable %_ptr_Function_float Function
%ie = OpVariable %_ptr_Function_int Function
%i = OpVariable %_ptr_Function_int Function
%t = OpVariable %_ptr_Function_float Function
OpStore %f1 %float_0
OpStore %f2 %float_1
%24 = OpLoad %float %fe
%25 = OpConvertFToS %int %24
OpStore %ie %25
OpStore %i %int_0

; DebugDeclare
%decl0 = OpExtInst %void %ext DebugDeclare %dbg_f1 %f1 %null_expr
%decl1 = OpExtInst %void %ext DebugDeclare %dbg_f2 %f2 %null_expr
%decl2 = OpExtInst %void %ext DebugDeclare %dbg_i  %i  %null_expr

; CHECK: %main = OpFunction %void None
; CHECK-NEXT: [[entry:%\w+]] = OpLabel

; CHECK: OpStore %f1 %float_0
; CHECK-NEXT: = OpExtInst %void [[ext:%\w+]] DebugValue [[dbg_f1:%\w+]] %float_0
; CHECK: OpStore %f2 %float_1
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_f2:%\w+]] %float_1
; CHECK: OpStore %i %int_0
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_i:%\w+]] %int_0

; CHECK: [[phi_f2:%\w+]] = OpPhi %float %float_1 [[entry]] [[phi_f1:%\w+]]
; CHECK: [[phi_f1]] = OpPhi %float %float_0 [[entry]] [[phi_f2]]
; CHECK: [[phi_i:%\w+]] = OpPhi %int %int_0 [[entry]]
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_f2]] [[phi_f2]]
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_f1]] [[phi_f1]]
; CHECK-NEXT: = OpExtInst %void [[ext]] DebugValue [[dbg_i]] [[phi_i]]

OpBranch %26
%26 = OpLabel
%s1 = OpExtInst %void %ext DebugScope %dbg_main
OpLoopMerge %27 %28 None
OpBranch %29
%29 = OpLabel
%s2 = OpExtInst %void %ext DebugScope %dbg_main
%30 = OpLoad %int %i
%31 = OpLoad %int %ie
%32 = OpSLessThan %bool %30 %31
OpBranchConditional %32 %33 %27
%33 = OpLabel
%s3 = OpExtInst %void %ext DebugScope %dbg_main
%34 = OpLoad %float %f1
OpStore %t %34
%35 = OpLoad %float %f2
OpStore %f1 %35
%36 = OpLoad %float %t
OpStore %f2 %36
OpBranch %28
%28 = OpLabel
%s4 = OpExtInst %void %ext DebugScope %dbg_main
%37 = OpLoad %int %i
%38 = OpIAdd %int %37 %int_1
OpStore %i %38
OpBranch %26
%27 = OpLabel
%s5 = OpExtInst %void %ext DebugScope %dbg_main
%39 = OpLoad %float %f1
OpStore %fo %39
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndMatch<SSARewritePass>(text, true);
}

// TODO(greg-lunarg): Add tests to verify handling of these cases:
//
//    No optimization in the presence of
//      access chains
//      function calls
//      OpCopyMemory?
//      unsupported extensions
//    Others?

}  // namespace
}  // namespace opt
}  // namespace spvtools
