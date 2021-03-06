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

#include "commands/polygon.hpp"
#include "system/server.hpp"
#include "player/player.hpp"
#include "util/stringutils.hpp"
#include "util/position.hpp"
#include "drawing/drawops.hpp"
#include <sstream>
#include <vector>
#include <cmath>

#include <queue>

#include <iostream> // DEBUG


namespace hCraft {
	namespace commands {
		
		namespace {
			struct polygon_data {
				std::vector<vector3> points;
				sparse_edit_stage es;
				blocki bl;
				bool fill;
				
				int reg_sides;
				double radius;
				
				polygon_data (world *wr, blocki bl, bool fill, int sides, double rad)
					: es (wr)
				{
					this->bl = bl;
					this->fill = fill;
					this->reg_sides = sides;
					this->radius = rad;
				}
			};
		}
		
		
		static bool
		fill_polygon (edit_stage& es, block_pos pt, blocki material, player *pl)
		{
			world *w = es.get_world ();
			
			std::queue<block_pos> q;
			q.push (pt);
			while (!q.empty ())
				{
					block_pos n = q.front ();
					q.pop ();
					
					if (!w->in_bounds (n.x, n.y, n.z))
						continue;
					if (!w->can_build_at (n.x, n.y, n.z, pl))
						return false;
					
					if (es.get (n.x, n.y, n.z) != material)
						{
							es.set (n.x, n.y, n.z, material.id, material.meta);
							q.emplace (n.x - 1, n.y, n.z);
							q.emplace (n.x + 1, n.y, n.z);
							q.emplace (n.x, n.y, n.z - 1);
							q.emplace (n.x, n.y, n.z + 1);
						}
				}
			
			return true;
		}
		
		static bool
		regular_polygon (player *pl, vector3 pt, int sides, double radius,
			blocki material, bool fill, cond_edit_stage& es)
		{
			double curr = -0.785398163; 
			double cang = 6.283185307 / sides;
			
			draw_ops draw (es);
			
			double sx, sz, lx, lz;
			for (int i = 0; i < sides; ++i)
				{
					double x = std::cos (curr) * radius + pt.x;
					double z = std::sin (curr) * radius + pt.z;
					
					if (i > 0)
						{
							draw.line ({lx, pt.y, lz}, {x, pt.y, z}, material);
						}
					else
						{
							sx = x;
							sz = z;
						}
					
					lx = x;
					lz = z;
					curr += cang;
				}
			draw.line ({lx, pt.y, lz}, {sx, pt.y, sz}, material);
			
			if (es.failed_blocks () > 0)
				return false;
			if (radius >= 2.0 && fill)
				{
					if (!fill_polygon (es, {(int)pt.x, (int)pt.y, (int)pt.z}, material, pl))
						return false;
				}
			
			es.commit ();
			return true;
		}
		
