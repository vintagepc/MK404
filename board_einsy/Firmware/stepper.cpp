/*
  stepper.c - stepper motor driver: executes motion plans using stepper motors
  Part of Grbl

  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

/* The timer calculations of this module informed by the 'RepRap cartesian firmware' by Zack Smith
   and Philipp Tiefenbacher. */

#include "Marlin.h"
#include "stepper.h"
#include "planner.h"
#include "temperature.h"
#include "ultralcd.h"
#include "language.h"
#include "cardreader.h"
#include "speed_lookuptable.h"
#if defined(DIGIPOTSS_PIN) && DIGIPOTSS_PIN > -1
#include <SPI.h>
#endif
#ifdef TMC2130
#include "tmc2130.h"
#endif //TMC2130

#if defined(FILAMENT_SENSOR) && defined(PAT9125)
#include "fsensor.h"
int fsensor_counter; //counter for e-steps
#endif //FILAMENT_SENSOR

#include "mmu.h"
#include "ConfigurationStore.h"

#ifdef DEBUG_STACK_MONITOR
uint16_t SP_min = 0x21FF;
#endif //DEBUG_STACK_MONITOR

//===========================================================================
//=============================public variables  ============================
//===========================================================================
block_t *current_block;  // A pointer to the block currently being traced
bool x_min_endstop = false;
bool x_max_endstop = false;
bool y_min_endstop = false;
bool y_max_endstop = false;
bool z_min_endstop = false;
bool z_max_endstop = false;
//===========================================================================
//=============================private variables ============================
//===========================================================================
//static makes it inpossible to be called from outside of this file by extern.!

// Variables used by The Stepper Driver Interrupt
static unsigned char out_bits;        // The next stepping-bits to be output
static dda_isteps_t
               counter_x,       // Counter variables for the bresenham line tracer
               counter_y,
               counter_z,
               counter_e;
volatile dda_usteps_t step_events_completed; // The number of step events executed in the current block
static int32_t  acceleration_time, deceleration_time;
//static unsigned long accelerate_until, decelerate_after, acceleration_rate, initial_rate, final_rate, nominal_rate;
static uint16_t acc_step_rate; // needed for deccelaration start point
static uint8_t  step_loops;
static uint16_t OCR1A_nominal;
static uint8_t  step_loops_nominal;

volatile long endstops_trigsteps[3]={0,0,0};
volatile long endstops_stepsTotal,endstops_stepsDone;
static volatile bool endstop_x_hit=false;
static volatile bool endstop_y_hit=false;
static volatile bool endstop_z_hit=false;
#ifdef ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED
bool abort_on_endstop_hit = false;
#endif
#ifdef MOTOR_CURRENT_PWM_XY_PIN
  int motor_current_setting[3] = DEFAULT_PWM_MOTOR_CURRENT;
  int motor_current_setting_silent[3] = DEFAULT_PWM_MOTOR_CURRENT;
  int motor_current_setting_loud[3] = DEFAULT_PWM_MOTOR_CURRENT_LOUD;
#endif

#if ( (defined(X_MAX_PIN) && (X_MAX_PIN > -1)) || defined(TMC2130_SG_HOMING) ) && !defined(DEBUG_DISABLE_XMAXLIMIT)
static bool old_x_max_endstop=false;
#endif
#if ( (defined(Y_MAX_PIN) && (Y_MAX_PIN > -1)) || defined(TMC2130_SG_HOMING) ) && !defined(DEBUG_DISABLE_YMAXLIMIT)
static bool old_y_max_endstop=false;
#endif

static bool old_x_min_endstop=false;
static bool old_y_min_endstop=false;
static bool old_z_min_endstop=false;
static bool old_z_max_endstop=false;

static bool check_endstops = true;

static bool check_z_endstop = false;
static bool z_endstop_invert = false;

volatile long count_position[NUM_AXIS] = { 0, 0, 0, 0};
volatile signed char count_direction[NUM_AXIS] = { 1, 1, 1, 1};

#ifdef LIN_ADVANCE
  void advance_isr_scheduler();
  void advance_isr();

  static const uint16_t ADV_NEVER      = 0xFFFF;
  static const uint8_t  ADV_INIT       = 0b01;
  static const uint8_t  ADV_DECELERATE = 0b10;

  static uint16_t nextMainISR;
  static uint16_t nextAdvanceISR;

  static uint16_t main_Rate;
  static uint16_t eISR_Rate;
  static uint16_t eISR_Err;

  static uint16_t current_adv_steps;
  static uint16_t final_adv_steps;
  static uint16_t max_adv_steps;
  static uint32_t LA_decelerate_after;

  static int8_t e_steps;
  static uint8_t e_step_loops;
  static int8_t LA_phase;

  #define _NEXT_ISR(T)    main_Rate = nextMainISR = T
#else
  #define _NEXT_ISR(T)    OCR1A = T
#endif

#ifdef DEBUG_STEPPER_TIMER_MISSED
extern bool stepper_timer_overflow_state;
extern uint16_t stepper_timer_overflow_last;
#endif /* DEBUG_STEPPER_TIMER_MISSED */

//===========================================================================
//=============================functions         ============================
//===========================================================================

void checkHitEndstops()
{
 if( endstop_x_hit || endstop_y_hit || endstop_z_hit) {
   SERIAL_ECHO_START;
   SERIAL_ECHORPGM(MSG_ENDSTOPS_HIT);
   if(endstop_x_hit) {
     SERIAL_ECHOPAIR(" X:",(float)endstops_trigsteps[X_AXIS]/cs.axis_steps_per_unit[X_AXIS]);
//     LCD_MESSAGERPGM(CAT2((MSG_ENDSTOPS_HIT), PSTR("X")));
   }
   if(endstop_y_hit) {
     SERIAL_ECHOPAIR(" Y:",(float)endstops_trigsteps[Y_AXIS]/cs.axis_steps_per_unit[Y_AXIS]);
//     LCD_MESSAGERPGM(CAT2((MSG_ENDSTOPS_HIT), PSTR("Y")));
   }
   if(endstop_z_hit) {
     SERIAL_ECHOPAIR(" Z:",(float)endstops_trigsteps[Z_AXIS]/cs.axis_steps_per_unit[Z_AXIS]);
//     LCD_MESSAGERPGM(CAT2((MSG_ENDSTOPS_HIT),PSTR("Z")));
   }
   SERIAL_ECHOLN("");
   endstop_x_hit=false;
   endstop_y_hit=false;
   endstop_z_hit=false;
#if defined(ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED) && defined(SDSUPPORT)
   if (abort_on_endstop_hit)
   {
     card.sdprinting = false;
     card.closefile();
     quickStop();
     setTargetHotend0(0);
     setTargetHotend1(0);
     setTargetHotend2(0);
   }
#endif
 }
}

bool endstops_hit_on_purpose()
{
  bool hit = endstop_x_hit || endstop_y_hit || endstop_z_hit;
  endstop_x_hit=false;
  endstop_y_hit=false;
  endstop_z_hit=false;
  return hit;
}

bool endstop_z_hit_on_purpose()
{
  bool hit = endstop_z_hit;
  endstop_z_hit=false;
  return hit;
}

bool enable_endstops(bool check)
{
  bool old = check_endstops;
  check_endstops = check;
  return old;
}

bool enable_z_endstop(bool check)
{
	bool old = check_z_endstop;
	check_z_endstop = check;
	endstop_z_hit = false;
	return old;
}

void invert_z_endstop(bool endstop_invert)
{
  z_endstop_invert = endstop_invert;
}

//         __________________________
//        /|                        |\     _________________         ^
//       / |                        | \   /|               |\        |
//      /  |                        |  \ / |               | \       s
//     /   |                        |   |  |               |  \      p
//    /    |                        |   |  |               |   \     e
//   +-----+------------------------+---+--+---------------+----+    e
//   |               BLOCK 1            |      BLOCK 2          |    d
//
//                           time ----->
//
//  The trapezoid is the shape the speed curve over time. It starts at block->initial_rate, accelerates
//  first block->accelerate_until step_events_completed, then keeps going at constant speed until
//  step_events_completed reaches block->decelerate_after after which it decelerates until the trapezoid generator is reset.
//  The slope of acceleration is calculated with the leib ramp alghorithm.

