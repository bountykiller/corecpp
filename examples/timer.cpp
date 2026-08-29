#include <iostream>
#include <vector>
#include <corecpp/cli/command_line.h>
#include <corecpp/io/event.h>

//TODO: add signal managment

struct ticker
{
	ticker(corecpp::ioloop &loop, std::chrono::seconds timeout, bool once) :
		m_loop { loop },
		m_timer {
			m_loop,
			[this, once] (corecpp::timer_event&, uint64_t)
			{
				return tick(once);
			}
		}
	{
		std::cout << "Will tick in " << timeout.count() << " seconds" << std::endl;
		if (once)
			m_timer.expires_in(timeout);
		else
			m_timer.expires_each(timeout);
	}

	void tick(bool once)
	{
		std::cout << "It's time!" << std::endl;
		if (once)
			m_loop.exit();
	}

private:
	corecpp::ioloop &m_loop;
	corecpp::timer_event m_timer;
};

void on_signal(corecpp::ioloop& loop, int signal, pid_t sender)
{
	switch(signal)
	{
		case SIGINT:
		case SIGQUIT:
			std::cout << "Exit signal received, leaving" << std::endl;
			loop.exit();
			break;

		case SIGHUP:
			std::cout << "SIGHUP signal received" << std::endl;
			break;

		default:
			corecpp::unreachable();
	};
}


int main(int argc, char** argv)
{
	corecpp::command_line args { argc, argv };
	corecpp::command_line_parser commands { args };
	bool show_help = false;
	bool once = false;
	commands.add_options(
		corecpp::program_option { 'o', "once", "trigger the timer once then exit", once },
		corecpp::program_option { 'h', "help", "Show help message", show_help });

	std::chrono::seconds timeout;
	commands.add_param("timeout", "timeout in seconds", timeout, false);

	auto res = commands.parse_options();
	if (!res)
	{
		std::cerr << "Invalid argument: " << res.error().what() << std::endl;
		return EXIT_FAILURE;
	}

	if (show_help)
	{
		commands.usage();
		return EXIT_SUCCESS;
	}

	commands.parse_parameters();

	corecpp::ioloop loop;
	ticker tick(loop, timeout, once);
	corecpp::signal_event signals { loop, on_signal };
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGHUP);
	signals.async_wait(mask);

	return loop.run();
};
