#ifndef CORE_CPP_UNITTEST_H
#define CORE_CPP_UNITTEST_H

#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

#include <corecpp/except.h>
#include <corecpp/command_line.h>
#include <corecpp/graphics.h>


namespace corecpp
{
	struct assertion_error : error<std::runtime_error>
	{
		template<typename... ArgsT>
		assertion_error(ArgsT&&... args)
		: error<std::runtime_error> { std::forward<ArgsT>(args)... }
		{
		};
	};

	enum struct test_result
	{
		SUCCESS = 0,
		FAILURE = 1,
		FATAL   = 3,
		EXCEPT  = 4,
	};

	struct test_case_result
	{
		test_result value = test_result::SUCCESS;
		std::vector<std::string> errors;

		void dump_errors(std::ostream& oss)
		{
			for (const auto& err : errors)
			{
				oss << "      - " << err << "\n";
			}
		}
	};

	template <typename T>
	class test_cases
	{
		std::vector<T> m_tests;
	public:
		using test_type = T;
		template <typename... ArgsT>
		test_cases(ArgsT&&... args)
		: m_tests { std::forward<ArgsT>(args)... }
		{}
		template <typename... ArgsT>
		test_cases(std::initializer_list<T> list)
		: m_tests { list }
		{}
		auto begin() const
		{ return m_tests.begin(); }
		auto end() const
		{ return m_tests.end(); }
	};

	class test_fixture
	{
		bool m_setted = false;
		mutable test_result m_result;
		mutable std::vector<std::string> m_errors;
		mutable uint32_t m_current_test_index;

		template <typename T, template<typename U> class Op>
		void assert_op(const T& actual, const T& expected) const
		{
			Op<T> op;
			bool res = op(actual, expected);
			if (!res)
			{
				std::ostringstream oss;
				if (m_result < test_result::FAILURE)
					m_result = test_result::FAILURE;
				oss << "Error at test " << m_current_test_index <<  ": " << "actual: " << actual << "\texpected:" << expected;
				m_errors.emplace_back(oss.str());
			}
		}

		template <typename T, template<typename U> class Op>
		void assert_fatal(const T& actual, const T& expected) const
		{
			Op<T> op;
			bool res = op(actual, expected);
			if (!res)
			{
				std::ostringstream oss;
				if (m_result < test_result::FATAL)
					m_result = test_result::FATAL;
				oss << "Fatal at test " << m_current_test_index <<  ": " <<"actual: " << actual << "\texpected:" << expected;
				m_errors.emplace_back(oss.str());
				corecpp::throws<assertion_error>(oss.str());
			}
		}

	protected:
		using test_function = std::function<test_case_result(void)>;
		using tests_type = std::map<std::string, test_function>;

		template <typename T>
		void assert_equal(const T& actual, const T& expected) const
		{
			assert_op<T, std::equal_to>(actual, expected);
		}
		template <typename T>
		void assert_not_equal(const T& actual, const T& expected) const
		{
			assert_op<T, std::not_equal_to>(actual, expected);
		}
		template <typename T>
		void assert_greater(const T& actual, const T& expected) const
		{
			assert_op<T, std::greater>(actual, expected);
		}
		template <typename T>
		void assert_greater_equal(const T& actual, const T& expected) const
		{
			assert_op<T, std::greater_equal>(actual, expected);
		}
		template <typename T>
		void assert_less(const T& actual, const T& expected) const
		{
			assert_op<T, std::less>(actual, expected);
		}
		template <typename T>
		void assert_less_equal(const T& actual, const T& expected) const
		{
			assert_op<T, std::less_equal>(actual, expected);
		}

		template <typename T>
		void fatal_equal(const T& actual, const T& expected) const
		{
			assert_fatal<T, std::equal_to>(actual, expected);
		}
		template <typename T>
		void fatal_not_equal(const T& actual, const T& expected) const
		{
			assert_fatal<T, std::not_equal_to>(actual, expected);
		}
		template <typename T>
		void fatal_greater(const T& actual, const T& expected) const
		{
			assert_fatal<T, std::greater>(actual, expected);
		}
		template <typename T>
		void fatal_greater_equal(const T& actual, const T& expected) const
		{
			assert_fatal<T, std::greater_equal>(actual, expected);
		}
		template <typename T>
		void fatal_less(const T& actual, const T& expected) const
		{
			assert_fatal<T, std::less>(actual, expected);
		}
		template <typename T>
		void fatal_less_equal(const T& actual, const T& expected) const
		{
			assert_fatal<T, std::less_equal>(actual, expected);
		}

