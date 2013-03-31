/* 
 * hCraft - A custom Minecraft server.
 * Copyright (C) 2012	Jacob Zhitomirsky
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

#include "utils.hpp"
#include <chrono>


namespace hCraft {
	namespace utils {
		
		/* 
		 * The amount of nanoseconds passed since epoch.
		 * Useful to initialize random number generators.
		 */
		unsigned long long
		ns_since_epoch ()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds> (
				std::chrono::high_resolution_clock::now ().time_since_epoch ()).count ()
				& 0x7FFFFFFF;
		}
	}
}