// "The Stepper Driver Interrupt" - This timer interrupt is the workhorse.
// It pops blocks from the block_buffer and executes them by pulsing the stepper pins appropriately.
ISR(TIMER1_COMPA_vect) {
#ifdef DEBUG_STACK_MONITOR
	uint16_t sp = SPL + 256 * SPH;
	if (sp < SP_min) SP_min = sp;
#endif //DEBUG_STACK_MONITOR

#ifdef LIN_ADVANCE
    advance_isr_scheduler();
#else
    isr();
#endif

  // Don't run the ISR faster than possible
  // Is there a 8us time left before the next interrupt triggers?
  if (OCR1A < TCNT1 + 16) {
#ifdef DEBUG_STEPPER_TIMER_MISSED
    // Verify whether the next planned timer interrupt has not been missed already.  
    // This debugging test takes < 1.125us
    // This skews the profiling slightly as the fastest stepper timer
    // interrupt repeats at a 100us rate (10kHz).
    if (OCR1A + 40 < TCNT1) {
      // The interrupt was delayed by more than 20us (which is 1/5th of the 10kHz ISR repeat rate).
      // Give a warning.
      stepper_timer_overflow_state = true;
      stepper_timer_overflow_last = TCNT1 - OCR1A;
      // Beep, the beeper will be cleared at the stepper_timer_overflow() called from the main thread.
      WRITE(BEEPER, HIGH);
    }
#endif
    // Fix the next interrupt to be executed after 8us from now.
    OCR1A = TCNT1 + 16; 
  }
}

uint8_t last_dir_bits = 0;

#ifdef BACKLASH_X
uint8_t st_backlash_x = 0;
#endif //BACKLASH_X
#ifdef BACKLASH_Y
uint8_t st_backlash_y = 0;
#endif //BACKLASH_Y

FORCE_INLINE void stepper_next_block()
{
  // Anything in the buffer?
  //WRITE_NC(LOGIC_ANALYZER_CH2, true);
  current_block = plan_get_current_block();
  if (current_block != NULL) {
#ifdef BACKLASH_X
	if (current_block->steps_x.wide)
	{ //X-axis movement
		if ((current_block->direction_bits ^ last_dir_bits) & 1)
		{
			printf_P(PSTR("BL %d\n"), (current_block->direction_bits & 1)?st_backlash_x:-st_backlash_x);
			if (current_block->direction_bits & 1)
				WRITE_NC(X_DIR_PIN, INVERT_X_DIR);
			else
				WRITE_NC(X_DIR_PIN, !INVERT_X_DIR);
			_delay_us(100);
			for (uint8_t i = 0; i < st_backlash_x; i++)
			{
				WRITE_NC(X_STEP_PIN, !INVERT_X_STEP_PIN);
				_delay_us(100);
				WRITE_NC(X_STEP_PIN, INVERT_X_STEP_PIN);
				_delay_us(900);
			}
		}
		last_dir_bits &= ~1;
		last_dir_bits |= current_block->direction_bits & 1;
	}
#endif
#ifdef BACKLASH_Y
	if (current_block->steps_y.wide)
	{ //Y-axis movement
		if ((current_block->direction_bits ^ last_dir_bits) & 2)
		{
			printf_P(PSTR("BL %d\n"), (current_block->direction_bits & 2)?st_backlash_y:-st_backlash_y);
			if (current_block->direction_bits & 2)
				WRITE_NC(Y_DIR_PIN, INVERT_Y_DIR);
			else
				WRITE_NC(Y_DIR_PIN, !INVERT_Y_DIR);
			_delay_us(100);
			for (uint8_t i = 0; i < st_backlash_y; i++)
			{
				WRITE_NC(Y_STEP_PIN, !INVERT_Y_STEP_PIN);
				_delay_us(100);
				WRITE_NC(Y_STEP_PIN, INVERT_Y_STEP_PIN);
				_delay_us(900);
			}
		}
		last_dir_bits &= ~2;
		last_dir_bits |= current_block->direction_bits & 2;
	}
#endif

    // The busy flag is set by the plan_get_current_block() call.
    // current_block->busy = true;
    // Initializes the trapezoid generator from the current block. Called whenever a new
    // block begins.
    deceleration_time = 0;
    // Set the nominal step loops to zero to indicate, that the timer value is not known yet.
    // That means, delay the initialization of nominal step rate and step loops until the steady
    // state is reached.
    step_loops_nominal = 0;
    acc_step_rate = uint16_t(current_block->initial_rate);
    acceleration_time = calc_timer(acc_step_rate, step_loops);

#ifdef LIN_ADVANCE
    if (current_block->use_advance_lead) {
        LA_decelerate_after = current_block->decelerate_after;
        final_adv_steps = current_block->final_adv_steps;
        max_adv_steps = current_block->max_adv_steps;
        e_step_loops = current_block->advance_step_loops;
    } else {
        e_steps = 0;
        e_step_loops = 1;
        current_adv_steps = 0;
    }
    nextAdvanceISR = ADV_NEVER;
    LA_phase = -1;
#endif

    if (current_block->flag & BLOCK_FLAG_E_RESET) {
        count_position[E_AXIS] = 0;
    }

    if (current_block->flag & BLOCK_FLAG_DDA_LOWRES) {
      counter_x.lo = -(current_block->step_event_count.lo >> 1);
      counter_y.lo = counter_x.lo;
      counter_z.lo = counter_x.lo;
      counter_e.lo = counter_x.lo;
    } else {
      counter_x.wide = -(current_block->step_event_count.wide >> 1);
      counter_y.wide = counter_x.wide;
      counter_z.wide = counter_x.wide;
      counter_e.wide = counter_x.wide;
    }
    step_events_completed.wide = 0;
    // Set directions.
    out_bits = current_block->direction_bits;
    // Set the direction bits (X_AXIS=A_AXIS and Y_AXIS=B_AXIS for COREXY)
    if((out_bits & (1<<X_AXIS))!=0){
      WRITE_NC(X_DIR_PIN, INVERT_X_DIR);
      count_direction[X_AXIS]=-1;
    } else {
      WRITE_NC(X_DIR_PIN, !INVERT_X_DIR);
      count_direction[X_AXIS]=1;
    }
    if((out_bits & (1<<Y_AXIS))!=0){
      WRITE_NC(Y_DIR_PIN, INVERT_Y_DIR);
      count_direction[Y_AXIS]=-1;
    } else {
      WRITE_NC(Y_DIR_PIN, !INVERT_Y_DIR);
      count_direction[Y_AXIS]=1;
    }
    if ((out_bits & (1<<Z_AXIS)) != 0) {   // -direction
      WRITE_NC(Z_DIR_PIN,INVERT_Z_DIR);
      count_direction[Z_AXIS]=-1;
    } else { // +direction
      WRITE_NC(Z_DIR_PIN,!INVERT_Z_DIR);
      count_direction[Z_AXIS]=1;
    }
    if ((out_bits & (1 << E_AXIS)) != 0) { // -direction
#ifndef LIN_ADVANCE
      WRITE(E0_DIR_PIN, 
  #ifdef SNMM
        (mmu_extruder == 0 || mmu_extruder == 2) ? !INVERT_E0_DIR :
  #endif // SNMM
        INVERT_E0_DIR);
#endif /* LIN_ADVANCE */
      count_direction[E_AXIS] = -1;
    } else { // +direction
#ifndef LIN_ADVANCE
      WRITE(E0_DIR_PIN,
  #ifdef SNMM
        (mmu_extruder == 0 || mmu_extruder == 2) ? INVERT_E0_DIR :
  #endif // SNMM
        !INVERT_E0_DIR);
#endif /* LIN_ADVANCE */
      count_direction[E_AXIS] = 1;
    }
#if defined(FILAMENT_SENSOR) && defined(PAT9125)
    fsensor_st_block_begin(count_direction[E_AXIS] < 0);
#endif //FILAMENT_SENSOR
  }
  else {
      _NEXT_ISR(2000); // 1kHz.

#ifdef LIN_ADVANCE
      // reset LA state when there's no block
      nextAdvanceISR = ADV_NEVER;
      e_steps = 0;

      // incrementally lose pressure to give a chance for
      // a new LA block to be scheduled and recover
      if(current_adv_steps)
          --current_adv_steps;
#endif
  }
  //WRITE_NC(LOGIC_ANALYZER_CH2, false);
}