		template <typename TestCaseT>
		test_case_result run(const TestCaseT& tests, std::function<void(const typename TestCaseT::test_type&)> fn) const
		{
			m_result = test_result::SUCCESS;
			m_errors.clear();
			m_current_test_index = 0;
			for (const auto& test : tests)
			{
				try {
					fn(test);
				}
				catch(const assertion_error& ae)
				{
				}
				catch(const std::exception& e)
				{
					std::ostringstream oss;

					m_result = test_result::EXCEPT;
					oss << "Exception at test " << m_current_test_index <<  ": " << e.what();
					m_errors.emplace_back(oss.str());
				}
				++m_current_test_index;
			}
			return { m_result, m_errors };;
		}

	public:
		virtual ~test_fixture()
		{
			if (m_setted)
				teardown();
		}

		virtual void setup() { m_setted = true; }
		virtual void reset() { }
		virtual void teardown() { m_setted = false; }
		virtual tests_type tests() const = 0;
	};

	class test_unit
	{
		bool m_verbose = false;
		std::string m_name;
		std::map<std::string, std::unique_ptr<test_fixture>> m_fixtures;

		int run_fixture(const std::string& name, test_fixture& fixture) const
		{
			int res = EXIT_SUCCESS;
			test_case_result result;

			fixture.setup();
			std::cout << name << "\n" << std::string(name.length(), '-') << std::endl;
			for (const auto test : fixture.tests())
			{
				size_t padding = 2;
				if (test.first.length() < 48)
					padding = 64 - test.first.length();
				std::cout << "  Tests: " << test.first;
				try
				{
					fixture.reset();
					result = test.second();
				}
				catch (...)
				{
					result = { test_result::FATAL, { "Uncaught error" } };
				}

				switch (result.value)
				{
					case test_result::SUCCESS:
						std::cout << std::string(padding, '.')
							<< graphic_rendition_v<sgr_p::fg_green> << "SUCCESS"
							<< graphic_rendition_v<sgr_p::all_off> << "\n";
						break;
					case test_result::FAILURE:
						res = EXIT_FAILURE;
						std::cout << std::string(padding, '.')
							<< graphic_rendition_v<sgr_p::fg_yellow> << "FAILURE"
							<< graphic_rendition_v<sgr_p::all_off> << "\n";
						if (m_verbose)
							result.dump_errors(std::cout);
						break;
					case test_result::FATAL:
						res = EXIT_FAILURE;
						std::cout << std::string(padding, '.')
							<< graphic_rendition_v<sgr_p::fg_red> << "FATAL"
							<< graphic_rendition_v<sgr_p::all_off> << "\n";
						result.dump_errors(std::cout);
						break;
					case test_result::EXCEPT:
						res = EXIT_FAILURE;
						std::cout << std::string(padding, '.')
							<< graphic_rendition_v<sgr_p::fg_red> << "!!! EXCEPTION !!!"
							<< graphic_rendition_v<sgr_p::all_off> << "\n";
						result.dump_errors(std::cout);
						break;
				}
			}
			fixture.teardown();
			return res;
		}

	public:
		template<typename T>
		test_unit(T name)
		: m_name { std::forward<T>(name) }
		{}
		test_unit(const test_unit&) = delete;
		test_unit(test_unit&&) = default;
		test_unit& operator=(const test_unit&) = delete;
		test_unit& operator=(test_unit&&) = default;

		template <typename TestFixtureT, typename... ArgsT>
		void add_fixture(const std::string& name, ArgsT&&... args)
		{
			m_fixtures[name] = std::make_unique<TestFixtureT>(std::forward<ArgsT>(args)...);
		}

		int run(int argc, char** argv)
		{
			command_line command { argc, argv };
			command_line_parser parser { command };

			std::cout << "\nSuite " << m_name << "\n" << std::string(m_name.length() + 6, '=')  << "\n\n";

			parser.add_option('v', "verbose", "activate verbose mode", m_verbose);
			for (const auto& f : m_fixtures)
			{
				parser.add_command(f.first, f.first, [&] (command_line& c){
					return run_fixture(f.first, *f.second);
				});
			}
			int res = parser.execute();
			if (res <= 0)
			{
				res = EXIT_SUCCESS;
				for (const auto& f : m_fixtures)
				{
					if (run_fixture(f.first, *f.second) != EXIT_SUCCESS)
						res = EXIT_FAILURE;
				}
			}
			std::cout << std::endl;

			return res;
		}
	};
}

#endif
