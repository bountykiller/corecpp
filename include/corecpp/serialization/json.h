#ifndef CORECPP_JSON_H
#define CORECPP_JSON_H

#include <codecvt>
#include <iterator>
#include <locale>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
#include <stdexcept>

#include <corecpp/variant.h>
#include <corecpp/algorithm.h>
#include <corecpp/serialization/common.h>

namespace corecpp
{
	namespace json
	{
		struct open_brace_token
		{};

		struct close_brace_token
		{};

		struct open_bracket_token
		{};

		struct close_bracket_token
		{};

		struct comma_token
		{};

		struct dot_token
		{};

		struct colon_token
		{};

		struct string_token
		{
			std::wstring value;
		};

		struct numeric_token
		{
			double value;
		};

		struct integral_token
		{
			long value;
		};

		struct null_token
		{};

		struct true_token
		{};

		struct false_token
		{};

		using token = corecpp::variant<open_brace_token, close_brace_token, open_bracket_token, close_bracket_token,
										comma_token, dot_token, colon_token,
										string_token, numeric_token, integral_token,
										null_token, true_token, false_token>;

		std::string to_string(const token& tk);

		class tokenizer
		{
			const std::string& m_document;
			std::string::const_iterator m_position;
			std::locale m_locale;
		private:
			wchar_t read_escaped_char();
			std::unique_ptr<token> read_string_literal();
			std::unique_ptr<token> read_numeric_literal();
		public:
			tokenizer(const std::string& document)
			:m_document(document), m_position(document.begin()), m_locale("en_US.utf8")
			{}
			std::string reminder() const
			{
				return std::string(m_position, m_document.end());
			}
			/* TODO: Use optional once they are available */
			std::unique_ptr<token> tokenize();
		};


		struct string_node
		{
			std::wstring value;
		};

		struct integral_node
		{
			long value;
		};

		struct numeric_node
		{
			double value;
		};

		struct char_node
		{
			char value;
		};

		struct boolean_node
		{
			bool value;
		};

		struct null_node
		{
		};
		struct array_node;
		struct object_node;

		using value_node = corecpp::variant<object_node, array_node, string_node, integral_node, numeric_node, char_node, boolean_node, null_node>;

		struct array_node
		{
			std::vector<value_node> values;
		};
		static inline auto begin(array_node& a) { return std::begin(a.values); }
		static inline auto begin(const array_node& a) { return std::begin(a.values); }
		static inline auto end(array_node& a) { return std::end(a.values); }
		static inline auto end(const array_node& a) { return std::end(a.values); }

		struct pair_node
		{
			string_node name;
			value_node value;
		};

		struct object_node
		{
			std::vector<pair_node> members;

			template <typename StringT, typename ValueT>
			value_node& emplace(StringT&& key, ValueT&& value)
			{
				for (auto& member : members)
					if (member.name.value == key)
						return (member.value = std::forward<ValueT>(value));
				members.emplace_back(key, std::forward<ValueT>(value));
				return members.back().value;
			}
			value_node& at (const std::wstring& key)
			{
				for (auto& value : members)
					if (value.name.value == key)
						return value.value;
				throw std::overflow_error(std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(key));
			}
			const value_node& at (const std::wstring& key) const
			{
				for (const auto& value : members)
					if (value.name.value == key)
						return value.value;
				throw std::overflow_error(std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(key));
			}
		};
		static inline auto begin(object_node& o) { return std::begin(o.members); }
		static inline auto begin(const object_node& o) { return std::begin(o.members); }
		static inline auto end(object_node& o) { return std::end(o.members); }
		static inline auto end(const object_node& o) { return std::end(o.members); }

		using node = corecpp::variant<object_node, pair_node, array_node, string_node, integral_node, numeric_node, char_node, boolean_node, null_node>;


		/* PARSING RULES */

		class object_rule;
		class pair_rule;
		class array_rule;
		class value_rule;
		using rule = corecpp::variant<object_rule, pair_rule, array_rule, value_rule>;

		struct shift_result
		{
			bool eaten;
			corecpp::variant<std::nullptr_t, rule> next_rule = nullptr;
		};

		class object_rule
		{
			enum struct status
			{
				start = 0,
				members = 1,
				end = 2
			};
			status m_status;
			std::vector<pair_node> m_members;
		public:
			object_rule()
			: m_status(status::start), m_members()
			{}
			object_rule(object_rule&& other)
			: m_status(other.m_status), m_members(std::move(other.m_members))
			{}
			shift_result shift(token&& tk);
			shift_result shift(node&& n);
			node reduce();
		};
		class pair_rule
		{
			enum struct status
			{
				start = 0,
				name = 1,
				colon = 2,
				value = 3
			};
			status m_status;
			std::wstring m_name;
			corecpp::variant<std::nullptr_t, value_node> m_value;
		public:
			pair_rule(pair_rule&& other)
			: m_status(other.m_status), m_name(std::move(other.m_name)), m_value(std::move(other.m_value))
			{
			}
			pair_rule()
			: m_status(status::start), m_name(), m_value(nullptr)
			{
			}
			shift_result shift(token&& n);
			shift_result shift(node&& n);
			node reduce();
		};
		class array_rule
		{
			enum struct status
			{
				start = 0,
				members = 1,
				end = 2
			};
			status m_status;
			std::vector<value_node> m_values;
		public:
			shift_result shift(token&& tk);
			shift_result shift(node&& n);
			node reduce();
		};
		class value_rule
		{
			corecpp::variant<std::nullptr_t, value_node> m_value;
		public:
			value_rule() : m_value(nullptr)
			{}
			value_rule(value_rule&& other) : m_value(std::move(other.m_value))
			{}
			shift_result shift(token&& n);
			shift_result shift(node&& n);
			node reduce();
		};


