#ifndef CORE_CPP_SERIALIZATION_COMMON_H
#define CORE_CPP_SERIALIZATION_COMMON_H

#include <stdexcept>
#include <string>
#include <typeinfo>
#include <type_traits>

#include "../meta/extensions.h"

namespace corecpp
{

	template<typename T, typename SerializerT, typename Enable = void>
	struct is_serializable
	{
		static constexpr bool value = false;
	};
	template<typename T, typename SerializerT>
	struct is_serializable<T, SerializerT,
						typename std::enable_if<
							std::is_void<typename std::result_of<decltype(&T::template serialize<SerializerT>)(T, SerializerT&)>::type
						>::value>::type>
	{
		static constexpr bool value = true;
	};


	template<typename T, typename DeserializerT, typename Enable = void>
	struct is_deserializable
	{
		static constexpr bool value = false;
	};
	template<typename T, typename DeserializerT>
	struct is_deserializable<T, DeserializerT,
							typename std::enable_if<
								std::is_void<typename std::result_of<decltype(&T::template deserialize<DeserializerT>)(T, DeserializerT&, std::string&)>::type
							>::value>::type>
	{
		static constexpr bool value = true;
	};


	template <typename SerializerT, typename ValueT, typename Enable = void>
	struct serialize_impl
	{
	public:
		void operator () (SerializerT& s, ValueT&& value)
		{
			s.write_object(std::forward<ValueT>(value), std::remove_reference<ValueT>::type::properties());
		}
	};
	template <typename SerializerT, typename ValueT>
	struct serialize_impl<SerializerT, ValueT,
						typename std::enable_if<corecpp::is_dereferencable<typename std::decay<ValueT>::type>::value>::type>
	{
		void operator () (SerializerT& s, ValueT&& value)
		{
			if (value)
				s.serialize(*value);
			else
				s.serialize(nullptr);
		}
	};
	template <typename SerializerT, typename ValueT>
	struct serialize_impl<SerializerT, ValueT,
						typename std::enable_if<
							is_serializable<typename std::decay<ValueT>::type, typename std::decay<SerializerT>::type>::value>
						::type>
	{
		void operator () (SerializerT& s, ValueT&& value)
		{
			s.template begin_object<ValueT>();
			value.serialize(s);
			s.end_object();
		}
	};
	template <typename SerializerT, typename ValueT>
	struct serialize_impl<SerializerT, ValueT,
						typename std::enable_if<is_iterable<typename std::decay<ValueT>::type>::value>::type>
	{
		void operator () (SerializerT& s, ValueT&& value)
		{
			s.write_array(std::forward<ValueT>(value));
		}
	};


	template <typename DeserializerT, typename ValueT, typename Enable = void>
	struct deserialize_impl
	{
	public:
		void operator () (DeserializerT& d, ValueT& value)
		{
			d.read_object(value, std::remove_reference<ValueT>::type::properties());
		}
	};
	template <typename DeserializerT, typename ValueT>
	struct deserialize_impl<DeserializerT, ValueT,
	typename std::enable_if<corecpp::is_dereferencable<typename std::decay<ValueT>::type>::value>::type>
	{
		void operator () (DeserializerT& d, ValueT& value)
		{
			value = ValueT(new typename std::remove_reference<decltype(*value)>::type());
			d.deserialize(*value);
		}
	};
	template <typename DeserializerT, typename ValueT>
	struct deserialize_impl<DeserializerT, ValueT,
							typename std::enable_if<
								is_deserializable<typename std::decay<ValueT>::type, typename std::decay<DeserializerT>::type>::value>
							::type>
	{
	public:
		void operator () (DeserializerT& d, ValueT& value)
		{
			d.read_object(value);
		}
	};
	template <typename DeserializerT, typename ValueT>
	struct deserialize_impl<DeserializerT, ValueT,
							typename std::enable_if<is_iterable<typename std::decay<ValueT>::type>::value>::type>
	{
		void operator () (DeserializerT& d, ValueT& value)
		{
			d.read_array(value);
		}
	};


	struct lexical_error : std::invalid_argument
	{
		lexical_error(const char* message)
		: invalid_argument(message)
		{}
		lexical_error(const std::string& message)
		: invalid_argument(message)
		{}
	};

	struct syntax_error : std::invalid_argument
	{
		syntax_error(const char* message)
		: invalid_argument(message)
		{}
		syntax_error(const std::string& message)
		: invalid_argument(message)
		{}
	};

	struct semantic_error : std::invalid_argument
	{
		semantic_error(const char* message)
		: invalid_argument(message)
		{}
		semantic_error(const std::string& message)
		: invalid_argument(message)
		{}
	};
}

#endif
