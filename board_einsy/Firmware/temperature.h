/*
  temperature.h - temperature controller
  Part of Marlin

  Copyright (c) 2011 Erik van der Zalm

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

#ifndef temperature_h
#define temperature_h 

#include "Marlin.h"
#include "planner.h"
#ifdef PID_ADD_EXTRUSION_RATE
  #include "stepper.h"
#endif

#include "config.h"


#ifdef SYSTEM_TIMER_2

#define ENABLE_TEMPERATURE_INTERRUPT()  TIMSK2 |= (1<<OCIE2B)
#define DISABLE_TEMPERATURE_INTERRUPT() TIMSK2 &= ~(1<<OCIE2B)

#else //SYSTEM_TIMER_2

#define ENABLE_TEMPERATURE_INTERRUPT()  TIMSK0 |= (1<<OCIE0B)
#define DISABLE_TEMPERATURE_INTERRUPT() TIMSK0 &= ~(1<<OCIE0B)

#endif //SYSTEM_TIMER_2


// public functions
void tp_init();  //initialize the heating
void manage_heater(); //it is critical that this is called periodically.

// low level conversion routines
// do not use these routines and variables outside of temperature.cpp
extern int target_temperature[EXTRUDERS];  
extern float current_temperature[EXTRUDERS];
#ifdef SHOW_TEMP_ADC_VALUES
  extern int current_temperature_raw[EXTRUDERS];
  extern int current_temperature_bed_raw;
#endif
extern int target_temperature_bed;
extern float current_temperature_bed;

#ifdef PINDA_THERMISTOR
extern uint16_t current_temperature_raw_pinda;
extern float current_temperature_pinda;
#endif

#ifdef AMBIENT_THERMISTOR
//extern int current_temperature_raw_ambient;
extern float current_temperature_ambient;
#endif

#ifdef VOLT_PWR_PIN
extern int current_voltage_raw_pwr;
#endif

#ifdef VOLT_BED_PIN
extern int current_voltage_raw_bed;
#endif

#if IR_SENSOR_ANALOG
extern int current_voltage_raw_IR;
#endif //IR_SENSOR_ANALOG

#if defined(CONTROLLERFAN_PIN) && CONTROLLERFAN_PIN > -1
  extern unsigned char soft_pwm_bed;
#endif

extern bool bedPWMDisabled;

#ifdef PIDTEMP
  extern int pid_cycle, pid_number_of_cycles;
  extern float Kc,_Kp,_Ki,_Kd;
  extern bool pid_tuning_finished;
  float scalePID_i(float i);
  float scalePID_d(float d);
  float unscalePID_i(float i);
  float unscalePID_d(float d);

#endif
  
  
#ifdef BABYSTEPPING
  extern volatile int babystepsTodo[3];
#endif

void resetPID(uint8_t extruder);

inline void babystepsTodoZadd(int n)
{
    if (n != 0) {
        CRITICAL_SECTION_START
        babystepsTodo[Z_AXIS] += n;
        CRITICAL_SECTION_END
    }
}

inline void babystepsTodoZsubtract(int n)
{
    if (n != 0) {
        CRITICAL_SECTION_START
        babystepsTodo[Z_AXIS] -= n;
        CRITICAL_SECTION_END
    }
}

//high level conversion routines, for use outside of temperature.cpp
//inline so that there is no performance decrease.
//deg=degreeCelsius

// Doesn't save FLASH when FORCE_INLINE removed.
FORCE_INLINE float degHotend(uint8_t extruder) {  
  return current_temperature[extruder];
};

#ifdef SHOW_TEMP_ADC_VALUES
  FORCE_INLINE float rawHotendTemp(uint8_t extruder) {  
    return current_temperature_raw[extruder];
  };

  FORCE_INLINE float rawBedTemp() {  
    return current_temperature_bed_raw;
  };
#endif

FORCE_INLINE float degBed() {
  return current_temperature_bed;
};

// Doesn't save FLASH when FORCE_INLINE removed.
FORCE_INLINE float degTargetHotend(uint8_t extruder) {  
  return target_temperature[extruder];
};

FORCE_INLINE float degTargetBed() {   
  return target_temperature_bed;
};

// Doesn't save FLASH when FORCE_INLINE removed.
FORCE_INLINE void setTargetHotend(const float &celsius, uint8_t extruder) {  
  target_temperature[extruder] = celsius;
  resetPID(extruder);
};

// Doesn't save FLASH when not inlined.
static inline void setTargetHotendSafe(const float &celsius, uint8_t extruder)
{
    if (extruder<EXTRUDERS) {
      target_temperature[extruder] = celsius;
      resetPID(extruder);
    }
}

// Doesn't save FLASH when not inlined.
static inline void setAllTargetHotends(const float &celsius)
{
    for(int i=0;i<EXTRUDERS;i++) setTargetHotend(celsius,i);
}

FORCE_INLINE void setTargetBed(const float &celsius) {  
  target_temperature_bed = celsius;
};

FORCE_INLINE bool isHeatingHotend(uint8_t extruder){  
  return target_temperature[extruder] > current_temperature[extruder];
};

FORCE_INLINE bool isHeatingBed() {
  return target_temperature_bed > current_temperature_bed;
};

FORCE_INLINE bool isCoolingHotend(uint8_t extruder) {  
  return target_temperature[extruder] < current_temperature[extruder];
};

FORCE_INLINE bool isCoolingBed() {
  return target_temperature_bed < current_temperature_bed;
};

#define degHotend0() degHotend(0)
#define degTargetHotend0() degTargetHotend(0)
#define setTargetHotend0(_celsius) setTargetHotend((_celsius), 0)
#define isHeatingHotend0() isHeatingHotend(0)
#define isCoolingHotend0() isCoolingHotend(0)
#if EXTRUDERS > 1
#define degHotend1() degHotend(1)
#define degTargetHotend1() degTargetHotend(1)
#define setTargetHotend1(_celsius) setTargetHotend((_celsius), 1)
#define isHeatingHotend1() isHeatingHotend(1)
#define isCoolingHotend1() isCoolingHotend(1)
#else
#define setTargetHotend1(_celsius) do{}while(0)
#endif
#if EXTRUDERS > 2
#define degHotend2() degHotend(2)
#define degTargetHotend2() degTargetHotend(2)
#define setTargetHotend2(_celsius) setTargetHotend((_celsius), 2)
#define isHeatingHotend2() isHeatingHotend(2)
#define isCoolingHotend2() isCoolingHotend(2)
#else
#define setTargetHotend2(_celsius) do{}while(0)
#endif
#if EXTRUDERS > 3
#error Invalid number of extruders
#endif

int getHeaterPower(int heater);
void disable_heater();
void updatePID();


FORCE_INLINE void autotempShutdown(){
 #ifdef AUTOTEMP
 if(autotemp_enabled)
 {
  autotemp_enabled=false;
  if(degTargetHotend(active_extruder)>autotemp_min)
    setTargetHotend(0,active_extruder);
 }
 #endif
}

void PID_autotune(float temp, int extruder, int ncycles);

void setExtruderAutoFanState(int pin, bool state);
void checkExtruderAutoFans();


#if (defined(FANCHECK) && defined(TACH_0) && (TACH_0 > -1))

enum { 
	EFCE_OK = 0,   //!< normal operation, both fans are ok
	EFCE_FIXED,    //!< previous fan error was fixed
	EFCE_DETECTED, //!< fan error detected, but not reported yet
	EFCE_REPORTED  //!< fan error detected and reported to LCD and serial
};
extern volatile uint8_t fan_check_error;

void countFanSpeed();
void checkFanSpeed();
void fanSpeedError(unsigned char _fan);

void check_fans();

#endif //(defined(TACH_0))

void check_min_temp();
void check_max_temp();


#endif

extern unsigned long extruder_autofan_last_check;
extern uint8_t fanSpeedBckp;
extern bool fan_measuring;

