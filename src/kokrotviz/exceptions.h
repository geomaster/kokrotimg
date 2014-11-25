#ifndef __KOKROTVIZ_EXCEPTIONS_H__
#define __KOKROTVIZ_EXCEPTIONS_H__
#include <exception>
#include <string>

namespace kokrotviz {
    class Exception : public std::exception {
    public:
        Exception(std::string What) : 
            std::exception(), mWhat(What)
        {
        }

        virtual const char* what() const throw() {
            return mWhat.c_str();
        }

        virtual ~Exception() { }

    protected:
        std::string mWhat;
        
    };

    class UnexpectedException : public Exception {
    public:
        UnexpectedException(std::string what) : Exception(what) {
            mWhat = "Unexpected exception: " + mWhat;
        }

        virtual ~UnexpectedException() { }
    };

    class FileException : public Exception {
    public:
        FileException(std::string what) : Exception(what) {
            mWhat = "File exception: " + mWhat;
        }

        virtual ~FileException() { }
    };

    class InvalidImageException : public Exception {
    public:
        InvalidImageException (std::string what) : Exception (what) {
            mWhat = "Invalid image exception: " + mWhat;
        }

        virtual ~InvalidImageException() { }
    };

    class DynamicLibraryException : public Exception {
    public:
        DynamicLibraryException (std::string what) : Exception (what) {
            mWhat = "Exception while loading shared library: " + mWhat;
        }

        virtual ~DynamicLibraryException() { }
    };

    class APIVersionMismatchException : public Exception {
    public:
        APIVersionMismatchException (std::string what) : Exception (what) {
            mWhat = "API version mismatch: " + mWhat;
        }

        virtual ~APIVersionMismatchException() { }
    };



}
#endif
