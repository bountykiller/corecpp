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
#include <corecpp/net/mailaddress.h>
#include <corecpp/serialization/json.h>
#include <corecpp/serialization/xml.h>
#include <corecpp/flags.h>
#include <corecpp/cli/command_line.h>

enum struct rights {
	read = 1 << 0,
	write = 1 << 1,
	execute = 1 << 2
};

corecpp::enum_map<rights> rights_strings = {
	{ rights::read, "Read" },
	{ rights::write, "Write" },
	{ rights::execute, "Execute" }
};

struct group
{
	int gid;
	std::string name;
	corecpp::flags<rights> permissions;
	std::optional<std::string> comment;

	static const auto& properties()
	{
		static auto result = std::make_tuple(
			corecpp::make_property("id", &group::gid),
			corecpp::make_property("name", &group::name),
			corecpp::make_property("permissions", &group::permissions),
			corecpp::make_property("comment", &group::comment)
		);
		return result;
	}
};
struct user
{
	int uid; /* TODO: use a uuid here */
	std::vector<std::shared_ptr<group>> groups;
	std::string firstname;
	std::string lastname;
	std::unique_ptr<corecpp::mailaddress> email;
	std::chrono::system_clock::time_point creation_date;
	static const auto& properties()
	{
		static auto result = std::make_tuple(
			corecpp::make_property("uid", &user::uid),
			corecpp::make_property("groups", &user::groups),
			corecpp::make_property("firstname", &user::firstname),
			corecpp::make_property("lastname", &user::lastname),
			corecpp::make_property("email", &user::email),
			corecpp::make_property("creation_date", &user::creation_date)
		);
		return result;
	}
};

struct complex_type
{
	double value;
	std::string symbol;
	template <typename SerializerT>
	void serialize(SerializerT& s) const
	{
		s.write_property("name", symbol.c_str());
		s.write_property("value", value);
	}
	template <typename DeserializerT>
	void deserialize(DeserializerT& d, const std::string& property)
	{
		if (property == "value")
			d.deserialize(value);
		else if (property == "name")
			d.deserialize(symbol);
		else
			corecpp::throws<std::runtime_error>(corecpp::concat<std::string>({"invalid property ", property}));
	}
};

int main(int argc, char** argv)
{
	unsigned int verbosity = 0;
	unsigned int number = 1000;
	bool pretty = false;
	bool deserialize = false;
	corecpp::command_line args { argc, argv };
	corecpp::command_line_parser commands { args };
	commands.add_options(
		corecpp::program_option { 'v', "verbose", "enable verbose", verbosity },
		corecpp::program_option { 'n', "number", "number of user to serialize", number },
		corecpp::program_option { 'p', "pretty", "enbale pretty print", pretty },
		corecpp::program_option { 'd', "deserialize", "also bench deserialisation", deserialize }
	);
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


	std::vector<user> users;
	auto g1 = std::make_shared<group>(group {1, "users", rights::read, std::nullopt});
	auto g2 = std::make_shared<group>(group {2, "mygroup", corecpp::make_flags(rights::read)|rights::write, "This is my group"});
	std::cout << "generating users" << std::endl;
	for ( int i = 0; i < number; ++i)
	{
		user u { i, { g1, g2 }, "jeronimo", "masse", {}, std::chrono::system_clock::now() };
		users.emplace_back(std::move(u));
	}

	std::cout << "now serializing" << std::endl;
	//corecpp::xml::serializer s(std::cout, false, true);
	if ( !deserialize )
	{
		corecpp::json::serializer s(std::cout, pretty);
		auto start = std::chrono::system_clock::now();
		s.serialize(users);
		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> diff = end - start;
		std::cout << "done, serialisation of " << number << " users took "
		          << std::setw(6) << diff.count() << " seconds" << std::endl;
	}
	else
	{
		std::ostringstream oss;
		corecpp::json::serializer s(oss, pretty);
		s.serialize(users);

		std::cout << "serialisation done, now deserializing" << std::endl;
		users.clear();
		std::istringstream iss;
		iss.str(oss.str());
		corecpp::json::deserializer d(iss);
		auto start = std::chrono::system_clock::now();
		d.deserialize(users);
		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> diff = end - start;
		std::cout << "done, deserialisation of " << number << " users took "
		          << std::setw(6) << diff.count() << " seconds" << std::endl;
	}

	return 0;
};