		static bool
		on_blocks_marked (player *pl, block_pos marked[], int markedc)
		{
			polygon_data *data = static_cast<polygon_data *> (pl->get_data ("polygon"));
			if (!data) return true; // shouldn't happen
			
			if (!pl->get_world ()->security ().can_build (pl))
				{
					pl->message ("§4 * §cYou are not allowed to build here§4.");
					pl->delete_data ("polygon");
					return true;
				}
			
			cond_edit_stage es {data->es,
				[] (world *w, int x, int y, int z, void *ctx) -> bool
					{
						return w->can_build_at (x, y, z, static_cast<player *> (ctx));
					}, pl};
			
			if (es.get_world () != pl->get_world ())
				{
					pl->message ("§4 * §cWorlds changed§4.");
					pl->delete_data ("polygon");
					return true;
				}
			
			if (data->reg_sides >= 3)
				{
					// handle stuff differently in this case
					double radius = data->radius;
					if (markedc == 2)
						radius = (vector3 (marked[1]) - vector3 (marked[0])).magnitude ();
						
					if (!regular_polygon (pl, marked[0], data->reg_sides, radius, data->bl, data->fill, es))
						{
							pl->delete_data ("polygon");
							pl->message ("§7 | §cPolygon could not be completed (zones)");
							return true;
						}
					
					pl->delete_data ("polygon");
					pl->message ("§7 | Regular polygon complete");
					return true;
				}
			
			vector3 pt = marked[0];
			if (!data->points.empty ())
				pt.y = data->points[0].y;
			data->points.push_back (pt);
			
			// preview
			if (data->points.size () > 1)
				{
					es.restore_to (pl);
					es.clear ();
				}
			draw_ops draw (es);
			draw.polygon (data->points, data->bl);
			es.preview_to (pl);
			
			{
				std::ostringstream ss;
				int fb = es.failed_blocks ();
				if (fb > 0)
					{
						ss << "§7 | " << fb << " §czoned block" << ((fb == 1) ? "" : "s") << " could not be replaced.";
						pl->message (ss.str ());
						ss.str (std::string ());
					}
			}
			
			return false;
		}
		
		
		static void
		draw_polygon (player *pl)
		{
			polygon_data *data = static_cast<polygon_data *> (pl->get_data ("polygon"));
			if (!data)
				{
					pl->message ("§4 * §cYou are not drawing any polygons§4.");
					return;
				}
			
			if (data->es.get_world () != pl->get_world ())
				{
					pl->message ("§4 * §cWorlds changed§4.");
					pl->stop_marking ();
					pl->delete_data ("polygon");
					return;
				}
			else if (data->points.empty ())
				{
					pl->stop_marking ();
					pl->delete_data ("polygon");
					return;
				}
			
			sparse_edit_stage &ses = data->es;
			cond_edit_stage es {ses,
				[] (world *w, int x, int y, int z, void *ctx) -> bool
					{
						return w->can_build_at (x, y, z, static_cast<player *> (ctx));
					}, pl};
			
			es.restore_to (pl);
			es.clear ();
			
			int blocks;
			draw_ops draw (es);
			blocks = draw.polygon (data->points, data->bl);
			es.commit ();
			
			pl->stop_marking ();
			pl->delete_data ("polygon");
			
			std::ostringstream ss;
			ss << "§7 | Polygon complete §f- §b" << blocks << " §7blocks modified";
			pl->message (ss.str ());
		}
		

		
		/* 
		 * /polygon -
		 * 
		 * Creates a wireframe rendering of the polygon specified by marked points.
		 * 
		 * Permissions:
		 *   - command.draw.polygon
		 *       Needed to execute the command.
		 */
		void
		c_polygon::execute (player *pl, command_reader& reader)
		{
			if (!pl->perm (this->get_exec_permission ()))
					return;
		
			reader.add_option ("fill", "f");
			reader.add_option ("regular", "r", true, true);
			if (!reader.parse (this, pl))
					return;
			if (reader.no_args () || reader.arg_count () > 3)
				{ this->show_summary (pl); return; }
			
			// parse options
			bool do_fill = reader.opt ("fill")->found ();
			int reg_sides = -1;
			if (reader.opt ("regular")->found ())
				{
					auto opt = reader.opt ("regular");
					auto arg = opt->arg (0);
					
					if (!arg.is_int ())
						{
							pl->message ("§c * §7Usage§f: §e/polygon \\r §4number §cblock §cradius");
							return;
						}
					reg_sides = arg.as_int ();
					if (reg_sides < 3 || reg_sides > 100)
						{
							pl->message ("§c * §7Too many points!");
							return;
						}
				}
			
			
			std::string& str = reader.next ().as_str ();
			if (sutils::iequals (str, "stop"))
				{
					draw_polygon (pl);
					return;
				}
			
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
			
			
			double radius = -1.0;
			if (reg_sides >= 3 && reader.has_next ())
				{
					auto arg = reader.next ();
					if (!arg.is_float ())
						{
							pl->message ("§c * §7Usage§f: §e/polygon \\r §4number §cblock §cradius");
							return;
						}
					
					radius = arg.as_float ();
					if (radius < 1)
						{
							pl->message ("§c * §7Invalid radius");
							return;
						}
				}
				 
			
			int pt_count = (reg_sides < 3) ? 1 : ((radius < 1) ? 2 : 1);
			polygon_data *data = new polygon_data (pl->get_world (), bl, do_fill, reg_sides, radius);
			pl->create_data ("polygon", data,
				[] (void *ptr) { delete static_cast<polygon_data *> (ptr); });
			pl->get_nth_marking_callback (pt_count) += on_blocks_marked;
			
			std::ostringstream ss;
			ss << "§5Draw§f: §3" << ((reg_sides != -1) ? "regular " : "") << "polygon §f[§7block§f: §8" << sutils::get_slot_name (bl);
			if (reg_sides >= 3)
				ss << "§f, §7sides§f: §8" << reg_sides;
			if (radius >= 1.0) 
				ss << "§f, §7Radius§f: §8" << radius;
			ss << "§f]:";
			pl->message (ss.str ());
			
			if (reg_sides < 3)
				{
					pl->message ("§7 | §ePlease mark the required points§f.");
					pl->message ("§7 | §eType §c/polygon stop §eto stop§f.");
				}
			else
				{
					ss.str (std::string ()); ss.clear ();
					ss << "§7 | §ePlease mark §b" << pt_count << " §eblock" << ((pt_count == 1) ? "" : "s") << "§f.";
					pl->message (ss.str ());
				}
		}
	}
}

