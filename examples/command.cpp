#include <iostream>
#include <corecpp/diagnostic.h>
#include <corecpp/cli/command_line.h>
//#include <corecpp/meta/validator.h>

int add(corecpp::command_line& line)
{
	int a, b;
	bool show_help = false;
	corecpp::command_line_parser addition(line);
	addition.add_option('h', "help", "Will compute the addition of 2 values", show_help);
	addition.add_param("a", "first number", a);
	addition.add_param("b", "second number", b);
	addition.parse_options();
	if (show_help)
	{
		addition.usage();
		return EXIT_SUCCESS;
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
	multiplication.add_option({ 'h', "help", "Will compute the product of 2 values", show_help });
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
	division.add_option({ 'h', "help", "Will divide the 1st number by the second", show_help });
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

	try
	{
		division.parse_parameters();
	}
	catch(const std::invalid_argument& e)
	{
		std::cerr << "Invalid argument: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "result is " << (float)a / (float)b << std::endl;

	return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
	unsigned int verbosity = 0;
	bool show_help = false;
	std::vector<std::string> dummy {};

	// corecpp::diagnostic::manager::default_channel().set_level(corecpp::diagnostic::diagnostic_level::debug);
	corecpp::command_line args { argc, argv };
	corecpp::command_line_parser commands { args };
	commands.add_options(
		corecpp::program_option { 'v', "verbose", "Enable verbose", verbosity },
		corecpp::program_option { 'h', "help", "Show help message", show_help }
	);
	commands.add_commands(
		corecpp::program_command { "add", "Add 2 values then write the result", add },
		corecpp::program_command { "mul", "Multiply 2 values then write the result", mul },
		corecpp::program_command { "div", "Divide 2 values then write the result", divise }
	);

	commands.add_param("dummy", "dummy parameter", dummy, true);

	auto res = commands.parse_options();
	if (!res)
	{
		std::cerr << "Invalid argument: " << res.error().what() << std::endl;
		return EXIT_FAILURE;
	}

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

	auto command = commands.parse_command();
	if (command)
		return command();

	commands.parse_parameters();
	if (!dummy.empty())
		std::cout << corecpp::concat<std::string>(dummy, std::string {", " }) << std::endl;
	else
		commands.usage();

	return EXIT_FAILURE;
};