// Check limit switches.
FORCE_INLINE void stepper_check_endstops()
{
  if(check_endstops) 
  {
    #ifndef COREXY
    if ((out_bits & (1<<X_AXIS)) != 0) // stepping along -X axis
    #else
    if ((((out_bits & (1<<X_AXIS)) != 0)&&(out_bits & (1<<Y_AXIS)) != 0)) //-X occurs for -A and -B
    #endif
    {
      #if ( (defined(X_MIN_PIN) && (X_MIN_PIN > -1)) || defined(TMC2130_SG_HOMING) ) && !defined(DEBUG_DISABLE_XMINLIMIT)
      #ifdef TMC2130_SG_HOMING
        // Stall guard homing turned on
        x_min_endstop = (READ(X_TMC2130_DIAG) != 0);
      #else
        // Normal homing
        x_min_endstop = (READ(X_MIN_PIN) != X_MIN_ENDSTOP_INVERTING);
      #endif
        if(x_min_endstop && old_x_min_endstop && (current_block->steps_x.wide > 0)) {
          endstops_trigsteps[X_AXIS] = count_position[X_AXIS];
          endstop_x_hit=true;
          step_events_completed.wide = current_block->step_event_count.wide;
        }
        old_x_min_endstop = x_min_endstop;
      #endif
    } else { // +direction
      #if ( (defined(X_MAX_PIN) && (X_MAX_PIN > -1)) || defined(TMC2130_SG_HOMING) ) && !defined(DEBUG_DISABLE_XMAXLIMIT)          
        #ifdef TMC2130_SG_HOMING
        // Stall guard homing turned on
            x_max_endstop = (READ(X_TMC2130_DIAG) != 0);
        #else
        // Normal homing
        x_max_endstop = (READ(X_MAX_PIN) != X_MAX_ENDSTOP_INVERTING);
        #endif
        if(x_max_endstop && old_x_max_endstop && (current_block->steps_x.wide > 0)){
          endstops_trigsteps[X_AXIS] = count_position[X_AXIS];
          endstop_x_hit=true;
          step_events_completed.wide = current_block->step_event_count.wide;
        }
        old_x_max_endstop = x_max_endstop;
      #endif
    }

    #ifndef COREXY
    if ((out_bits & (1<<Y_AXIS)) != 0) // -direction
    #else
    if ((((out_bits & (1<<X_AXIS)) != 0)&&(out_bits & (1<<Y_AXIS)) == 0)) // -Y occurs for -A and +B
    #endif
    {        
      #if ( (defined(Y_MIN_PIN) && (Y_MIN_PIN > -1)) || defined(TMC2130_SG_HOMING) ) && !defined(DEBUG_DISABLE_YMINLIMIT)          
      #ifdef TMC2130_SG_HOMING
      // Stall guard homing turned on
          y_min_endstop = (READ(Y_TMC2130_DIAG) != 0);
      #else
      // Normal homing
      y_min_endstop = (READ(Y_MIN_PIN) != Y_MIN_ENDSTOP_INVERTING);
      #endif
        if(y_min_endstop && old_y_min_endstop && (current_block->steps_y.wide > 0)) {
          endstops_trigsteps[Y_AXIS] = count_position[Y_AXIS];
          endstop_y_hit=true;
          step_events_completed.wide = current_block->step_event_count.wide;
        }
        old_y_min_endstop = y_min_endstop;
      #endif
    } else { // +direction
      #if ( (defined(Y_MAX_PIN) && (Y_MAX_PIN > -1)) || defined(TMC2130_SG_HOMING) ) && !defined(DEBUG_DISABLE_YMAXLIMIT)                
        #ifdef TMC2130_SG_HOMING
        // Stall guard homing turned on
            y_max_endstop = (READ(Y_TMC2130_DIAG) != 0);
        #else
        // Normal homing
        y_max_endstop = (READ(Y_MAX_PIN) != Y_MAX_ENDSTOP_INVERTING);
        #endif
        if(y_max_endstop && old_y_max_endstop && (current_block->steps_y.wide > 0)){
          endstops_trigsteps[Y_AXIS] = count_position[Y_AXIS];
          endstop_y_hit=true;
          step_events_completed.wide = current_block->step_event_count.wide;
        }
        old_y_max_endstop = y_max_endstop;
      #endif
    }

    if ((out_bits & (1<<Z_AXIS)) != 0) // -direction
    {
      #if defined(Z_MIN_PIN) && (Z_MIN_PIN > -1) && !defined(DEBUG_DISABLE_ZMINLIMIT)
      if (! check_z_endstop) {
        #ifdef TMC2130_SG_HOMING
          // Stall guard homing turned on
#ifdef TMC2130_STEALTH_Z
		  if ((tmc2130_mode == TMC2130_MODE_SILENT) && !(tmc2130_sg_homing_axes_mask & 0x04))
	          z_min_endstop = (READ(Z_MIN_PIN) != Z_MIN_ENDSTOP_INVERTING);
		  else
#endif //TMC2130_STEALTH_Z
	          z_min_endstop = (READ(Z_MIN_PIN) != Z_MIN_ENDSTOP_INVERTING) || (READ(Z_TMC2130_DIAG) != 0);
        #else
          z_min_endstop = (READ(Z_MIN_PIN) != Z_MIN_ENDSTOP_INVERTING);
        #endif //TMC2130_SG_HOMING
        if(z_min_endstop && old_z_min_endstop && (current_block->steps_z.wide > 0)) {
          endstops_trigsteps[Z_AXIS] = count_position[Z_AXIS];
          endstop_z_hit=true;
          step_events_completed.wide = current_block->step_event_count.wide;
        }
        old_z_min_endstop = z_min_endstop;
      }
      #endif
    } else { // +direction
      #if defined(Z_MAX_PIN) && (Z_MAX_PIN > -1) && !defined(DEBUG_DISABLE_ZMAXLIMIT)
        #ifdef TMC2130_SG_HOMING
        // Stall guard homing turned on
#ifdef TMC2130_STEALTH_Z
		  if ((tmc2130_mode == TMC2130_MODE_SILENT) && !(tmc2130_sg_homing_axes_mask & 0x04))
	          z_max_endstop = false;
		  else
#endif //TMC2130_STEALTH_Z
        z_max_endstop = (READ(Z_TMC2130_DIAG) != 0);
        #else
        z_max_endstop = (READ(Z_MAX_PIN) != Z_MAX_ENDSTOP_INVERTING);
        #endif //TMC2130_SG_HOMING
        if(z_max_endstop && old_z_max_endstop && (current_block->steps_z.wide > 0)) {
          endstops_trigsteps[Z_AXIS] = count_position[Z_AXIS];
          endstop_z_hit=true;
          step_events_completed.wide = current_block->step_event_count.wide;
        }
        old_z_max_endstop = z_max_endstop;
      #endif
    }
  }

  // Supporting stopping on a trigger of the Z-stop induction sensor, not only for the Z-minus movements.
  #if defined(Z_MIN_PIN) && (Z_MIN_PIN > -1) && !defined(DEBUG_DISABLE_ZMINLIMIT)
  if (check_z_endstop) {
      // Check the Z min end-stop no matter what.
      // Good for searching for the center of an induction target.
      #ifdef TMC2130_SG_HOMING
      // Stall guard homing turned on
#ifdef TMC2130_STEALTH_Z
		  if ((tmc2130_mode == TMC2130_MODE_SILENT) && !(tmc2130_sg_homing_axes_mask & 0x04))
	          z_min_endstop = (READ(Z_MIN_PIN) != Z_MIN_ENDSTOP_INVERTING);
		  else
#endif //TMC2130_STEALTH_Z
       z_min_endstop = (READ(Z_MIN_PIN) != Z_MIN_ENDSTOP_INVERTING) || (READ(Z_TMC2130_DIAG) != 0);
      #else
        z_min_endstop = (READ(Z_MIN_PIN) != Z_MIN_ENDSTOP_INVERTING);
      #endif //TMC2130_SG_HOMING
      if(z_min_endstop && old_z_min_endstop) {
        endstops_trigsteps[Z_AXIS] = count_position[Z_AXIS];
        endstop_z_hit=true;
        step_events_completed.wide = current_block->step_event_count.wide;
      }
      old_z_min_endstop = z_min_endstop;
  }
  #endif
}


