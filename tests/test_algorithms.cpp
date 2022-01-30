#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <vector>
#include <typeinfo>

#include <corecpp/algorithm.h>
#include <corecpp/unittest.h>

using namespace corecpp;

class test_get_or_def final : public test_fixture
{
	test_case_result test_pchar() const
	{
		struct test { const char* in; const char* def; const char* expected; };
		test_cases<test> tests ({
			{ "", "", "" },
			{ " ", "", " " },
			{ "", " ", "" },
			{ "", nullptr, "" },

			{ nullptr, "", "" },
			{ nullptr, nullptr, nullptr },
			{ nullptr, "a", "a" },
			{ nullptr, "\0", "\0" },
			{ nullptr, "\\0", "\\0" },

			{ "a", nullptr, "a" },
			{ "\0", nullptr, "\0" },
			{ "\\0", nullptr, "\\0" },

			{ "a", "a", "a" },
			{ "a", "b", "a" },
		});

		return run(tests, [&](const test& t){
			auto res = corecpp::get_or_def(t.in, t.def);
			assert_str_equal(res, t.expected);
		});
	}

	test_case_result test_optional() const
	{
		struct test { std::optional<int> in; int def; int expected; };
		test_cases<test> tests ({
			{ std::nullopt, 0, 0 },
			{ std::nullopt, 1, 1 },

			{ 1, 0, 1 },
			{ 1, 1, 1 },
			{ 1, 2, 1 },
		});

		return run(tests, [&](const test& t){
			int res = corecpp::get_or_def(t.in, t.def);
			assert_equal(res, t.expected);
		});
	}

	test_case_result test_pointer() const
	{
		struct test { const int* in; int def; int expected; };
		const int one = 1;
		test_cases<test> tests ({
			{ nullptr, 0, 0 },
			{ nullptr, 1, 1 },

			{ &one, 0, 1 },
			{ &one, 1, 1 },
			{ &one, 2, 1 },
		});

		return run(tests, [&](const test& t){
			int res = corecpp::get_or_def(t.in, t.def);
			assert_equal(res, t.expected);
		});
	}
public:
	tests_type tests() const override
	{
		return {
			{ "test_pointer_to_char", [&] () { return test_pchar(); } },
			{ "test_optional", [&] () { return test_optional(); } },
			{ "test_pointer", [&] () { return test_pointer(); } },
		};
	}

};

int main(int argc, char** argv)
{
	test_unit unit { "Algotithms" };
	unit.add_fixture<test_get_or_def>("test_get_or_def");

	return unit.run(argc, argv);
};


