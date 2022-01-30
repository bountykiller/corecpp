#include <cstring>
#include <iostream>
#include <array>

#include <corecpp/diagnostic.h>
#include <corecpp/cli/command_line.h>
//#include <corecpp/meta/validator.h>
#include <corecpp/unittest.h>

using namespace corecpp;

class test_option final : public test_fixture
{
	template <typename TestT>
	void do_test(const program_option& option, const TestT& t,
				decltype(TestT::expected)& actual) const
	{
		actual = decltype(TestT::expected) { }; // reset the value to avoid side effects between tests

		if (t.arg.length() == 1)
		{
			assert_equal(option.match(t.arg[0]), t.match);
			if (t.match)
			{
				/* split the given string after making a copy of it
				 * this reflect the fact that separate strings are separate parameters
				 */
				const char* progname = "test";
				std::vector<const char*> argv { progname };
				std::locale l = std::locale();
				std::string value = t.value;
				std::string::iterator it = value.begin();
				while (true)
				{
					argv.emplace_back(it.operator->());
					while (it != value.end() && !std::isspace(*it, l))
						++it;
					if (it == value.end())
						break;
					*it = '\0';
					++it;
					if (it == value.end())
						break;
				};

				command_line line { static_cast<int>(argv.size()), const_cast<char**>(argv.data()) }; // ugly, I know
				short_option_parser p { line };
				logger().trace(corecpp::concat<std::string>({"short option"}), __FILE__, __LINE__);
				if (t.should_throw)
					assert_throws<std::ios_base::failure>([&] { option.read(p); });
				else
					option.read(p);
			}
		}
		else
		{
			assert_equal(option.match(t.arg), t.match);
			if (t.match)
			{
				long_option_parser p { t.value };
				logger().trace(corecpp::concat<std::string>({"long option"}), __FILE__, __LINE__);
				if (t.should_throw)
					assert_throws<std::ios_base::failure>([&] { option.read(p); });
				else
					option.read(p);
			}
		}
		if (!t.should_throw)
			assert_equal(actual, t.expected);
		// else actual is undefined
	}

	test_case_result test_int() const
	{
		struct test {
			std::string arg;
			std::string value;
			bool should_throw;
			bool match;
			int expected;
		};
		test_cases<test> tests ({
			{ "i", "1", false, true, 1 },
			{ "int", "2", false, true, 2 },
			{ "i", "aaa", true, true, 0 },
			{ "int", "aaa", true, true, 0 },
			{ "int", "-1", false, true, -1 },
			{ "into", "1", false, false, 0 },
			{ "in", "1", false, false, 0 },
			{ "anint", "-1", false, false, 0 },
			{ "", "-1", false, false, 0 },
			{ "1", "i", false, false, 0 },
		});

		return run(tests, [&](const test& t){
			int actual;
			program_option option('i',"int", "an integer value", actual);
			do_test(option, t, actual);
		});
	}

	test_case_result test_bool() const
	{
		struct test {
			std::string arg;
			std::string value;
			bool should_throw;
			bool match;
			bool expected;
		};

		test_cases<test> tests ({
			{ "b", "", false, true, true },
			{ "b", "1", false, true, true },
			{ "b", "0", false, true, true },
			{ "a_bool", "false", false, true, false },
			{ "a_bool", "aaa", true, true, false },
			{ "a_bool", "", false, true, true },
			{ "i", "", false, false, false },
			{ "i", "true", false, false, false },
			{ "unknown", "", false, false, false },
			{ "unknown", "true", false, false, false },
		});

		return run(tests, [&](const test& t){
			bool actual = false;
			program_option option('b',"a_bool", "a boolean", actual);
			do_test(option, t, actual);
		});
	}

	test_case_result test_str() const
	{
		struct test {
			std::string arg;
			std::string value;
			bool should_throw;
			bool match;
			std::string expected;
		};

		test_cases<test> tests ({
			{ "s", "", false, true, "" },
			{ "s", "1", false, true, "1" },
			{ "s", "0", false, true, "0" },
			{ "str", "false", false, true, "false" },
			{ "str", "aaa", false, true, "aaa" },
			{ "str", "", false, true, "" },
			{ "i", "", false, false, "" },
			{ "i", "true", false, false, "" },
			{ "unknown", "", false, false, "" },
			{ "unknown", "true", false, false, "" },
			/* TODO: add locale testing */
		});

		return run(tests, [&](const test& t){
			std::string actual;
			program_option option('s',"str", "a string value", actual);
			do_test(option, t, actual);
		});
	}

	test_case_result test_vector() const
	{
		struct test {
			std::string arg;
			std::string value;
			bool should_throw;
			bool match;
			std::vector<int> expected;
		};

		test_cases<test> tests ({
			{ "v",       "",      false, true,  { } },
			{ "v",       "1",     false, true,  { 1 } },
			{ "v",       "1 2 3", false, true,  { 1, 2, 3 } },
			{ "v",       "1 a 3", true,  true,  { 1 } },
			{ "v",       "1 2 -- 3", false, true,  { 1, 2 } },

			{ "vector",  "",      true,  true,  { } },
			{ "vector",  "1",     false, true,  { 1 } },
			{ "vector",  "1,2,3", false, true,  { 1, 2, 3 } },
			{ "vector",  "1,a,3", true,  true,  { 1 } },

			{ "i",       "",      false, false, { } },
			{ "i",       "true",  false, false, { } },
			{ "unknown", "",      false, false, { } },
			{ "unknown", "true",  false, false, { } },
		});

		return run(tests, [&](const test& t){
			std::vector<int> actual;
			program_option option('v',"vector", "a vector of integers", actual);
			do_test(option, t, actual);
		});
	}

public:
	tests_type tests() const override
	{
		/* TODO:
		 * Add tests for utf8 & co
		 */
		return {
			{ "test_int",  [&] () { return test_int(); } },
			{ "test_bool", [&] () { return test_bool(); } },
			{ "test_str",  [&] () { return test_str(); } },
			{ "test_vector", [&] () { return test_vector(); } },
		};
	}
};

