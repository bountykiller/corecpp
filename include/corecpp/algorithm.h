#ifndef CORE_CPP_ALGORITHM_H
#define CORE_CPP_ALGORITHM_H

#include <algorithm>
#include <locale>
#include <type_traits>
#include <unordered_map>
#include <string>


namespace corecpp
{
	template<class ContainerT>
	struct self
	{
		inline auto operator()(ContainerT& t) const
		{ return t; }
	};

	template<class ContainerT>
	struct size
	{
		inline auto operator()(ContainerT& t) const
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
	template<typename ValueT, template<typename T> typename ContainerT = std::initializer_list,
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

	template<typename ValueT, template<typename T> typename ContainerT = std::initializer_list,
			typename ParamT = ContainerT<ValueT>>
	ValueT concat(const ParamT& a)
	{
		ValueT res;
		res.reserve(corecpp::aggregate(a, corecpp::size<ValueT>(), std::plus<std::size_t>()));
		for (auto tmp : a)
			res.append(tmp);

		return res;
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
	
	
	
	template<typename StringT, typename EnumT>
	bool enum_parse(const StringT& str, EnumT& type, const std::unordered_map<EnumT, StringT>& mapping, std::locale l = std::locale())
	{
		auto& comparer = std::use_facet<std::collate<typename StringT::char_type>>(l);
		for (const auto& tmp : mapping)
		{
			if (!comparer.compare(str.data(), str.data() + str.size(),
								tmp->second.data(), tmp->second.data() + tmp->second.size()))
			{
				type = tmp->first;
				return true;
			}
		}
		return false;
	}

}
#endif
