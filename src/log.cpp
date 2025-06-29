#include "log.h"

#include "gui.h"

Log_strm::Log_strm(GUI& gui, std::streambuf* original_buf)
    : m_gui(gui), m_original_buf(original_buf)
{
  EZY_ASSERT(original_buf);
}

int Log_strm::overflow(int c)
{
  if (c != EOF)
  {
    if (c == '\n') // Treat as flush
    {
      if (!m_buffer.empty())
        m_gui.log_message(m_buffer);

      m_buffer.clear();
    }
    else
      m_buffer += static_cast<char>(c);

    // Forward to original console
    m_original_buf->sputc(c);
  }
  return c;
}

int Log_strm::sync()
{
  if (!m_buffer.empty())
  {
    m_gui.log_message(m_buffer);
    m_buffer.clear();
  }

  return m_original_buf ? m_original_buf->pubsync() : 0;
}