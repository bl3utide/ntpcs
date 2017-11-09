#pragma once

#include <fstream>
#include <iomanip>
#include <ctime>
#include <vector>

enum LogAlign
{
    kLogAlignLeft = 0,
    kLogAlignRight
};

class Logger
{
public:
    Logger(const char*);
    ~Logger();
    void addFrontDate();
    void addEnd();

    template<typename T> void addOneLine(T msg)
    {
        addFrontDate();
        log_ << msg;
        addNewLine();
        log_.flush();
    }

    template<typename T> void addMessage(T msg)
    {
        log_ << msg;
    }

    template<typename T> void addMessage(LogAlign align, std::streamsize width, T msg)
    {
        if (align == kLogAlignLeft)
            log_ << std::left << std::setw(width) << std::setfill(' ') << msg;
        else if (align == kLogAlignRight)
            log_ << std::right << std::setw(width) << std::setfill(' ') << msg;
    }
    /*
    template<typename ... Msg>
    void addMessage(Msg ... msg)
    {
        std::vector<Msg> msgVec = { msg... };
        for (size_t i = 0; i < msgVec.size(); ++i)
        {
            log_ << msgVec.at(i);
            if (i != msgVec.size() - 1)
                log_ << ' ';
        }
        //log_ << msg;
    }

    template<typename ...Msg>
    void addMessage(LogAlign align, std::streamsize width, Msg ...msg)
    {
        std::vector<std::string> msgVec = { msg... };
        for (size_t i = 0; i < msgVec.size(); ++i)
        {
            if (align == kLogAlignLeft)
                log_ << std::setw(width) << std::left << msgVec.at(i);
            else if (align == kLogAlignRight)
                log_ << std::setw(width) << std::right << msgVec.at(i);

            if (i != msgVec.size() - 1)
                log_ << ' ';
        }
    }
    */

    /*
    void add(const char* msg, std::streamsize width, char fillchar)
    {
        log_ << std::setw(width) << std::setfill(fillchar) << msg;
        log_ << "\r\n";
        log_.flush();
    }
    */

protected:
    void addNewLine();

private:
    const char* file_path_;
    std::ofstream log_;
    void addNumFormat(int, std::streamsize, char);
    tm* getTime();
};