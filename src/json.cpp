#include <codecvt>
#include <locale>
#include <memory>
#include <cmath>
#include <corecpp/serialization/common.h>
#include <corecpp/serialization/json.h>
#include <corecpp/algorithm.h>

namespace corecpp
{
namespace json
{

std::string to_string(const token& tk)
{
	switch(tk.which())
	{
		case token::index_of<open_brace_token>::value:
			return "{";
		case token::index_of<close_brace_token>::value:
			return "}";
		case token::index_of<open_bracket_token>::value:
			return "[";
		case token::index_of<close_bracket_token>::value:
			return "]";
		case token::index_of<comma_token>::value:
			return ",";
		case token::index_of<dot_token>::value:
			return ".";
		case token::index_of<colon_token>::value:
			return ":";
		case token::index_of<string_token>::value:
			return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(tk.get<string_token>().value);
		case token::index_of<numeric_token>::value:
			return std::to_string(tk.get<numeric_token>().value);
		case token::index_of<integral_token>::value:
			return std::to_string(tk.get<integral_token>().value);
		case token::index_of<null_token>::value:
			return "null";
		case token::index_of<true_token>::value:
			return "true";
		case token::index_of<false_token>::value:
			return "false";
	}
	throw std::runtime_error("unreachable");
}

wchar_t tokenizer::read_escaped_char()
{
	/* TODO: support not only UTF8 */
	static wchar_t conversion_tab[256] = {
		// 16 chars per line
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
		-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	};

	assert (m_position + 4 <= m_buffer.size());

	wchar_t a,b,c,d;
	a = conversion_tab[(int)m_buffer[++m_position]];
	b = conversion_tab[(int)m_buffer[++m_position]];
	c = conversion_tab[(int)m_buffer[++m_position]];
	d = conversion_tab[(int)m_buffer[++m_position]];
	if ((a < 0) || (b < 0) || (c < 0) || (d < 0))
		throw lexical_error(corecpp::concat<std::string>({"invalid unicode escape sequence: ", std::to_string(a), ",",
											std::to_string(b),  ",", std::to_string(c),  ",", std::to_string(d)}));
	return (a << 12) + (b << 8) + (c << 4) + d;
}

std::unique_ptr<token> tokenizer::read_string_literal()
{
	if(++m_position == m_buffer.size())
		return nullptr;

	std::wstring literal;
	bool good = false;
	do
	{
		switch(m_buffer[m_position])
		{
			case '\"':
				good = true;
				break;
			/* control-characters */
			case '\r':
			case '\n':
				throw lexical_error("invalid string expression : unexpected end of line");
			case '\\':
				if(++m_position == m_buffer.size())
					return nullptr;
				switch(m_buffer[m_position])
				{
					case '"': literal += '"'; break;
					case '\\': literal += '\\'; break;
					case '/': literal += '/'; break;
					case 'b': literal += '\b'; break;
					case 'f': literal += '\f'; break;
					case 'n': literal += '\n'; break;
					case 'r': literal += '\r'; break;
					case 't': literal += '\t'; break;
					case 'u':
						if (m_position + 4 > m_buffer.size())
							throw lexical_error("invalid string expression : unterminated escape sequence");
						/* \u four-hex-digits */
						literal += read_escaped_char();
						break;
					default:
						throw lexical_error("invalid string expression : unknown escape sequence");
				}
				break;
			default:
				literal += m_buffer[m_position];
				break;
		}
	}
	while (++m_position != m_buffer.size() && !good);

	if(!good)
		return nullptr;

	return std::make_unique<token>(string_token { literal });
}

std::unique_ptr<token> tokenizer::read_numeric_literal()
{
	bool negative = false;
	long integral_value = 0;
	bool done = false;
	if (m_buffer[m_position] == '-')
	{
		negative = true;
		++m_position;
	}
	do
	{
		switch(m_buffer[m_position])
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				integral_value = (10 * integral_value) + (m_buffer[m_position] - '0');
				break;
			case '.':
			{
				long decimal_value = 0;
				unsigned int decimal_precision = 1;
				while(!done && ++m_position != m_buffer.size())
				{
					switch(m_buffer[m_position])
					{
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							decimal_value = (10 * decimal_value) + (m_buffer[m_position] - '0');
							decimal_precision *= 10;
							break;
						case 'e':
						case 'E':
						{
							long exponential_value = 0;
							bool negative_exponential = false;
							if (m_buffer[m_position] == '-')
							{
								negative_exponential = true;
								++m_position;
							}
							else if (m_buffer[m_position] == '+')
							{
								++m_position;
							}
							while(!done && ++m_position != m_buffer.size())
							{
								switch(m_buffer[m_position])
								{
									case '0':
									case '1':
									case '2':
									case '3':
									case '4':
									case '5':
									case '6':
									case '7':
									case '8':
									case '9':
										exponential_value = (10 * decimal_value) + (m_buffer[m_position] - '0');
										break;
									default:
									{
										double value = exp((double)integral_value + ((double)decimal_value/decimal_precision));
										return std::make_unique<token>(numeric_token { negative ? -value : value });
									}
								}
							}
							break;
						}
						default:
						{
							double value = (double)integral_value + ((double)decimal_value/decimal_precision);
							return std::make_unique<token>(numeric_token { negative ? -value : value });
						}
					}
				}
				break;
			}
			default:
				return std::make_unique<token>(integral_token { negative ? -integral_value : integral_value });
		}
	}
	while(++m_position != m_buffer.size());

