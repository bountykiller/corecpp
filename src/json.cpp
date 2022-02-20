#include <cassert>
#include <cmath>

#include <codecvt>
#include <locale>
#include <memory>
#include <iomanip>

#include <corecpp/serialization/json.h>


namespace corecpp::json
{

corecpp::diagnostic::channel& json_channel()
{
	static auto& channel = corecpp::diagnostic::manager::get_channel("corecpp::json");
	return channel;
}

corecpp::diagnostic::event_producer& json_logger()
{
	static auto logger = corecpp::diagnostic::event_producer(json_channel());
	return logger;
}

std::string to_string(const token& tk)
{
	switch(tk.index())
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
	corecpp::throws<std::logic_error>("unreachable");
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

	assert (m_buffer.in_avail() >= 4);

	wchar_t a,b,c,d;
	a = conversion_tab[(int)m_buffer.sbumpc()];
	b = conversion_tab[(int)m_buffer.sbumpc()];
	c = conversion_tab[(int)m_buffer.sbumpc()];
	d = conversion_tab[(int)m_buffer.sbumpc()];
	if ((a < 0) || (b < 0) || (c < 0) || (d < 0))
		corecpp::throws<lexical_error>(corecpp::concat<std::string>({"invalid unicode escape sequence: ", std::to_string(a), ",",
											std::to_string(b),  ",", std::to_string(c),  ",", std::to_string(d)}));
	return (a << 12) + (b << 8) + (c << 4) + d;
}

std::unique_ptr<token> tokenizer::read_string_literal()
{
	if (m_buffer.in_avail() <= 0)
		return nullptr;

	std::wstring literal;
	bool good = false;

	while (!good && (m_buffer.in_avail() > 0))
	{
		int c = m_buffer.sbumpc();
		json_logger().debug("string_literal : read char", std::string(1, c), __FILE__, __LINE__);
		switch (c)
		{
			case '\"':
				good = true;
				break;
			/* control-characters */
			case '\r':
			case '\n':
				corecpp::throws<lexical_error>("invalid string expression : unexpected end of line");
			case '\\':
				if (m_buffer.in_avail() <= 0)
					return nullptr;
				switch (c = m_buffer.sbumpc())
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
						if (m_buffer.in_avail() < 4)
							corecpp::throws<lexical_error>("invalid string expression : unterminated escape sequence");
						/* \u four-hex-digits */
						literal += read_escaped_char();
						break;
					default:
						corecpp::throws<lexical_error>("invalid string expression : unknown escape sequence");
				}
				break;
			default:
				literal += c;
				break;
		}
	}

	if(!good)
		return nullptr;

	json_logger().trace("string_literal read", std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(literal), __FILE__, __LINE__);
	return std::make_unique<token>(string_token { literal });
}

std::unique_ptr<token> tokenizer::read_numeric_literal(char c)
{
	bool negative = false;
	long integral_value = 0;

	if (c == '-')
	{
		if (m_buffer.in_avail() <= 0)
			return nullptr;
		negative = true;
		c = m_buffer.sbumpc();
	}

	for(; m_buffer.in_avail() >= 0; c = m_buffer.sbumpc())
	{
		switch (c)
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
				integral_value = (10 * integral_value) + (c - '0');
				break;
			case '.':
			{
				long decimal_value = 0;
				unsigned int decimal_precision = 1;
				while (m_buffer.in_avail() >= 0)
				{
					c = m_buffer.sbumpc();
					switch (c)
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
							decimal_value = (10 * decimal_value) + (c - '0');
							decimal_precision *= 10;
							break;
						case 'e':
						case 'E':
						{
							long exponential_value = 0;
							bool negative_exponential = false;

							if (m_buffer.in_avail() < 0)
								return nullptr;

							c = m_buffer.sbumpc();
							if (c == '-' || c == '+')
							{
								negative_exponential = (c == '-');
								if (m_buffer.in_avail() < 0)
									return nullptr;
								c = m_buffer.sbumpc();
							}

							for (; m_buffer.in_avail() >= 0; c = m_buffer.sbumpc())
							{
								switch(c)
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
										exponential_value = (10 * decimal_value) + (c - '0');
										break;
									default:
									{
										double value = exp((double)integral_value + ((double)decimal_value/decimal_precision));
										m_buffer.sungetc();
										return std::make_unique<token>(numeric_token { negative ? -value : value });
									}
								}
							}
							break;
						}
						default:
						{
							double value = (double)integral_value + ((double)decimal_value/decimal_precision);
							m_buffer.sungetc();
							return std::make_unique<token>(numeric_token { negative ? -value : value });
						}
					}
				}
				break;
			}
			default:
				m_buffer.sungetc();
				return std::make_unique<token>(integral_token { negative ? -integral_value : integral_value });
		}
	}

	return nullptr;
}

