#ifndef CORE_CPP_UNITTEST_H
#define CORE_CPP_UNITTEST_H

#include <cstring>

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

#include <corecpp/except.h>

#include <corecpp/cli/command_line.h>
#include <corecpp/cli/graphics.h>

#include <corecpp/serialization/json.h>


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

		void dump_errors(std::ostream& oss) const
		{
			for (const auto& err : errors)
			{
				oss << "      - " << err << "\n";
			}
		}

		void print(bool verbose, size_t padding)
		{
			switch (value)
			{
				case test_result::SUCCESS:
					std::cout << std::string(padding, '.')
						<< graphic_rendition_v<sgr_p::fg_green> << "SUCCESS"
						<< graphic_rendition_v<sgr_p::all_off> << "\n";
					break;
				case test_result::FAILURE:
					std::cout << std::string(padding, '.')
						<< graphic_rendition_v<sgr_p::fg_yellow> << "FAILURE"
						<< graphic_rendition_v<sgr_p::all_off> << "\n";
					if (verbose)
						dump_errors(std::cout);
					break;
				case test_result::FATAL:
					std::cout << std::string(padding, '.')
						<< graphic_rendition_v<sgr_p::fg_red> << "FATAL"
						<< graphic_rendition_v<sgr_p::all_off> << "\n";
					if (verbose)
						dump_errors(std::cout);
					break;
				case test_result::EXCEPT:
					std::cout << std::string(padding, '.')
						<< graphic_rendition_v<sgr_p::fg_red> << "!!! EXCEPTION !!!"
						<< graphic_rendition_v<sgr_p::all_off> << "\n";
					if (verbose)
						dump_errors(std::cout);
					break;
			}
		}

		test_case_result operator + (test_case_result&& other) const
		{
			test_case_result res;
			std::copy(errors.begin(), errors.end(), std::back_inserter(res.errors));
			std::move(other.errors.begin(), other.errors.end(), std::back_inserter(res.errors));
			res.value = std::max(value, other.value);
			return res;
		}

		test_case_result operator + (const test_case_result& other) const
		{
			test_case_result res;
			std::copy(errors.begin(), errors.end(), std::back_inserter(res.errors));
			std::copy(other.errors.begin(), other.errors.end(), std::back_inserter(res.errors));
			res.value = std::max(value, other.value);
			return res;
		}

		test_case_result& operator += (test_case_result&& other)
		{
			std::move(other.errors.begin(), other.errors.end(), std::back_inserter(errors));
			value = std::max(value, other.value);
			return *this;
		}

		test_case_result& operator += (const test_case_result& other)
		{
			std::copy(other.errors.begin(), other.errors.end(), std::back_inserter(errors));
			value = std::max(value, other.value);
			return *this;
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

		template <typename T>
		void register_failure(const T& actual, const T& expected) const
		{
			std::ostringstream oss;
			corecpp::json::serializer serializer { oss };

			if (m_result < test_result::FAILURE)
				m_result = test_result::FAILURE;
			oss << "Error at index " << m_current_test_index <<  ": " << "actual: ";
			serializer.serialize(actual);
			oss << "\texpected:";
			serializer.serialize(expected);
			logger().fail(oss.str(), __FILE__, __LINE__);
			m_errors.emplace_back(oss.str());
		}

		template <typename T, template<typename U> class Op>
		void assert_op(const T& actual, const T& expected) const
		{
			Op<T> op;
			bool res = op(actual, expected);
			if (!res)
				register_failure(actual, expected);
			else
				logger().success("", [this]{ return corecpp::concat<std::string>({"Success at index ", std::to_string(m_current_test_index)}); }, __FILE__, __LINE__);
		}

		template <typename T, template<typename U> class Op>
		void assert_fatal(const T& actual, const T& expected) const
		{
			Op<T> op;
			bool res = op(actual, expected);
			if (!res)
			{
				std::ostringstream oss;
				corecpp::json::serializer serializer { oss };
				if (m_result < test_result::FATAL)
					m_result = test_result::FATAL;
				oss << "Fatal at index " << m_current_test_index <<  ": " << "actual: ";
				serializer.serialize(actual);
				oss << "\texpected:";
				serializer.serialize(expected);
				logger().fatal(oss.str(), __FILE__, __LINE__);
				m_errors.emplace_back(oss.str());

				corecpp::throws<assertion_error>(oss.str());
			}
			else
				logger().success("", [this]{ return corecpp::concat<std::string>({"Success at index ", std::to_string(m_current_test_index)}); }, __FILE__, __LINE__);
		}

	protected:
		using test_function = std::function<test_case_result(void)>;
		using tests_type = std::map<std::string, test_function>;

		static corecpp::diagnostic::event_producer& logger();

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
		void assert_str_equal(const T* actual, const T* expected) const
		{
			assert_op<const T*, corecpp::str_equal_to>(actual, expected);
		}
		template <typename T>
		void assert_str_not_equal(const T* actual, const T* expected) const
		{
			assert_op<const T*, corecpp::str_not_equal_to>(actual, expected);
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
		template <typename T>
		void fatal_str_equal(const T* actual, const T* expected) const
		{
			assert_fatal<const T*, corecpp::str_equal_to>(actual, expected);
		}
		template <typename T>
		void fatal_str_not_equal(const T* actual, const T* expected) const
		{
			assert_fatal<const T*, corecpp::str_not_equal_to>(actual, expected);
		}
		template <typename ExceptionT>
		void assert_throws(std::function<void(void)> f) const
		{
			try {
				f();
				register_failure<std::string>("no exception thrown", "exception thrown");
			}
			catch(const ExceptionT&)
			{}
			catch(...)
			{
				/* compiler-dependent stuff */
				int status;
				std::unique_ptr<char> name { abi::__cxa_demangle(typeid(ExceptionT).name(), 0, 0, &status) };
				register_failure<std::string>("wrong exception thrown", name.get());
			}
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
					logger().info("Running test ", std::to_string(m_current_test_index) , __FILE__, __LINE__);
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
					logger().error(oss.str(), __FILE__, __LINE__);
					m_errors.emplace_back(oss.str());
				}
				++m_current_test_index;
			}
			return { m_result, m_errors };;
		}

		int run(const std::string& name, test_function test, bool verbose)
		{
			test_case_result result;
			size_t padding = 2;
			if (name.length() < 48)
				padding = 64 - name.length();

			std::cout << "  Tests: " << name;
			try
			{
				reset();
				result = test();
			}
			catch (...)
			{
				result = { test_result::FATAL, { "Uncaught error" } };
			}
			result.print(verbose, padding);

			return (result.value == test_result::SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE;
		}

	public:
		virtual ~test_fixture()
		{
			if (m_setted)
				teardown();
		}


		int run(const std::string& name, bool verbose)
		{
			int res = EXIT_SUCCESS;

			setup();
			std::cout << name << "\n" << std::string(name.length(), '-') << std::endl;
			for (const auto& test : tests())
			{
				int test_res = run(test.first, test.second, verbose);
				if (test_res == EXIT_FAILURE)
					res = EXIT_FAILURE;
			}
			teardown();
			return res;
		}


		int run(command_line& c, const std::string& name, bool verbose)
		{
			bool help = false;
			command_line_parser parser { c };

			parser.add_option('h', "help", "show help message", help);
			for (const auto& t : tests())
			{
				parser.add_command(t.first, corecpp::concat<std::string>({ "Run test '", t.first, "'"}),
					[this, name {t.first}, method {t.second}, verbose] ([[maybe_unused]] command_line& c)
					{
						setup();
						int res = run(name, method, verbose);
						teardown();
						return res;
					});
			}

			auto res = parser.parse_options();
			if ( !res )
			{
				std::cout  << graphic_rendition_v<sgr_p::fg_red> << res.error().what() << graphic_rendition_v<sgr_p::all_off> << std::endl;
				parser.usage();
				return EXIT_FAILURE;
			}

			if (help)
			{
				parser.usage();
				return EXIT_SUCCESS;
			}

			int ret;
			auto cmd = parser.parse_command();
			if (!cmd) /* no commands found */
			{
				ret = run(name, verbose);
			}
			else
				ret = cmd();
			std::cout << std::endl;

			return ret;
		}

		virtual void setup() { m_setted = true; }
		virtual void reset() { }
		virtual void teardown() { m_setted = false; }
		virtual tests_type tests() const = 0;
	};

	class test_unit final
	{
		bool m_verbose = false;
		std::string m_name;
		std::map<std::string, std::unique_ptr<test_fixture>> m_fixtures;

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
			command_line cmd_line { argc, argv };
			command_line_parser parser { cmd_line };
			diagnostic::diagnostic_level dbg_lvl = diagnostic::diagnostic_level::fatal;
			bool help = false;

			parser.add_option('v', "verbose", "activate verbose mode", m_verbose);
			parser.add_option('h', "help", "show help message", help);
			parser.add_option('d', "debug", "debug level [default:info]", dbg_lvl);
			for (const auto& f : m_fixtures)
			{
				parser.add_command(f.first,
					corecpp::concat<std::string>({ "Run test '", f.first, "'"}),
					[&] (command_line& c){
						return f.second->run(c, f.first, m_verbose);
					});
			}

			auto res = parser.parse_options();
			if ( !res )
			{
				std::cout << graphic_rendition_v<sgr_p::fg_red> << res.error().what() << graphic_rendition_v<sgr_p::all_off> << std::endl;
				parser.usage();
				return EXIT_FAILURE;
			}

			diagnostic::manager::default_channel().set_level(dbg_lvl);
			if (help)
			{
				parser.usage();
				return EXIT_SUCCESS;
			}

			std::cout << "\nSuite " << m_name << "\n" << std::string(m_name.length() + 6, '=')  << "\n\n";
			int ret;
			auto cmd = parser.parse_command();
			if (!cmd) /* no commands found */
			{
				ret = EXIT_SUCCESS;
				for (const auto& f : m_fixtures)
				{
					if (f.second->run(f.first, m_verbose) != EXIT_SUCCESS)
						ret = EXIT_FAILURE;
				}
			}
			else
				ret = cmd();
			std::cout << std::endl;

			return ret;
		}
	};


corecpp::diagnostic::event_producer& test_fixture::logger()
{
	static auto& channel = corecpp::diagnostic::manager::get_channel("test_fixture");
	static auto logger = corecpp::diagnostic::event_producer(channel);
	return logger;
}

}

#endif
