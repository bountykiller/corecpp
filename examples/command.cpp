#include <iostream>
#include <corecpp/diagnostic.h>
#include <corecpp/command_line.h>
//#include <corecpp/meta/validator.h>

int add(corecpp::command_line& line)
{
	int a, b;
	bool show_help = false;
	corecpp::command_line_parser addition(line);
	addition.add_option('h', "help", "show help message", show_help);
	addition.add_param("a", "first number", a);
	addition.add_param("b", "second number", b);
	addition.parse_options();
	if (show_help)
	{
		addition.usage();
		return show_help ? EXIT_SUCCESS : EXIT_FAILURE;
	}
	addition.parse_parameters();

	std::cout << "result is " << a + b << std::endl;

	return EXIT_SUCCESS;
}

int mul(corecpp::command_line& line)
{
	int a, b, c = 1;
	bool show_help = false;
	corecpp::command_line_parser multiplication(line);
	multiplication.add_option({ 'h', "help", "show help message", show_help });
	multiplication.add_params(
			corecpp::program_parameter { "a", "first number", a, },
			corecpp::program_parameter { "b", "second number", b },
			corecpp::program_parameter { "c", "third number", c, true }
	);

	multiplication.parse_options();
	if (show_help)
	{
		multiplication.usage();
		return EXIT_SUCCESS;
	}
	multiplication.parse_parameters();

	std::cout << "result is " << a * b * c << std::endl;

	return EXIT_SUCCESS;
}

int divise(corecpp::command_line& line)
{
	int a, b;
	bool show_help = false;
	corecpp::command_line_parser division(line);
	division.add_option({ 'h', "help", "show help message", show_help });
	division.add_params(
		corecpp::program_parameter { "a", "first number", a },
		corecpp::program_parameter { "b", "second number (should be != 0)", b/*, corecpp::is_not_equal(0)*/}
	);

	division.parse_options();
	if (show_help)
	{
		division.usage();
		return EXIT_SUCCESS;
	}
	division.parse_parameters();

	std::cout << "result is " << (float)a / (float)b << std::endl;

	return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
	unsigned int verbosity = 0;
	bool show_help = false;
	corecpp::diagnostic::manager::default_channel().set_level(corecpp::diagnostic::diagnostic_level::debug);
	corecpp::command_line args { argc, argv };
	corecpp::command_line_parser commands { args };
	commands.add_options(
		corecpp::program_option { 'v', "verbose", "enable verbose", verbosity },
		corecpp::program_option { 'h', "help", "show help message", show_help }
	);
	commands.add_commands(
		corecpp::program_command { "add", "Add 2 values then write the result", add },
		corecpp::program_command { "mul", "Multiply 2 values then write the result", mul },
		corecpp::program_command { "div", "Divide 2 values then write the result", divise }
	);

	commands.parse_options();

	if (verbosity >= 3)
		corecpp::diagnostic::manager::default_channel().set_level(corecpp::diagnostic::diagnostic_level::debug);
	else if (verbosity == 2)
		corecpp::diagnostic::manager::default_channel().set_level(corecpp::diagnostic::diagnostic_level::trace);
	else if (verbosity == 1)
		corecpp::diagnostic::manager::default_channel().set_level(corecpp::diagnostic::diagnostic_level::info);
	else
		corecpp::diagnostic::manager::default_channel().set_level(corecpp::diagnostic::diagnostic_level::success);

	if (show_help)
	{
		commands.usage();
		return EXIT_SUCCESS;
	}

	int ret_code = commands.execute();
	if (ret_code >= 0)
		return ret_code;

	commands.usage();
	return EXIT_SUCCESS;
};

