#ifndef CORECPP_XML_H
#define CORECPP_XML_H

#include <codecvt>
#include <cwchar>
#include <functional>
#include <iterator>
#include <iostream>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
#include <stdexcept>

#include <corecpp/algorithm.h>
#include <corecpp/diagnostic.h>
#include <corecpp/variant.h>
#include <corecpp/visibility.h>
#include <corecpp/except.h>
#include <corecpp/serialization/common.h>

namespace corecpp::xml
{
	_internal corecpp::diagnostic::event_producer& xml_logger();


	class serializer
	{
		std::ostream& m_stream;
		bool m_use_attributes; //TODO
		bool m_pretty;
		bool m_started;
		unsigned int m_indent_level;
		void convert_and_escape(const std::string& value);
		void convert_and_escape(const std::wstring& value);
		void convert_and_escape(const std::u16string& value);
		void convert_and_escape(const std::u32string& value);
		void indent()
		{
			for (int i = 0; i < m_indent_level; ++i)
				m_stream << "\t";
		}
		void start_document()
		{
			m_stream << "<?xml version=\"1.0\" standalone=\"yes\" ?>";
			if (m_pretty)
				m_stream << "\n";
			m_stream << "<root>";
		}
		void end_document()
		{
			m_stream << "</root>";
		}
		template<typename T>
		void start_tag(T&& name)
		{
			m_stream << '<';
			convert_and_escape(std::forward<T>(name));
			m_stream << '>';
		}
		template<typename T>
		void end_tag(T&& name)
		{
			m_stream << "</";
			convert_and_escape(std::forward<T>(name));
			m_stream << '>';
		}
	public:
		serializer(std::ostream& s, bool use_attributes = true, bool pretty = false)
		: m_stream { s }, m_use_attributes { use_attributes }, m_pretty { pretty }, m_started { false }, m_indent_level { 0 }
		{}
		void serialize(bool value)
		{
			m_stream << (value ? "true" : "false");
		}
		void serialize(int8_t value)
		{
			m_stream << std::to_string(value);
		}
		void serialize(int16_t value)
		{
			m_stream << std::to_string(value);
		}
		void serialize(int32_t value)
		{
			m_stream << std::to_string(value);
		}
		void serialize(int64_t value)
		{
			m_stream << std::to_string(value);
		}
		void serialize(uint8_t value)
		{
			m_stream << std::to_string(value);
		}
		void serialize(uint16_t value)
		{
			m_stream << std::to_string(value);
		}
		void serialize(char16_t value)
		{
			m_stream << std::to_string(value);
		}
		void serialize(uint32_t value)
		{
			m_stream << std::to_string(value);
		}
		void serialize(uint64_t value)
		{
			m_stream << std::to_string(value);
		}
		void serialize(std::nullptr_t)
		{
			m_stream << "null";
		}
		void serialize(float value)
		{
			m_stream << std::to_string(value);
		}
		void serialize(double value)
		{
			m_stream << std::to_string(value);
		}
		void serialize(const char *value)
		{
			convert_and_escape(value);
		}
		void serialize(const wchar_t *value)
		{
			convert_and_escape(value);
		}
		/* TODO: Allows string to be r-value references
		 */
		void serialize(const std::string& value)
		{
			convert_and_escape(value);
		}
		void serialize(const std::wstring& value)
		{
			convert_and_escape(value);
		}
		void serialize(const std::u16string& value)
		{
			convert_and_escape(value);
		}
		void serialize(const std::u32string& value)
		{
			convert_and_escape(value);
		}
		template <typename ValueT, typename Enable = void>
		void serialize(ValueT&& value)
		{
			bool started = false;
			if (!m_started)
			{
				start_document();
				started = m_started = true;
			}
			serialize_impl<serializer, ValueT> impl;
			impl(*this, std::forward<ValueT>(value));
			if (started)
			{
				end_document();
				m_started = false;
			}
		}

		/* "Low level" methods */
		template <typename ValueT>
		void begin_object()
		{
//				abi::__cxa_demangle
			xml_logger().trace("begin object", typeid(ValueT).name(), __FILE__, __LINE__);
		}
		void end_object()
		{
			xml_logger().trace("end object", __FILE__, __LINE__);
		}

