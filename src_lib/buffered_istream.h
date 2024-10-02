#ifndef BUFFERED_ISTREAM_H
#define BUFFERED_ISTREAM_H

#include <istream>
#include <sstream>
#include <streambuf>

class buffered : public std::streambuf
{
    std::streambuf *m_streambuf;
    std::string m_buffer;
    int m_next_index;

public:
    buffered(
        std::streambuf *a_streambuf) : m_streambuf(a_streambuf),
                                       m_next_index(0)
    {
        setg(nullptr, nullptr, nullptr);
    }

    virtual ~buffered()
    {
    }

    virtual int_type underflow() override
    {
        if (m_next_index == m_buffer.size())
        {
            // reached eof of internal buffer

            // read 1 char and advance ptr of internal streambuf
            int l_source_char = m_streambuf->sbumpc();
            if (l_source_char == traits_type::eof())
                return traits_type::eof();

            // add the character to the restoration buffer
            m_buffer.push_back(l_source_char);

            // increment the cursor
            ++m_next_index;

            // set information pertaining to readable buffer space
            setg(m_buffer.data(),
                 m_buffer.data() + m_next_index,
                 m_buffer.data() + m_buffer.size());
        }

        return m_buffer[m_next_index];
    }
};

#endif
