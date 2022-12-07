#include <cassert>
#include <cmath>

#include <codecvt>
#include <locale>
#include <memory>
#include <iomanip>

#include <corecpp/serialization/xml.h>


namespace corecpp::xml
{
//TODO move this definition

corecpp::diagnostic::channel& xml_channel()
{
	static auto& channel = corecpp::diagnostic::manager::get_channel("corecpp::xml");
	return channel;
}

corecpp::diagnostic::event_producer& xml_logger()
{
	static auto logger = corecpp::diagnostic::event_producer(xml_channel());
	return logger;
}

void serializer::convert_and_escape(const std::string& value)
{
	 for(size_t pos = 0; pos != value.size(); ++pos)
	 {
        switch(value[pos])
		{
			case '&':  m_stream << "&amp;";       break;
			case '\"': m_stream << "&quot;";      break;
			case '\'': m_stream << "&apos;";      break;
			case '<':  m_stream << "&lt;";        break;
			case '>':  m_stream << "&gt;";        break;
			default:   m_stream << value[pos];   break;
		}
	}
}


void serializer::convert_and_escape(const std::wstring& value)
{
	//TODO: utf8-encode
	for(size_t pos = 0; pos != value.size(); ++pos)
	{
		switch(value[pos])
		{
			case '&':  m_stream << "&amp;";       break;
			case '\"': m_stream << "&quot;";      break;
			case '\'': m_stream << "&apos;";      break;
			case '<':  m_stream << "&lt;";        break;
			case '>':  m_stream << "&gt;";        break;
			default:   m_stream << (char)value[pos];   break;
		}
	}
}


void serializer::convert_and_escape(const std::u16string& value)
{
	//TODO
}


void serializer::convert_and_escape(const std::u32string& value)
{
	//TODO
}


}
