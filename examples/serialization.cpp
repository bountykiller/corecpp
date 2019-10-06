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
#include <corecpp/serialization/json.h>
#include <corecpp/flags.h>

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
	int uid;
	std::vector<group> groups;
	std::string firstname;
	std::string lastname;
	static const auto& properties()
	{
		static auto result = std::make_tuple(
			corecpp::make_property("uid", &user::uid),
			corecpp::make_property("groups", &user::groups),
			corecpp::make_property("firstname", &user::firstname),
			corecpp::make_property("lastname", &user::lastname)
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
			throw std::runtime_error(corecpp::concat<std::string>({"invalid property ", property}));
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
	corecpp::json::deserializer d(iss);

	iss.str(json.str());
	d.deserialize(groups_rights_copy);
	for (const auto& group_rights : groups_rights_copy)
	{
		std::cout << group_rights.first << " : ";
		for (const auto& right : group_rights.second)
			std::cout << corecpp::etos(right, rights_strings) << " ";
		std::cout << std::endl;
	}
}

int main(int argc, char** argv)
{
	corecpp::diagnostic::manager::default_channel().set_level(corecpp::diagnostic::diagnostic_level::trace);
	static const std::string json_simple = "{\"uid\":1,\"lastname\":\"masse\",\"firstname\":\"jeronimo\",\"groups\":[{\"name\":\"users\",\"id\":1, \"comment\":{}, \"permissions\": { \"value\": 2 }},{\"name\":\"mygroup\",\"id\":2, \"comment\":{ \"value\":\"This is my group\"} }]}";
	std::istringstream iss;

	std::cout << "\nSIMPLE Type:" << std::endl;
	/* store the json into a stream, then deserialize it into usr */
	user usr;
	iss.str(json_simple);
	corecpp::json::deserializer d(iss);
	d.deserialize(usr);
	std::cout << "usr name =>" << usr.firstname << " " << usr.lastname << std::endl;
	std::cout << "Now serialize it!" << std::endl;
	/* construct a serializer which will write the result on stdout */
	corecpp::json::serializer s(std::cout);
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

	return 0;
};