std::unique_ptr<token> tokenizer::next()
{
	char c;
	if (m_buffer.in_avail() <= 0)
	{
		m_buffer.pubsync();
		if (m_buffer.in_avail() <= 0)
		{
			json_logger().trace("No more token to parse", __FILE__, __LINE__);
			return nullptr;
		}
	}

	for (c = m_buffer.sbumpc(); std::isspace(c, m_locale); c = m_buffer.sbumpc())
	{
		if (m_buffer.in_avail() <= 0)
		{
			json_logger().trace("Skipped blank caracters, no more token to parse", __FILE__, __LINE__);
			return nullptr;
		}
		json_logger().trace("Skipped blank caracter", __FILE__, __LINE__);
	}

	pos_type pos = m_buffer.pubseekoff(0, std::ios_base::cur, std::ios_base::in);
	json_logger().debug("parse token", corecpp::concat<std::string>({ "at pos ", std::to_string(pos), "'", std::to_string(c), "'"  }),
			__FILE__, __LINE__);
	switch (c)
	{
		case '{':
		{
			return std::make_unique<token>(open_brace_token());
		}
		case '}':
		{
			return std::make_unique<token>(close_brace_token());
		}
		case '[':
		{
			return std::make_unique<token>(open_bracket_token());
		}
		case ']':
		{
			return std::make_unique<token>(close_bracket_token());
		}
		case ',':
		{
			return std::make_unique<token>(comma_token());
		}
		case ':':
		{
			return std::make_unique<token>(colon_token());
		}
		case '"':
		{
			auto res = read_string_literal();
			if (!res)
				m_buffer.pubseekpos(pos, std::ios_base::in);
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
			auto res = read_numeric_literal(c);
			if (!res)
				m_buffer.pubseekpos(pos, std::ios_base::in);
			return res;
		}
		case 'f':
		{
			if ((c = m_buffer.sbumpc()) == 'a'
				&& (c = m_buffer.sbumpc()) == 'l'
				&& (c = m_buffer.sbumpc()) == 's'
				&& (c = m_buffer.sbumpc()) == 'e'
				&& (!isalnum((c = m_buffer.sgetc()), m_locale)))
			{
				return std::make_unique<token>(false_token());
			}
			else
			{
				if (c == EOF)
				{
					m_buffer.pubseekpos(pos, std::ios_base::in);
					return nullptr;
				}
				else
				{
					m_buffer.pubseekpos(pos, std::ios_base::in);
					corecpp::throws<corecpp::syntax_error>(reminder());
				}
			}
		}
		case 'n':
			if ((c = m_buffer.sbumpc()) == 'u'
				&& (c = m_buffer.sbumpc()) == 'l'
				&& (c = m_buffer.sbumpc()) == 'l'
				&& (!isalnum((c = m_buffer.sgetc()), m_locale)))
			{
				return std::make_unique<token>(null_token());
			}
			else
			{
				if (c == EOF)
				{
					m_buffer.pubseekpos(pos, std::ios_base::in);
					return nullptr;
				}
				else
				{
					m_buffer.pubseekpos(pos, std::ios_base::in);
					corecpp::throws<corecpp::syntax_error>(reminder());
				}
			}

		case 't':
			if ((c = m_buffer.sbumpc()) == 'r'
				&& (c = m_buffer.sbumpc()) == 'u'
				&& (c = m_buffer.sbumpc()) == 'e'
				&& (!isalnum((c = m_buffer.sgetc()), m_locale)))
			{
				return std::make_unique<token>(true_token());
			}
			else
			{
				if (c == EOF)
				{
					m_buffer.pubseekpos(pos, std::ios_base::in);
					return nullptr;
				}
				else
				{
					m_buffer.pubseekpos(pos, std::ios_base::in);
					corecpp::throws<corecpp::syntax_error>(reminder());
				}
			}
		default:
			m_buffer.pubseekpos(pos, std::ios_base::in);
			return nullptr;
	}
}

