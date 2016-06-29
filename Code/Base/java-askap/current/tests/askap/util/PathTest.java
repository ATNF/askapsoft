/**
 *  Copyright (c) 2016 CSIRO - Australia Telescope National Facility (ATNF)
 *
 *  Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 *  PO Box 76, Epping NSW 1710, Australia
 *  atnf-enquiries@csiro.au
 *
 *  This file is part of the ASKAP software distribution.
 *
 *  The ASKAP software distribution is free software: you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the License,
 *  or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * @author Daniel Collins <daniel.collins@csiro.au>
 */
package askap.util;

import static org.junit.Assert.*;

import java.lang.reflect.Field;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import askap.util.Path;

public class PathTest {

	private HashMap<String, String> env;

	@Before
	public void setUp() throws Exception {
		env = new HashMap<>();
	}

	@After
	public void tearDown() throws Exception {
	}

	@Test
	public void testExpandUser() {
		String homeDir = "/home/roger_rabbit";
		String testPath = "~/code";
		String expectedPath = homeDir + "/code";
		System.setProperty("user.home", homeDir);
		assertEquals(expectedPath, Path.expanduser(testPath));
	}

	@Test
	public void testExpandUser_noTilde() {
		String testPath = "/home/somewhere/somewhereelse";
		String expectedPath = testPath;
		assertEquals(expectedPath, Path.expanduser(testPath));
	}

	@Test
	public void testExpandVars_twoVars() {
		env.put("key_one", "one");
		env.put("key_two", "two");
		setEnv(env);
		String input = "blah ${key_one} blah blah ${key_two}";
		String expectedOutput = "blah one blah blah two";
		assertEquals(expectedOutput, Path.expandvars(input));
	}

	@Test
	public void testExpandVars_missingVars() {
		env.put("not_missing", "ok");
		setEnv(env);
		String input = "blah ${missing} blah blah ${not_missing}";
		String expectedOutput = "blah ${missing} blah blah ok";
		assertEquals(expectedOutput, Path.expandvars(input));
	}

	@Test
	public void testExpandVars_args_string() {
		env.put("ASKAP_ROOT", "/home/dc/askap");
		setEnv(env);
		String input = "-s -c cpingest.in -l $ASKAP_ROOT/Code/Components/Services/manager/current/functests/processingest/cpingest.log_cfg";
		String expectedOutput = "-s -c cpingest.in -l /home/dc/askap/Code/Components/Services/manager/current/functests/processingest/cpingest.log_cfg";
		assertEquals(expectedOutput, Path.expandvars(input));
	}
	
	/**
	 * For portability reasons, Java does not provide methods to alter
	 * environment variables. This function modifies the in-memory cached Map of
	 * environment variables but does not modify the actual environment. For
	 * unit tests this is good enough.
	 *
	 * Taken from
	 * http://stackoverflow.com/questions/318239/how-do-i-set-environment-variables-from-java
	 *
	 * @param newenv A map specifying key-value pairs for environment variables
	 * to set.
	 */
	protected static void setEnv(Map<String, String> newenv) {
		try {
			Class<?> processEnvironmentClass = Class.forName("java.lang.ProcessEnvironment");
			Field theEnvironmentField = processEnvironmentClass.getDeclaredField("theEnvironment");
			theEnvironmentField.setAccessible(true);
			Map<String, String> env = (Map<String, String>) theEnvironmentField.get(null);
			env.putAll(newenv);
			Field theCaseInsensitiveEnvironmentField = processEnvironmentClass.getDeclaredField("theCaseInsensitiveEnvironment");
			theCaseInsensitiveEnvironmentField.setAccessible(true);
			Map<String, String> cienv = (Map<String, String>) theCaseInsensitiveEnvironmentField.get(null);
			cienv.putAll(newenv);
		} catch (NoSuchFieldException e) {
			try {
				Class[] classes = Collections.class.getDeclaredClasses();
				Map<String, String> env = System.getenv();
				for (Class cl : classes) {
					if ("java.util.Collections$UnmodifiableMap".equals(cl.getName())) {
						Field field = cl.getDeclaredField("m");
						field.setAccessible(true);
						Object obj = field.get(env);
						Map<String, String> map = (Map<String, String>) obj;
						map.clear();
						map.putAll(newenv);
					}
				}
			} catch (Exception e2) {
				e2.printStackTrace();
			}
		} catch (Exception e1) {
			e1.printStackTrace();
		}
	}
}
