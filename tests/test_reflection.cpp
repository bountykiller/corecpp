#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <vector>
#include <typeinfo>

#include <corecpp/meta/extensions.h>
#include <corecpp/unittest.h>

using namespace corecpp;

class test_reflection final : public test_fixture
{
	test_case_result test_is_iterable() const
	{
		struct test { bool actual; bool expected; };
		test_cases<test> cases ({
			{ corecpp::is_iterable<std::vector<int>>::value, true },
			{ corecpp::is_iterable<std::string>::value, true },
			{ corecpp::is_iterable<std::optional<int>>::value, false },
			{ corecpp::is_iterable<std::unique_ptr<std::string>>::value, false },
			{ corecpp::is_iterable<int*>::value, false }
		});

		return run(cases, [&](const test& t){
			assert_equal(t.actual, t.expected);
		});
	}

	test_case_result test_is_dereferencable() const
	{
		struct test { bool actual; bool expected; };
		test_cases<test> cases ({
			{ corecpp::is_dereferencable<std::vector<int>>::value, false },
			{ corecpp::is_dereferencable<std::string>::value, false },
			{ corecpp::is_dereferencable<std::optional<int>>::value, true },
			{ corecpp::is_dereferencable<std::unique_ptr<std::string>>::value, true },
			{ corecpp::is_dereferencable<int*>::value, true }
		});

		return run(cases, [&](const test& t){
			assert_equal(t.actual, t.expected);
		});
	}

	test_case_result test_is_associative() const
	{
		struct test { bool actual; bool expected; };
		test_cases<test> cases ({
			{ corecpp::is_associative<std::vector<int>>::value, false },
			{ corecpp::is_associative<std::string>::value, false },
			{ corecpp::is_associative<std::optional<int>>::value, false },
			{ corecpp::is_associative<std::unique_ptr<std::string>>::value, false },
			{ corecpp::is_associative<std::map<int, int>>::value, true },
			{ corecpp::is_associative<int*>::value, false }
		});

		return run(cases, [&](const test& t){
			assert_equal(t.actual, t.expected);
		});
	}

	test_case_result test_is_time_point() const
	{
		struct test { bool actual; bool expected; };
		test_cases<test> cases ({
			{ corecpp::is_time_point<std::vector<int>>::value, false },
			{ corecpp::is_time_point<std::string>::value, false },
			{ corecpp::is_time_point<std::optional<int>>::value, false },
			{ corecpp::is_time_point<std::unique_ptr<std::string>>::value, false },
			{ corecpp::is_time_point<int*>::value, false },
			{ corecpp::is_time_point<std::chrono::steady_clock::time_point>::value, true }
		});

		return run(cases, [&](const test& t){
			assert_equal(t.actual, t.expected);
		});
	}

	test_case_result test_is_strong_enum() const
	{
		enum weak
		{
			A, B, C
		};
		enum class strong
		{
			A, B, C
		};
		enum class strong2 : uint8_t
		{
			A = 1, B = 2, C = 4
		};
		struct test { bool actual; bool expected; };
		test_cases<test> cases ({
			{ corecpp::is_strong_enum<int>::value, false },
			{ corecpp::is_strong_enum<int*>::value, false },
			{ corecpp::is_strong_enum<std::optional<int>>::value, false },
			{ corecpp::is_strong_enum<std::string>::value, false },
			{ corecpp::is_strong_enum<weak>::value, false },
			{ corecpp::is_strong_enum<strong>::value, true },
			{ corecpp::is_strong_enum<strong2>::value, true },
		});

		return run(cases, [&](const test& t){
			assert_equal(t.actual, t.expected);
		});
	}

	test_case_result test_alltype() const
	{
		struct test { bool actual; bool expected; };
		enum struct A { val1, val2 };
		enum struct B { val1, val2 };
		enum struct C : uint { val1, val2 };
		test_cases<test> cases ({
			{ corecpp::all_type_v<std::is_enum, A, B, C>, true },
			{ corecpp::all_type_v<std::is_enum, A, B, int>, false },
			{ corecpp::all_type_v<std::is_scalar, A, B, C, int>, true },
			{ corecpp::all_type_v<std::is_scalar, double, A, B, C>, true },
			{ corecpp::all_type_v<std::is_scalar, std::string, A, B, C>, false },
		});

		return run(cases, [&](const test& t){
			assert_equal(t.actual, t.expected);
		});
	}

public:
	tests_type tests() const override
	{
		return {
			{ "test_is_iterable", [&] () { return test_is_iterable(); } },
			{ "test_is_dereferencable", [&] () { return test_is_dereferencable(); } },
			{ "test_is_associative", [&] () { return test_is_associative(); } },
			{ "test_is_time_point", [&] () { return test_is_time_point(); } },
			{ "test_is_strong_enum", [&] () { return test_is_strong_enum(); } },
			{ "test_alltype", [&] () { return test_alltype(); } }
		};
	}

};

int main(int argc, char** argv)
{
	test_unit unit { "Reflection" };
	unit.add_fixture<test_reflection>("test_reflection");

	return unit.run(argc, argv);
};

