/*
	# Helper class to exercise failure routes of certain scriptable items that are not normally reachable
    # unless there is a deliberate programmatic error that goes unnoticed or a bug in the script validation.

	Copyright 2021 VintagePC <https://github.com/vintagepc/>

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

#include "IScriptable.h"
#include "3rdParty/catch2/catch.hpp"
#include <string>
#include <vector>

class Internal_UnhandledActionsTests{

    public:
        Internal_UnhandledActionsTests();

        bool RunTests();

		std::vector<std::string> GetFailures();

    private:
        std::vector<IScriptable> m_scriptables;

}