class test_parser final : public test_fixture
{
	test_case_result test_options() const
	{
		struct test {
			int expected_a;
			int expected_b;
			int expected_c;
			bool expected_d;
			int argc;
			std::vector<char const *> argv;
		};

		test_cases<test> tests ({
			{ 0, 0, 0, false, 0, { "" } },
			{ 0, 0, 0, false, 1, { "program" } },
			{ 1, 0, 0, false, 2, { "program", "--an-option=1" } },
			{ -1, 0, 0, false, 3, { "program", "-a", "-1" } },
			{ 2, 0, 0, false, 4, { "program", "--an-option=1", "-a", "2" } },
			{ 1, 2, 0, false, 4, { "program", "--an-option=1", "-b", "2" } },
			{ 0, 2, 3, false, 4, { "program", "-c", "3", "--option-b=2" } },
			{ 2, 0, 3, false, 5, { "program", "-c", "3", "-a", "2" } },
			{ 0, 0, 0, true, 2, { "program", "--deep-option" } },
			{ 0, 2, 0, true, 3, { "program", "--deep-option", "--option-b=2" } },

			{ 0, 0, 0, false, 3, { "program", "1", "2" } },
			{ 0, 0, 0, false, 4, { "program", "--", "1", "2" } },
			{ 0, 0, 0, false, 3, { "program", "--", "-d" } },
			{ 0, 0, 0, false, 3, { "program", "--", "--deep-option" } },

			{ 0, 0, 0, false, 3, { "program", "-a", "-b" } },
			{ 0, 1, 0, false, 3, { "program", "-ab", "1", } },
			{ 4, 0, 0, true, 3, { "program", "-da", "4" } },
			{ 0, 0, 4, true, 4, { "program", "-ad", "-c", "4" } },
		});

		return run(tests, [&](const test& t){
			int a = 0, b = 0, c = 0;
			bool d = false;
			command_line line { t.argc, const_cast<char**>(t.argv.data()) }; /* const_cast required due to C API */
			command_line_parser parser { line };
			parser.add_options(
				program_option {'a', "an-option", "The option A", a },
				program_option {'b', "option-b", "The option B", b },
				program_option {'c', "option-c", "The option C", c },
				program_option {'d',"deep-option", "The option D", d });
			parser.parse_options();
			assert_equal(a, t.expected_a);
			assert_equal(b, t.expected_b);
			assert_equal(c, t.expected_c);
			assert_equal(d, t.expected_d);
		});
	}

	test_case_result test_params() const
	{
		struct test {
			int argc;
			std::vector<char const *> argv;
			std::vector<int> expected;
		};

		test_cases<test> tests ({
			{ 1, { "program" }, {} },
			{ 2, { "program", "1" }, { 1 } },
			{ 3, { "program", "1", "2" }, { 1, 2 } },
			{ 4, { "program", "0", "2", "-3" }, { 0, 2, -3 } },

			{ 3, { "program","-a", "1" }, { } },
			{ 4, { "program","-a", "1", "2" }, { 2 } },
			{ 5, { "program","-a", "1", "2", "-3" }, { 2, -3 } },

			{ 2, { "program","--an-option=1" }, { } },
			{ 3, { "program","--an-option=1", "3" }, { 3 } },
			{ 4, { "program","--an-option=1", "3", "5" }, { 3, 5 } },

			{ 3, { "program","-b", "--an-option=2" }, { } },
			{ 4, { "program","-b", "--an-option=2", "4" }, { 4 } },
			{ 8,
				{ "program","-b", "--an-option=2", "4", "0", "666", "007", "8" },
				{ 4, 0, 666, 7, 8 }
			},
			{ 9,
				{ "program","-b", "--an-option=2", "--", "-4", "0", "-666", "007", "8" },
				{ -4, 0, -666, 7, 8 }
			},
			{ 9,
				{ "program","-b", "--an-option=2", "4", "0", "-666", "007", "8", "--" },
				{ 4, 0, -666, 7, 8 }
			},
			{ 9,
				{ "program","-b", "--", "4", "0", "-666", "007", "8", "--" },
				{ 4, 0, -666, 7, 8 }
			},

			{ 3, { "program","-b", "" }, { } },
			{ 3, { "program","-b", "NaN" }, { 0 } },
			{ 5, { "program","-b", "0", "", "1" }, { 0, 1 } },
			{ 5, { "program","-b", "0", "NaN", "1" }, { 0, 0, 1 } },
		});

		return run(tests, [&](const test& t){
			int a = 0;
			bool b;
			std::vector<int> value = {};
			command_line line { t.argc, const_cast<char**>(t.argv.data()) }; /* const_cast required due to C API */
			command_line_parser parser { line };
			parser.add_option('a', "an-option", "The option A", a);
			parser.add_option('b', "another-option", "The option A", b);
			parser.add_param("value", "the value", value, true);
			parser.parse_options();
			parser.parse_parameters();
			assert_equal(value, t.expected);
		});
	}

public:
	tests_type tests() const override
	{
		return {
			{ "test_options_parser", [&] () { return test_options(); } },
			{ "test_params_parser", [&] () { return test_params(); } },
		};
	}
};

int main(int argc, char** argv)
{
	test_unit unit { "command line" };
	unit.add_fixture<test_option>("test_option");
	unit.add_fixture<test_parser>("test_parser");

	return unit.run(argc, argv);
};