FORCE_INLINE void stepper_tick_lowres()
{
  for (uint8_t i=0; i < step_loops; ++ i) { // Take multiple steps per interrupt (For high speed moves)
    MSerial.checkRx(); // Check for serial chars.
    // Step in X axis
    counter_x.lo += current_block->steps_x.lo;
    if (counter_x.lo > 0) {
      WRITE_NC(X_STEP_PIN, !INVERT_X_STEP_PIN);
#ifdef DEBUG_XSTEP_DUP_PIN
      WRITE_NC(DEBUG_XSTEP_DUP_PIN,!INVERT_X_STEP_PIN);
#endif //DEBUG_XSTEP_DUP_PIN
      counter_x.lo -= current_block->step_event_count.lo;
      count_position[X_AXIS]+=count_direction[X_AXIS];
      WRITE_NC(X_STEP_PIN, INVERT_X_STEP_PIN);
#ifdef DEBUG_XSTEP_DUP_PIN
      WRITE_NC(DEBUG_XSTEP_DUP_PIN,INVERT_X_STEP_PIN);
#endif //DEBUG_XSTEP_DUP_PIN
    }
    // Step in Y axis
    counter_y.lo += current_block->steps_y.lo;
    if (counter_y.lo > 0) {
      WRITE_NC(Y_STEP_PIN, !INVERT_Y_STEP_PIN);
#ifdef DEBUG_YSTEP_DUP_PIN
      WRITE_NC(DEBUG_YSTEP_DUP_PIN,!INVERT_Y_STEP_PIN);
#endif //DEBUG_YSTEP_DUP_PIN
      counter_y.lo -= current_block->step_event_count.lo;
      count_position[Y_AXIS]+=count_direction[Y_AXIS];
      WRITE_NC(Y_STEP_PIN, INVERT_Y_STEP_PIN);
#ifdef DEBUG_YSTEP_DUP_PIN
      WRITE_NC(DEBUG_YSTEP_DUP_PIN,INVERT_Y_STEP_PIN);
#endif //DEBUG_YSTEP_DUP_PIN    
    }
    // Step in Z axis
    counter_z.lo += current_block->steps_z.lo;
    if (counter_z.lo > 0) {
      WRITE_NC(Z_STEP_PIN, !INVERT_Z_STEP_PIN);
      counter_z.lo -= current_block->step_event_count.lo;
      count_position[Z_AXIS]+=count_direction[Z_AXIS];
      WRITE_NC(Z_STEP_PIN, INVERT_Z_STEP_PIN);
    }
    // Step in E axis
    counter_e.lo += current_block->steps_e.lo;
    if (counter_e.lo > 0) {
#ifndef LIN_ADVANCE
      WRITE(E0_STEP_PIN, !INVERT_E_STEP_PIN);
#endif /* LIN_ADVANCE */
      counter_e.lo -= current_block->step_event_count.lo;
      count_position[E_AXIS] += count_direction[E_AXIS];
#ifdef LIN_ADVANCE
      e_steps += count_direction[E_AXIS];
#else
	#ifdef FILAMENT_SENSOR
	  fsensor_counter += count_direction[E_AXIS];
	#endif //FILAMENT_SENSOR
      WRITE(E0_STEP_PIN, INVERT_E_STEP_PIN);
#endif
    }
    if(++ step_events_completed.lo >= current_block->step_event_count.lo)
      break;
  }
}

FORCE_INLINE void stepper_tick_highres()
{
  for (uint8_t i=0; i < step_loops; ++ i) { // Take multiple steps per interrupt (For high speed moves)
    MSerial.checkRx(); // Check for serial chars.
    // Step in X axis
    counter_x.wide += current_block->steps_x.wide;
    if (counter_x.wide > 0) {
      WRITE_NC(X_STEP_PIN, !INVERT_X_STEP_PIN);
#ifdef DEBUG_XSTEP_DUP_PIN
      WRITE_NC(DEBUG_XSTEP_DUP_PIN,!INVERT_X_STEP_PIN);
#endif //DEBUG_XSTEP_DUP_PIN
      counter_x.wide -= current_block->step_event_count.wide;
      count_position[X_AXIS]+=count_direction[X_AXIS];   
      WRITE_NC(X_STEP_PIN, INVERT_X_STEP_PIN);
#ifdef DEBUG_XSTEP_DUP_PIN
      WRITE_NC(DEBUG_XSTEP_DUP_PIN,INVERT_X_STEP_PIN);
#endif //DEBUG_XSTEP_DUP_PIN
    }
    // Step in Y axis
    counter_y.wide += current_block->steps_y.wide;
    if (counter_y.wide > 0) {
      WRITE_NC(Y_STEP_PIN, !INVERT_Y_STEP_PIN);
#ifdef DEBUG_YSTEP_DUP_PIN
      WRITE_NC(DEBUG_YSTEP_DUP_PIN,!INVERT_Y_STEP_PIN);
#endif //DEBUG_YSTEP_DUP_PIN
      counter_y.wide -= current_block->step_event_count.wide;
      count_position[Y_AXIS]+=count_direction[Y_AXIS];
      WRITE_NC(Y_STEP_PIN, INVERT_Y_STEP_PIN);
#ifdef DEBUG_YSTEP_DUP_PIN
      WRITE_NC(DEBUG_YSTEP_DUP_PIN,INVERT_Y_STEP_PIN);
#endif //DEBUG_YSTEP_DUP_PIN    
    }
    // Step in Z axis
    counter_z.wide += current_block->steps_z.wide;
    if (counter_z.wide > 0) {
      WRITE_NC(Z_STEP_PIN, !INVERT_Z_STEP_PIN);
      counter_z.wide -= current_block->step_event_count.wide;
      count_position[Z_AXIS]+=count_direction[Z_AXIS];
      WRITE_NC(Z_STEP_PIN, INVERT_Z_STEP_PIN);
    }
    // Step in E axis
    counter_e.wide += current_block->steps_e.wide;
    if (counter_e.wide > 0) {
#ifndef LIN_ADVANCE
      WRITE(E0_STEP_PIN, !INVERT_E_STEP_PIN);
#endif /* LIN_ADVANCE */
      counter_e.wide -= current_block->step_event_count.wide;
      count_position[E_AXIS]+=count_direction[E_AXIS];
#ifdef LIN_ADVANCE
      e_steps += count_direction[E_AXIS];
#else
    #ifdef FILAMENT_SENSOR
      fsensor_counter += count_direction[E_AXIS];
    #endif //FILAMENT_SENSOR
      WRITE(E0_STEP_PIN, INVERT_E_STEP_PIN);
#endif
    }
    if(++ step_events_completed.wide >= current_block->step_event_count.wide)
      break;
  }
}


#ifdef LIN_ADVANCE
// @wavexx: fast uint16_t division for small dividends<5
//          q/3 based on "Hacker's delight" formula
FORCE_INLINE uint16_t fastdiv(uint16_t q, uint8_t d)
{
    if(d != 3) return q >> (d / 2);
    else return ((uint32_t)0xAAAB * q) >> 17;
}

FORCE_INLINE void advance_spread(uint16_t timer)
{
    if(eISR_Err > timer)
    {
        // advance-step skipped
        eISR_Err -= timer;
        eISR_Rate = timer;
        nextAdvanceISR = timer;
        return;
    }

    // at least one step
    uint8_t ticks = 1;
    uint32_t block = current_block->advance_rate;
    uint16_t max_t = timer - eISR_Err;
    while (block < max_t)
    {
        ++ticks;
        block += current_block->advance_rate;
    }
    if (block > timer)
        eISR_Err += block - timer;
    else
        eISR_Err -= timer - block;

    if (ticks <= 4)
        eISR_Rate = fastdiv(timer, ticks);
    else
    {
        // >4 ticks are still possible on slow moves
        eISR_Rate = timer / ticks;
    }

    nextAdvanceISR = eISR_Rate / 2;
}
#endif


