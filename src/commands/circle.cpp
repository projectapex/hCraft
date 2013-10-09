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

#include "commands/circle.hpp"
#include "player/player.hpp"
#include "system/server.hpp"
#include "util/stringutils.hpp"
#include "util/position.hpp"
#include "drawing/drawops.hpp"
#include <sstream>
#include <vector>
#include <cmath>


namespace hCraft {
	namespace commands {
		
		namespace {
			struct circle_data {
				blocki bl;
				int radius;
				draw_ops::plane pn;
				bool fill;
			};
		}
		
		
		static bool
		on_blocks_marked (player *pl, block_pos marked[], int markedc)
		{
			circle_data *data = static_cast<circle_data *> (pl->get_data ("circle"));
			if (!data) return true; // shouldn't happen
			
			int radius = data->radius;
			if (radius == -1)
				{
					radius = std::round ((vector3 (marked[1]) - vector3 (marked[0])).magnitude ());
				}
			
			int blocks;
			dense_edit_stage es (pl->get_world ());
			draw_ops draw (es);
			if (data->fill)
				blocks = draw.filled_circle (marked[0], radius, data->bl, data->pn, pl->get_rank ().fill_limit ());
			else
				blocks = draw.circle (marked[0], radius, data->bl, data->pn, pl->get_rank ().fill_limit ());
			if (blocks == -1)
				{
					pl->message (messages::over_fill_limit (pl->get_rank ().fill_limit ()));
					pl->delete_data ("circle");
					return true;
				}
			es.commit ();
			
			std::ostringstream ss;
			ss << "§7 | Circle complete §f- §b" << blocks << " §7blocks modified";
			pl->message (ss.str ());
			
			pl->delete_data ("circle");
			return true;
		}
		
		/* 
		 * /circle -
		 * 
		 * Draws a two-dimensional circle centered at a point.
		 * 
		 * Permissions:
		 *   - command.draw.circle
		 *       Needed to execute the command.
		 */
		void
		c_circle::execute (player *pl, command_reader& reader)
		{
			if (!pl->perm (this->get_exec_permission ()))
					return;
		
			reader.add_option ("fill", "f");
			if (!reader.parse (this, pl))
					return;
			if (reader.no_args () || reader.arg_count () > 3)
				{ this->show_summary (pl); return; }
			
			bool do_fill = reader.opt ("fill")->found ();
			
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
			
			int radius = -1;
			if (reader.has_next ())
				{
					command_reader::argument arg = reader.next ();
					if (!arg.is_int ())
						{
							pl->message ("§c * §7Usage§f: §e/circle §cblock §8[§cradius§8] §8[§cplane§8]");
							return;
						}
					
					radius = arg.as_int ();
					if (radius <= 0)
						{
							pl->message ("§c * §7Radius must be greater than zero.§f");
							return;
						}
				}
			
			draw_ops::plane pn = draw_ops::XZ_PLANE;
			if (reader.has_next ())
				{
					std::string& str = reader.next ().as_str ();
					if (sutils::iequals (str, "XZ") || sutils::iequals (str, "ZX"))
						;
					else if (sutils::iequals (str, "YX") || sutils::iequals (str, "XY"))
						pn = draw_ops::YX_PLANE;
					else if (sutils::iequals (str, "YZ") || sutils::iequals (str, "ZY"))
						pn = draw_ops::YZ_PLANE;
					else
						{
							pl->message ("§c * §7The plane must be one of§f: §cXZ, YX, YZ§f.");
							return;
						}
				}
			
			circle_data *data = new circle_data {bl, radius, pn, do_fill};
			pl->create_data ("circle", data,
				[] (void *ptr) { delete static_cast<circle_data *> (ptr); });
			pl->get_nth_marking_callback ((radius == -1) ? 2 : 1) += on_blocks_marked;
			
			std::ostringstream ss;
			ss << "§5Draw§f: §3" << (do_fill ? "filled" : "hollow")
				 << " circle §f[§7block§f: §8" << str << "§f, §7plane§f: §8";
			switch (pn)
				{
					case draw_ops::XZ_PLANE: ss << "xz§f, "; break;
					case draw_ops::YX_PLANE: ss << "yx§f, "; break;
					case draw_ops::YZ_PLANE: ss << "yz§f, "; break;
				}
			ss << "§7radius§f: §8";
			if (radius == -1)
				ss << "?";
			else
				ss << radius;
			ss << "§f]:";
			pl->message (ss.str ());
			
			ss.str (std::string ()); ss.clear ();
			ss << "§7 | §ePlease mark §b" << ((radius == -1) ? "two" : "one") << " §eblock"
				 << ((radius == -1) ? "s" : "") << "§f.";
			pl->message (ss.str ());
		}
	}
}

