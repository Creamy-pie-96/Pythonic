#pragma once

#include <stdexcept>
#include <string>
#include <source_location>

namespace pythonic {

// ============================================================================
// Pythonic Error Hierarchy
// ============================================================================
// All exceptions inherit from PythonicError so users can catch all library
// errors with a single catch block, while still having granular control.
//
// Usage:
//   try { ... }
//   catch (const PythonicTypeError& e) { /* specific */ }
//   catch (const PythonicError& e) { /* any pythonic error */ }
// ============================================================================

/**
 * @brief Base class for all pythonic library exceptions.
 * 
 * Provides consistent "pythonic: " prefix in error messages and
 * optional source location tracking for debugging.
 */
class PythonicError : public std::runtime_error {
public:
    explicit PythonicError(const std::string& what) 
        : std::runtime_error("pythonic: " + what) {}

    // Constructor with source location for debugging
    PythonicError(const std::string& what, 
                  const std::source_location& loc)
        : std::runtime_error(format_with_location(what, loc)) {}

private:
    static std::string format_with_location(const std::string& what,
                                            const std::source_location& loc) {
        return std::string("pythonic: ") + what + 
               " [" + loc.file_name() + ":" + std::to_string(loc.line()) + "]";
    }
};

/**
 * @brief Warning base class (not an error, can be caught separately)
 */
class PythonicWarning : public std::exception {
public:
    explicit PythonicWarning(const std::string& what) 
        : msg_("pythonic warning: " + what) {}
    
    [[nodiscard]] const char* what() const noexcept override { 
        return msg_.c_str(); 
    }

private:
    std::string msg_;
};

// ============================================================================
// Type Errors - Wrong type operations
// ============================================================================

/**
 * @brief Raised when an operation receives a value of wrong type.
 * 
 * Examples: "hello" * "world", int("not a number")
 */
class PythonicTypeError : public PythonicError {
public:
    explicit PythonicTypeError(const std::string& what) 
        : PythonicError("TypeError: " + what) {}
};

// ============================================================================
// Value Errors - Right type, wrong value
// ============================================================================

/**
 * @brief Raised when a value is inappropriate (right type, wrong content).
 * 
 * Examples: range(0, 10, 0), int(""), sqrt(-1)
 */
class PythonicValueError : public PythonicError {
public:
    explicit PythonicValueError(const std::string& what) 
        : PythonicError("ValueError: " + what) {}
};

// ============================================================================
// Index Errors - Sequence access out of bounds
// ============================================================================

/**
 * @brief Raised when a sequence index is out of range.
 * 
 * Examples: list[100] when len(list) == 5
 */
class PythonicIndexError : public PythonicError {
public:
    explicit PythonicIndexError(const std::string& what) 
        : PythonicError("IndexError: " + what) {}
    
    // Convenience constructor with index info
    PythonicIndexError(const std::string& container_type, 
                       long long index, 
                       size_t size)
        : PythonicError("IndexError: " + container_type + 
                       " index " + std::to_string(index) + 
                       " out of range (size=" + std::to_string(size) + ")") {}
};

// ============================================================================
// Key Errors - Dict/Map key not found
// ============================================================================

/**
 * @brief Raised when a mapping key is not found.
 * 
 * Examples: dict["nonexistent_key"]
 */
class PythonicKeyError : public PythonicError {
public:
    explicit PythonicKeyError(const std::string& what) 
        : PythonicError("KeyError: " + what) {}
    
    // Convenience: KeyError with the actual key
    static PythonicKeyError for_key(const std::string& key) {
        return PythonicKeyError("'" + key + "'");
    }
};

// ============================================================================
// Arithmetic Errors
// ============================================================================

/**
 * @brief Raised when a numeric operation overflows.
 * 
 * Examples: INT_MAX + 1 after type promotion exhausted
 */
class PythonicOverflowError : public PythonicError {
public:
    explicit PythonicOverflowError(const std::string& what) 
        : PythonicError("OverflowError: " + what) {}
};

/**
 * @brief Raised when division or modulo by zero occurs.
 */
class PythonicZeroDivisionError : public PythonicError {
public:
    explicit PythonicZeroDivisionError(const std::string& what) 
        : PythonicError("ZeroDivisionError: " + what) {}
    
    // Common case factory methods
    static PythonicZeroDivisionError division() {
        return PythonicZeroDivisionError(std::string("division by zero"));
    }
    
