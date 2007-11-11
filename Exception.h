#ifndef GFRACT_EXCEPTION_H
#define GFRACT_EXCEPTION_H

#include <exception>
#include <string>

/** Base class for all of gfract's exceptions. */
class GfractException : public std::exception
{
public:
    GfractException(const std::string& msg)
    {
        this->msg = msg;
    }

    virtual ~GfractException() throw() { }

    virtual const char* what() const throw ()
    {
        return msg.c_str();
    }

private:
    std::string msg;
};

#endif