		class parser
		{
			std::vector<rule> m_stack;
		public:
			template <typename RuleT>
			void start(void)
			{
				if (!m_stack.empty())
					throw std::logic_error("");
				m_stack.emplace_back(RuleT{});
			}
			void push(token&& tk);
			node end();
		};



		template <typename StreamT = std::basic_ostream<char>>
		struct serializer
		{
			using stream_type = StreamT;
		private:
			stream_type& m_stream;
			bool m_first = true;
			void convert_and_escape(const std::string& value)
			{
				static const char hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
				for(unsigned char c : value)
				{
					if (std::isprint((char)c))
						m_stream << c;
					else if (c < 0x10)
						m_stream << "\\u000" << hex_chars[c & 0x0F];
					else
						m_stream << "\\u00" << (hex_chars[c & 0xF0] << 4) << hex_chars[c & 0x0F];
				}
			}
			void convert_and_escape(const std::wstring& value)
			{
				static const char hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
				/* TODO: support not only UTF8 */
				for(wchar_t c : value)
				{
					if (c < 0x10)
						m_stream << "\\u000" << hex_chars[c & 0x0F];
					else if (c < 0x100)
					{
						if (std::isprint((char)c))
							m_stream << c;
						else
							m_stream << "\\u00" << (hex_chars[c & 0xF0] << 4) << hex_chars[c & 0x0F];
					}
					else if (c <= 0x7FF)
					{
						m_stream << (c >> 6) + 0xC0;
						m_stream << (c & 0x3F) + 0x80;
					}
					else if (c <= 0xFFFF)
					{
						m_stream << (c >> 12) + 0xE0;
						m_stream << ((c >> 6) & 0x3F) + 0x80;
						m_stream << (c & 0x3F) + 0x80;
					}
					else if (c <= 0x10FFFF)
					{
						m_stream << (c >> 18) + 0xF0;
						m_stream << ((c >> 12) & 0x3F) + 0x80;
						m_stream << ((c >> 6) & 0x3F) + 0x80;
						m_stream << (c & 0x3F) + 0x80;
					}
					else
						throw std::range_error(corecpp::concat<std::string>({ "Unable to convert charcode ", std::to_string((uint32_t)c) }));
				}
			}
		public:
			serializer(stream_type& s) : m_stream(s)
			{}
			void serialize(bool value)
			{
				m_stream << (value ? "true" : "false");
			}
			void serialize(int16_t value)
			{
				m_stream << value;
			}
			void serialize(int32_t value)
			{
				m_stream << value;
			}
			void serialize(int64_t value)
			{
				m_stream << value;
			}
			void serialize(uint16_t value)
			{
				m_stream << value;
			}
			void serialize(uint32_t value)
			{
				m_stream << value;
			}
			void serialize(uint64_t value)
			{
				m_stream << value;
			}
			void serialize(std::nullptr_t) const
			{
				m_stream << "null";
			}
			void serialize(float value)
			{
				m_stream << value;
			}
			void serialize(double value)
			{
				m_stream << value;
			}
			void serialize(const char *value)
			{
				m_stream << '"';
				convert_and_escape(value);
				m_stream << '"';
			}
			void serialize(const wchar_t *value)
			{
				m_stream << '"';
				convert_and_escape(value);
				m_stream << '"';
			}
			void serialize(const std::string& value)
			{
				m_stream << '"';
				convert_and_escape(value);
				m_stream << '"';
			}
			void serialize(const std::wstring& value)
			{
				m_stream << '"';
				convert_and_escape(value);
				m_stream << '"';
			}
			template <typename ValueT, typename Enable = void>
			void serialize(ValueT&& value)
			{
				serialize_impl<serializer<StreamT>, ValueT> impl;
				impl(*this, std::forward<ValueT>(value));
			}

			/* "Low level" methods */
			template <typename ValueT>
			void begin_object()
			{
				m_first = true;
				m_stream << '{';
			}
			void end_object()
			{
				m_stream << '}';
			}

			template <typename ValueT>
			void begin_array()
			{
				m_first = true;
				m_stream << '[';
			}
			void end_array()
			{
				m_stream << ']';
			}

			template <typename StringT, typename ValueT>
			void write_property(const StringT& name, ValueT&& value)
			{
				if (!m_first)
					m_stream << ',';
				serialize(name);
				m_stream << ':';
				serialize(value);
				m_first = false;
			}
			template <typename ValueT, typename PropertiesT>
			void write_object(ValueT&& value, const PropertiesT& properties)
			{
				begin_object<ValueT>();
				tuple_apply([&](const auto& prop) {
					this->write_property(prop.name, prop.cget(value));
				}, properties);
				end_object();
			}
			template <typename ValueT>
			void write_array(ValueT&& value)
			{
				begin_array<ValueT>();
				for (auto item : value)
				{
					if (!m_first)
						m_stream << ',';
					serialize(item);
					m_first = false;
				}
				end_array();
			}
		};
	}
}

#endif
