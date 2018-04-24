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

#define KALEIDOSCOPE_PAPAGENO_POSTPONE_INITIALIZATION
#include <Kaleidoscope-Papageno.h>
#include <kaleidoscope/hid.h>

extern "C" {
#include "detail/ppg_pattern_matching_detail.h"
#include "detail/ppg_active_tokens_detail.h"
#include "detail/ppg_context_detail.h"
}

// #define PPG_KLS_LOGGING_ENABLED

#ifdef PPG_KLS_LOGGING_ENABLED 

#include <iostream>

#define PPG_KLS_LOG(TOKEN) \
      std::cout << TOKEN;
#define PPG_KLS_LOGN(TOKEN) \
      std::cout << TOKEN << std::endl;
   
#else
#define PPG_KLS_LOG(TOKEN)
#define PPG_KLS_LOGN(TOKEN)
#endif

extern "C" {
   
   // The initialization method for the global papageno context
   // is define in the code that is generated by the Glockenspiel
   // compiler.
   //
   __attribute__((weak)) 
   void papageno_initialize_context() {}
   
//    void serial_print(const char *c) {
//       Serial.print(c);
//    }
}

kaleidoscope::papageno::Papageno Papageno;

namespace kaleidoscope {
namespace papageno {
   
extern void blockInput(uint8_t inputId);
extern void unblockInput(uint8_t inputId);
extern bool isInputBlocked(uint8_t inputId);

// constexpr uint8_t loopEventId = 255;
   
static uint8_t eventsFlushed_ = 0;

static bool eventHandlerDisabled = false;
static bool failureOccurred = false;
static bool justAddedLoopEvent = false;
static bool enabled = true;

struct TemporarilyDisableEventHandler
{
   TemporarilyDisableEventHandler() { 
      oldState_ = eventHandlerDisabled;
      eventHandlerDisabled = true; 
   }
   ~TemporarilyDisableEventHandler() { 
      eventHandlerDisabled = oldState_; 
   }
   bool oldState_;
};

inline
static uint8_t getKeystate(bool pressed)
{
   return ((pressed) ? (IS_PRESSED) : (WAS_PRESSED));
}

static void processEventCallback(   
                              PPG_Event *event,
                              void *)
{
//    Serial.print("Flushing input ");
//    Serial.print(event->input);
//    Serial.print(", active = ");
//    Serial.print(event->flags);
//    Serial.print(", n events = ");
//    Serial.print(ppg_event_buffer_size());
//    Serial.println();
//    Serial.flush();
   
   // Check for loop events
   //
//    if(event->input == kaleidoscope::papageno::loopEventId) {
//       
//       PPG_LOG("Loop event\n")
//       
//       Kaleidoscope.preClearLoopHooks();
//       kaleidoscope::hid::sendKeyboardReport();
//       Kaleidoscope.postClearLoopHooks();
//       return;
//    }
   
   // Ignore events that were considered, i.e. swallowed by Papageno
   //
   if(event->flags & PPG_Event_Considered) {
      return;
   }
   
   PPG_LOG("processEventCallback\n")
   
   uint8_t keyState = kaleidoscope::papageno::getKeystate(
      event->flags & PPG_Event_Active);
   
   if(!(event->flags & PPG_Event_Active)) {
      
      // Mark the input as unblocked
      //
      unblockInput(event->input);
   }
      
   // Note: Input-IDs are assigned contiguously
   //
   //       Input ids assigned to keypos-inputs are {0..PPG_Highest_Keypos_Input}
   
   TemporarilyDisableEventHandler tdh;
   
   PPG_KLS_LOG("event ")
   PPG_KLS_LOG((uint16_t)event)
   PPG_KLS_LOG(", input ")
   PPG_KLS_LOG((int)event->input)
   PPG_KLS_LOG(", group ")
   PPG_KLS_LOGN((int)event->groupId)
   
   PPG_KLS_LOG("flushing event by keypos (")
   PPG_KLS_LOG((int)ppg_kls_keypos_lookup[event->input].row)
   PPG_KLS_LOG(", ")
   PPG_KLS_LOG((int)ppg_kls_keypos_lookup[event->input].col)
   PPG_KLS_LOG("), keystate = ")
   PPG_KLS_LOG((int)keyState)
   PPG_KLS_LOGN("")
   
   handleKeyswitchEvent(Key_NoKey, 
                        ppg_kls_keypos_lookup[event->input].row,
                        ppg_kls_keypos_lookup[event->input].col,
                        keyState);
   
//       Kaleidoscope.preClearLoopHooks();
//       kaleidoscope::hid::sendKeyboardReport();
//       Kaleidoscope.postClearLoopHooks();
   
   ++papageno::eventsFlushed_;
}

static void flushEvents()
{  
   PPG_LOG("Flushing events\n")
   
   ppg_event_buffer_iterate(
         processEventCallback,
         NULL
      );
}

void time(PPG_Time *time)
{
   *time = static_cast<uint16_t>(millis());
}

void timeDifference(PPG_Time time1, PPG_Time time2, PPG_Time *delta)
{
   uint16_t *delta_t = (uint16_t *)delta;
   
   *delta_t = (uint16_t)time2 - (uint16_t)time1; 
}

int8_t timeComparison(
                        PPG_Time time1,
                        PPG_Time time2)
{
   if((uint16_t)time1 > (uint16_t)time2) {
      return 1;
   }
   else if((uint16_t)time1 == (uint16_t)time2) {
      return 0;
   }
    
   return -1;
}

static void signalCallback(PPG_Signal_Id signal_id, void *)
{
   switch(signal_id) {
      case PPG_On_Abort:
         PPG_KLS_LOGN("Abort")
         failureOccurred = true;
         papageno::flushEvents();
         break;
      case PPG_On_Timeout:
         PPG_KLS_LOGN("Timeout")
         failureOccurred = true;
         papageno::flushEvents();
         break;
      case PPG_On_Match_Failed:
         PPG_KLS_LOGN("Match failed")
         failureOccurred = true;
         // Events are flushed automatically in case of failure
         break;      
      case PPG_On_Flush_Events:
         PPG_KLS_LOGN("Flush events")
         papageno::flushEvents();
         break;
      case PPG_On_Initialization:
         PPG_KLS_LOGN("On initialization")
         break;
      case PPG_Before_Action:
         PPG_KLS_LOGN("Before action")
         break;
      default:
         return;
   }
} 

static Key eventHandlerHook(Key keycode, byte row, byte col, uint8_t key_state)
{ 
   // When flushing events, this event handler must 
   // be disabled as the events are meant to be handled 
   // by the rest of Kaleidoscope.
   //
   if(eventHandlerDisabled) {
      return keycode;
   }
   
   if(key_state == 0) { return keycode; }
   
   // Don't react on injected keys
   //
   if(key_state & INJECTED) { return keycode; }
   
   if(!enabled) {
      return keycode;
   }
   
   PPG_KLS_LOG("eventHandlerHook")
   PPG_KLS_LOG(", row ")
   PPG_KLS_LOG((int)row)
   PPG_KLS_LOG(", col ")
   PPG_KLS_LOG((int)col)
   PPG_KLS_LOG(", keycode ")
   PPG_KLS_LOG((int)keycode.raw)
   PPG_KLS_LOG(", key_state ")
   PPG_KLS_LOG((int)key_state)
   PPG_KLS_LOGN("")
   
   TemporarilyDisableEventHandler tdh;
   
   PPG_Count flags = PPG_Event_Flags_Empty;
   bool keyStateChanged = true;
   if (keyToggledOn(key_state)) {
      flags = PPG_Event_Active;
   }
   else if(!keyToggledOff(key_state)) {
      keyStateChanged = false;
   }
         
   uint8_t input = inputIdFromKeypos(row, col);
   
   if(input == PPG_KLS_Not_An_Input) { 
      
      // Only interrupt pattern recognition if 
      // unrelated (non input) keys are pressed (rather than release)
      //
      if(flags != PPG_Event_Active) {
         return keycode;
      }
         
//          PPG_LOG("not an input\n");
      
      // Only if another (unrelated) key was pressed we abort.
      //
//       if(flags == PPG_Event_Active) {

         PPG_KLS_LOGN("before: keycode " << keycode.raw
            << ", layerState " << Layer.getLayerState())
         
         papageno::eventsFlushed_ = 0;
         
         // Whenever a key occurs that is not an input,
         // we immediately abort pattern matching
         //
         ppg_global_abort_pattern_matching();
         
         if(papageno::eventsFlushed_) {
            
            // Note: The current layer might have been changed during abort
            //       of pattern matching as, e.g. a tap dance might have
            //       toggled a layer switch. 
            
            // Update the live composite keymap for the current key
            //
            Layer.updateLiveCompositeKeymap(row, col);
            
            // To be on the safe side, we lookup on the current layer.
            //
            keycode = Layer.lookupOnActiveLayer(row, col);
         }
         
         PPG_KLS_LOGN("Events flushed: " << (int)papageno::eventsFlushed_)
         
         PPG_KLS_LOGN("after: keycode " << keycode.raw
            << ", layerState " << Layer.getLayerState())
//       }
      
      // Let Kaleidoscope process the key in a regular way
      //
      return keycode;
   }
   
   if(!keyStateChanged) {
      
//       PPG_KLS_LOGN("papageno::state = " << (int)papageno::state)
      
      // If the input is not blocked, it apparently has already been
      // flushed and we pass the keycode on
      //
      if(!isInputBlocked(input)) {
         PPG_KLS_LOG("Passing keycode of unblocked input\n")
         return keycode;
      }
            
      // The key is obviously an input. If there are nevertheless no events in the buffer this means that a previous match with the key failed.
      // In such a case, we pass it back to Kaleidoscope immediately.
      //
      if(failureOccurred
         && !ppg_pattern_matching_in_progress()
      ) {
         PPG_KLS_LOG("Bad engine state => passing keycode\n")
         return keycode;
      }
      
      // Just ignore keys that represent inputs that already became 
      // activated and that are active but whose state did not change.
      //
      return Key_NoKey;
   }
   
   // Clear failure state
   //
   failureOccurred = false;
   
   // Non activation events are ignored if there are no active tokens
   // and no queued events.
   //
   if(   (ppg_event_buffer_size() == 0) 
      && (ppg_active_tokens_get_size() == 0)
      && (flags == PPG_Event_Flags_Empty)) 
   {
      return keycode;
   }
      
   PPG_Event p_event = {
      .input = input,
      .time = (PPG_Time)millis(),
      .flags = flags,
      
      // The group id is used to code the loop count a event occured
      // within
      //
      .groupId = 0 //kaleidoscope::papageno::loopCycleCount % 0xFF
   };
   
   uint8_t cur_layer = Layer.top();
   
   ppg_global_set_layer(cur_layer);
   
   PPG_KLS_LOGN("Feeding event")
   
   if(flags == PPG_Event_Active) {
      
      // Mark the input as blocked
      //
      blockInput(input);
   }

   justAddedLoopEvent = false;
   ppg_event_process(&p_event);
   
   return Key_NoKey;
}

void 
   Papageno
      ::begin() 
{
   // Initialize the static global pattern matching tree
   //
    papageno_initialize_context();
//    
   this->init();
   
   Kaleidoscope.useEventHandlerHook(
         kaleidoscope::papageno::eventHandlerHook);
}

void 
   Papageno
      ::init()
{
   Kaleidoscope.setKeyboardReportSendPolicy(
         kaleidoscope::KeyboardReportSendOnEvent);
   
   ppg_global_set_default_event_processor(
      (PPG_Event_Processor_Fun)kaleidoscope::papageno::processEventCallback);

   ppg_global_set_signal_callback(
      (PPG_Signal_Callback) {
            .func = (PPG_Signal_Callback_Fun)kaleidoscope::papageno::signalCallback,
            .user_data = NULL
      }
   );

   ppg_global_set_time_manager(
      (PPG_Time_Manager) {
         .time
            = (PPG_Time_Fun)kaleidoscope::papageno::time,
         .time_difference
            = (PPG_Time_Difference_Fun)kaleidoscope::papageno::timeDifference,
         .compare_times
            = (PPG_Time_Comparison_Fun)kaleidoscope::papageno::timeComparison
      }
   );
   
//    ppg_timeout_set_state(false); // Generally disable timeout to
//                                  // prevent asynchronous timout handling
}

void  
   Papageno
      ::processKeycode(PPG_Count activation_flags, void *user_data)
{   
   Key key;
   key.raw = (uint16_t)user_data;
      
   uint8_t keyState = kaleidoscope::papageno::getKeystate(
                  activation_flags & PPG_Action_Activation_Flags_Active);
   
   if(   (keyState & IS_PRESSED) 
      && (activation_flags & PPG_Action_Activation_Flags_Repeated)) {
      keyState |= WAS_PRESSED;
   }
   
   PPG_KLS_LOG("keycode action. keycode ")
   PPG_KLS_LOG((int)key.raw)
   PPG_KLS_LOG(", keyState ")
   PPG_KLS_LOG((int)keyState)
   PPG_KLS_LOGN("")
   
   // Note: Setting UNKNOWN_KEYSWITCH_LOCATION will skip keymap lookup
   //
   {
      TemporarilyDisableEventHandler tdh;
      handleKeyswitchEvent(key, UNKNOWN_KEYSWITCH_LOCATION, keyState);
   }
}

void processKeypos(PPG_Count activation_flags, void *user_data)
{
   uint16_t raw = (uint16_t)user_data;
   uint8_t row = raw >> 8;
   uint8_t col = raw & 0x00FF;
         
   uint8_t keyState = kaleidoscope::papageno::getKeystate(
                  activation_flags & PPG_Action_Activation_Flags_Active);
   
   if(   (keyState & IS_PRESSED) 
      && (activation_flags & PPG_Action_Activation_Flags_Repeated)) {
      keyState |= WAS_PRESSED;
   }
   
   PPG_KLS_LOG("keypos action. row ")
   PPG_KLS_LOG((int)row)
   PPG_KLS_LOG(", col ")
   PPG_KLS_LOG((int)col)
   PPG_KLS_LOG(", keyState ")
   PPG_KLS_LOG((int)keyState)
   PPG_KLS_LOGN("")

   {
      TemporarilyDisableEventHandler tdh;
      handleKeyswitchEvent(Key_NoKey, row, col, keyState);
   }
}

// static bool conditionallyAddLoopEvent()
// {   
//    if(!ppg_pattern_matching_in_progress()) {
//       return false;
//    }
//    
//    if(justAddedLoopEvent) { return false; }
//       
//    PPG_Event p_event = {
//       .input = loopEventId,
//       .time = (PPG_Time)millis(),
//       .flags = PPG_Event_Flags_Empty,
//       
//       // The group id is used to code the loop count an event occured
//       // within
//       //
//       .groupId = 0//kaleidoscope::papageno::loopCycleCount % 0xFF
//    };
//    
//    // Be careful not to spam the event queue with loop events
//    //
//    justAddedLoopEvent = true;
//    ppg_event_process(&p_event);
//    
//    return true;
// }

void 
   Papageno
      ::loop()
{
   if(!enabled) {
      Kaleidoscope.loop();
      return;
   }
   
//    bool haveLoopHandlers = true;
   
   PPG_KLS_LOGN("Kaleidoscope process key events")
   Kaleidoscope.processKeyEvents();

   
   // As timeout might cause events e.g. when a tap-dance is 
   // activated, we have to make sure that we do not run into a loop
   // here.
   //
   {
      TemporarilyDisableEventHandler tdh;
//       PPG_KLS_LOGN("Timeout check")
//       ppg_timeout_set_state(true);
      ppg_timeout_check();
//       ppg_timeout_set_state(false);
   }
   
//    if(ppg_pattern_matching_in_progress()) {
//       if(conditionallyAddLoopEvent()) {
//          PPG_KLS_LOGN("Conditionally added loop event")
//          
//          haveLoopHandlers = false;
//       }
//    }
   
//    if(haveLoopHandlers) {
      PPG_KLS_LOGN("Kaleidoscope loop hooks")
//       Kaleidoscope.processLoopHooks();
      
      Kaleidoscope.preClearLoopHooks();
      kaleidoscope::hid::sendKeyboardReport();
      Kaleidoscope.postClearLoopHooks();
//    }
      
   //       PPG_KLS_LOG("active tokens")
   PPG_KLS_LOGN("# active tokens: " << (int)PPG_GAT.n_tokens)
   ppg_active_tokens_repeat_actions();
}

void 
   Papageno
      ::setEnabled(bool state)
{
   enabled = state;
}

bool 
   Papageno
      ::getEnabled()
{
   return enabled;
}

} // end namespace papageno
} // end namepace kaleidoscope
