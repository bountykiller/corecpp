#ifndef CORECPP_COMMAND_LINE_H_
#define CORECPP_COMMAND_LINE_H_

#include <sstream>
#include <vector>
#include <map>
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

	struct param_descriptor_type
	{
		char shortname;
		std::string name;
		std::string helpmsg;
	};

	class command_line
	{
		public:
			command_line();
			virtual ~command_line();
			template <typename T>
			void add_param( const std::string& name, const char shortname, const std::string& helpmsg,
							typename generic_parameter<T>::value_type& value)
			{
				m_parameters_by_shortname[shortname] = new generic_parameter<T>(value);
				m_parameters_by_name[name] = new generic_parameter<T>(value);
				m_parameters_summary.emplace_back(param_descriptor_type { shortname, name, helpmsg });
			}
			void add_param( const std::string& name, const char shortname, const std::string& helpmsg, parameter* param);
			void load(int argc, char** argv);
			void display_help_message();
		private:
			std::map<char, parameter*> m_parameters_by_shortname;
			std::map<std::string, parameter*> m_parameters_by_name;
			std::vector<param_descriptor_type> m_parameters_summary;
	};

}

#endif
