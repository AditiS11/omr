/*******************************************************************************
 * Copyright IBM Corp. and others 2021
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#include "codegen/OMRInstOpCode.hpp"
#include "codegen/CodeGenerator.hpp"

const OMR::X86::InstOpCode::OpCodeMetaData OMR::X86::InstOpCode::metadata[NumOpCodes] =
   {
   #include "codegen/OMRInstOpCodeProperties.hpp"
   };

 // Heuristics for X87 second byte opcode
 // It could be eliminated if GCC/MSVC fully support initializer list
#define X87_________________(x) ((uint8_t)((x & 0xE0) >> 5)), ((uint8_t)((x & 0x18) >> 3)), (uint8_t)(x & 0x07)
#define BINARY(...) _
#define PROPERTY0(value) +0
#define PROPERTY1(value) +0
#define PROPERTY2(value) +0
#define FEATURES(value) +0

#define GET_MACRO(_1,_2,_3,_4,NAME,...) NAME

#define INSTRUCTION_4(name, mnemonic, binary, a, b, c, d) a b c d
#define INSTRUCTION_3(name, mnemonic, binary, a, b, c) a b c
#define INSTRUCTION_2(name, mnemonic, binary, a, b) a b
#define INSTRUCTION_1(name, mnemonic, binary, a) a
#define INSTRUCTION_0(name, mnemonic, binary) 0

#define EXPAND(...) __VA_ARGS__
#define INSTRUCTION(name, mnemonic, binary, ...) EXPAND(GET_MACRO(__VA_ARGS__, INSTRUCTION_4, INSTRUCTION_3, INSTRUCTION_2, INSTRUCTION_1, INSTRUCTION_0)(name, mnemonic, binary, __VA_ARGS__)) + 0

const uint32_t OMR::X86::InstOpCode::_properties[] =
   {
#undef PROPERTY0
#define PROPERTY0(value) value
#include "codegen/X86Ops.ins"
#undef PROPERTY0
#define PROPERTY0(value) +0
   };

const uint32_t OMR::X86::InstOpCode::_properties1[] =
   {
#undef PROPERTY1
#define PROPERTY1(value) +value
#include "codegen/X86Ops.ins"
#undef PROPERTY1
#define PROPERTY1(value) +0
   };

const uint32_t OMR::X86::InstOpCode::_properties2[] =
   {
#undef PROPERTY2
#define PROPERTY2(value) +value
#include "codegen/X86Ops.ins"
#undef PROPERTY2
#define PROPERTY2(value) +0
   };

const uint32_t OMR::X86::InstOpCode::_features[] =
   {
#undef FEATURES
#define FEATURES(value) +value
#include "codegen/X86Ops.ins"
#undef FEATURES
#define FEATURES(value) +0
   };

#undef INSTRUCTION

// see compiler/x/codegen/OMRInstruction.hpp for structural information.
const OMR::X86::InstOpCode::OpCode_t OMR::X86::InstOpCode::_binaries[] =
   {
#define INSTRUCTION(name, mnemonic, binary, ...) binary
#undef BINARY
#define BINARY(...) {__VA_ARGS__}
#include "codegen/X86Ops.ins"
#undef INSTRUCTION
#undef BINARY
   };

void OMR::X86::InstOpCode::trackUpperBitsOnReg(TR::Register *reg, TR::CodeGenerator *cg)
   {
   if (cg->comp()->target().is64Bit())
      {
      if (clearsUpperBits())
         {
         reg->setUpperBitsAreZero(true);
         }
      else if (setsUpperBits())
         {
         reg->setUpperBitsAreZero(false);
         }
      }
   }

template <typename TBuffer> typename TBuffer::cursor_t OMR::X86::InstOpCode::OpCode_t::encode(typename TBuffer::cursor_t cursor, OMR::X86::Encoding encoding, uint8_t rexbits) const
   {
   TR::Compilation *comp = TR::comp();
   uint32_t enc = encoding;

   if (encoding == OMR::X86::Default)
      {
      enc = (comp->target().cpu.supportsAVX() || vex_l == VEX_LZ) ? vex_l : OMR::X86::Legacy;
      }

   TBuffer buffer(cursor);
   if (isX87())
      {
      buffer.append(opcode);
      // Heuristics for X87 second byte opcode
      // It could be eliminated if GCC/MSVC fully support initializer list
      buffer.append((uint8_t)((modrm_opcode << 5) | (modrm_form << 3) | immediate_size));
      return buffer;
      }

   // Prefixes
   TR::Instruction::REX rex(rexbits);
   rex.W = rex_w;

   if (enc != VEX_L___)
      {
      if (enc >> 2 && enc != VEX_LZ)
         {
         TR::Instruction::EVEX vex(rex, modrm_opcode);
         vex.mm = escape;
         vex.L = enc & 0x3;
         vex.p = prefixes;
         vex.opcode = opcode;
         buffer.append(vex);
         }
      else
         {
         TR::Instruction::VEX<3> vex(rex);
         vex.m = escape;
         vex.L = enc;
         vex.p = prefixes;
         vex.opcode = opcode;

         if (vex.CanBeShortened())
            {
            buffer.append(TR::Instruction::VEX<2>(vex));
            }
         else
            {
            buffer.append(vex);
            }
         }
      }
   else
      {
      switch (prefixes)
         {
         case PREFIX___:
            break;
         case PREFIX_66:
            buffer.append('\x66');
            break;
         case PREFIX_F2:
            buffer.append('\xf2');
            break;
         case PREFIX_F3:
            buffer.append('\xf3');
            break;
         case PREFIX_66_F2:
            buffer.append('\x66');
            buffer.append('\xf2');
            break;
         case PREFIX_66_F3:
            buffer.append('\x66');
            buffer.append('\xf3');
            break;
         default:
            break;
         }

#if defined(TR_TARGET_64BIT)
      // REX
      if (rex.value() || rexbits)
         {
         buffer.append(rex);
         }
#endif

      // OpCode escape
      switch (escape)
         {
         case ESCAPE_____:
            break;
         case ESCAPE_0F__:
            buffer.append('\x0f');
            break;
         case ESCAPE_0F38:
            buffer.append('\x0f');
            buffer.append('\x38');
            break;
         case ESCAPE_0F3A:
            buffer.append('\x0f');
            buffer.append('\x3a');
            break;
         default:
            break;
         }
      // OpCode
      buffer.append(opcode);
      }

   // ModRM
   if (modrm_form)
      {
      buffer.append(TR::Instruction::ModRM(modrm_opcode));
      }

   return buffer;
   }

void OMR::X86::InstOpCode::OpCode_t::finalize(uint8_t* cursor) const
   {
   // Finalize VEX prefix
   switch (*cursor)
      {
      case 0xC4:
         {
         auto pVEX = (TR::Instruction::VEX<3>*) cursor;
         auto modRM = (TR::Instruction::ModRM*) (cursor + 4);
         if (vex_v == VEX_vReg_)
            {
            pVEX->v = ~(modrm_form == ModRM_EXT_ ? pVEX->RM(*modRM) : pVEX->Reg(*modRM));
            }
         }
         break;
      case 0xC5:
         {
         auto pVEX = (TR::Instruction::VEX<2>*)cursor;
         auto modRM = (TR::Instruction::ModRM*) (cursor + 3);
         if (vex_v == VEX_vReg_)
            {
            pVEX->v = ~(modrm_form == ModRM_EXT_ ? pVEX->RM(*modRM) : pVEX->Reg(*modRM));
            }
         }
         break;
      case 0x62:
         {
         auto pVEX = (TR::Instruction::EVEX*)cursor;
         auto modRM = (TR::Instruction::ModRM*) (cursor + 5);
         if (vex_v == VEX_vReg_)
            {
            pVEX->v = ~(modrm_form == ModRM_EXT_ ? pVEX->RM(*modRM) : pVEX->Reg(*modRM));
            }
         }
         break;
      default:
         break;
      }
   }

void OMR::X86::InstOpCode::CheckAndFinishGroup07(uint8_t* cursor) const
   {
   if(info().isGroup07())
      {
      auto pModRM = (TR::Instruction::ModRM*)(cursor-1);
      switch(_mnemonic)
         {
         case OMR::InstOpCode::XEND:
            pModRM->rm = 0x05; // 0b101
            break;
         default:
            TR_ASSERT(false, "INVALID OPCODE.");
            break;
         }
      }
   }

uint8_t OMR::X86::InstOpCode::length(OMR::X86::Encoding encoding, uint8_t rex) const
   {
   return encode<Estimator>(0, encoding, rex);
   }
uint8_t* OMR::X86::InstOpCode::binary(uint8_t* cursor, OMR::X86::Encoding encoding, uint8_t rex) const
   {
   uint8_t* ret = encode<Writer>(cursor, encoding, rex);
   CheckAndFinishGroup07(ret);
   return ret;
   }
void OMR::X86::InstOpCode::finalize(uint8_t* cursor) const
   {
   if (!isPseudoOp())
      info().finalize(cursor);
   }

#ifdef DEBUG

#include "codegen/CodeGenerator.hpp"
#include "compile/Compilation.hpp"
#include "ras/Debug.hpp"
#include "codegen/InstOpCode.hpp"

const char *
OMR::X86::InstOpCode::getOpCodeName(TR::CodeGenerator *cg)
   {
   return cg->comp()->getDebug()->getOpCodeName(reinterpret_cast<TR::InstOpCode*>(this));
   }

const char *
OMR::X86::InstOpCode::getMnemonicName(TR::CodeGenerator *cg)
   {
   return cg->comp()->getDebug()->getMnemonicName(reinterpret_cast<TR::InstOpCode*>(this));
   }

#endif // ifdef DEBUG
