#ifndef CORECPP_JSON_H
#define CORECPP_JSON_H

#include <codecvt>
#include <functional>
#include <iterator>
#include <locale>
#include <sstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>
#include <stdexcept>
#include <cwchar>

#include <corecpp/algorithm.h>
#include <corecpp/diagnostic.h>
#include <corecpp/variant.h>
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
			std::string m_buffer;
			std::string::size_type m_position;
			std::locale m_locale;

			wchar_t read_escaped_char();
			std::unique_ptr<token> read_string_literal();
			std::unique_ptr<token> read_numeric_literal();
			void shrink()
			{
				if (!m_position)
					return;
				m_buffer.assign(m_buffer.begin() + m_position, m_buffer.end());
				m_position = 0;
			}
		public:
			tokenizer()
			: m_buffer(), m_position(0), m_locale("en_US.utf8")
			{}
			/**
			 * \brief aliment the tokenizer so that it can read the next tokens
			 */
			void eat(const std::string& str)
			{
				m_buffer.append(str);
			}
			/**
			 * \brief extract the next token
			 * \return a pointer to the next token, or an empty unique_ptr if no more token to read.
			 * \remark if the next token read doesn't require to read all the parameter, the unread characters are still
			 * reachable through the reminder method
			 * TODO: Use optional once they are available
			 */
			std::unique_ptr<token> next();
			/**
			 * \brief get the unread chars
			 */
			std::string reminder() const
			{
				return m_buffer.substr(m_position);
			}
			/**
			 * \brief return false if there is still unread characters, true otherwise
			 */
			bool done() const
			{
				return (m_buffer.length() == 0);
			}
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

		struct pair_node;
		struct value_node;

		struct array_node
		{
			std::vector<value_node> values;
		};
		static inline auto begin(array_node& a) { return std::begin(a.values); }
		static inline auto begin(const array_node& a) { return std::begin(a.values); }
		static inline auto end(array_node& a) { return std::end(a.values); }
		static inline auto end(const array_node& a) { return std::end(a.values); }

		struct object_node
		{
			std::vector<pair_node> members;

			template <typename StringT, typename ValueT>
			value_node& emplace(StringT&& key, ValueT&& value);
			value_node& at (const std::wstring& key);
			const value_node& at (const std::wstring& key) const;
		};
		static inline auto begin(object_node& o) { return std::begin(o.members); }
		static inline auto begin(const object_node& o) { return std::begin(o.members); }
		static inline auto end(object_node& o) { return std::end(o.members); }
		static inline auto end(const object_node& o) { return std::end(o.members); }

		struct value_node
		: public corecpp::variant<object_node, array_node, string_node,
			integral_node, numeric_node, char_node, boolean_node, null_node>
		{
			template <typename T>
			value_node(T&& value)
			: corecpp::variant<object_node, array_node, string_node,
				integral_node, numeric_node, char_node, boolean_node, null_node>(std::forward<T>(value))
			{
			}
		};

		struct pair_node
		{
			string_node name;
			value_node value;
		};


		/* Should be declared here because it needs pair_node to be fully declared
		 */
		template <typename StringT, typename ValueT>
		value_node& object_node::emplace(StringT&& key, ValueT&& value)
		{
			for (auto& member : members)
				if (member.name.value == key)
					return (member.value = std::forward<ValueT>(value));
				members.emplace_back(key, std::forward<ValueT>(value));
			return members.back().value;
		}

		using node = corecpp::variant<object_node, pair_node, array_node, string_node, integral_node, numeric_node, char_node, boolean_node, null_node>;


		/* PARSING RULES */

		class object_rule;
		class pair_rule;
		class array_rule;
		class value_rule;
		struct shift_result;
		using rule = corecpp::variant<object_rule, pair_rule, array_rule, value_rule>;

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


		struct shift_result
		{
			bool eaten;
			corecpp::variant<std::nullptr_t, rule> next_rule = nullptr;
		};


		class parser
		{
			std::vector<rule> m_stack;
		public:
			template <typename RuleT>
			void start(void)
			{
				if (!m_stack.empty())
					throw std::logic_error("last parsing not finished, can't start a new one!");
				m_stack.emplace_back(RuleT{});
			}
			void push(token&& tk);
			node end();
		};

		/* TODO:
		 * Add serialization/deserialization of char
		 * Add serialization/deserialization of strong enums
		 */
		class serializer
		{
			std::ostream& m_stream;
			bool m_first = true;
			void convert_and_escape(const std::string& value);
			void convert_and_escape(const std::wstring& value);
		public:
			serializer(std::ostream& s) : m_stream(s)
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
			void serialize(uint8_t value)
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
			void serialize(std::nullptr_t)
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
				serialize_impl<serializer, ValueT> impl;
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

			template <typename ValueT>
			void write_element(ValueT&& value)
			{
				if (!m_first)
					m_stream << ',';
				serialize(std::forward<ValueT>(value));
				m_first = false;
			}
			template <typename ValueT>
			void write_element(const ValueT& value)
			{
				if (!m_first)
					m_stream << ',';
				serialize(value);
				m_first = false;
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
					this->write_property(prop.name(), prop.cget(value));
				}, properties);
				end_object();
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
				begin_array<ValueT>();
				for (typename std::decay_t<ValueT>::const_iterator iter = std::begin(value);
					iter != std::end(value);
					++iter)
					write_element(*iter);
				end_array();
			}
			template <typename ValueT>
			void write_associative_array(ValueT&& value)
			{
				begin_array<ValueT>();
				for (typename std::decay_t<ValueT>::const_iterator iter = std::begin(value);
					iter != std::end(value);
					++iter)
				{
					if (!m_first)
						m_stream << ',';
					begin_object<typename std::decay_t<ValueT>::value_type>();
					write_element<typename std::decay_t<ValueT>::key_type>(iter->first);
					write_element<typename std::decay_t<ValueT>::mapped_type>(iter->second);
					end_object();
				}
				end_array();
			}
		};


		class deserializer
		{
			std::istream& m_stream;
			tokenizer m_tokenizer;
			bool m_first;

			token read();
			template<typename IntegralT, typename = std::enable_if<std::is_integral<IntegralT>::value, IntegralT>>
			void deserialize_integral(IntegralT& value)
			{
				token tk = read();
				if (tk.index() != token::index_of<integral_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "integral token expected, got ", to_string(tk) }));
				auto result = tk.get<integral_token>().value;
				if (result > std::numeric_limits<IntegralT>::max()
					|| result < std::numeric_limits<IntegralT>::lowest())
					throw std::overflow_error(std::to_string(result));
				value = result;
			}
			template<typename FloatT, typename = std::enable_if<std::is_integral<FloatT>::value, FloatT>>
			void deserialize_float(FloatT& value)
			{
				token tk = read();
				if (tk.index() != token::index_of<numeric_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "numeric token expected, got ", to_string(tk) }));
				auto result = tk.get<numeric_token>().value;
				if (result > std::numeric_limits<FloatT>::max()
					|| result < std::numeric_limits<FloatT>::lowest())
					throw std::overflow_error(std::to_string(result));
				value = result;
			}
			void read_string(const string_token& wstr, std::string& value)
			{
				const wchar_t* wchar = wstr.value.data();
				std::mbstate_t state = std::mbstate_t();
				auto len = std::wcsrtombs(nullptr, &wchar, 0, &state);
				if (len == static_cast<std::size_t>(-1))
					throw std::runtime_error("converion error");
				value.resize(len);
				std::wcsrtombs((char*)value.data(), &wchar, len, &state);
			}
		public:
			deserializer(std::istream& s)
			: m_stream(s), m_tokenizer(), m_first(true)
			{}
			void deserialize(bool& value)
			{
				token tk = read();
				if (tk.index() == token::index_of<true_token>::value)
					value = true;
				else if (tk.index() == token::index_of<false_token>::value)
					value = false;
				else
					throw std::runtime_error(corecpp::concat<std::string>({ "boolean token expected, got ", to_string(tk) }));
			}
			void deserialize(int16_t& value)
			{
				deserialize_integral(value);
			}
			void deserialize(int32_t& value)
			{
				deserialize_integral(value);
			}
			void deserialize(int64_t& value)
			{
				deserialize_integral(value);
			}
			void deserialize(uint8_t& value)
			{
				deserialize_integral(value);
			}
			void deserialize(uint16_t& value)
			{
				deserialize_integral(value);
			}
			void deserialize(uint32_t& value)
			{
				deserialize_integral(value);
			}
			void deserialize(uint64_t& value)
			{
				deserialize_integral(value);
			}
			void deserialize(std::nullptr_t)
			{
				token tk = read();
				if (tk.index() != token::index_of<null_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "null token expected, got ", to_string(tk) }));
			}
			void deserialize(float& value)
			{
				deserialize_float(value);
			}
			void deserialize(double& value)
			{
				deserialize_float(value);
			}
			void deserialize(std::string& value)
			{
				token tk = read();
				if (tk.index() != token::index_of<string_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "string token expected, got ", to_string(tk) }));
				read_string(tk.get<string_token>(), value);
			}
			void deserialize(std::wstring& value)
			{
				token tk = read();
				if (tk.index() != token::index_of<string_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "string token expected, got ", to_string(tk) }));
				value = std::move(tk.get<string_token>().value);
			}
			template <typename ValueT, typename Enable = void>
			void deserialize(ValueT& value)
			{
				deserialize_impl<deserializer, ValueT> impl;
				impl(*this, value);
			}

			/* "Low level" methods */
			template <typename ValueT>
			void begin_object()
			{
				m_first = true;
				token tk = read();
				if (tk.index() != token::index_of<open_brace_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "open brace token expected, got ", to_string(tk) }));
			}
			void end_object()
			{
				token tk = read();
				if (tk.index() != token::index_of<close_brace_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "close brace token expected, got ", to_string(tk) }));
			}

			template <typename ValueT>
			void begin_array()
			{
				m_first = true;
				token tk = read();
				if (tk.index() != token::index_of<open_bracket_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "open bracket token expected, got ", to_string(tk) }));
			}
			void end_array()
			{
				m_first = false;
				token tk = read();
				if (tk.index() != token::index_of<close_bracket_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "close bracket token expected, got ", to_string(tk) }));
			}
			template <typename ValueT>
			void read_element(ValueT& value)
			{
				if (!m_first)
				{
					auto tk = read();
					if (tk.index() != token::index_of<comma_token>::value)
						throw std::runtime_error(corecpp::concat<std::string>({ "comma token expected, got ", to_string(tk) }));
				}
				deserialize(value);
				m_first = false;
			}
			template <typename ValueT>
			void read_object(ValueT& value)
			{
				token tk = read();
				if (tk.index() != token::index_of<open_brace_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "open brace token expected, got ", to_string(tk) }));

				for (tk = read(); tk.index() == token::index_of<string_token>::value; tk = read())
				{
					std::string pname;
					read_string(tk.get<string_token>(), pname);
					tk = read();
					if (tk.index() != token::index_of<colon_token>::value)
						throw std::runtime_error(corecpp::concat<std::string>({ "colon token expected, got ", to_string(tk) }));
					value.deserialize(*this, pname);
					tk = read();
					if (tk.index() != token::index_of<comma_token>::value)
						break;
				}
				if (tk.index() != token::index_of<close_brace_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "close brace token expected, got ", to_string(tk) }));
			}
			template <typename ValueT, typename PropertiesT>
			void read_object(ValueT& value, const PropertiesT& properties)
			{
				token tk = read();
				if (tk.index() != token::index_of<open_brace_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "open brace token expected, got ", to_string(tk) }));

				for (tk = read(); tk.index() == token::index_of<string_token>::value; tk = read())
				{
					std::wstring pname = std::move(tk.get<string_token>().value);
					tk = read();
					if (tk.index() != token::index_of<colon_token>::value)
						throw std::runtime_error(corecpp::concat<std::string>({ "colon token expected, got ", to_string(tk) }));
					tuple_apply([&](const auto& prop) {
						if (prop.name() == pname)
							this->deserialize(prop.get(value));
					}, properties);
					tk = read();
					if (tk.index() != token::index_of<comma_token>::value)
						break;
				}
				if (tk.index() != token::index_of<close_brace_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "close brace expected, got ", to_string(tk) }));
			}
			void read_object_cb(std::function<void(const std::wstring&)> func)
			{
				token tk = read();
				if (tk.index() != token::index_of<open_brace_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "open brace token expected, got ", to_string(tk) }));

				for (tk = read(); tk.index() == token::index_of<string_token>::value; tk = read())
				{
					std::wstring pname = std::move(tk.get<string_token>().value);
					tk = read();
					if (tk.index() != token::index_of<colon_token>::value)
						throw std::runtime_error(corecpp::concat<std::string>({ "colon token expected, got ", to_string(tk) }));
					func(pname);
					tk = read();
					if (tk.index() != token::index_of<comma_token>::value)
						break;
				}
				if (tk.index() != token::index_of<close_brace_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "close brace expected, got ", to_string(tk) }));
			}
			template <typename ValueT>
			void read_array(ValueT& value)
			{
				token tk = read();
				if (tk.index() != token::index_of<open_bracket_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "open bracket token expected, got ", to_string(tk) }));
				do
				{
					using ElemT = typename ValueT::value_type;
					ElemT elem;

					deserialize(elem);
					value.emplace_back(std::move(elem));
					tk = read();
				} while (tk.index() == token::index_of<comma_token>::value);
				if (tk.index() != token::index_of<close_bracket_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "close bracket token expected, got ", to_string(tk) }));
			}
			template <typename ValueT>
			void read_associative_array(ValueT& value)
			{
				token tk = read();
				if (tk.index() != token::index_of<open_bracket_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "open bracket token expected, got ", to_string(tk) }));
				do
				{
					using KeyT = typename ValueT::key_type;
					using MappedT = typename ValueT::mapped_type;
					KeyT key;
					MappedT mapped;

					begin_object<typename ValueT::value_type>();
					read_element<KeyT>(key);
					read_element<MappedT>(mapped);
					end_object();
					value.emplace(std::move(key), std::move(mapped));
					tk = read();
				} while (tk.index() == token::index_of<comma_token>::value);
				if (tk.index() != token::index_of<close_bracket_token>::value)
					throw std::runtime_error(corecpp::concat<std::string>({ "close bracket token expected, got ", to_string(tk) }));
			}
		};
	}
}

#endif
