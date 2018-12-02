/*
 * Copyright (c) 2016 CSIRO - Australia Telescope National Facility (ATNF)
 * 
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * PO Box 76, Epping NSW 1710, Australia
 * atnf-enquiries@csiro.au
 * 
 * This file is part of the ASKAP software distribution.
 * 
 * The ASKAP software distribution is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 * 
 * @author Daniel Collins <daniel.collins@csiro.au>
 */
package askap.util;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 *
 * @author Daniel Collins <daniel.collins@csiro.au>
 */
public class Path {

	static Pattern pv = Pattern.compile("\\$\\{(\\w+)\\}");

	/**
	 * Expands the ~ symbol into a fully qualified path.
	 *
	 * @param path The path to expand.
	 * @return The expanded path.
	 */
	public static String expanduser(String path) {
		String user = System.getProperty("user.home");
		return path.replaceFirst("~", user);
	}

	/**
	 * Expands all environment variable tokens in the given string with the
	 * variable value. Missing values are not expanded.
	 *
	 * @param input A string containing the environment variable tokens. Each
	 * token should be in the form ${variable_name}.
	 * @return The expanded string.
	 */
	public static final String expandvars(String input) {
		String result = input;

		Matcher m = pv.matcher(input);
		while (m.find()) {
			String var = m.group(1);
			String value = System.getenv(var);
			if (value != null) {
				result = result.replace("${" + var + "}", value);
			}
		}
		return result;
	}
}
