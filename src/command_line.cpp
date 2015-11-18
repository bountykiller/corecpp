/*
 * command_line.cpp
 *
 *  Created on: May 14, 2010
 *      Author: bountykiller
 */
#include <iostream>
#include <corecpp/command_line.h>

namespace corecpp
{

command_line::command_line()
{}

command_line::~command_line()
{}

void command_line::load(int argc, char** argv)
{
	int i = 1;
	while(i < argc)
	{
		std::string param_name(argv[i]);
		i++;
		if(param_name.substr(0,2) == "--")
		{
			std::string param_value = (i == argc) ? "" : argv[i];
			param_name = param_name.substr(2);
			if(param_value[0] == '-')
				param_value = ""; //param with no value => value is set to empty_string
			else
				i++; //param with value => must skip one string
			std::map<std::string, parameter*>::const_iterator param = m_parameters_by_name.find(param_name);
			if(param == m_parameters_by_name.end())
				throw std::invalid_argument(param_name);
			param->second->read_value(param_value);
		}
		else if(param_name[0] == '-')
		{
			param_name = param_name.substr(1);
			std::string param_value = (i == argc) ? "" : argv[i];
			if(param_value[0] == '-')
				param_value = "";
			else
				i++;
			//for loop because we can have multiple params
			for(std::string::const_iterator iter = param_name.begin();
				iter != param_name.end(); )
			{
				char shortname = *iter;
				std::map<char, parameter*>::const_iterator param = m_parameters_by_shortname.find(shortname);
				if(param == m_parameters_by_shortname.end())
					throw std::invalid_argument(std::string(1, shortname));
				if(++iter == param_name.end())//last param => must give the value to it
					param->second->read_value(param_value);
				else //not the last parameter => we suppose he doesn't need a value
					param->second->read_value("");
			}
		}
		/**
		else
		{
			for now, do nothing
		}
		*/
	}
}

void command_line::display_help_message()
{
	for (const auto& sumary : m_parameters_summary)
	{
		if(!sumary.shortname)
			std::cout << "\t--" << sumary.name << '\t' << sumary.helpmsg << std::endl;
		else
			std::cout << "\t-" << sumary.shortname << ", --" << sumary.name << '\t' << sumary.helpmsg << std::endl;
	}
}

}
