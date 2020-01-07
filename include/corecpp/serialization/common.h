#ifndef CORE_CPP_SERIALIZATION_COMMON_H
#define CORE_CPP_SERIALIZATION_COMMON_H

#include <stdexcept>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <locale>
#include <codecvt>

#include <corecpp/meta/extensions.h>

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
						typename std::enable_if<corecpp::is_dereferencable<std::decay_t<ValueT>>::value>::type>
	{
		void operator () (SerializerT& s, ValueT&& value)
		{
			s.write_object_cb(std::forward<ValueT>(value),
				[&](ValueT&& v){
					if (value)
						s.write_property("value", *v);
				});
		}
	};
	template <typename SerializerT, typename ValueT>
	struct serialize_impl<SerializerT, ValueT,
						typename std::enable_if<std::is_enum<std::decay_t<ValueT>>::value>::type>
	{
		void operator () (SerializerT& s, ValueT&& value)
		{
			s.serialize(std::underlying_type_t<std::decay_t<ValueT>>(value));
		}
	};
	template <typename SerializerT, typename ValueT>
	struct serialize_impl<SerializerT, ValueT,
						typename std::enable_if<
							corecpp::is_time_point<std::decay_t<ValueT>>::value>
							::type>
	{
		void operator () (SerializerT& s, ValueT&& value)
		{
			s.serialize(value.time_since_epoch().count());
		}
	};
	template <typename SerializerT, typename ValueT>
	struct serialize_impl<SerializerT, ValueT,
						typename std::enable_if<
							is_serializable<std::decay_t<ValueT>, std::decay_t<SerializerT>>::value>
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
						typename std::enable_if<is_associative<std::decay_t<ValueT>>::value>::type>
	{
		void operator () (SerializerT& s, ValueT&& value)
		{
			s.write_associative_array(std::forward<ValueT>(value));
		}
	};
	template <typename SerializerT, typename ValueT>
	struct serialize_impl<SerializerT, ValueT,
						typename std::enable_if<!is_associative<std::decay_t<ValueT>>::value
							&& is_iterable<std::decay_t<ValueT>>::value>::type>
	{
		void operator () (SerializerT& s, ValueT&& value)
		{
			s.write_array(std::forward<ValueT>(value));
		}
	};

	/*
	template <typename SerializerT>
	struct serialize_visitor
	{
		SerializerT& s;
		template <typename ValueT>
		void operator () (ValueT&& value)
		{
			s.serialize(std::forward<ValueT>(value));
		}
	};
	template <typename SerializerT, typename ValueT>
	struct serialize_impl<SerializerT, ValueT,
		typename std::enable_if<is_visitable<std::decay_t<ValueT>, serialize_visitor<SerializerT>>::value >::type>
	{
		void operator () (SerializerT& s, ValueT&& value)
		{
			visit(serialize_visitor<SerializerT>(s), std::forward<ValueT>(value));
		}
	};
	*/

	template <typename DeserializerT, typename ValueT, typename Enable = void>
	struct deserialize_impl
	{
	public:
		void operator () (DeserializerT& d, ValueT& value)
		{
			d.read_object(value, std::remove_reference_t<ValueT>::properties());
		}
	};
	template <typename DeserializerT, typename ValueT>
	struct deserialize_impl<DeserializerT, ValueT,
		typename std::enable_if_t<corecpp::is_dereferencable_v<std::decay_t<ValueT>>
			&& std::is_constructible_v<std::decay_t<ValueT>,
				std::add_pointer_t<typename std::decay_t<ValueT>::element_type>>> >
	{
		void operator () (DeserializerT& d, ValueT& value)
		{
			value = ValueT {};
			d.read_object_cb([&](const std::wstring& property){
				using value_type = typename std::remove_reference<decltype(*value)>::type;
				if (property != L"value")
				{
					std::string name = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>{}.to_bytes(property.data());
					throw std::runtime_error(corecpp::concat<std::string>({ "invalid property ", name}));
				}
				value = ValueT(new value_type());
				d.deserialize(*value);
			});
		}
	};
	template <typename DeserializerT, typename ValueT>
	struct deserialize_impl<DeserializerT, ValueT,
	typename std::enable_if_t<corecpp::is_dereferencable_v<std::decay_t<ValueT>>
		&& std::is_default_constructible_v<typename std::decay_t<ValueT>::value_type>> >
	{
		void operator () (DeserializerT& d, ValueT& value)
		{
			value = ValueT {};
			d.read_object_cb([&](const std::wstring& property){
				using value_type = typename std::remove_reference<decltype(*value)>::type;
				if (property != L"value")
				{
					std::string name = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>{}.to_bytes(property.data());
					throw std::runtime_error(corecpp::concat<std::string>({ "invalid property ", name}));
				}
				value = value_type();
				d.deserialize(*value);
			});
		}
	};
	template <typename DeserializerT, typename ValueT>
	struct deserialize_impl<DeserializerT, ValueT,
						typename std::enable_if<std::is_enum<std::decay_t<ValueT>>::value>::type>
	{
		void operator () (DeserializerT& d, ValueT& value)
		{
			using underlying_t = std::underlying_type_t<std::decay_t<ValueT>>;
			d.deserialize(reinterpret_cast<std::add_lvalue_reference_t<underlying_t>>(value));
		}
	};
	template <typename DeserializerT, typename ValueT>
	struct deserialize_impl<DeserializerT, ValueT,
						typename std::enable_if<
							corecpp::is_time_point<std::decay_t<ValueT>>::value>
							::type>
	{
		void operator () (DeserializerT& d, ValueT& value)
		{
			typename ValueT::rep ticks;
			d.deserialize(ticks);
			value -= value.time_since_epoch(); //reset the value to 0
			value += typename ValueT::duration { ticks };
		}
	};
	template <typename DeserializerT, typename ValueT>
	struct deserialize_impl<DeserializerT, ValueT,
							typename std::enable_if<
								is_deserializable<std::decay_t<ValueT>, typename std::decay<DeserializerT>::type>::value>
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
		typename std::enable_if<is_associative<std::decay_t<ValueT>>::value>::type>
	{
		void operator () (DeserializerT& d, ValueT& value)
		{
			/* When inserting elements, associative_array differs semantically from array.
			 * As a consequence, another method is necessary
			 */
			d.read_associative_array(value);
		}
	};
	template <typename DeserializerT, typename ValueT>
	struct deserialize_impl<DeserializerT, ValueT,
		typename std::enable_if<!is_associative<std::decay_t<ValueT>>::value
			&& is_iterable<std::decay_t<ValueT>>::value>::type>
	{
		void operator () (DeserializerT& d, ValueT& value)
		{
			d.read_array(value);
		}
	};

	/*
	template <typename DeserializerT>
	struct deserialize_visitor
	{
		DeserializerT& s;
		template <typename ValueT>
		void operator () (ValueT& value)
		{
			s.deserialize(value);
		}
	};
	template <typename DeserializerT, typename ValueT>
	struct deserialize_impl<DeserializerT, ValueT,
		typename std::enable_if<is_visitable<std::decay_t<ValueT>, deserialize_visitor<DeserializerT>>::value >::type>
	{
		void operator () (DeserializerT& d, ValueT& value)
		{
			visit(serialize_visitor<DeserializerT>(d), value);
		}
	};
	*/
}

#endif