		template <typename ValueT>
		void begin_array()
		{
			xml_logger().trace("begin array", typeid(ValueT).name(), __FILE__, __LINE__);
		}
		void end_array()
		{
			xml_logger().trace("end array", __FILE__, __LINE__);
		}

		template <typename ValueT>
		void write_element(ValueT&& value)
		{
			m_stream << "<item>";
			if (m_pretty)
			{
				m_indent_level++;
				m_stream << "\n";
				indent();
			}
			serialize(std::forward<ValueT>(value));
			if (m_pretty)
			{
				m_indent_level--;
				m_stream << "\n";
				indent();
			}
			m_stream << "</item>";
		}
		template <typename ValueT>
		void write_element(const ValueT& value)
		{
			m_stream << "<item>";
			if (m_pretty)
			{
				m_indent_level++;
				m_stream << "\n";
				indent();
			}
			serialize(value);
			if (m_pretty)
			{
				m_indent_level--;
				m_stream << "\n";
				indent();
			}
			m_stream << "</item>";
		}

		template <typename StringT, typename ValueT>
		void write_property(const StringT& name, ValueT&& value)
		{
			start_tag(name);
			serialize(value);
			end_tag(name);
		}
		template <typename StringT, typename FuncT>
		void write_property_cb(const StringT& name, FuncT func)
		{
			start_tag(name);
			func();
			end_tag(name);
		}

		template <typename ValueT, typename PropertiesT>
		void write_object(ValueT&& value, const PropertiesT& properties)
		{
			if (m_pretty)
			{
				m_indent_level++;
			}
			begin_object<ValueT>();
			tuple_apply([&](const auto& prop) {
				if (m_pretty)
				{
					m_stream << "\n";
					indent();
				}
				this->write_property(prop.name(), prop.cget(value));
			}, properties);
			end_object();
			if (m_pretty)
			{
				// set the position for next element
				m_indent_level--;
				m_stream << "\n";
				indent();
			}
		}
		template <typename ValueT, typename FuncT>
		void write_object_cb(ValueT&& value, FuncT func)
		{
			begin_object<ValueT>();
			func(std::forward<ValueT>(value));
			end_object();
		}

		template <typename ValueT>
		void write_array(ValueT&& value)
		{
			if (m_pretty)
			{
				m_indent_level++;
			}
			begin_array<ValueT>();
			for (typename std::decay_t<ValueT>::const_iterator iter = std::cbegin(value);
				iter != std::cend(value);
				++iter)
			{
				if (m_pretty)
				{
					m_stream << "\n";
					indent();
				}
				write_element(*iter);
			}
			end_array();
			if (m_pretty)
			{
				// set the position for next element
				m_indent_level--;
				m_stream << "\n";
				indent();
			}
		}
		template <typename ValueT>
		void write_associative_array(ValueT&& value)
		{
			if (m_pretty)
			{
				m_indent_level++;
			}
			begin_array<ValueT>();
			for (typename std::decay_t<ValueT>::const_iterator iter = std::cbegin(value);
				iter != std::cend(value);
				++iter)
			{
				begin_object<typename std::decay_t<ValueT>::value_type>();
				if (m_pretty)
				{
					m_stream << '\n';
					indent();
					m_indent_level++;
				}
				m_stream << "<key>";
				write_element<typename std::decay_t<ValueT>::key_type>(iter->first);
				if (m_pretty)
				{
					m_stream << '\n';
					indent();
				}
				m_stream << "</key>";
				if (m_pretty)
				{
					m_stream << '\n';
					indent();
				}
				m_stream << "<value>";
				write_element<typename std::decay_t<ValueT>::mapped_type>(iter->second);
				m_stream << "</value>";
				if (m_pretty)
				{
					m_stream << '\n';
					indent();
					m_indent_level--;
				}
				end_object();
			}
			end_array();
			if (m_pretty)
			{
				// set the position for next element
				m_indent_level--;
				m_stream << "\n";
				indent();
			}
		}
	};
}

#endif
