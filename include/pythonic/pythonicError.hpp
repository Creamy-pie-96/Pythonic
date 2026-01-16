#pragma once
#include <stdexcept>

class PythonicError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
    PythonicError(const std::string &what) : std::runtime_error(what) {}
};

class PythonicWarning : public std::exception  // Changed to std::exception since warnings aren't errors
{
public:
    PythonicWarning(const std::string &what) : msg_(what) {}
    const char *what() const noexcept override { return msg_.c_str(); }

private:
    std::string msg_;
};

class PythonicIndexError : public PythonicError
{
public:
    PythonicIndexError(const std::string &what) : PythonicError(what) {}
};
class PythonicKeyError : public PythonicError
{
public:
    PythonicKeyError(const std::string &what) : PythonicError(what) {}
};
class PythonicValueError : public PythonicError
{
public:
    PythonicValueError(const std::string &what) : PythonicError(what) {}
};
class PythonicTypeError : public PythonicError
{
public:
    PythonicTypeError(const std::string &what) : PythonicError(what) {}
};

class PythonicOverflowError : public PythonicError
{
public:
    PythonicOverflowError(const std::string &what) : PythonicError(what) {}
};
class PythonicZeroDivisionError : public PythonicError
{
public:
    PythonicZeroDivisionError(const std::string &what) : PythonicError(what) {}
};
