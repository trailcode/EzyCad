#include "log.h"
#include "gui.h"

Log_strm::Log_strm(GUI& gui, std::streambuf* original_buf)
    : m_gui(gui), m_original_buf(original_buf) {}

int Log_strm::overflow(int c)
{
  if (c != EOF)
  {
    m_buffer += static_cast<char>(c);
    if (c == '\n')
    {
      // Remove trailing newline for log_message
      if (!m_buffer.empty() && m_buffer.back() == '\n')
        m_buffer.pop_back();
      if (!m_buffer.empty())
        m_gui.log_message(m_buffer);
      m_buffer.clear();
    }
    // Forward to original console
    if (m_original_buf)
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