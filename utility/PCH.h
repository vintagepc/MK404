/*
	PCH.h - Precompiled header file for MK404.
	System and 3rdParty files ONLY, do not include in-project headers
	unless they change VERY rarely.

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK404.

	MK404 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK404 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK404.  If not, see <http://www.gnu.org/licenses/>.
 */
//#include <GL/glew.h>
//#include <GL/freeglut_std.h>
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include "gsl-lite.hpp"
#include "avr_uart.h"
#include "sim_avr.h"
#include "sim_avr_types.h"
#include "sim_cycle_timers.h"
#include "sim_irq.h"
#include "sim_io.h"

#include "BasePeripheral.h"
#include "IKeyClient.h"
#include "IScriptable.h"
#include "Scriptable.h"