FORCE_INLINE void isr() {
  //WRITE_NC(LOGIC_ANALYZER_CH0, true);

	//if (UVLO) uvlo();
  // If there is no current block, attempt to pop one from the buffer
  if (current_block == NULL)
    stepper_next_block();

  if (current_block != NULL) 
  {
    stepper_check_endstops();
    if (current_block->flag & BLOCK_FLAG_DDA_LOWRES)
      stepper_tick_lowres();
    else
      stepper_tick_highres();


#ifdef LIN_ADVANCE
    if (e_steps) WRITE_NC(E0_DIR_PIN, e_steps < 0? INVERT_E0_DIR: !INVERT_E0_DIR);
    uint8_t la_state = 0;
#endif

    // Calculate new timer value
    // 13.38-14.63us for steady state,
    // 25.12us for acceleration / deceleration.
    {
      //WRITE_NC(LOGIC_ANALYZER_CH1, true);
      if (step_events_completed.wide <= (unsigned long int)current_block->accelerate_until) {
        // v = t * a   ->   acc_step_rate = acceleration_time * current_block->acceleration_rate
        MultiU24X24toH16(acc_step_rate, acceleration_time, current_block->acceleration_rate);
        acc_step_rate += uint16_t(current_block->initial_rate);
        // upper limit
        if(acc_step_rate > uint16_t(current_block->nominal_rate))
          acc_step_rate = current_block->nominal_rate;
        // step_rate to timer interval
        uint16_t timer = calc_timer(acc_step_rate, step_loops);
        _NEXT_ISR(timer);
        acceleration_time += timer;
#ifdef LIN_ADVANCE
        if (current_block->use_advance_lead) {
            if (step_events_completed.wide <= (unsigned long int)step_loops)
                la_state = ADV_INIT;
        }
#endif
      }
      else if (step_events_completed.wide > (unsigned long int)current_block->decelerate_after) {
        uint16_t step_rate;
        MultiU24X24toH16(step_rate, deceleration_time, current_block->acceleration_rate);
        step_rate = acc_step_rate - step_rate; // Decelerate from aceleration end point.
        if ((step_rate & 0x8000) || step_rate < uint16_t(current_block->final_rate)) {
          // Result is negative or too small.
          step_rate = uint16_t(current_block->final_rate);
        }
        // Step_rate to timer interval.
        uint16_t timer = calc_timer(step_rate, step_loops);
        _NEXT_ISR(timer);
        deceleration_time += timer;
#ifdef LIN_ADVANCE
        if (current_block->use_advance_lead) {
            la_state = ADV_DECELERATE;
            if (step_events_completed.wide <= (unsigned long int)current_block->decelerate_after + step_loops)
                la_state |= ADV_INIT;
        }
#endif
      }
      else {
        if (! step_loops_nominal) {
          // Calculation of the steady state timer rate has been delayed to the 1st tick of the steady state to lower
          // the initial interrupt blocking.
          OCR1A_nominal = calc_timer(uint16_t(current_block->nominal_rate), step_loops);
          step_loops_nominal = step_loops;
        }
        _NEXT_ISR(OCR1A_nominal);
      }
      //WRITE_NC(LOGIC_ANALYZER_CH1, false);
    }

#ifdef LIN_ADVANCE
    // avoid multiple instances or function calls to advance_spread
    if (la_state & ADV_INIT) eISR_Err = current_block->advance_rate / 4;
    if (la_state & ADV_INIT || nextAdvanceISR != ADV_NEVER) {
        advance_spread(main_Rate);
        if (la_state & ADV_DECELERATE) {
            if (step_loops == e_step_loops)
                LA_phase = (eISR_Rate > main_Rate);
            else {
                // avoid overflow through division. warning: we need to _guarantee_ step_loops
                // and e_step_loops are <= 4 due to fastdiv's limit
                LA_phase = (fastdiv(eISR_Rate, step_loops) > fastdiv(main_Rate, e_step_loops));
            }
        }
    }

    // Check for serial chars. This executes roughtly inbetween 50-60% of the total runtime of the
    // entire isr, making this spot a much better choice than checking during esteps
    MSerial.checkRx();
#endif

    // If current block is finished, reset pointer
    if (step_events_completed.wide >= current_block->step_event_count.wide) {
#if !defined(LIN_ADVANCE) && defined(FILAMENT_SENSOR)
		fsensor_st_block_chunk(fsensor_counter);
		fsensor_counter = 0;
#endif //FILAMENT_SENSOR

      current_block = NULL;
      plan_discard_current_block();
    }
#if !defined(LIN_ADVANCE) && defined(FILAMENT_SENSOR)
	else if ((abs(fsensor_counter) >= fsensor_chunk_len))
  	{
      fsensor_st_block_chunk(fsensor_counter);
  	  fsensor_counter = 0;
  	}
#endif //FILAMENT_SENSOR
  }

#ifdef TMC2130
	tmc2130_st_isr();
#endif //TMC2130

  //WRITE_NC(LOGIC_ANALYZER_CH0, false);
}

#ifdef LIN_ADVANCE
// Timer interrupt for E. e_steps is set in the main routine.

FORCE_INLINE void advance_isr() {
    if (step_events_completed.wide > LA_decelerate_after && current_adv_steps > final_adv_steps) {
        // decompression
        e_steps -= e_step_loops;
        if (e_steps) WRITE_NC(E0_DIR_PIN, e_steps < 0? INVERT_E0_DIR: !INVERT_E0_DIR);
        if(current_adv_steps > e_step_loops)
            current_adv_steps -= e_step_loops;
        else
            current_adv_steps = 0;
        nextAdvanceISR = eISR_Rate;
    }
    else if (step_events_completed.wide < LA_decelerate_after && current_adv_steps < max_adv_steps) {
        // compression
        e_steps += e_step_loops;
        if (e_steps) WRITE_NC(E0_DIR_PIN, e_steps < 0? INVERT_E0_DIR: !INVERT_E0_DIR);
        current_adv_steps += e_step_loops;
        nextAdvanceISR = eISR_Rate;
    }
    else {
        // advance steps completed
        nextAdvanceISR = ADV_NEVER;
        LA_phase = -1;
        e_step_loops = 1;
    }
}

FORCE_INLINE void advance_isr_scheduler() {
    // Integrate the final timer value, accounting for scheduling adjustments
    if(nextAdvanceISR && nextAdvanceISR != ADV_NEVER)
    {
        if(nextAdvanceISR > OCR1A)
            nextAdvanceISR -= OCR1A;
        else
            nextAdvanceISR = 0;
    }
    if(nextMainISR > OCR1A)
        nextMainISR -= OCR1A;
    else
        nextMainISR = 0;

    // Run main stepping ISR if flagged
    if (!nextMainISR)
    {
#ifdef LA_DEBUG_LOGIC
        WRITE_NC(LOGIC_ANALYZER_CH0, true);
#endif
        isr();
#ifdef LA_DEBUG_LOGIC
        WRITE_NC(LOGIC_ANALYZER_CH0, false);
#endif
    }

    // Run the next advance isr if triggered
    bool eisr = !nextAdvanceISR;
    if (eisr)
    {
#ifdef LA_DEBUG_LOGIC
        WRITE_NC(LOGIC_ANALYZER_CH1, true);
#endif
        advance_isr();
#ifdef LA_DEBUG_LOGIC
        WRITE_NC(LOGIC_ANALYZER_CH1, false);
#endif
    }

    // Tick E steps if any
    if (e_steps && (LA_phase < 0 || LA_phase == eisr)) {
        uint8_t max_ticks = (eisr? e_step_loops: step_loops);
        max_ticks = min(abs(e_steps), max_ticks);
        bool rev = (e_steps < 0);
        do
        {
            WRITE_NC(E0_STEP_PIN, !INVERT_E_STEP_PIN);
            e_steps += (rev? 1: -1);
            WRITE_NC(E0_STEP_PIN, INVERT_E_STEP_PIN);
#if defined(FILAMENT_SENSOR) && defined(PAT9125)
            fsensor_counter += (rev? -1: 1);
#endif
        }
        while(--max_ticks);

#if defined(FILAMENT_SENSOR) && defined(PAT9125)
        if (abs(fsensor_counter) >= fsensor_chunk_len)
        {
            fsensor_st_block_chunk(fsensor_counter);
            fsensor_counter = 0;
        }
#endif
    }

    // Schedule the next closest tick, ignoring advance if scheduled too
    // soon in order to avoid skewing the regular stepper acceleration
    if (nextAdvanceISR != ADV_NEVER && (nextAdvanceISR + TCNT1 + 40) < nextMainISR)
        OCR1A = nextAdvanceISR;
    else
        OCR1A = nextMainISR;
}
#endif // LIN_ADVANCE