/*
 * NODES
 */
value_node& object_node::at (const std::wstring& key)
{
	for (auto& value : members)
	{
		if (value.name.value == key)
			return value.value;
	}
	corecpp::throws<std::overflow_error>(std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(key));
}
const value_node& object_node::at (const std::wstring& key) const
{
	for (const auto& value : members)
	{
		if (value.name.value == key)
			return value.value;
	}
	corecpp::throws<std::overflow_error>(std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(key));
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
			switch (tk.index())
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
			switch (tk.index())
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
	if(n.index() != node::index_of<pair_node>::value)
		return { false, nullptr };
	m_members.emplace_back(std::move(n.get<pair_node>()));
	return { true, nullptr };
}

node object_rule::reduce()
{
	if (m_status != status::end)
		corecpp::throws<syntax_error>("close_brace_token expected");
	return object_node { std::move(m_members) };
}


shift_result array_rule::shift(token&& tk)
{
	switch (m_status)
	{
		case status::start:
		{
			switch (tk.index())
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
			switch (tk.index())
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
	switch(n.index())
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
		corecpp::throws<syntax_error>("close_brace_token expected");
	return array_node { std::move(m_values) };
}


shift_result pair_rule::shift(token&& tk)
{
	switch(m_status)
	{
		case status::start:
			if(tk.index() != token::index_of<string_token>::value)
				return { false, nullptr };
			m_status = status::name;
			m_name = tk.get<string_token>().value;
			return { true, nullptr };
		case status::name:
			if(tk.index() != token::index_of<colon_token>::value)
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
	switch(n.index())
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
			corecpp::throws<corecpp::syntax_error>(corecpp::concat<std::string>({ __func__," called when in state ", std::to_string((int)m_status)}));
		case status::value:
		default:
			return pair_node { { m_name }, std::move(m_value.value()) };
	}
}


shift_result value_rule::shift(token&& tk)
{
	switch (tk.index())
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
	if (m_value.index() != corecpp::variant<std::nullptr_t, value_node>::index_of<std::nullptr_t>::value)
 		return { false, nullptr };
	switch(n.index())
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
	if (m_value.index() == corecpp::variant<std::nullptr_t, value_node>::index_of<std::nullptr_t>::value)
		corecpp::throws<corecpp::syntax_error>(corecpp::concat<std::string>({ __func__," called when no value" }));
	return m_value.get<value_node>().visit([](auto& value) -> node { return { std::move(value) }; });
}


void parser::push(token&& tk)
{
	auto pushed = m_stack.back().visit([&tk](auto& r) -> shift_result { return r.shift(std::move(tk)); });
	while (!pushed.eaten)
	{
		if (pushed.next_rule.index() == corecpp::variant<std::nullptr_t, value_node>::index_of<std::nullptr_t>::value)
		{
			node n = m_stack.back().visit([](auto& r) -> node { return r.reduce(); });
			m_stack.pop_back();
			if (m_stack.empty())
				corecpp::throws<corecpp::syntax_error>(corecpp::concat<std::string>({ __func__," unexpected token : ", to_string(tk)}));
			pushed = m_stack.back().visit([&n](auto& r) -> shift_result { return r.shift(std::move(n)); });
			if (!pushed.eaten)
				corecpp::throws<corecpp::syntax_error>(corecpp::concat<std::string>({ __func__," unexpected token : ", to_string(tk)}));
			if (pushed.next_rule.index() == corecpp::variant<std::nullptr_t, value_node>::index_of<std::nullptr_t>::value)
				m_stack.emplace_back(std::move(pushed.next_rule.get<rule>()));
			pushed = m_stack.back().visit([&tk](auto& r) -> shift_result { return r.shift(std::move(tk)); });
		}
		else
		{
			m_stack.emplace_back(std::move(pushed.next_rule.get<rule>()));
			pushed = m_stack.back().visit([&tk](auto& r) -> shift_result { return r.shift(std::move(tk)); });
		}
	}
	if (pushed.next_rule.index() == corecpp::variant<std::nullptr_t, value_node>::index_of<std::nullptr_t>::value)
		m_stack.emplace_back(std::move(pushed.next_rule.get<rule>()));
}

node parser::end(void)
{
	for(;;)
	{
		node n = m_stack.back().visit([](auto& r) { return r.reduce(); });
		m_stack.pop_back();
		if (m_stack.empty())
			return n;
		auto pushed = m_stack.back().visit([&n](auto& r) -> shift_result { return r.shift(std::move(n)); });
		if (!pushed.eaten)
			corecpp::throws<corecpp::syntax_error>(corecpp::concat<std::string>({ __func__," unable to reduce node"}));
		if (pushed.next_rule.index() == corecpp::variant<std::nullptr_t, value_node>::index_of<std::nullptr_t>::value)
			m_stack.emplace_back(std::move(pushed.next_rule.get<rule>()));
	}
}


void serializer::convert_and_escape(const std::string& value)
{
	static const char hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	for(unsigned char c : value)
	{
		if (std::isprint((char)c))
		{
			if ( (char)c == '\\' || (char)c == '\"' )
				m_stream << '\\';
			m_stream << (char)c;
		}
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
	for (wchar_t c : value)
	{
		if (std::isprint((char)c))
		{
			if ( (char)c == '\\' || (char)c == '\"' )
				m_stream << '\\';
			m_stream << (char)c;
		}
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
			corecpp::throws<std::range_error>(corecpp::concat<std::string>({ "Unable to convert charcode ", std::to_string((uint32_t)c) }));
	}
}


void serializer::convert_and_escape(const std::u16string& value)
{
	static const char hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	/* TODO: support not only UTF8 */
	for (char16_t c : value)
	{
		if (std::isprint((char)c))
		{
			if ( (char)c == '\\' || (char)c == '\"' )
				m_stream << '\\';
			m_stream << (char)c;
		}
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
		else
			corecpp::throws<std::range_error>(corecpp::concat<std::string>({ "Unable to convert charcode ", std::to_string((uint32_t)c) }));
	}
}


void serializer::convert_and_escape(const std::u32string& value)
{
	static const char hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	/* TODO: support not only UTF8 */
	for (char32_t c : value)
	{
		if (std::isprint((char)c))
		{
			if ( (char)c == '\\' || (char)c == '\"' )
				m_stream << '\\';
			m_stream << (char)c;
		}
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
			corecpp::throws<std::range_error>(corecpp::concat<std::string>({ "Unable to convert charcode ", std::to_string((uint32_t)c) }));
	}
}


void deserializer::read_string(const string_token& wstr, std::string& value)
{
	const wchar_t* wchar = wstr.value.data();
	std::mbstate_t state = std::mbstate_t();
	auto len = std::wcsrtombs(nullptr, &wchar, 0, &state);
	if (len == static_cast<std::size_t>(-1))
		corecpp::throws<std::runtime_error>("converion error");
	value.resize(len);
	std::wcsrtombs((char*)value.data(), &wchar, len, &state);
}


void deserializer::read_string(const string_token& wstr, std::u16string& value)
{
	const wchar_t* wchar = wstr.value.data();
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convw;
	auto u8 = convw.to_bytes(wchar, wchar + wstr.value.size());
	std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> conv16;
	value = conv16.from_bytes(u8);
}


void deserializer::read_string(const string_token& wstr, std::u32string& value)
{
	const wchar_t* wchar = wstr.value.data();
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convw;
	auto u8 = convw.to_bytes(wchar, wchar + wstr.value.size());
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv32;
	value = conv32.from_bytes(u8);
}


void deserializer::read()
{
	// 1st try, no read
	auto token = m_tokenizer.next();
	if (token)
	{
		json_logger().trace("read token", to_string(*token), __FILE__, __LINE__);
		m_current = std::move(*token);
		return;
	}
	do
	{
		if (m_stream.eof())
			corecpp::throws<std::runtime_error>("eof reached unexpectedly");
		m_stream.peek(); /* does this block until next caracters are available?  */
		token = m_tokenizer.next();
	}
	while (!token);
	json_logger().trace("read token", to_string(*token), __FILE__, __LINE__);
	m_current = std::move(*token);
}


}
