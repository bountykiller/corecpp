#ifndef CORECPP_COMMAND_LINE_H_
#define CORECPP_COMMAND_LINE_H_

#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <string>


namespace corecpp
{

	class parameter
	{
		public:
			virtual void read_value(const std::string& arg_value) = 0;
	};

	template <typename T>
	class generic_parameter : public parameter
	{
		public:
			typedef T value_type;
			generic_parameter(T& value) : m_value(value)
			{}
			void read_value(const std::string& arg_value)
			{
				if(arg_value.empty()) return;
				std::istringstream iss(arg_value);
				iss >> m_value;
			}
		private:
			T& m_value;
	};

	template <>
	class generic_parameter<bool> : public parameter
	{
		public:
			typedef bool value_type;
			generic_parameter(bool& value) : m_value(value)
			{}
			void read_value(const std::string& arg_value)
			{
				if(arg_value.empty())
					m_value = true;
				else
				{
					std::istringstream iss(arg_value);
					iss >> m_value;
				}
			};
		private:
			bool& m_value;
	};

	template <typename T>
	struct param_descriptor
	{
		char shortname;
		std::string name;
		std::string helpmsg;
		typename generic_parameter<T>::value_type& value;
	};
	template <typename T>
	param_descriptor<T> make_param(char shortname, const std::string& name, const std::string& helpmsg, T& value)
	{
		return { shortname, name, helpmsg, value };
	}
	template <typename T>
	param_descriptor<T> make_param(char shortname, std::string&& name, std::string&& helpmsg, T& value)
	{
		return { shortname, name, helpmsg, value };
	}

	class command_line final
	{
		struct parameter_usage
		{
			char shortname;
			std::string name;
			std::string helpmsg;
			parameter_usage(char s, std::string&& n, std::string&& h)
			: shortname(s), name(n), helpmsg(h)
			{}
		};
		public:
			command_line()
			{}
			template <typename T>
			command_line(param_descriptor<T>&& descriptor)
			{
				add_param(std::forward<param_descriptor<T>>(descriptor));
			}
			template <typename T, typename... Args>
			command_line(param_descriptor<T>&& descriptor, Args&&... args)
			: command_line(std::forward<param_descriptor<T>>(descriptor))
			{
				add_param(std::forward<Args>(args)...);
			}
			~command_line()
			{}
			template <typename T>
			void add_param(param_descriptor<T>&& descriptor)
			{
				m_parameters_by_shortname.emplace(descriptor.shortname, new generic_parameter<T>(descriptor.value));
				m_parameters_by_name.emplace(descriptor.name, new generic_parameter<T>(descriptor.value));
				m_parameters_summary.emplace_back(descriptor.shortname, std::move(descriptor.name), std::move(descriptor.helpmsg));
			}
			template <typename T, typename... Args>
			void add_param(param_descriptor<T>&& descriptor, Args&&... values)
			{
				add_param(std::forward<param_descriptor<T>>(descriptor));
				add_param(std::forward<Args>(values)...);
			}
			void load(int argc, char** argv);
			void usage();
		private:
			std::map<char, std::unique_ptr<parameter>> m_parameters_by_shortname;
			std::map<std::string, std::unique_ptr<parameter>> m_parameters_by_name;
			std::vector<parameter_usage> m_parameters_summary;
	};

}

#endif