void st_init()
{
#ifdef TMC2130
	tmc2130_init();
#endif //TMC2130

  st_current_init(); //Initialize Digipot Motor Current
  microstep_init(); //Initialize Microstepping Pins

  //Initialize Dir Pins
  #if defined(X_DIR_PIN) && X_DIR_PIN > -1
    SET_OUTPUT(X_DIR_PIN);
  #endif
  #if defined(X2_DIR_PIN) && X2_DIR_PIN > -1
    SET_OUTPUT(X2_DIR_PIN);
  #endif
  #if defined(Y_DIR_PIN) && Y_DIR_PIN > -1
    SET_OUTPUT(Y_DIR_PIN);
		
	#if defined(Y_DUAL_STEPPER_DRIVERS) && defined(Y2_DIR_PIN) && (Y2_DIR_PIN > -1)
	  SET_OUTPUT(Y2_DIR_PIN);
	#endif
  #endif
  #if defined(Z_DIR_PIN) && Z_DIR_PIN > -1
    SET_OUTPUT(Z_DIR_PIN);

    #if defined(Z_DUAL_STEPPER_DRIVERS) && defined(Z2_DIR_PIN) && (Z2_DIR_PIN > -1)
      SET_OUTPUT(Z2_DIR_PIN);
    #endif
  #endif
  #if defined(E0_DIR_PIN) && E0_DIR_PIN > -1
    SET_OUTPUT(E0_DIR_PIN);
  #endif
  #if defined(E1_DIR_PIN) && (E1_DIR_PIN > -1)
    SET_OUTPUT(E1_DIR_PIN);
  #endif
  #if defined(E2_DIR_PIN) && (E2_DIR_PIN > -1)
    SET_OUTPUT(E2_DIR_PIN);
  #endif

  //Initialize Enable Pins - steppers default to disabled.

  #if defined(X_ENABLE_PIN) && X_ENABLE_PIN > -1
    SET_OUTPUT(X_ENABLE_PIN);
    if(!X_ENABLE_ON) WRITE(X_ENABLE_PIN,HIGH);
  #endif
  #if defined(X2_ENABLE_PIN) && X2_ENABLE_PIN > -1
    SET_OUTPUT(X2_ENABLE_PIN);
    if(!X_ENABLE_ON) WRITE(X2_ENABLE_PIN,HIGH);
  #endif
  #if defined(Y_ENABLE_PIN) && Y_ENABLE_PIN > -1
    SET_OUTPUT(Y_ENABLE_PIN);
    if(!Y_ENABLE_ON) WRITE(Y_ENABLE_PIN,HIGH);
	
	#if defined(Y_DUAL_STEPPER_DRIVERS) && defined(Y2_ENABLE_PIN) && (Y2_ENABLE_PIN > -1)
	  SET_OUTPUT(Y2_ENABLE_PIN);
	  if(!Y_ENABLE_ON) WRITE(Y2_ENABLE_PIN,HIGH);
	#endif
  #endif
  #if defined(Z_ENABLE_PIN) && Z_ENABLE_PIN > -1
    SET_OUTPUT(Z_ENABLE_PIN);
    if(!Z_ENABLE_ON) WRITE(Z_ENABLE_PIN,HIGH);

    #if defined(Z_DUAL_STEPPER_DRIVERS) && defined(Z2_ENABLE_PIN) && (Z2_ENABLE_PIN > -1)
      SET_OUTPUT(Z2_ENABLE_PIN);
      if(!Z_ENABLE_ON) WRITE(Z2_ENABLE_PIN,HIGH);
    #endif
  #endif
  #if defined(E0_ENABLE_PIN) && (E0_ENABLE_PIN > -1)
    SET_OUTPUT(E0_ENABLE_PIN);
    if(!E_ENABLE_ON) WRITE(E0_ENABLE_PIN,HIGH);
  #endif
  #if defined(E1_ENABLE_PIN) && (E1_ENABLE_PIN > -1)
    SET_OUTPUT(E1_ENABLE_PIN);
    if(!E_ENABLE_ON) WRITE(E1_ENABLE_PIN,HIGH);
  #endif
  #if defined(E2_ENABLE_PIN) && (E2_ENABLE_PIN > -1)
    SET_OUTPUT(E2_ENABLE_PIN);
    if(!E_ENABLE_ON) WRITE(E2_ENABLE_PIN,HIGH);
  #endif

  //endstops and pullups

  #ifdef TMC2130_SG_HOMING
    SET_INPUT(X_TMC2130_DIAG);
    WRITE(X_TMC2130_DIAG,HIGH);
    
    SET_INPUT(Y_TMC2130_DIAG);
    WRITE(Y_TMC2130_DIAG,HIGH);
    
    SET_INPUT(Z_TMC2130_DIAG);
    WRITE(Z_TMC2130_DIAG,HIGH);

	SET_INPUT(E0_TMC2130_DIAG);
    WRITE(E0_TMC2130_DIAG,HIGH);
    
  #endif
    
  #if defined(X_MIN_PIN) && X_MIN_PIN > -1
    SET_INPUT(X_MIN_PIN);
    #ifdef ENDSTOPPULLUP_XMIN
      WRITE(X_MIN_PIN,HIGH);
    #endif
  #endif

  #if defined(Y_MIN_PIN) && Y_MIN_PIN > -1
    SET_INPUT(Y_MIN_PIN);
    #ifdef ENDSTOPPULLUP_YMIN
      WRITE(Y_MIN_PIN,HIGH);
    #endif
  #endif

  #if defined(Z_MIN_PIN) && Z_MIN_PIN > -1
    SET_INPUT(Z_MIN_PIN);
    #ifdef ENDSTOPPULLUP_ZMIN
      WRITE(Z_MIN_PIN,HIGH);
    #endif
  #endif

  #if defined(X_MAX_PIN) && X_MAX_PIN > -1
    SET_INPUT(X_MAX_PIN);
    #ifdef ENDSTOPPULLUP_XMAX
      WRITE(X_MAX_PIN,HIGH);
    #endif
  #endif

  #if defined(Y_MAX_PIN) && Y_MAX_PIN > -1
    SET_INPUT(Y_MAX_PIN);
    #ifdef ENDSTOPPULLUP_YMAX
      WRITE(Y_MAX_PIN,HIGH);
    #endif
  #endif

  #if defined(Z_MAX_PIN) && Z_MAX_PIN > -1
    SET_INPUT(Z_MAX_PIN);
    #ifdef ENDSTOPPULLUP_ZMAX
      WRITE(Z_MAX_PIN,HIGH);
    #endif
  #endif

  #if (defined(FANCHECK) && defined(TACH_0) && (TACH_0 > -1))
	SET_INPUT(TACH_0);
    #ifdef TACH0PULLUP
	  WRITE(TACH_0, HIGH);
    #endif
  #endif


  //Initialize Step Pins
#if defined(X_STEP_PIN) && (X_STEP_PIN > -1)
    SET_OUTPUT(X_STEP_PIN);
    WRITE(X_STEP_PIN,INVERT_X_STEP_PIN);
#ifdef DEBUG_XSTEP_DUP_PIN
    SET_OUTPUT(DEBUG_XSTEP_DUP_PIN);
    WRITE(DEBUG_XSTEP_DUP_PIN,INVERT_X_STEP_PIN);
#endif //DEBUG_XSTEP_DUP_PIN
    disable_x();
  #endif
  #if defined(X2_STEP_PIN) && (X2_STEP_PIN > -1)
    SET_OUTPUT(X2_STEP_PIN);
    WRITE(X2_STEP_PIN,INVERT_X_STEP_PIN);
    disable_x();
  #endif
  #if defined(Y_STEP_PIN) && (Y_STEP_PIN > -1)
    SET_OUTPUT(Y_STEP_PIN);
    WRITE(Y_STEP_PIN,INVERT_Y_STEP_PIN);
#ifdef DEBUG_YSTEP_DUP_PIN
    SET_OUTPUT(DEBUG_YSTEP_DUP_PIN);
    WRITE(DEBUG_YSTEP_DUP_PIN,INVERT_Y_STEP_PIN);
#endif //DEBUG_YSTEP_DUP_PIN
    #if defined(Y_DUAL_STEPPER_DRIVERS) && defined(Y2_STEP_PIN) && (Y2_STEP_PIN > -1)
      SET_OUTPUT(Y2_STEP_PIN);
      WRITE(Y2_STEP_PIN,INVERT_Y_STEP_PIN);
    #endif
    disable_y();
  #endif
  #if defined(Z_STEP_PIN) && (Z_STEP_PIN > -1)
    SET_OUTPUT(Z_STEP_PIN);
    WRITE(Z_STEP_PIN,INVERT_Z_STEP_PIN);
    #if defined(Z_DUAL_STEPPER_DRIVERS) && defined(Z2_STEP_PIN) && (Z2_STEP_PIN > -1)
      SET_OUTPUT(Z2_STEP_PIN);
      WRITE(Z2_STEP_PIN,INVERT_Z_STEP_PIN);
    #endif
    #ifdef PSU_Delta
      init_force_z();
    #endif // PSU_Delta
    disable_z();
  #endif
  #if defined(E0_STEP_PIN) && (E0_STEP_PIN > -1)
    SET_OUTPUT(E0_STEP_PIN);
    WRITE(E0_STEP_PIN,INVERT_E_STEP_PIN);
    disable_e0();
  #endif
  #if defined(E1_STEP_PIN) && (E1_STEP_PIN > -1)
    SET_OUTPUT(E1_STEP_PIN);
    WRITE(E1_STEP_PIN,INVERT_E_STEP_PIN);
    disable_e1();
  #endif
  #if defined(E2_STEP_PIN) && (E2_STEP_PIN > -1)
    SET_OUTPUT(E2_STEP_PIN);
    WRITE(E2_STEP_PIN,INVERT_E_STEP_PIN);
    disable_e2();
  #endif

  // waveform generation = 0100 = CTC
  TCCR1B &= ~(1<<WGM13);
  TCCR1B |=  (1<<WGM12);
  TCCR1A &= ~(1<<WGM11);
  TCCR1A &= ~(1<<WGM10);

  // output mode = 00 (disconnected)
  TCCR1A &= ~(3<<COM1A0);
  TCCR1A &= ~(3<<COM1B0);

  // Set the timer pre-scaler
  // Generally we use a divider of 8, resulting in a 2MHz timer
  // frequency on a 16MHz MCU. If you are going to change this, be
  // sure to regenerate speed_lookuptable.h with
  // create_speed_lookuptable.py
  TCCR1B = (TCCR1B & ~(0x07<<CS10)) | (2<<CS10);

  // Plan the first interrupt after 8ms from now.
  OCR1A = 0x4000;
  TCNT1 = 0;

#ifdef LIN_ADVANCE
#ifdef LA_DEBUG_LOGIC
  LOGIC_ANALYZER_CH0_ENABLE;
  LOGIC_ANALYZER_CH1_ENABLE;
  WRITE_NC(LOGIC_ANALYZER_CH0, false);
  WRITE_NC(LOGIC_ANALYZER_CH1, false);
#endif

  // Initialize state for the linear advance scheduler
  nextMainISR = 0;
  nextAdvanceISR = ADV_NEVER;
  main_Rate = ADV_NEVER;
  e_steps = 0;
  e_step_loops = 1;
  LA_phase = -1;
  current_adv_steps = 0;
#endif

  enable_endstops(true); // Start with endstops active. After homing they can be disabled

  ENABLE_STEPPER_DRIVER_INTERRUPT();
  sei();
}


