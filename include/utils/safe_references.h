#pragma once

#include <stdexcept>
#include <cassert>
#include <vector>

// ============================================================================
// PHASE 1: Safe Pointer Validation Layer
// ============================================================================
// Provides bounds-checked access to raw pointers and arrays
// Replaces unsafe raw pointer dereferences with validated lookups

namespace SafeReferences {

// Result wrapper for optional-like behavior without C++17 std::optional
template <typename T>
struct OptionalResult {
    bool has_value;
    size_t value;
    
    OptionalResult() : has_value(false), value(0) {}
    explicit OptionalResult(size_t v) : has_value(true), value(v) {}
    
    bool hasValue() const { return has_value; }
    size_t getValue() const { return value; }
};

// Validator for raw pointer access within container bounds
template <typename T>
class PointerValidator {
public:
    // Check if pointer belongs to container and return its index
    static OptionalResult<size_t> getIndex(const T* ptr, const std::vector<T*>& container) {
        if (!ptr) return OptionalResult<size_t>();
        for (size_t i = 0; i < container.size(); ++i) {
            if (container[i] == ptr) return OptionalResult<size_t>(i);
        }
        return OptionalResult<size_t>(); // Pointer not in container
    }

    // Safe dereference with bounds checking
    static T* safeDeref(const T* ptr, const std::vector<T*>& container) {
        if (!ptr) {
            throw std::runtime_error("Null pointer dereference attempted");
        }
        OptionalResult<size_t> idx = getIndex(ptr, container);
        if (!idx.hasValue()) {
            throw std::runtime_error("Pointer not found in container (likely invalidated)");
        }
        return const_cast<T*>(ptr);
    }

    // Validate that all pointers in container are valid (no nullptrs)
    static bool validateContainer(const std::vector<T*>& container) {
        for (const auto* ptr : container) {
            if (!ptr) return false;
        }
        return true;
    }
};

// Safe wrapper for vector access by index
template <typename T>
class SafeVectorAccess {
public:
    static bool tryAt(std::vector<T>& vec, size_t index, T*& outRef) {
        if (index >= vec.size()) return false;
        outRef = &vec[index];
        return true;
    }

    static bool tryAt(const std::vector<T>& vec, size_t index, const T*& outRef) {
        if (index >= vec.size()) return false;
        outRef = &vec[index];
        return true;
    }

    static bool isValidIndex(const std::vector<T>& vec, size_t index) {
        return index < vec.size();
    }
};

// Type-safe index for team lookups
struct TeamIndex {
    size_t value;
    bool isInvalid;
    
    explicit TeamIndex(size_t v) : value(v), isInvalid(v == static_cast<size_t>(-1)) {
        if (isInvalid) {
            throw std::invalid_argument("Invalid TeamIndex value");
        }
    }
    
    bool isValid() const { return !isInvalid; }
};

} // namespace SafeReferences
