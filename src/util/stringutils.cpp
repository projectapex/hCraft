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

#include "util/stringutils.hpp"
#include "util/wordwrap.hpp"
#include "slot/slot.hpp"
#include "slot/items.hpp"
#include <cctype>
#include <sstream>


namespace hCraft {
	namespace sutils {
		
		/* 
		 * Removes leading whitespace from the given string.
		 */
		std::string&
		ltrim (std::string& s)
		{
			int j = s.size (), i = 0;
			for (; i < j; ++i)
				if (!std::isspace (s[i]))
					break;
			s.erase (0, i);
			return s;
		}
		
		/* 
		 * Removes trailing whitespace from the given string.
		 */
		std::string&
		rtrim (std::string& s)
		{
			int i;
			for (i = s.size () - 1; i >= 0; --i)
				if (!std::isspace (s[i]))
					break;
			if (i != (int)(s.size () + 1))
				s.erase (i + 1, s.size () - (i + 1));
			return s;
		}
		
		/* 
		 * Removes whitespace from both ends of the given string.
		 */
		std::string&
		trim (std::string& s)
			{ return ltrim (rtrim (s)); }
		
		
		
		/* 
		 * Check whether the specified string will be empty after trimming from
		 * both ends.
		 */
		bool
		is_empty (const std::string& s)
		{
			int char_count = s.size ();
			int i, j;
			
			for (i = 0; i < (int)s.size (); ++i)
				{
					if (s[i] == ' ')
						-- char_count;
					else
						break;
				}
			
			for (j = (i - 1); j >= 0; --j)
				{
					if (s[j] == ' ')
						-- char_count;
					else
						break;
				}
			
			return char_count == 0;
		}
		
		
		/* 
		 * Case-insensitive string equality check.
		 */
		bool
		iequals (const std::string& a, const char *b)
		{
			int len = a.length (), i = 0;
			for (; i < len; ++i)
				{
					char ca = a[i];
					char cb = b[i];
					if (cb == 0)
						return false;
					
					if (std::tolower (ca) != std::tolower (cb))
						return false;
				}
			
			if (b[i] != 0)
				return false;
			return true;
		}
		
		bool
		iequals (const std::string& a, const std::string& b)
		{
			return iequals (a, b.c_str ());
		}
		
		
		
		bool
		is_int (const std::string& str)
		{
			bool minus = false;
			for (char c : str)
				{
					if (c == '-')
						{
							if (minus)
								return false;
							minus = true;
						}
					else if (!std::isdigit (c))
						return false;
				}
			
			return true;
		}
		
		int
		to_int (const std::string& str)
		{
			std::istringstream ss (str);
			int num;
			ss >> num;
			return num;
		}
		
		
		
		bool
		is_float (const std::string& str)
		{
			bool minus = false;
			bool exp = false;
			bool dot = false;
			for (int i = 0; i < (int)str.size (); ++i)
				{
					char c = str[i];
					if (c == '-')
						{
							if (minus)
								return false;
							minus = true;
						}
					else if (c == '.')
						{
							if (dot)
								return false;
							dot = true;
						}
					else if (!std::isdigit (c))
						{
						 	if (c == 'e' || c == 'E')
						 		{
						 			if (exp)
						 				return false;
						 			exp = true;
						 			minus = false;
						 		}
						}
				}
			
			return true;
		}
		
		double
		to_float (const std::string& str)
		{
			double f;
			std::istringstream ss (str);
			ss >> f;
			return f;
		}
		
		
		
		bool
		is_block (const std::string& str)
		{
			// Syntax: <name/id>[:metadata]
			
			int alpha_count = 0;
			
			for (size_t i = 0; i < str.size (); ++i)
				{
					int c = str[i];
					if (!(std::isalnum (c) || c == '_' || c == '-'))
						{
							// metadata
							if (c == ':')
								{
									++ i;
									for (; i < str.size (); ++i)
										if (!std::isdigit (str[i]))
											return false;
									break;
								}
							else
								return false;
						}
					else
						{
							if (std::isalpha (c))
								++ alpha_count;
								/*
							else if (std::isdigit (c))
								++ digit_count;
							if (alpha_count > 0 && digit_count > 0)
								return false;
								*/
						}
				}
		
			return true;
		}
		
		blocki
		to_block (const std::string& str)
		{
			std::ostringstream name, meta;
		
			bool have_meta = false;
			for (size_t i = 0; i < str.size (); ++i)
				{
					if (str[i] == ':')
						{
							have_meta = true;
							++ i;
							for (; i < str.size (); ++i)
								meta << (char)str[i];
						}
					else
						name << (char)str[i];
				}
		
			int id = 0, m = 0;
			block_info *binf = block_info::from_id_or_name (name.str ().c_str ());
			if (!binf)
				return blocki (BT_UNKNOWN, 0xFF);
			id = binf->id;
		
			if (have_meta)
				{
					std::istringstream imeta ((meta.str ()));
					imeta >> m;
				}
			return blocki (id, m);
		}
		
		
		
		/* 
		 * Returns the specified item's name with its damage/meta value appended
		 * (unless it's 0).
		 */
		std::string
		get_slot_name (const slot_item& sl)
		{
			if (!sl.is_valid ())
				return "unknown";
			
			std::ostringstream ss;
			if (sl.is_item ())
				{
					item_info *iinf = item_info::from_id (sl.id ());
					if (iinf)
						ss << iinf->name;
					else
						ss << sl.id ();
				}
			else
				{
					block_info *binf = block_info::from_id (sl.id ());
					if (binf)
						ss << binf->name;
					else
						ss << sl.id ();
					
					if (sl.damage () != 0)
						{
							ss << ':' << (int)sl.damage ();
						}
				}
			
			if (sl.amount () != 1)
				{
					ss << '{' << (int)sl.amount () << '}';
				}
			
			return ss.str ();
		}
		
		std::string
		get_slot_name (blocki bl)
		{
			return get_slot_name (slot_item (bl.id, bl.meta, 1));
		}
		
		
		
		/* 
		 * Checks whether the specified string is an IPv4 address.
		 */
		bool
		is_ip_addr (const std::string& str)
		{
			const char *ptr = str.c_str ();
			int dots = 0;
			bool f = false;
			while (*ptr)
				{
					if (*ptr == '.')
						{
							if (!f || dots == 3)
								return false;
							++ dots;
							f = false;
						}
					else if (!std::isdigit (*ptr))
						return false;
					
					f = true;
					++ ptr;
				}
			
			return f && (dots == 3);
		}
		
		
		/* 
		 * MC usernames may only contain digits, letters and underscores.
		 */
		bool
		is_valid_username (const std::string& str)
		{
			if (str.length () < 2 || str.length () > 16)
				return false;
			
			for (char c : str)
				if (!(std::isalnum (c) || c == '_'))
					return false;
			return true;
		}
		
		
		
		/* 
		 * Converts color codes from &X to §X.
		 */
		std::string
		convert_colors (const std::string& str)
		{
			std::string out;
			out.reserve (str.size ());
			
			for (size_t i = 0; i < str.size (); ++i)
				{
					if ((str[i] == '&') && ((i + 1) < str.size ()) && is_chat_code (str[i + 1]))
						{
							out.push_back (0xC2);
							out.push_back (0xA7);
							
							++ i;
							out.push_back (str[i]);
						}
					else
						out.push_back (str[i]);
				}
			
			return out;
		}
	}
}

