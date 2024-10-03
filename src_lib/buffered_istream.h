#ifndef BUFFERED_ISTREAM_H
#define BUFFERED_ISTREAM_H

#include <istream>
#include <sstream>
#include <streambuf>

class buffered : public std::streambuf
{
    std::streambuf *m_streambuf;
    std::string m_buffer;

public:
    buffered(
        std::streambuf *a_streambuf) : m_streambuf(a_streambuf)
    {
        setg(nullptr, nullptr, nullptr);
    }

    virtual ~buffered()
    {
    }

protected:

    virtual int_type underflow() override
    {
        
        return m_buffer[m_next_index];
    }

    virtual std::streampos seekoff(
        std::streamoff a_offset,
        std::ios_base::seekdir a_way,
        std::ios_base::openmode a_which = std::ios_base::in
    ) override
    {
        // handle which
        
        if (!(a_which & std::ios::in))
            // indicate that this streambuf only works with
            //     input data.
            return std::streampos(std::streamoff(-1));

        // handle way

        std::streamoff l_new_position;

        switch (a_way)
        {
            case std::ios_base::beg:
            {
                l_new_position = a_offset;
                break;
            }
            case std::ios_base::cur:
            {
                l_new_position = m_next_index + a_offset;
                break;
            }
            case std::ios_base::end:
            {
                l_new_position = m_buffer.size() + a_offset;
                break;
            }
            default:
            {
                return std::streampos(std::streamoff(-1));
            }
        }

        // return error if negative index

        if (l_new_position < 0)
            return std::streampos(std::streamoff(-1));

        if (m_buffer.size() < static_cast<size_t>(l_new_position))
        {

        }
        
        
    }

    int load_more_data(int a_n)
    {
        int i = 0;

        for (; i < a_n; ++i)
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

        

    }
    
};

#endif
