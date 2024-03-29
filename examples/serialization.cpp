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
	void deserialize(DeserializerT& d, const std::wstring& property)
	{
		if (property == L"value")
			d.deserialize(value);
		else if (property == L"name")
			d.deserialize(symbol);
		else
			corecpp::throws<std::runtime_error>("invalid property");
	}
};

void map_example(void)
{
	static const std::map<int, std::vector<rights>> groups_rights {
		{ 1, { rights::read, rights::write, rights::execute } },
		{ 2, { rights::read, rights::write } },
		{ 3, { rights::read } }
	};

	std::ostringstream json;
	corecpp::json::serializer s(json);
	s.serialize(groups_rights);
	std::cout << json.str() << std::endl;

	std::remove_const_t<decltype(groups_rights)> groups_rights_copy;
	std::istringstream iss;
	iss.str(json.str());
	corecpp::json::deserializer d(iss);
	d.deserialize(groups_rights_copy);

	for (const auto& group_rights : groups_rights_copy)
	{
		std::cout << group_rights.first << " : ";
		for (const auto& right : group_rights.second)
			std::cout << corecpp::etos(right, rights_strings) << " ";
		std::cout << std::endl;
	}
}

void tuple_example(void)
{
	static const std::tuple<int, std::vector<rights>> group_rights =
		std::make_tuple(1, std::vector<rights> { rights::read, rights::write, rights::execute });
	/*
	,
	{ 2, { rights::read, rights::write } },
	{ 3, { rights::read } }
	*/

	std::ostringstream json;
	corecpp::json::serializer s(json);
	s.serialize(group_rights);
	std::cout << json.str() << std::endl;

	std::cout << "Now deserialize it!" << std::endl;
	std::remove_const_t<decltype(group_rights)> group_rights_copy;
	std::istringstream iss;
	iss.str(json.str());
	corecpp::json::deserializer d(iss);
	d.deserialize(group_rights_copy);

	std::cout << "Re-serialize in xml!" << std::endl;
	std::ostringstream xml;
	corecpp::xml::serializer x(xml);
	x.serialize(group_rights_copy);
	std::cout << xml.str() << std::endl;
}

int main(int argc, char** argv)
{
	unsigned int verbosity = 0;
	corecpp::command_line args { argc, argv };
	corecpp::command_line_parser commands { args };
	commands.add_options(
		corecpp::program_option { 'v', "verbose", "enable verbose", verbosity }
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

	static const std::string json_simple =
		"{\"uid\":1,\"lastname\":\"masse\",\"firstname\":\"jeronimo\",\"creation_date\":1570835059013794610,\"groups\":["
			"{ \"value\": {\"name\":\"users\",\"id\":1, \"comment\":{}, \"permissions\": { \"value\": 2 }} },"
			"{ \"value\": {\"name\":\"mygroup\",\"id\":2, \"comment\":{ \"value\":\"This is my group\"} } }"
		"]}";
	std::istringstream iss;

	std::cout << "\nSIMPLE Type:" << std::endl;
	/* store the json into a stream, then deserialize it into usr */
	user usr;
	iss.str(json_simple);
	corecpp::json::deserializer d(iss);
	d.deserialize(usr);
	std::time_t created = std::chrono::system_clock::to_time_t(usr.creation_date);
	std::cout << "usr name =>" << usr.firstname << " " << usr.lastname
		<< " (created on " << std::put_time(std::localtime(&created), "%c %Z") << ")" << std::endl;
	std::cout << "Now serialize it!" << std::endl;
	/* construct a serializer which will write the result on stdout */
	//corecpp::json::serializer s(std::cout, true);
	corecpp::xml::serializer s(std::cerr, false, true);
	/* now deserialize usr */
	s.serialize(usr);

	const std::string json_complex= "{\"name\":\"PI\",\"value\":3.1415926}";

	std::cout << "\n\nCOMPLEX Type:" << std::endl;
	/* store the json into a stream, then deserialize it into c */
	complex_type c;
	iss.str(json_complex);
	corecpp::json::deserializer d2(iss);
	d2.deserialize(c);
	std::cout << "c symbol =>" << c.symbol << " value => " << c.value << std::endl;
	std::cout << "Now serialize it!" << std::endl;
	s.serialize(c);

	std::cout << std::endl << std::endl << "STD::MAP example:" << std::endl;
	map_example();

	std::cout << std::endl << std::endl << "STD::TUPLE example:" << std::endl;
	tuple_example();

	return 0;
};