void st_reset_timer()
{
  // Clear a possible pending interrupt on OCR1A overflow.
  TIFR1 |= 1 << OCF1A;
  // Reset the counter.
  TCNT1 = 0;
  // Wake up after 1ms from now.
  OCR1A = 2000;

#ifdef LIN_ADVANCE
  nextMainISR = 0;
  if(nextAdvanceISR && nextAdvanceISR != ADV_NEVER)
      nextAdvanceISR = 0;
#endif
}


// Block until all buffered steps are executed
void st_synchronize()
{
	while(blocks_queued())
	{
#ifdef TMC2130
		manage_heater();
		// Vojtech: Don't disable motors inside the planner!
		if (!tmc2130_update_sg())
		{
			manage_inactivity(true);
			lcd_update(0);
		}
#else //TMC2130
		manage_heater();
		// Vojtech: Don't disable motors inside the planner!
		manage_inactivity(true);
		lcd_update(0);
#endif //TMC2130
	}
}

void st_set_position(const long &x, const long &y, const long &z, const long &e)
{
  CRITICAL_SECTION_START;
  // Copy 4x4B.
  // This block locks the interrupts globally for 4.56 us,
  // which corresponds to a maximum repeat frequency of 219.18 kHz.
  // This blocking is safe in the context of a 10kHz stepper driver interrupt
  // or a 115200 Bd serial line receive interrupt, which will not trigger faster than 12kHz.
  count_position[X_AXIS] = x;
  count_position[Y_AXIS] = y;
  count_position[Z_AXIS] = z;
  count_position[E_AXIS] = e;
  CRITICAL_SECTION_END;
}

void st_set_e_position(const long &e)
{
  CRITICAL_SECTION_START;
  count_position[E_AXIS] = e;
  CRITICAL_SECTION_END;
}

long st_get_position(uint8_t axis)
{
  long count_pos;
  CRITICAL_SECTION_START;
  count_pos = count_position[axis];
  CRITICAL_SECTION_END;
  return count_pos;
}

void st_get_position_xy(long &x, long &y)
{
  CRITICAL_SECTION_START;
  x = count_position[X_AXIS];
  y = count_position[Y_AXIS];
  CRITICAL_SECTION_END;
}

float st_get_position_mm(uint8_t axis)
{
  float steper_position_in_steps = st_get_position(axis);
  return steper_position_in_steps / cs.axis_steps_per_unit[axis];
}


void finishAndDisableSteppers()
{
  st_synchronize();
  disable_x();
  disable_y();
  disable_z();
  disable_e0();
  disable_e1();
  disable_e2();
}

void quickStop()
{
  DISABLE_STEPPER_DRIVER_INTERRUPT();
  while (blocks_queued()) plan_discard_current_block(); 
  current_block = NULL;
#ifdef LIN_ADVANCE
  nextAdvanceISR = ADV_NEVER;
  current_adv_steps = 0;
#endif
  st_reset_timer();
  ENABLE_STEPPER_DRIVER_INTERRUPT();
}

#ifdef BABYSTEPPING
void babystep(const uint8_t axis,const bool direction)
{
  //MUST ONLY BE CALLED BY A ISR, it depends on that no other ISR interrupts this
    //store initial pin states
  switch(axis)
  {
  case X_AXIS:
  {
    enable_x();   
    uint8_t old_x_dir_pin= READ(X_DIR_PIN);  //if dualzstepper, both point to same direction.
   
    //setup new step
    WRITE(X_DIR_PIN,(INVERT_X_DIR)^direction);
    
    //perform step 
    WRITE(X_STEP_PIN, !INVERT_X_STEP_PIN); 
#ifdef DEBUG_XSTEP_DUP_PIN
    WRITE(DEBUG_XSTEP_DUP_PIN,!INVERT_X_STEP_PIN);
#endif //DEBUG_XSTEP_DUP_PIN
    delayMicroseconds(1);
    WRITE(X_STEP_PIN, INVERT_X_STEP_PIN);
#ifdef DEBUG_XSTEP_DUP_PIN
    WRITE(DEBUG_XSTEP_DUP_PIN,INVERT_X_STEP_PIN);
#endif //DEBUG_XSTEP_DUP_PIN

    //get old pin state back.
    WRITE(X_DIR_PIN,old_x_dir_pin);
  }
  break;
  case Y_AXIS:
  {
    enable_y();   
    uint8_t old_y_dir_pin= READ(Y_DIR_PIN);  //if dualzstepper, both point to same direction.
   
    //setup new step
    WRITE(Y_DIR_PIN,(INVERT_Y_DIR)^direction);
    
    //perform step 
    WRITE(Y_STEP_PIN, !INVERT_Y_STEP_PIN); 
#ifdef DEBUG_YSTEP_DUP_PIN
    WRITE(DEBUG_YSTEP_DUP_PIN,!INVERT_Y_STEP_PIN);
#endif //DEBUG_YSTEP_DUP_PIN
    delayMicroseconds(1);
    WRITE(Y_STEP_PIN, INVERT_Y_STEP_PIN);
#ifdef DEBUG_YSTEP_DUP_PIN
    WRITE(DEBUG_YSTEP_DUP_PIN,INVERT_Y_STEP_PIN);
#endif //DEBUG_YSTEP_DUP_PIN

    //get old pin state back.
    WRITE(Y_DIR_PIN,old_y_dir_pin);

  }
  break;
 
  case Z_AXIS:
  {
    enable_z();
    uint8_t old_z_dir_pin= READ(Z_DIR_PIN);  //if dualzstepper, both point to same direction.
    //setup new step
    WRITE(Z_DIR_PIN,(INVERT_Z_DIR)^direction^BABYSTEP_INVERT_Z);
    #ifdef Z_DUAL_STEPPER_DRIVERS
      WRITE(Z2_DIR_PIN,(INVERT_Z_DIR)^direction^BABYSTEP_INVERT_Z);
    #endif
    //perform step 
    WRITE(Z_STEP_PIN, !INVERT_Z_STEP_PIN); 
    #ifdef Z_DUAL_STEPPER_DRIVERS
      WRITE(Z2_STEP_PIN, !INVERT_Z_STEP_PIN);
    #endif
    delayMicroseconds(1);
    WRITE(Z_STEP_PIN, INVERT_Z_STEP_PIN);
    #ifdef Z_DUAL_STEPPER_DRIVERS
      WRITE(Z2_STEP_PIN, INVERT_Z_STEP_PIN);
    #endif

    //get old pin state back.
    WRITE(Z_DIR_PIN,old_z_dir_pin);
    #ifdef Z_DUAL_STEPPER_DRIVERS
      WRITE(Z2_DIR_PIN,old_z_dir_pin);
    #endif

  }
  break;
 
  default:    break;
  }
}
#endif //BABYSTEPPING

