#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <vector>
#include <typeinfo>

#include <corecpp/algorithm.h>
#include <corecpp/flags.h>
#include <corecpp/unittest.h>

using namespace corecpp;

class test_operators final : public test_fixture
{
	enum weak
	{
		A = 1 << 0,
		B = 1 << 1,
		C = 1 << 2,
		D = 1 << 3,
		ALL = A | B | C | D,
	};
	enum struct strong
	{
		A = 1 << 0,
		B = 1 << 1,
		C = 1 << 2,
		D = 1 << 3,
	};
	enum struct ustrong : uint16_t
	{
		A = 1 << 0,
		B = 1 << 1,
		C = 1 << 2,
		D = 1 << 3,
	};

	test_case_result test_and() const
	{
		struct test_weak
		{
			weak first; weak second;
			const std::type_info &expected_type;
			int expected;
		};
		test_cases<test_weak> tests_weak ({
			{ A, A, typeid(int), A },
			{ A, B, typeid(int), A & B },
			{ A, C, typeid(int), A & C },
			{ C, D, typeid(int), A & D },
			{ A, ALL, typeid(int), A & ALL },
		});

		struct test_strong
		{
			strong first; strong second;
			const std::type_info &expected_type;
			int expected;
		};
		test_cases<test_strong> tests_strong ({
			{ strong::A, strong::A, typeid(corecpp::flags<strong>), static_cast<int>(A) },
			{ strong::A, strong::B, typeid(corecpp::flags<strong>), static_cast<int>(A & B) },
			{ strong::A, strong::C, typeid(corecpp::flags<strong>), static_cast<int>(A & C) },
			{ strong::C, strong::D, typeid(corecpp::flags<strong>), static_cast<int>(C & D) }
		});

		struct test_ustrong
		{
			ustrong first; ustrong second;
			const std::type_info &expected_type;
			uint16_t expected;
		};
		test_cases<test_ustrong> tests_ustrong ({
			{ ustrong::A, ustrong::A, typeid(corecpp::flags<ustrong>), static_cast<uint16_t>(A) },
			{ ustrong::A, ustrong::B, typeid(corecpp::flags<ustrong>), static_cast<uint16_t>(A & B) },
			{ ustrong::A, ustrong::C, typeid(corecpp::flags<ustrong>), static_cast<uint16_t>(A & C) },
			{ ustrong::C, ustrong::D, typeid(corecpp::flags<ustrong>), static_cast<uint16_t>(C & D) }
		});

		return run(tests_weak, [&](const test_weak& t){
			assert_equal(typeid(decltype(t.first & t.second)).name(), t.expected_type.name());
			assert_equal(t.first & t.second, t.expected);
		}) + run(tests_strong, [&](const test_strong& t){
			assert_equal(typeid(decltype(t.first & t.second)).name(), t.expected_type.name());
			assert_equal((t.first & t.second).get(), t.expected);
		}) + run(tests_ustrong, [&](const test_ustrong& t){
			assert_equal(typeid(decltype(t.first & t.second)).name(), t.expected_type.name());
			assert_equal((t.first & t.second).get(), t.expected);
		});
	}

	test_case_result test_or() const
	{
		struct test_weak
		{
			weak first; weak second;
			const std::type_info &expected_type;
			int expected;
		};
		test_cases<test_weak> tests_weak ({
			{ A, A, typeid(int), A },
			{ A, B, typeid(int), A | B },
			{ A, C, typeid(int), A | C },
			{ C, D, typeid(int), C | D },
			{ A, ALL, typeid(int), A | ALL },
		});

		struct test_strong
		{
			strong first; strong second;
			const std::type_info &expected_type;
			int expected;
		};
		test_cases<test_strong> tests_strong ({
			{ strong::A, strong::A, typeid(corecpp::flags<strong>), static_cast<int>(A) },
			{ strong::A, strong::B, typeid(corecpp::flags<strong>), static_cast<int>(A | B) },
			{ strong::A, strong::C, typeid(corecpp::flags<strong>), static_cast<int>(A | C) },
			{ strong::C, strong::D, typeid(corecpp::flags<strong>), static_cast<int>(C | D) }
		});

		struct test_ustrong
		{
			ustrong first; ustrong second;
			const std::type_info &expected_type;
			uint16_t expected;
		};
		test_cases<test_ustrong> tests_ustrong ({
			{ ustrong::A, ustrong::A, typeid(corecpp::flags<ustrong>), static_cast<uint16_t>(A) },
			{ ustrong::A, ustrong::B, typeid(corecpp::flags<ustrong>), static_cast<uint16_t>(A | B) },
			{ ustrong::A, ustrong::C, typeid(corecpp::flags<ustrong>), static_cast<uint16_t>(A | C) },
			{ ustrong::C, ustrong::D, typeid(corecpp::flags<ustrong>), static_cast<uint16_t>(C | D) }
		});

		return run(tests_weak, [&](const test_weak& t){
			assert_equal(typeid(decltype(t.first & t.second)).name(), t.expected_type.name());
			assert_equal(t.first | t.second, t.expected);
		}) + run(tests_strong, [&](const test_strong& t){
			assert_equal(typeid(decltype(t.first & t.second)).name(), t.expected_type.name());
			assert_equal((t.first | t.second).get(), t.expected);
		}) + run(tests_ustrong, [&](const test_ustrong& t){
			assert_equal(typeid(decltype(t.first & t.second)).name(), t.expected_type.name());
			assert_equal((t.first | t.second).get(), t.expected);
		});
	}
public:
	tests_type tests() const override
	{
		return {
			{ "test_and", [&] () { return test_and(); } },
			{ "test_or", [&] () { return test_or(); } },
		};
	}

};

int main(int argc, char** argv)
{
	test_unit unit { "Flags" };
	unit.add_fixture<test_operators>("test_operators");

	return unit.run(argc, argv);
};



