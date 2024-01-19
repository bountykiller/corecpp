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


enum struct my_enum
{
	first  = 1,
	second = 2,
	tierce = 4
};

std::ostream& operator << (std::ostream& oss, my_enum e)
{
	return oss << static_cast<std::underlying_type_t<my_enum>>(e);
}

struct structured
{
	int i;
	bool b;
	std::string s;
	bool operator == (const structured& other) const
	{
		return other.i == i && other.b == b && other.s == s;
	};

	static const auto& properties()
	{
		static auto result = std::make_tuple(
			corecpp::make_property("i", &structured::i),
			corecpp::make_property("b", &structured::b),
			corecpp::make_property("str", &structured::s)
		);
		return result;
	}
};

std::ostream& operator << (std::ostream& oss, const structured& e)
{
	return oss << e.i << " | " << e.b << " | " << e.s;
}


struct complex
{
	int real;
	int imag;
	bool operator == (const complex& other) const
	{
		return other.real == real && other.imag == imag;
	};

	template <typename SerializerT>
	void serialize(SerializerT& s) const
	{
		s.write_property("real_part", real);
		s.write_property("imaginary_part", imag);
	}
	template <typename DeserializerT>
	void deserialize(DeserializerT& d, const std::wstring& property)
	{
		if (property == L"real_part")
			d.deserialize(real);
		else if (property == L"imaginary_part")
			d.deserialize(imag);
		else
			throw std::runtime_error("invalid property");
	}
};

std::ostream& operator << (std::ostream& oss, const complex& e)
{
	return oss << e.real << "+" << e.imag << "i";
}

class test_json_serialization final : public test_fixture
{
	template<typename T>
	struct type_test {
		using value_type = T;
		T native;
		std::string str;
	};

	template<typename T>
	test_case_result run_tests(const T& cases) const
	{
		return run(cases, [&](const auto& t){
			typename T::test_type::value_type value;

			std::ostringstream oss;
			corecpp::json::serializer serializer { oss };
			oss = std::ostringstream();
			serializer.serialize(t.native);
			assert_equal(oss.str(), t.str);

			std::istringstream iss { t.str };
			corecpp::json::deserializer deserializer { iss };
			deserializer.deserialize(value);
			assert_equal(value, t.native);
		});
	}

public:
	test_json_serialization()
	{}

	test_case_result test_bool() const
	{
		test_cases<type_test<bool>> cases {
			{ false, "false" },
			{ true, "true" },
		};

		return run_tests(cases);
	}

	test_case_result test_int() const
	{
		test_cases<type_test<int>> cases {
			{ 0, "0" },
			{ 1, "1" },
			{ 10, "10" },
			{ 666, "666" },
			{ 1+1, "2" },
			{ 1-2, "-1" },
			{ -999, "-999" },
		};

		return run_tests(cases);
	}

	test_case_result test_numeric() const
	{
		test_cases<type_test<double>> cases {
			{ 0.0, "0.0" },
			{ 1.0, "1" },
			{ 10000, "10e3" },
			{ 6.99, "6.99" },
			{ 7.09e2, "709" },
			{ 9.07e-1, "0.907" },
			{ -1.5, "-1.5" },
			{ 666.87e-2, "6.6687" },
		};

		return run_tests(cases);
	}

	test_case_result test_str() const
	{
		test_cases<type_test<std::string>> cases {
			{ "", "\"\"" },
			{ "\\", "\"\\\\\"" },
			{ "", "\"\"" },
			{ "a string", "\"a string\"" },
			{ "another string", "\"another string\"" },
			{ "\"another\" string", "\"\\\"another\\\" string\"" },
			{ "12345", "\"12345\"" },
		};

		return run_tests(cases);
	}
	test_case_result test_enum() const
	{

		static const corecpp::enum_map<my_enum> my_enum_strings = {
			{ my_enum::first, "First" },
			{ my_enum::second, "Second" },
			{ my_enum::tierce, "Last" }
		};

		test_cases<type_test<my_enum>> my_enums {
			{ my_enum::first, "1" },
			{ my_enum::second, "2" },
			{ my_enum::tierce, "4" }
		};

		test_cases<type_test<flags<my_enum>>> flags {
			{ my_enum::first,  "{\"value\":1}" },
			{ my_enum::second, "{\"value\":2}" },
			{ my_enum::tierce, "{\"value\":4}" },
			{ my_enum::first | my_enum::second, "{\"value\":3}" },
			{ my_enum::first | my_enum::tierce, "{\"value\":5}" },
			{ my_enum::second| my_enum::tierce, "{\"value\":6}" },
			{ my_enum::first | my_enum::second | my_enum::tierce, "{\"value\":7}" },
		};

		return run_tests(my_enums) + run_tests(flags);
	}

