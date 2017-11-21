#include <list>
#include <type_traits>
#include <iostream>
#include <functional>
#include <utility>
#include <tuple>
#include <set>
#include <vector>
#include <memory>
#include <iterator>
#include <typeinfo>
#include <fstream>

#include <corecpp/algorithm.h>
#include <corecpp/serialization/json.h>

struct group
{
	int gid;
	std::string name;
	static const auto& properties()
	{
		static auto result = std::make_tuple(
			corecpp::make_property("id", &group::gid),
			corecpp::make_property("name", &group::name)
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

int main(int argc, char** argv)
{
	const std::string json_simple = "{\"uid\":1,\"lastname\":\"masse\",\"firstname\":\"jeronimo\",\"groups\":[{\"name\":\"users\",\"id\":1},{\"name\":\"mygroup\",\"id\":2}]}";
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

	return 0;
};

