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

struct test_reflection final : public test_fixture
{
	test_case_result test_is_iterable() const
	{
		struct test { bool actual; bool expected; };
		test_cases<test> tests ({
			{ corecpp::is_iterable<std::vector<int>>::value, true },
			{ corecpp::is_iterable<std::string>::value, true },
			{ corecpp::is_iterable<std::optional<int>>::value, false },
			{ corecpp::is_iterable<std::unique_ptr<std::string>>::value, false },
			{ corecpp::is_iterable<int*>::value, false }
		});

		return run(tests, [&](const test& t){
			assert_equal(t.actual, t.expected);
		});
	}

	test_case_result test_is_dereferencable() const
	{
		struct test { bool actual; bool expected; };
		test_cases<test> tests ({
			{ corecpp::is_dereferencable<std::vector<int>>::value, false },
			{ corecpp::is_dereferencable<std::string>::value, false },
			{ corecpp::is_dereferencable<std::optional<int>>::value, true },
			{ corecpp::is_dereferencable<std::unique_ptr<std::string>>::value, true },
			{ corecpp::is_dereferencable<int*>::value, true }
		});

		return run(tests, [&](const test& t){
			assert_equal(t.actual, t.expected);
		});
	}

	test_case_result test_is_associative() const
	{
		struct test { bool actual; bool expected; };
		test_cases<test> tests ({
			{ corecpp::is_associative<std::vector<int>>::value, false },
			{ corecpp::is_associative<std::string>::value, false },
			{ corecpp::is_associative<std::optional<int>>::value, false },
			{ corecpp::is_associative<std::unique_ptr<std::string>>::value, false },
			{ corecpp::is_associative<int*>::value, false }
		});

		return run(tests, [&](const test& t){
			assert_equal(t.actual, t.expected);
		});
	}

	test_case_result test_is_time_point() const
	{
		struct test { bool actual; bool expected; };
		test_cases<test> tests ({
			{ corecpp::is_time_point<std::vector<int>>::value, false },
			{ corecpp::is_time_point<std::string>::value, false },
			{ corecpp::is_time_point<std::optional<int>>::value, false },
			{ corecpp::is_time_point<std::unique_ptr<std::string>>::value, true },
			{ corecpp::is_time_point<int*>::value, false },
			{ corecpp::is_time_point<std::chrono::steady_clock::time_point>::value, true }
		});

		return run(tests, [&](const test& t){
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
			{ "test_is_time_point", [&] () { return test_is_time_point(); } }
		};
	}

};

int main(int argc, char** argv)
{
	test_unit unit { "Reflection" };
	unit.add_fixture<test_reflection>("test_reflection");

	return unit.run(argc, argv);
};

