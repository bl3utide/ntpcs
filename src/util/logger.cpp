#include "logger.h"

Logger::Logger(const char* file_path)
    : file_path_(file_path)
{
    log_.open(file_path, std::ios::out | std::ios::app);
}

Logger::~Logger()
{
    log_.close();
}

void Logger::addFrontDate()
{
    tm* time = getTime();
    addNumFormat(time->tm_year + 1900, 4, '0');
    log_ << '-';
    addNumFormat(time->tm_mon, 2, '0');
    log_ << '-';
    addNumFormat(time->tm_mday, 2, '0');
    log_ << ' ';
    addNumFormat(time->tm_hour, 2, '0');
    log_ << ':';
    addNumFormat(time->tm_min, 2, '0');
    log_ << ':';
    addNumFormat(time->tm_sec, 2, '0');
    log_ << ' ';
}

void Logger::addEnd()
{
    addNewLineCharacter();
    log_.flush();
}

void Logger::addNewLineCharacter()
{
    log_ << "\r\n";
}

void Logger::addNumFormat(int v, std::streamsize w, char fc)
{
    log_ << std::right << std::setw(w) << std::setfill(fc) << v;
}

tm* Logger::getTime()
{
    time_t now = std::time(nullptr);
    return localtime(&now);
}