	return nullptr;
}

std::unique_ptr<token> tokenizer::next()
{
	if(m_position == m_buffer.size())
	{
		shrink();
		return nullptr;
	}
	while (std::isspace(m_buffer[m_position], m_locale))
	{
		if(++m_position == m_buffer.size())
		{
			shrink();
			return nullptr;
		}
	}
	auto position = m_position;
	switch(m_buffer[m_position])
	{
		case '{':
		{
			++m_position;
			return std::make_unique<token>(open_brace_token());
		}
		case '}':
		{
			++m_position;
			return std::make_unique<token>(close_brace_token());
		}
		case '[':
		{
			++m_position;
			return std::make_unique<token>(open_bracket_token());
		}
		case ']':
		{
			++m_position;
			return std::make_unique<token>(close_bracket_token());
		}
		case ',':
		{
			++m_position;
			return std::make_unique<token>(comma_token());
		}
		case ':':
		{
			m_position++;
			return std::make_unique<token>(colon_token());
		}
		case '"':
		{
			auto res = read_string_literal();
			if (!res)
			{
				m_position = position;
				shrink();
			}
			return res;
		}
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		{
			auto res = read_numeric_literal();
			if (!res)
			{
				m_position = position;
				shrink();
			}
			return res;
		}
		case 'f':
			if (m_position++ != m_buffer.size() && m_buffer[m_position] == 'a'
				&& m_position++ != m_buffer.size() && m_buffer[m_position] == 'l'
				&& m_position++ != m_buffer.size() && m_buffer[m_position] == 's'
				&& m_position++ != m_buffer.size() && m_buffer[m_position] == 'e'
				&& (m_position++ == m_buffer.size() || !isalnum(m_buffer[m_position], m_locale)))
			{
				return std::make_unique<token>(false_token());
			}
			else
			{
				if (m_position == m_buffer.size())
				{
					m_position = position;
					shrink();
					return nullptr;
				}
				else
				{
					m_position = position; /* to stay in a valid state */
					throw corecpp::syntax_error(reminder());
				}
			}

		case 't':
			if(m_position++ != m_buffer.size() && m_buffer[m_position] == 'r'
				&& m_position++ != m_buffer.size() && m_buffer[m_position] == 'u'
				&& m_position++ != m_buffer.size() && m_buffer[m_position] == 'e'
				&& (m_position++ == m_buffer.size() || !isalnum(m_buffer[m_position], m_locale)))
			{
				return std::make_unique<token>(false_token());
			}
			else
			{
				if (m_position == m_buffer.size())
				{
					m_position = position;
					shrink();
					return nullptr;
				}
				else
				{
					m_position = position; /* to stay in a valid state */
					throw corecpp::syntax_error(reminder());
				}
			}
		default:
			m_position = position;
			return nullptr;
	}
}

/*
 * PARSING RULES
 */
shift_result object_rule::shift(token&& tk)
{
	switch (m_status)
	{
		case status::start:
		{
			switch (tk.which())
			{
				case token::index_of<open_brace_token>::value:
					m_status = status::members;
					return { true, rule { pair_rule {} } };
				default:
					return { false, nullptr };
			}
		}
		case status::members:
		{
			switch (tk.which())
			{
				case token::index_of<colon_token>::value:
					return { true, rule { pair_rule {} } };
				case token::index_of<close_brace_token>::value:
					m_status = status::end;
					return { true, nullptr };
				default:
					return { false, nullptr };
			}
		}
		case status::end:
		default:
			return { false, nullptr };
	}
}

shift_result object_rule::shift(node&& n)
{
	if (m_status != status::members)
		return { false, nullptr };
	if(n.which() != node::index_of<pair_node>::value)
		return { false, nullptr };
	m_members.emplace_back(std::move(n.get<pair_node>()));
	return { true, nullptr };
}

node object_rule::reduce()
{
	if (m_status != status::end)
		throw syntax_error("close_brace_token expected");
	return object_node { std::move(m_members) };
}


shift_result array_rule::shift(token&& tk)
{
	switch (m_status)
	{
		case status::start:
		{
			switch (tk.which())
			{
				case token::index_of<open_bracket_token>::value:
					m_status = status::members;
					return { true, rule { value_rule {} } };
				default:
					return { false, nullptr };
			}
		}
		case status::members:
		{
			switch (tk.which())
			{
				case token::index_of<colon_token>::value:
					return { true, rule { value_rule {} } };
				case token::index_of<close_bracket_token>::value:
					m_status = status::end;
					return { true, nullptr };
				default:
					return { false, nullptr };
			}
		}
		case status::end:
		default:
			return { false, nullptr };
	}
}

shift_result array_rule::shift(node&& n)
{
	if (m_status != status::members)
		return { false, nullptr };
	switch(n.which())
	{
		case node::index_of<object_node>::value:
			m_values.emplace_back(std::move(n.get<object_node>()));
			return { true, nullptr };
		case node::index_of<array_node>::value:
			m_values.emplace_back(std::move(n.get<array_node>()));
			return { true, nullptr };
		case node::index_of<string_node>::value:
			m_values.emplace_back(std::move(n.get<string_node>()));
			return { true, nullptr };
		case node::index_of<integral_node>::value:
			m_values.emplace_back(std::move(n.get<integral_node>()));
			return { true, nullptr };
		case node::index_of<numeric_node>::value:
			m_values.emplace_back(std::move(n.get<numeric_node>()));
			return { true, nullptr };
		case node::index_of<char_node>::value:
			m_values.emplace_back(std::move(n.get<char_node>()));
			return { true, nullptr };
		case node::index_of<boolean_node>::value:
			m_values.emplace_back(std::move(n.get<boolean_node>()));
			return { true, nullptr };
		case node::index_of<null_node>::value:
			m_values.emplace_back(std::move(n.get<null_node>()));
			return { true, nullptr };
		default:
			return { false, nullptr };
	}
}

node array_rule::reduce()
{
	if (m_status != status::end)
		throw syntax_error("close_brace_token expected");
	return array_node { std::move(m_values) };
}


shift_result pair_rule::shift(token&& tk)
{
	switch(m_status)
	{
		case status::start:
			if(tk.which() != token::index_of<string_token>::value)
				return { false, nullptr };
			m_status = status::name;
			m_name = tk.get<string_token>().value;
			return { true, nullptr };
		case status::name:
			if(tk.which() != token::index_of<colon_token>::value)
				return { false, nullptr };
			m_status = status::colon;
			return { true, rule { value_rule {} } };
		case status::colon:
		case status::value:
		default:
			return { false, nullptr };
	}
}

shift_result pair_rule::shift(node&& n)
{
	if (m_status != status::colon)
		return { false, nullptr };
	switch(n.which())
	{
		case node::index_of<object_node>::value:
			m_value = value_node { std::move(n.get<object_node>()) };
			m_status = status::value;
			return { true, nullptr };
		case node::index_of<array_node>::value:
			m_value = value_node { std::move(n.get<array_node>()) };
			m_status = status::value;
			return { true, nullptr };
		case node::index_of<string_node>::value:
			m_value = value_node { n.get<string_node>() };
			m_status = status::value;
			return { true, nullptr };
		case node::index_of<integral_node>::value:
			m_value = value_node { n.get<integral_node>() };
			m_status = status::value;
			return { true, nullptr };
		case node::index_of<numeric_node>::value:
			m_value = value_node { n.get<numeric_node>() };
			m_status = status::value;
			return { true, nullptr };
		case node::index_of<char_node>::value:
			m_value = value_node { n.get<char_node>() };
			m_status = status::value;
			return { true, nullptr };
		case node::index_of<boolean_node>::value:
			m_value = value_node { n.get<boolean_node>() };
			m_status = status::value;
			return { true, nullptr };
		case node::index_of<null_node>::value:
			m_value = value_node { n.get<null_node>() };
			m_status = status::value;
			return { true, nullptr };
		default:
			return { true, nullptr };
	}
}

node pair_rule::reduce()
{
	switch(m_status)
	{
		case status::start:
		case status::name:
		case status::colon:
			throw corecpp::syntax_error(corecpp::concat<std::string>({ __func__," called when in state ", std::to_string((int)m_status)}));
		case status::value:
		default:
			return pair_node { { m_name }, std::move(m_value.get<value_node>()) };
	}
}


shift_result value_rule::shift(token&& tk)
{
	switch (tk.which())
	{
		case token::index_of<open_brace_token>::value:
			return { false, rule { object_rule {} } };
		case token::index_of<open_bracket_token>::value:
			return { false, rule { array_rule {} } };
		case token::index_of<string_token>::value:
			m_value = value_node { string_node { tk.get<string_token>().value } };
			return { true, nullptr };
		case token::index_of<numeric_token>::value:
			m_value = value_node { numeric_node { tk.get<numeric_token>().value } };
			return { true, nullptr };
		case token::index_of<integral_token>::value:
			m_value = value_node { integral_node { tk.get<integral_token>().value } };
			return { true, nullptr };
		case token::index_of<null_token>::value:
			m_value = value_node { null_node { } };
			return { true, nullptr };
		case token::index_of<true_token>::value:
			m_value = value_node { boolean_node { true } };
			return { true, nullptr };
		case token::index_of<false_token>::value:
			m_value = value_node { boolean_node { false } };
			return { true, nullptr };
		case token::index_of<close_brace_token>::value:
		case token::index_of<close_bracket_token>::value:
		case token::index_of<comma_token>::value:
		case token::index_of<dot_token>::value:
		case token::index_of<colon_token>::value:
		default:
			return { false, nullptr };
	}
}

shift_result value_rule::shift(node&& n)
{
	if (m_value.which() != corecpp::variant<std::nullptr_t, value_node>::index_of<std::nullptr_t>::value)
 		return { false, nullptr };
	switch(n.which())
	{
		case node::index_of<object_node>::value:
			m_value = value_node { std::move(n.get<object_node>()) };
			return { true, nullptr };
		case node::index_of<array_node>::value:
			m_value = value_node { std::move(n.get<array_node>()) };
			return { true, nullptr };
		default:
			return { false, nullptr };
	}
}

node value_rule::reduce()
{
	if (m_value.which() == corecpp::variant<std::nullptr_t, value_node>::index_of<std::nullptr_t>::value)
		throw corecpp::syntax_error(corecpp::concat<std::string>({ __func__," called when no value" }));
	return m_value.get<value_node>().apply([](auto& value) -> node { return { std::move(value) }; });
}


void parser::push(token&& tk)
{
	auto pushed = m_stack.back().apply([&tk](auto& r) -> shift_result { return r.shift(std::move(tk)); });
	while (!pushed.eaten)
	{
		if (pushed.next_rule.which() == corecpp::variant<std::nullptr_t, value_node>::index_of<std::nullptr_t>::value)
		{
			node n = m_stack.back().apply([](auto& r) -> node { return r.reduce(); });
			m_stack.pop_back();
			if (m_stack.empty())
				throw corecpp::syntax_error(corecpp::concat<std::string>({ __func__," unexpected token : ", to_string(tk)}));
			pushed = m_stack.back().apply([&n](auto& r) -> shift_result { return r.shift(std::move(n)); });
			if (!pushed.eaten)
				throw corecpp::syntax_error(corecpp::concat<std::string>({ __func__," unexpected token : ", to_string(tk)}));
			if (pushed.next_rule.which() == corecpp::variant<std::nullptr_t, value_node>::index_of<std::nullptr_t>::value)
				m_stack.emplace_back(std::move(pushed.next_rule.get<rule>()));
			pushed = m_stack.back().apply([&tk](auto& r) -> shift_result { return r.shift(std::move(tk)); });
		}
		else
		{
			m_stack.emplace_back(std::move(pushed.next_rule.get<rule>()));
			pushed = m_stack.back().apply([&tk](auto& r) -> shift_result { return r.shift(std::move(tk)); });
		}
	}
	if (pushed.next_rule.which() == corecpp::variant<std::nullptr_t, value_node>::index_of<std::nullptr_t>::value)
		m_stack.emplace_back(std::move(pushed.next_rule.get<rule>()));
}

node parser::end(void)
{
	for(;;)
	{
		node n = m_stack.back().apply([](auto& r) -> node { return r.reduce(); });
		m_stack.pop_back();
		if (m_stack.empty())
			return n;
		auto pushed = m_stack.back().apply([&n](auto& r) -> shift_result { return r.shift(std::move(n)); });
		if (!pushed.eaten)
			throw corecpp::syntax_error(corecpp::concat<std::string>({ __func__," unable to reduce node"}));
		if (pushed.next_rule.which() == corecpp::variant<std::nullptr_t, value_node>::index_of<std::nullptr_t>::value)
			m_stack.emplace_back(std::move(pushed.next_rule.get<rule>()));
	}
}


void serializer::convert_and_escape(const std::string& value)
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


void serializer::convert_and_escape(const std::wstring& value)
{
	static const char hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	/* TODO: support not only UTF8 */
	for(wchar_t c : value)
	{
		if (std::isprint((char)c))
			m_stream << (char)c;
		else if (c < 0x10)
			m_stream << "\\u000" << hex_chars[c & 0x0F];
		else if (c < 0x100)
			m_stream << "\\u00" << hex_chars[(c & 0xF0) << 4] << hex_chars[c & 0x0F];
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

token deserializer::read()
{
	// 1st try, no read
	auto token = m_tokenizer.next();
	if (token)
		return std::move(*token);
	// 2nd try, read available data
	auto available = m_stream.rdbuf()->in_avail();
	std::string buffer;
	buffer.resize(available);
	m_stream.read((char*)buffer.data(), available);
	m_tokenizer.eat(buffer);
	token = m_tokenizer.next();
	if (token)
		return std::move(*token);
	// 3rd try, read by block
	do
	{
		if (m_stream.eof())
			throw std::runtime_error("eof reached unexpectedly");
		buffer.resize(1024);
		m_stream.read((char*)buffer.data(), 1024);
		m_tokenizer.eat(buffer);
		token = m_tokenizer.next();
	}
	while (!token);
	return std::move(*token);
}

}


}