    static PythonicZeroDivisionError modulo() {
        return PythonicZeroDivisionError(std::string("modulo by zero"));
    }
};

// ============================================================================
// File/IO Errors
// ============================================================================

/**
 * @brief Raised when a file operation fails.
 * 
 * Examples: File not found, permission denied, unable to open
 */
class PythonicFileError : public PythonicError {
public:
    explicit PythonicFileError(const std::string& what) 
        : PythonicError("FileError: " + what) {}
    
    // Convenience constructors
    static PythonicFileError not_found(const std::string& filename) {
        return PythonicFileError("file not found: '" + filename + "'");
    }
    
    static PythonicFileError cannot_open(const std::string& filename) {
        return PythonicFileError("cannot open file: '" + filename + "'");
    }
    
    static PythonicFileError not_open() {
        return PythonicFileError(std::string("file is not open"));
    }
};

// ============================================================================
// Attribute Errors
// ============================================================================

/**
 * @brief Raised when an attribute reference or assignment fails.
 * 
 * Examples: Calling .upper() on an int
 */
class PythonicAttributeError : public PythonicError {
public:
    explicit PythonicAttributeError(const std::string& what) 
        : PythonicError("AttributeError: " + what) {}
    
    // Convenience: type has no attribute
    static PythonicAttributeError no_attribute(const std::string& type, 
                                                const std::string& attr) {
        return PythonicAttributeError("'" + type + "' has no attribute '" + attr + "'");
    }
};

// ============================================================================
// Graph-Specific Errors
// ============================================================================

/**
 * @brief Raised for graph-specific operation failures.
 * 
 * Examples: Topological sort on cyclic graph, invalid node/edge
 */
class PythonicGraphError : public PythonicError {
public:
    explicit PythonicGraphError(const std::string& what) 
        : PythonicError("GraphError: " + what) {}
    
    // Common graph errors
    static PythonicGraphError invalid_node(size_t node) {
        return PythonicGraphError("invalid node " + std::to_string(node));
    }
    
    static PythonicGraphError invalid_node(size_t node, size_t num_nodes) {
        return PythonicGraphError("invalid node " + std::to_string(node) + 
                                  " (graph has " + std::to_string(num_nodes) + " nodes)");
    }
    
    static PythonicGraphError edge_not_found(size_t from, size_t to) {
        return PythonicGraphError("edge not found: " + std::to_string(from) + 
                                  " -> " + std::to_string(to));
    }
    
    static PythonicGraphError has_cycle() {
        return PythonicGraphError(std::string("graph contains a cycle"));
    }
    
    static PythonicGraphError not_implemented(const std::string& feature) {
        return PythonicGraphError(feature + " not implemented");
    }
};

// ============================================================================
// Iterator Errors
// ============================================================================

/**
 * @brief Raised when an iterator is exhausted or used incorrectly.
 */
class PythonicIterationError : public PythonicError {
public:
    explicit PythonicIterationError(const std::string& what) 
        : PythonicError("IterationError: " + what) {}
};

/**
 * @brief Signals iterator exhaustion (like Python's StopIteration).
 * 
 * This is typically caught internally, not by end users.
 */
class PythonicStopIteration : public PythonicError {
public:
    PythonicStopIteration() : PythonicError(std::string("StopIteration")) {}
    
    explicit PythonicStopIteration(const std::string& what) 
        : PythonicError("StopIteration: " + what) {}
};

// ============================================================================
// Runtime & Not Implemented Errors
// ============================================================================

/**
 * @brief General runtime errors that don't fit other categories.
 */
class PythonicRuntimeError : public PythonicError {
public:
    explicit PythonicRuntimeError(const std::string& what) 
        : PythonicError("RuntimeError: " + what) {}
};

/**
 * @brief Raised when a feature is not yet implemented.
 */
class PythonicNotImplementedError : public PythonicError {
public:
    explicit PythonicNotImplementedError(const std::string& what) 
        : PythonicError("NotImplementedError: " + what) {}
    
    PythonicNotImplementedError() 
        : PythonicError(std::string("NotImplementedError")) {}
};

// ============================================================================
// Helper Macros
// ============================================================================

/**
 * Throw a pythonic exception with source location info (debug builds)
 */
#ifdef NDEBUG
    #define PYTHONIC_THROW(ExType, msg) throw ExType(std::string(msg))
#else
    #define PYTHONIC_THROW(ExType, msg) \
        throw ExType(std::string(msg), std::source_location::current())
#endif

/**
 * Assert-like macro that throws on failure
 */
#define PYTHONIC_ASSERT(cond, ExType, msg) \
    do { if (!(cond)) { throw ExType(std::string(msg)); } } while(0)

} // namespace pythonic