#if defined(DIGIPOTSS_PIN) && DIGIPOTSS_PIN > -1
void digitalPotWrite(int address, int value) // From Arduino DigitalPotControl example
{
    digitalWrite(DIGIPOTSS_PIN,LOW); // take the SS pin low to select the chip
    SPI.transfer(address); //  send in the address and value via SPI:
    SPI.transfer(value);
    digitalWrite(DIGIPOTSS_PIN,HIGH); // take the SS pin high to de-select the chip:
    //_delay(10);
}
#endif

void EEPROM_read_st(int pos, uint8_t* value, uint8_t size)
{
    do
    {
        *value = eeprom_read_byte((unsigned char*)pos);
        pos++;
        value++;
    }while(--size);
}


void st_current_init() //Initialize Digipot Motor Current
{
#ifdef MOTOR_CURRENT_PWM_XY_PIN
  uint8_t SilentMode = eeprom_read_byte((uint8_t*)EEPROM_SILENT);
  SilentModeMenu = SilentMode;
    pinMode(MOTOR_CURRENT_PWM_XY_PIN, OUTPUT);
    pinMode(MOTOR_CURRENT_PWM_Z_PIN, OUTPUT);
    pinMode(MOTOR_CURRENT_PWM_E_PIN, OUTPUT);
    if((SilentMode == SILENT_MODE_OFF) || (farm_mode) ){

     motor_current_setting[0] = motor_current_setting_loud[0];
     motor_current_setting[1] = motor_current_setting_loud[1];
     motor_current_setting[2] = motor_current_setting_loud[2];

    }else{

     motor_current_setting[0] = motor_current_setting_silent[0];
     motor_current_setting[1] = motor_current_setting_silent[1];
     motor_current_setting[2] = motor_current_setting_silent[2];

    }
    st_current_set(0, motor_current_setting[0]);
    st_current_set(1, motor_current_setting[1]);
    st_current_set(2, motor_current_setting[2]);
    //Set timer5 to 31khz so the PWM of the motor power is as constant as possible. (removes a buzzing noise)
    TCCR5B = (TCCR5B & ~(_BV(CS50) | _BV(CS51) | _BV(CS52))) | _BV(CS50);
#endif
}



#ifdef MOTOR_CURRENT_PWM_XY_PIN
void st_current_set(uint8_t driver, int current)
{
  if (driver == 0) analogWrite(MOTOR_CURRENT_PWM_XY_PIN, (long)current * 255L / (long)MOTOR_CURRENT_PWM_RANGE);
  if (driver == 1) analogWrite(MOTOR_CURRENT_PWM_Z_PIN, (long)current * 255L / (long)MOTOR_CURRENT_PWM_RANGE);
  if (driver == 2) analogWrite(MOTOR_CURRENT_PWM_E_PIN, (long)current * 255L / (long)MOTOR_CURRENT_PWM_RANGE);
}
#else //MOTOR_CURRENT_PWM_XY_PIN
void st_current_set(uint8_t, int ){}
#endif //MOTOR_CURRENT_PWM_XY_PIN

void microstep_init()
{

  #if defined(E1_MS1_PIN) && E1_MS1_PIN > -1
  pinMode(E1_MS1_PIN,OUTPUT);
  pinMode(E1_MS2_PIN,OUTPUT); 
  #endif

  #if defined(X_MS1_PIN) && X_MS1_PIN > -1
  const uint8_t microstep_modes[] = MICROSTEP_MODES;
  pinMode(X_MS1_PIN,OUTPUT);
  pinMode(X_MS2_PIN,OUTPUT);  
  pinMode(Y_MS1_PIN,OUTPUT);
  pinMode(Y_MS2_PIN,OUTPUT);
  pinMode(Z_MS1_PIN,OUTPUT);
  pinMode(Z_MS2_PIN,OUTPUT);
  pinMode(E0_MS1_PIN,OUTPUT);
  pinMode(E0_MS2_PIN,OUTPUT);
  for(int i=0;i<=4;i++) microstep_mode(i,microstep_modes[i]);
  #endif
}


#ifndef TMC2130

void microstep_ms(uint8_t driver, int8_t ms1, int8_t ms2)
{
  if(ms1 > -1) switch(driver)
  {
    case 0: digitalWrite( X_MS1_PIN,ms1); break;
    case 1: digitalWrite( Y_MS1_PIN,ms1); break;
    case 2: digitalWrite( Z_MS1_PIN,ms1); break;
    case 3: digitalWrite(E0_MS1_PIN,ms1); break;
    #if defined(E1_MS1_PIN) && E1_MS1_PIN > -1
    case 4: digitalWrite(E1_MS1_PIN,ms1); break;
    #endif
  }
  if(ms2 > -1) switch(driver)
  {
    case 0: digitalWrite( X_MS2_PIN,ms2); break;
    case 1: digitalWrite( Y_MS2_PIN,ms2); break;
    case 2: digitalWrite( Z_MS2_PIN,ms2); break;
    case 3: digitalWrite(E0_MS2_PIN,ms2); break;
    #if defined(E1_MS2_PIN) && E1_MS2_PIN > -1
    case 4: digitalWrite(E1_MS2_PIN,ms2); break;
    #endif
  }
}

void microstep_mode(uint8_t driver, uint8_t stepping_mode)
{
  switch(stepping_mode)
  {
    case 1: microstep_ms(driver,MICROSTEP1); break;
    case 2: microstep_ms(driver,MICROSTEP2); break;
    case 4: microstep_ms(driver,MICROSTEP4); break;
    case 8: microstep_ms(driver,MICROSTEP8); break;
    case 16: microstep_ms(driver,MICROSTEP16); break;
  }
}

void microstep_readings()
{
      SERIAL_PROTOCOLPGM("MS1,MS2 Pins\n");
      SERIAL_PROTOCOLPGM("X: ");
      SERIAL_PROTOCOL(   digitalRead(X_MS1_PIN));
      SERIAL_PROTOCOLLN( digitalRead(X_MS2_PIN));
      SERIAL_PROTOCOLPGM("Y: ");
      SERIAL_PROTOCOL(   digitalRead(Y_MS1_PIN));
      SERIAL_PROTOCOLLN( digitalRead(Y_MS2_PIN));
      SERIAL_PROTOCOLPGM("Z: ");
      SERIAL_PROTOCOL(   digitalRead(Z_MS1_PIN));
      SERIAL_PROTOCOLLN( digitalRead(Z_MS2_PIN));
      SERIAL_PROTOCOLPGM("E0: ");
      SERIAL_PROTOCOL(   digitalRead(E0_MS1_PIN));
      SERIAL_PROTOCOLLN( digitalRead(E0_MS2_PIN));
      #if defined(E1_MS1_PIN) && E1_MS1_PIN > -1
      SERIAL_PROTOCOLPGM("E1: ");
      SERIAL_PROTOCOL(   digitalRead(E1_MS1_PIN));
      SERIAL_PROTOCOLLN( digitalRead(E1_MS2_PIN));
      #endif
}
#endif //TMC2130


#if defined(FILAMENT_SENSOR) && defined(PAT9125)
void st_reset_fsensor()
{
    CRITICAL_SECTION_START;
    fsensor_counter = 0;
    CRITICAL_SECTION_END;
}
#endif //FILAMENT_SENSOR
