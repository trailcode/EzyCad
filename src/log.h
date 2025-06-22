#pragma once

#include <streambuf>
#include <string>

class GUI;

// Custom stream buffer to redirect stdout/stderr to log_message
class Log_strm : public std::streambuf
{
 public:
  Log_strm(GUI& gui, std::streambuf* original_buf);

 protected:
  int overflow(int c) override;
  int sync() override;

 private:
  GUI&            m_gui;
  std::streambuf* m_original_buf;
  std::string     m_buffer;
}; 