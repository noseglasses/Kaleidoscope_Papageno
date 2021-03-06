/* -*- mode: c++ -*-
 * Kaleidoscope-Papageno -- Papageno features for Kaleidoscope
 * Copyright (C) 2017 noseglasses <shinynoseglasses@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

} // This ends region extern "C"

#include <assert.h>

// A note on the use of the __NL__ macro below:
//
//       Preprocessor macro functions can be 
//       hard to debug.
//
//       One approach is to take a look at
//       the preprocessed code to find out 
//       what goes wrong. 
//
//       Unfortunatelly macro replacement cannot deal with newlines.
//       Because of this, code that emerges from excessive macro
//       replacement looks very unreadable due to the absence of 
//       any line breaks.
//
//       To deal with this restriction, comment the
//       definition of the __NL__ macro below, during
//       preprocessor debugging. In that case, the
//       occurrences of __NL__ will not be replaced by
//       and empty string as in the compiled version but
//       will still be present in the preprocessed code.
//       Replace all occurrences of __NL__ in the preprocessed
//       with newlines using a text editor to gain a
//       readable version of the generated code.

#define __NL__
#define __NN__

namespace kaleidoscope {
namespace papageno {

// The inputs that are defined by Papageno in the firmware sketch
// are mapped to a range of compile time constants whose value starts from 
// zero.
//
// To allow for automatic numbering, we use the __COUNTER__ macro. 
// This macro returns a consecutive set of integer numbers starting from
// zero. It is is important to note that __COUNTER__ is not uniqe for a 
// given include file but for the overall compilation unit. That's 
// why we have to define an offset to make sure that our input ids actually
// start from zero.
//
static constexpr unsigned PPG_Keypos_Input_Offset = __COUNTER__;

#define PPG_KLS_DEFINE_KEYPOS_INPUT_ID(UNIQUE_ID, USER_ID, ROW, COL)                           \
__NL__   static constexpr unsigned PPG_KLS_KEYPOS_INPUT(UNIQUE_ID)                    \
__NL__               = __COUNTER__ - PPG_Keypos_Input_Offset - 1;

GLS_INPUTS___KEYPOS(PPG_KLS_DEFINE_KEYPOS_INPUT_ID)

// Attention! PPG_Highest_Keypos_Input may be negative in case
// that no keypos inputs are defined
//
static constexpr int16_t PPG_Highest_Keypos_Input
           = (int16_t)(__COUNTER__) - PPG_Keypos_Input_Offset - 2 ;
           
static_assert(PPG_Highest_Keypos_Input >= 0, "PPG_Highest_Keypos_Input negative");

int16_t highestKeyposInputId() {
   return PPG_Highest_Keypos_Input;
}

static constexpr unsigned PPG_Keycode_Input_Offset = __COUNTER__;

// To attach to Kaleidoscope's event handling, we 
// need to be able to determine an input id from a keypos.
//
PPG_Input_Id inputIdFromKeypos(byte row, byte col)
{
   uint16_t id = 256*row + col;

   switch(id) {
   
#     define PPG_KLS_KEYPOS_CASE_LABEL(UNIQUE_ID, USER_ID, ROW, COL)                           \
__NL__   case 256*ROW + COL:                                                   \
__NL__      return PPG_KLS_KEYPOS_INPUT(UNIQUE_ID);                                   \
__NL__      break;

      GLS_INPUTS___KEYPOS(PPG_KLS_KEYPOS_CASE_LABEL)
   }

   return PPG_KLS_Not_An_Input;
}

PPG_KLS_Keypos ppg_kls_keypos_lookup[] = {

#  define PPG_KLS_KEYPOS_TO_LOOKUP_ENTRY(UNIQUE_ID, USER_ID, ROW, COL)                         \
      { .row = ROW, .col = COL },
      
   GLS_INPUTS___KEYPOS(PPG_KLS_KEYPOS_TO_LOOKUP_ENTRY)

   { .row = 0xFF, .col = 0xFFL }
};

#define PPG_KLS_ADD_ONE(...) + 1

static constexpr unsigned PPG_KLS_N_Inputs = 0
   GLS_INPUTS___KEYPOS(PPG_KLS_ADD_ONE)
//    GLS_INPUTS___KEYCODE(PPG_KLS_ADD_ONE)
//    GLS_INPUTS___COMPLEX_KEYCODE(PPG_KLS_ADD_ONE)
;

PPG_Bitfield_Storage_Type inputsBlockedBits[(GLS_NUM_BITS_LEFT(PPG_KLS_N_Inputs) != 0) ? (GLS_NUM_BYTES(PPG_KLS_N_Inputs) + 1) : GLS_NUM_BYTES(PPG_KLS_N_Inputs)]
   = GLS_ZERO_INIT;
   
PPG_Bitfield inputsBlocked = { inputsBlockedBits, PPG_KLS_N_Inputs };
   
void blockInput(uint8_t inputId) {
   ppg_bitfield_set_bit(&inputsBlocked, inputId, true);
}

void unblockInput(uint8_t inputId) {
   ppg_bitfield_set_bit(&inputsBlocked, inputId, false);
}

bool isInputBlocked(uint8_t inputId) {
   return ppg_bitfield_get_bit(&inputsBlocked, inputId);
}

// int8_t inputsBlocked[PPG_KLS_N_Inputs] = GLS_ZERO_INIT;
// 
// void blockInput(uint8_t inputId) {
//    ++inputsBlocked[inputId];
// }
// 
// void unblockInput(uint8_t inputId) {
//    --inputsBlocked[inputId];
//    assert(inputsBlocked[inputId] >= 0);
// }
// 
// bool isInputBlocked(uint8_t inputId) {
//    return inputsBlocked[inputId] > 0;
// }

} // namespace papageno
} // namespace kaleidoscope

extern "C" {
