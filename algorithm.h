#ifndef EXPERIMENTALPP_ALGORITHM_H
#define EXPERIMENTALPP_ALGORITHM_H

#include <algorithm>
#include <type_traits>
#include <string>


namespace experimental
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

	template<typename ContainerT, typename ValueT = typename ContainerT::value_type, typename SeparatorT = ValueT>
	ValueT concat(const ContainerT& a, const SeparatorT& separator)
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

	template<typename ContainerT, typename ValueT = typename ContainerT::value_type>
	ValueT concat(const ContainerT& a)
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
		StringT camelString = "";
		auto pos = in.cbegin();
		auto start = pos;
		camelString.reserve(in.size());
		bool shouldBeUpper = true;
		while(pos != in.cend())
		{
			if(!isalnum(*pos))
			{
				if(!shouldBeUpper)
				{
					camelString.append(start, pos);
					shouldBeUpper = true;
				}
				start = ++pos;
			}
			else
			{
				if (isupper(*pos) != shouldBeUpper)
				{
					if(start != pos)
						camelString.append(start, pos);
					camelString += std::toupper(*pos);
					start = ++pos;
				}
				else
					++pos;
				shouldBeUpper = false;
			}
		}
		if(start != pos)
			camelString.append(start, pos);

		return camelString;
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

}
#endif