	test_case_result test_structured_types() const
	{
		test_cases<type_test<structured>> cases {
			{ { 0, false, "" }, { "{\"i\":0,\"b\":false,\"str\":\"\"}" } },
			{ { 1, true, "true" }, { "{\"i\":1,\"b\":true,\"str\":\"true\"}" } },
			{ { -1, true, "a string" }, { "{\"i\":-1,\"b\":true,\"str\":\"a string\"}" } },
			{ { -999, false, "'a'" }, { "{\"i\":-999,\"b\":false,\"str\":\"'a'\"}" } },
		};

		return run_tests(cases);
	}

	test_case_result test_complex_types() const
	{
		test_cases<type_test<complex>> cases {
			{ { 0, 0 }, "{\"real_part\":0,\"imaginary_part\":0}" },
			{ { -1, 1 }, "{\"real_part\":-1,\"imaginary_part\":1}" },
			{ { 999, 999 }, "{\"real_part\":999,\"imaginary_part\":999}" },
		};

		return run_tests(cases);
	}

	test_case_result test_array_types() const
	{
		test_cases<type_test<std::vector<int>>> cases {
			{ { }, "[]" },
			{ { 0, 1 }, "[0,1]" },
			{ { -1, 1 }, "[-1,1]" },
			{ { 999, 999 }, "[999,999]" },
			{ { 1, 2, 3, 999, 999 }, "[1,2,3,999,999]" },
		};

		return run_tests(cases);
	}

	test_case_result test_variant() const
	{
		using variant_type = corecpp::variant<int, double, std::string>;
		test_cases<type_test<variant_type>> cases {
			{ { }, "{\"-1\":null}" },
			{ { 1 }, "{\"0\":1}" },
			{ { -1.1 }, "{\"1\":-1.100000}" },
			{ std::string { "test" }, "{\"2\":\"test\"}" },
			{ std::string { "" }, "{\"2\":\"\"}" },
		};

		return run_tests(cases);
	}

	test_case_result test_tuple() const
	{
		using pair_type = std::tuple<bool, char>;
		test_cases<type_test<pair_type>> pair_cases {
			{ { false, 0 }, "{\"0\":false,\"1\":0}" },
			{ { true, 'a' }, "{\"0\":true,\"1\":97}" }
		};

		using tuple_type = std::tuple<int, double, std::string>;
		test_cases<type_test<tuple_type>> tuple_cases {
			{ { 0, 0, "" }, "{\"0\":0,\"1\":0.000000,\"2\":\"\"}" },
			{ { 1, 1.0, "hello" }, "{\"0\":1,\"1\":1.000000,\"2\":\"hello\"}" },
		};

		return run_tests(pair_cases) + run_tests(tuple_cases);
	}

public:
	tests_type tests() const override
	{
		return {
			{ "booleans", [&] () { return test_bool(); } },
			{ "int", [&] () { return test_int(); } },
			{ "numeric", [&] () { return test_numeric(); } },
			{ "string", [&] () { return test_str(); } },
			{ "enumerations", [&] () { return test_enum(); } },
			{ "structured_types", [&] () { return test_structured_types(); } },
			{ "complex_types", [&] () { return test_complex_types(); } },
			{ "array_types", [&] () { return test_array_types(); } },
			{ "variant", [&] () { return test_variant(); } },
			{ "tuple", [&] () { return test_tuple(); } },
		};
	}
};

int main(int argc, char** argv)
{
	test_unit unit { "Serialisation" };
	/* corecpp::diagnostic::manager::default_channel().set_level(corecpp::diagnostic::diagnostic_level::debug); */
	unit.add_fixture<test_json_serialization>("JSON");

	return unit.run(argc, argv);
};

