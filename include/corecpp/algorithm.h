#ifndef CORE_CPP_ALGORITHM_H
#define CORE_CPP_ALGORITHM_H

#include <algorithm>
#include <initializer_list>
#include <locale>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <string>
#include <mutex>
#include <utility>


namespace corecpp
{
	template<class ContainerT>
	struct self
	{
		inline ContainerT& operator()(ContainerT& t) const
		{ return t; }
	};

	template<class ContainerT>
	struct size
	{
		inline std::size_t operator()(ContainerT& t) const
		{ return t.size(); }
	};

	template<typename ContainerT, typename TransformerT, typename AccumulatorT, typename ResultT = typename AccumulatorT::result_type>
	ResultT aggregate(ContainerT& container, TransformerT transform, AccumulatorT accumul, ResultT init = {})
	{
		for (auto t : container)
			accumul(init, transform(t));
		return init;
	}
	/*
	template<typename ContainerT, typename ValueT = typename ContainerT::value_type, typename SeparatorT = ValueT>
	*/
	template<typename ValueT, template<typename T> class ContainerT = std::initializer_list,
			typename SeparatorT = ValueT, typename ParamT = ContainerT<ValueT>>
	ValueT concat(const ParamT& a, const SeparatorT& separator)
	{
		ValueT res;
		res.reserve(corecpp::aggregate(a, corecpp::size<ValueT>(), std::plus<std::size_t>()) + separator.size() * a.size());
		bool first = true;
		for (auto tmp : a)
		{
			if (!first)
				res.append(separator);
			else
				first = false;
			res.append(tmp);
		}

		return res;
	}

	template<typename ValueT, template<typename T> class ContainerT = std::initializer_list,
			typename ParamT = ContainerT<ValueT>>
	ValueT concat(const ParamT& a)
	{
		ValueT res;
		res.reserve(corecpp::aggregate(a, corecpp::size<ValueT>(), std::plus<std::size_t>()));
		for (auto tmp : a)
			res.append(tmp);

		return res;
	}

	template <typename ContainerT>
	bool starts_with(const ContainerT& haystack, const ContainerT& needle)
	{
		if (haystack.size() < needle.size())
			return false;
		auto pos = haystack.begin();
		auto el = needle.begin();
		while (el != needle.end())
		{
			if (*el != *pos)
				return false;
			++el; ++pos;
		}
		return true;
	}

	template <typename ContainerT>
	bool ends_with(const ContainerT& haystack, const ContainerT& needle)
	{
		if (haystack.size() < needle.size())
			return false;
		auto pos = haystack.rbegin();
		auto el = needle.rbegin();
		while (el != needle.rend())
		{
			if (*el != *pos)
				return false;
			++el; ++pos;
		}
		return true;
	}

	template<typename StringT>
	StringT toPascalCase(const StringT& in)
	{
		StringT PascalString = "";
		auto pos = in.cbegin();
		auto start = pos;
		PascalString.reserve(in.size());
		bool shouldBeUpper = true;
		while(pos != in.cend())
		{
			if(!isalnum(*pos))
			{
				if(!shouldBeUpper)
				{
					PascalString.append(start, pos);
					shouldBeUpper = true;
				}
				start = ++pos;
			}
			else
			{
				if (isupper(*pos) != shouldBeUpper)
				{
					if(start != pos)
						PascalString.append(start, pos);
					PascalString += std::toupper(*pos);
					start = ++pos;
				}
				else
					++pos;
				shouldBeUpper = false;
			}
		}
		if(start != pos)
			PascalString.append(start, pos);

		return PascalString;
	}

	template<typename StringT>
	StringT toCamelCase(const StringT& in)
	{
		//yes, it's the best implementation I found :)
		StringT res = toPascalCase(in);
		if(res.length() > 0)
			res[0] = std::tolower(res[0]);
		return std::move(res);
	}

	template<typename EnumT>
	using enum_map = std::initializer_list<std::pair<EnumT, std::string>>;

	template<typename EnumT, typename MapT = enum_map<EnumT>>
	static const std::string& etos(EnumT value, const MapT& mapping)
	{
		for (const auto& tmp : mapping)
		{
			if (value == tmp.first)
				return tmp.second;
		}
		throw std::logic_error("Unknow enum value");
	}

	template<typename StringT, typename EnumT, typename MapT = enum_map<EnumT>>
	static bool stoe(const StringT& str, EnumT& value, const MapT& mapping, std::locale l = std::locale())
	{
		auto& comparer = std::use_facet<std::collate<typename StringT::value_type>>(l);
		for (const auto& tmp : mapping)
		{
			if (!comparer.compare(str.data(), str.data() + str.size(),
								tmp.second.data(), tmp.second.data() + tmp.second.size()))
			{
				value = tmp.first;
				return true;
			}
		}
		return false;
	}
}
#endif
