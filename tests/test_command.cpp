#include <iostream>
#include <corecpp/diagnostic.h>
#include <corecpp/command_line.h>
//#include <corecpp/meta/validator.h>
#include <corecpp/unittest.h>

using namespace corecpp;

class test_option final : public test_fixture
{
	test_case_result test_int() const
	{
		struct test {
			std::string arg;
			std::string value;
			bool match;
			int expected;
		};
		test_cases<test> tests ({
			{ "i", "1", true, 1 },
			{ "int", "2", true, 2 },
			{ "i", "aaa", true, 0 },
			{ "int", "aaa", true, 0 },
			{ "int", "-1", true, -1 },
			{ "anint", "-1", false, 0 },
			{ "", "-1", false, 0 },
			{ "1", "i", false, 0 },
		});

		return run(tests, [&](const test& t){
			int actual = 0;
			program_option option('i',"int", "an integer value", actual);
			if (t.arg.length() == 1)
				assert_equal(option.match(t.arg[0]), t.match);
			else
				assert_equal(option.match(t.arg), t.match);
			if (t.match)
				option.read(t.value);
			assert_equal(actual, t.expected);
		});
	}

	test_case_result test_bool() const
	{
		struct test {
			std::string arg;
			std::string value;
			bool match;
			bool expected;
		};

		test_cases<test> tests ({
			{ "b", "", true, true },
			{ "b", "1", true, true },
			{ "b", "0", true, false },
			{ "a_bool", "false", true, false },
			{ "a_bool", "aaa", true, false },
			{ "a_bool", "", true, true },
			{ "i", "", false, false },
			{ "i", "true", false, false },
			{ "unknown", "", false, false },
			{ "unknown", "true", false, false },
		});

		return run(tests, [&](const test& t){
			bool actual = false;
			program_option option('b',"a_bool", "a boolean", actual);
			if (t.arg.length() == 1)
				assert_equal(option.match(t.arg[0]), t.match);
			else
				assert_equal(option.match(t.arg), t.match);
			if (t.match)
				option.read(t.value);
			assert_equal(actual, t.expected);
		});
	}

	test_case_result test_str() const
	{
		struct test {
			std::string arg;
			std::string value;
			bool match;
			std::string expected;
		};

		test_cases<test> tests ({
			{ "s", "", true, "" },
			{ "s", "1", true, "1" },
			{ "s", "0", true, "0" },
			{ "str", "false", true, "false" },
			{ "str", "aaa", true, "aaa" },
			{ "str", "", true, "" },
			{ "i", "", false, "" },
			{ "i", "true", false, "" },
			{ "unknown", "", false, "" },
			{ "unknown", "true", false, "" },
		});

		return run(tests, [&](const test& t){
			std::string actual;
			program_option option('s',"str", "a boolean", actual);
			if (t.arg.length() == 1)
				assert_equal(option.match(t.arg[0]), t.match);
			else
				assert_equal(option.match(t.arg), t.match);
			if (t.match)
				option.read(t.value);
			assert_equal(actual, t.expected);
		});
	}

public:
	tests_type tests() const override
	{
		return {
			{ "test_int", [&] () { return test_int(); } },
			{ "test_bool", [&] () { return test_bool(); } },
			{ "test_str", [&] () { return test_str(); } }
		};
	}
};

class test_parser final : public test_fixture
{
	test_case_result test_params() const
	{
		struct test {
			int argc;
			char** argv;
			bool match;
			int expected;
		};
		test_cases<test> tests ({
		});

		return run(tests, [&](const test& t){
			/* TODO */
		});
	}
public:
	tests_type tests() const override
	{
		return {
			{ "test_params", [&] () { return test_params(); } },
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
