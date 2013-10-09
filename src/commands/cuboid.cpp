/* 
 * hCraft - A custom Minecraft server.
 * Copyright (C) 2012-2013	Jacob Zhitomirsky (BizarreCake)
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

#include "commands/cuboid.hpp"
#include "player/player.hpp"
#include "system/server.hpp"
#include "world/world.hpp"
#include "util/stringutils.hpp"
#include "util/utils.hpp"
#include "drawing/drawops.hpp"
#include "system/messages.hpp"
#include <sstream>


namespace hCraft {
	namespace commands {
	
		namespace {
			struct cuboid_data {
				blocki bl;
			};
		}
		
		
		static bool
		on_blocks_marked (player *pl, block_pos marked[], int markedc)
		{
			cuboid_data *data = static_cast<cuboid_data *> (pl->get_data ("cuboid"));
			if (!data) return true; // shouldn't happen
			
			int bc;
			
			dense_edit_stage es (pl->get_world ());
			draw_ops draw (es);
			bc = draw.filled_cuboid (marked[0], marked[1], data->bl, pl->get_rank ().fill_limit ());
			if (bc == -1)
				{
					pl->message (messages::over_fill_limit (pl->get_rank ().fill_limit ()));
					pl->delete_data ("cuboid");
					return true;
				}
			es.commit ();
			
			pl->delete_data ("cuboid");
			
			{
				std::ostringstream ss;
				ss << "§7 | Cuboid complete - §b" << bc << " §7blocks modified";
				pl->message (ss.str ());
			}
			
			return true;
		}
		
		/* 
		 * /cuboid -
		 * 
		 * Fills a region marked by two points with a specified block.
		 * 
		 * Permissions:
		 *   - command.draw.cuboid
		 *       Needed to execute the command.
		 */
		void
		c_cuboid::execute (player *pl, command_reader& reader)
		{
			if (!pl->perm (this->get_exec_permission ()))
					return;
		
			if (!reader.parse (this, pl))
					return;
			if (reader.no_args () || reader.arg_count () > 1)
				{ this->show_summary (pl); return; }
			
			std::string& str = reader.next ().as_str ();
			if (!sutils::is_block (str))
				{
					pl->message ("§c * §7Invalid block§f: §c" + str);
					return;
				}
			
			blocki bl = sutils::to_block (str);
			if (bl.id == BT_UNKNOWN)
				{
					pl->message ("§c * §7Unknown block§f: §c" + str);
					return;
				}
			
			cuboid_data *data = new cuboid_data {bl};
			pl->create_data ("cuboid", data,
				[] (void *ptr) { delete static_cast<cuboid_data *> (ptr); });
			pl->get_nth_marking_callback (2) += on_blocks_marked;
			
			pl->message ("§5Draw§f: §3regular cuboid §f[§7block§f: §8" + str + "§f]:");
			pl->message ("§7 | §ePlease mark §btwo §eblocks to determine the edges§f.");
		}
	}
}
		
