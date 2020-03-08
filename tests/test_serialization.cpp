#include <chrono>
#include <fstream>
#include <functional>
#include <iterator>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#include <corecpp/algorithm.h>
#include <corecpp/flags.h>
#include <corecpp/unittest.h>
#include <corecpp/net/mailaddress.h>
#include <corecpp/serialization/json.h>

using namespace corecpp;

class test_json_serialization final : public test_fixture
{
	template<typename T>
	struct type_test {
		using value_type = T;
		T native;
		std::string str;
	};

	template<typename T>
	test_case_result run_tests(const T& tests) const
	{
		return run(tests, [&](const auto& t){
			typename T::test_type::value_type value;
			std::ostringstream oss;
			corecpp::json::serializer serializer { oss };
			std::istringstream iss { t.str };
			corecpp::json::deserializer deserializer { iss };

			oss = std::ostringstream();
			serializer.serialize(t.native);
			assert_equal(oss.str(), t.str);

			deserializer.deserialize(value);
			assert_equal(value, t.native);
		});
	}

public:
	test_json_serialization()
	{}

	test_case_result test_bool() const
	{
		test_cases<type_test<bool>> tests_bool ({
			{ false, "false" },
			{ true, "true" },
		});

		return run_tests(tests_bool);
	}

	test_case_result test_structured_types() const
	{
		test_cases<type_test<bool>> tests_bool ({
			{ false, "false" },
			{ true, "true" },
		});

		return run_tests(tests_bool);
	}

	test_case_result test_complex_types() const
	{
		test_cases<type_test<bool>> tests_bool ({
			{ false, "false" },
			{ true, "true" },
		});

		return run_tests(tests_bool);
	}

public:
	tests_type tests() const override
	{
		return {
			{ "booleans", [&] () { return test_bool(); } },
			{ "int", [&] () { return test_bool(); } },
			{ "string", [&] () { return test_bool(); } },
			{ "enumerations", [&] () { return test_bool(); } },
			{ "structured types", [&] () { return test_structured_types(); } },
			{ "complex types", [&] () { return test_complex_types(); } }
		};
	}
};

int main(int argc, char** argv)
{
	test_unit unit { "Serialisation" };
	unit.add_fixture<test_json_serialization>("JSON");

	return unit.run(argc, argv);
};

