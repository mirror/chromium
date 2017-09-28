// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WTF_ContainerTypeOperations_h
#define WTF_ContainerTypeOperations_h

// *****************************************************************************
// EXPERIMENTAL: DO NOT USE IN PRODUCTION CODE YET!
// *****************************************************************************

#include <cstring>
#include <type_traits>
#include <utility>

namespace WTF {
namespace experimental {

// ContainerTypeOperations defines type operations in containers that are not
// specific to container implementation, such as placement initialization and
// copying or moving elements.
//
// The class template ContainerTypeOperations can be specialized so you can
// define customized operations such as fast copy with memcpy().
//
// TODO(yutak): Implement good enough defaults for common data types such as
// fundamental types or RefPtr<T>, so in most cases people don't have to care
// about those functions.
//
// ====================================
// ContainerTypeOperations requirements
// ====================================
//
// Each ContainerTypeOperations implementation must meet all the requiements
// marked as "minimal complete definition" below. The absent functions will be
// supplemented with sensible defaults created from present functions.
//
// Naming conventions used below:
//
//     uninitialized_storage (or simply "uninitialized")
//         A variable name indicating the memory it points to is in the
//         container's storage, and is uninitialized, that is, T's constructor
//         is not called on it.
//     storage
//         A variable name indicating the memory it points to is in the
//         container's storage, and is initialized.
//     value
//         A variable name indicating the memory it points to is not necessarily
//         in the container's storage. Generally, it indicates a value specified
//         by the user of the container.
//     T
//         The main value type of the container. The container's storage will
//         contain an array of Ts.
//     InT
//         A type name that's possibly different from T. The actual type depends
//         on what the user code has specified to a member function of a
//         container. This is useful to avoid type conversions to T, and this
//         technique is called "heterogeneous lookup" (introduced in C++14
//         version of std containers).
//
//         In the actual traits implementation, the corresponding function does
//         not have to be a function template. It can be a set of overloaded
//         functions, or even a combination of function templates and
//         functions, as long as all the desirable input types can be accepted.
//         For example, suppose you are implementing Assign() explained below.
//         In this case, both of the following are OK:
//
//         (a) Accepts any input value
//             template <typename InT>
//             static void Assign(T& storage, InT&& value);
//
//         (b) Accepts String and const char* (suppose T is String-ish)
//             static void Assign(T& storage, const String& value);
//             static void Assign(T& storage, const char* value);
//
//         At least, T must be acceptable as a replacement of InT in any
//         function definitions.
//
// 1. Default initialization
//
//     static void DefaultInitialize(T& uninitialized);
//     static void DefaultInitializeRange(T* uninitialized_begin,
//                                        T* uninitialized_end);
//
//     Minimal complete definition:
//     DefaultInitialize() or DefaultInitializeRange().
//
//     After one of these functions is called, the memory range will become
//     initialized.
//
// 2. Destruction
//
//     static void Destruct(T& storage);
//     static void DestructRange(T* storage_begin,
//                               T* storage_end);
//
//     Minimal complete definition: Destruct() or DestructRange().
//
//     After one of these functions is called, the memory range will become
//     uninitialized.
//
// 3. Copy and move
//
//     static void Assign(T& storage, const InT& value);  // Copy
//     static void Assign(T& storage, InT&& value);  // Move
//     // Note: you can implement both in one function template if you use
//     // "forwarding reference".
//
//     static void CopyRange(const InT* value_begin,
//                           const InT* value_end,
//                           T* storage_begin);
//     static void MoveRange(InT* value_begin,
//                           InT* value_end,
//                           T* storage_begin);
//
//     static void CopyOverlappingRange(const InT* value_begin,
//                                      const InT* value_end,
//                                      T* storage_begin);
//     static void MoveOverlappingRange(InT* value_begin,
//                                      InT* value_end,
//                                      T* storage_begin);
//
//     Minimal complete definition: Copying Assign().
//
//     The storage will remain initialized even after moving, as if you call
//     std::move(). You need to destruct the elements separately.
//
// 4. Uninitialized copy and fill
//
//     static void UninitializedCopy(const InT* value_begin,
//                                   const InT* value_end,
//                                   T* uninitialized_begin);
//
//     static void UninitializedFill(const InT& value,
//                                   T* uninitialized_begin,
//                                   T* uninitialized_end);
//
//     Minimal complete definition: None (Will be implemented as initialize +
//     copy).
//
// 5. Equality
//
//     static bool Equal(const T& stored_value,
//                       const InT& other_value);
//     static bool EqualRange(const T* storage_begin,
//                            const T* storage_end,
//                            const InT* other_begin);
//
//     Minimal complete definition: Equal() or EqualRange().

//
// GenericContainerTypeOperations
//

// This class template defines type operations that work for any type T.
template <typename T>
struct GenericContainerTypeOperations {
  static void DefaultInitialize(T& uninitialized);

  static void Destruct(T& storage);

  template <typename InT>
  static void Assign(T& storage, InT&& value);

  template <typename InT>
  static void UninitializedCopy(const InT* value_begin,
                                const InT* value_end,
                                T* uninitialized_begin);

  template <typename InT>
  static void UninitializedFill(const InT& value,
                                T* uninitialized_begin,
                                T* uninitialized_end);

  template <typename InT>
  static bool Equal(const T& stored_value, const InT& other_value);
};

//
// PodContainerTypeOperations
//

// This class template controls whether WTF containers regard the type T as
// "POD-like". WTF containers assume POD-like types don't need an explicit
// constructor/destructor call, and it's safe to use memset, memcpy, memmove or
// memcmp functions over an array of POD-like types.
//
// You can specialize this traits to specify your own type that isn't POD as
// a POD-like type.
template <typename T>
struct IsPodlike : public std::is_pod<T> {};

// This predicate returns true if it's safe to memcpy an array of |From|s to an
// array of |To|.
template <typename From, typename To>
struct SafelyMemcpyable
    : public std::integral_constant<
          bool,
          (IsPodlike<From>::value && IsPodlike<To>::value &&
           std::is_same<From, To>::value) ||
              (std::is_scalar<From>::value && std::is_scalar<To>::value &&
               sizeof(From) == sizeof(To))> {};

// This predicate returns true if it's safe to memset a value of type |From|
// to an array of |To|.
//
// Currently we only support filling a one-byte data.
template <typename From, typename To>
struct SafelyMemsettable
    : public std::integral_constant<bool,
                                    IsPodlike<From>::value &&
                                        IsPodlike<To>::value &&
                                        sizeof(From) == 1 && sizeof(To) == 1> {
};

// This predicate returns true if it's safe to memcmp two ranges of values of
// types T and U, respectively.
//
// For now, we think it's safe to compare T and U if T and U are the same type.
template <typename T, typename U>
struct SafelyMemcmpable
    : public std::integral_constant<bool,
                                    IsPodlike<T>::value &&
                                        IsPodlike<U>::value &&
                                        std::is_same<T, U>::value> {};

// This class templates deals with a POD type.
template <typename T>
struct PodContainerTypeOperations {
  static_assert(IsPodlike<T>::value, "|T| must be a POD-like type.");

  // The range is zero-initialized.
  static void DefaultInitializeRange(T* uninitialized_begin,
                                     T* uninitialized_end);

  static void DestructRange(T* storage_begin, T* storage_end);

  template <typename InT>
  static void Assign(T& storage, InT&& value);

  // We have two versions: memcpy-safe overload and memcpy-unsafe overload.
  template <typename InT,
            typename = std::enable_if_t<SafelyMemcpyable<InT, T>::value>>
  static void CopyRange(const InT* value_begin,
                        const InT* value_end,
                        T* storage_begin);
  // |dummy| template argument is necessary to disambiguate the overloads.
  template <typename InT,
            int dummy = 0,
            typename = std::enable_if_t<!SafelyMemcpyable<InT, T>::value>>
  static void CopyRange(const InT* value_begin,
                        const InT* value_end,
                        T* storage_begin);

  // For PODs, copying and moving are the same. MoveRange() is just an alias of
  // CopyRange() (of either overload).
  template <typename InT>
  static void MoveRange(InT* value_begin, InT* value_end, T* storage_begin);

  template <typename InT,
            typename = std::enable_if_t<SafelyMemcpyable<InT, T>::value>>
  static void CopyOverlappingRange(const InT* value_begin,
                                   const InT* value_end,
                                   T* storage_begin);
  template <typename InT,
            int dummy = 0,
            typename = std::enable_if_t<!SafelyMemcpyable<InT, T>::value>>
  static void CopyOverlappingRange(const InT* value_begin,
                                   const InT* value_end,
                                   T* storage_begin);

  template <typename InT>
  static void MoveOverlappingRange(InT* value_begin,
                                   InT* value_end,
                                   T* storage_begin);

  // For PODs, UninitializedCopy and CopyRange are the same.
  template <typename InT>
  static void UninitializedCopy(const InT* value_begin,
                                const InT* value_end,
                                T* uninitialized_begin);

  // UninitializedFill uses memset if T and InT are both one-byte types.
  template <typename InT,
            typename = std::enable_if_t<SafelyMemsettable<InT, T>::value>>
  static void UninitializedFill(InT value,
                                T* uninitialized_begin,
                                T* uninitialized_end);
  template <typename InT,
            int dummy = 0,
            typename = std::enable_if_t<!SafelyMemsettable<InT, T>::value>>
  static void UninitializedFill(const InT& value,
                                T* uninitialized_begin,
                                T* uninitialized_end);

  template <typename InT,
            typename = std::enable_if_t<SafelyMemcmpable<T, InT>::value>>
  static bool EqualRange(const T* storage_begin,
                         const T* storage_end,
                         const InT* other_begin);
  template <typename InT,
            int dummy = 0,
            typename = std::enable_if_t<!SafelyMemcmpable<T, InT>::value>>
  static bool EqualRange(const T* storage_begin,
                         const T* storage_end,
                         const InT* other_begin);
};

//
// ContainerTypeOperations
//

template <typename T, bool is_podlike = IsPodlike<T>::value>
struct ContainerTypeOperationsImpl;

template <typename T>
struct ContainerTypeOperationsImpl<T, true> : PodContainerTypeOperations<T> {};

template <typename T>
struct ContainerTypeOperationsImpl<T, false>
    : GenericContainerTypeOperations<T> {};

// The global definition of ContainerTypeOperations.
template <typename T>
struct ContainerTypeOperations : ContainerTypeOperationsImpl<T> {
  static_assert(!std::is_reference<T>::value, "|T| must not be a reference.");
  static_assert(std::is_same<T, std::remove_cv_t<T>>::value,
                "|T| must not be cv-qualified.");
};

// TODO(yutak): More specializations for smart pointers.

//
// CompleteContainerTypeOperations
//

// CompleteContainerTypeOperations supplements missing functions as defined
// above and creates a class template that has every function listed above.

namespace internal {
// This internal block contains all the implementation detail needed to
// supplement the missing functions in ContainerTypeOperations<T>. You can skip
// this whole block if you are not interested in details.

// Define a predicate struct named Has##FunctionName##Function that tests the
// existence of a static member function |FunctionName| in TypeOperation taking
// the arguments specified in the "..." part of the macro.
#define DEFINE_FUNCTION_DETECTOR(FunctionName, ...)                 \
  template <typename TypeOperations, typename T>                    \
  auto test##FunctionName##Function(int)->decltype(                 \
      TypeOperations::FunctionName(__VA_ARGS__), std::true_type()); \
  template <typename TypeOperations, typename T>                    \
  std::false_type test##FunctionName##Function(long);               \
  template <typename TypeOperations, typename T>                    \
  struct Has##FunctionName##Function                                \
      : decltype(test##FunctionName##Function<TypeOperations, T>(0)) {}
// A semicolon is required after the macro use.

// In the following, we define a number of class templates named
// <FunctionName>Supplement, which provides the default implementation of the
// function if it is missing in TypeOperations.

DEFINE_FUNCTION_DETECTOR(DefaultInitialize, std::declval<T&>());
DEFINE_FUNCTION_DETECTOR(DefaultInitializeRange,
                         std::declval<T*>(),
                         std::declval<T*>());

template <typename TypeOperations,
          typename T,
          bool hasDefaultInitializeFunction =
              HasDefaultInitializeFunction<TypeOperations, T>::value,
          bool hasDefaultInitializeRangeFunction =
              HasDefaultInitializeRangeFunction<TypeOperations, T>::value>
struct DefaultInitializeSupplement;

template <typename TypeOperations, typename T>
struct DefaultInitializeSupplement<TypeOperations, T, true, true> {};

template <typename TypeOperations, typename T>
struct DefaultInitializeSupplement<TypeOperations, T, true, false> {
  static void DefaultInitializeRange(T* uninitialized_begin,
                                     T* uninitialized_end) {
    for (T* uninitialized = uninitialized_begin;
         uninitialized != uninitialized_end; ++uninitialized) {
      TypeOperations::DefaultInitialize(*uninitialized);
    }
  }
};

template <typename TypeOperations, typename T>
struct DefaultInitializeSupplement<TypeOperations, T, false, true> {
  static void DefaultInitialize(T& uninitialized) {
    TypeOperations::DefaultInitializeRange(&uninitialized, &uninitialized + 1);
  }
};

// DefaultInitializeSupplement<TypeOperations, T, false, false> is invalid.

DEFINE_FUNCTION_DETECTOR(Destruct, std::declval<T&>());
DEFINE_FUNCTION_DETECTOR(DestructRange, std::declval<T*>(), std::declval<T*>());

template <typename TypeOperations,
          typename T,
          bool hasDestructFunction =
              HasDestructFunction<TypeOperations, T>::value,
          bool hasDestructRangeFunction =
              HasDestructRangeFunction<TypeOperations, T>::value>
struct DestructSupplement;

template <typename TypeOperations, typename T>
struct DestructSupplement<TypeOperations, T, true, true> {};

template <typename TypeOperations, typename T>
struct DestructSupplement<TypeOperations, T, true, false> {
  static void DestructRange(T* storage_begin, T* storage_end) {
    for (T* storage = storage_begin; storage != storage_end; ++storage) {
      TypeOperations::Destruct(*storage);
    }
  }
};

template <typename TypeOperations, typename T>
struct DestructSupplement<TypeOperations, T, false, true> {
  static void Destruct(T& storage) {
    TypeOperations::DestructRange(&storage, &storage + 1);
  }
};

// DestructSupplement<TypeOperations, T, false, false> is invalid.

DEFINE_FUNCTION_DETECTOR(CopyRange,
                         std::declval<const T*>(),
                         std::declval<const T*>(),
                         std::declval<T*>());

template <typename TypeOperations,
          typename T,
          bool hasCopyRangeFunction =
              HasCopyRangeFunction<TypeOperations, T>::value>
struct CopyRangeSupplement;

template <typename TypeOperations, typename T>
struct CopyRangeSupplement<TypeOperations, T, true> {};

template <typename TypeOperations, typename T>
struct CopyRangeSupplement<TypeOperations, T, false> {
  template <typename InT>
  static void CopyRange(const InT* value_begin,
                        const InT* value_end,
                        T* storage_begin) {
    T* storage = storage_begin;
    for (const InT *value = value_begin; value < value_end;
         ++value, ++storage) {
      TypeOperations::Assign(*storage, *value);
    }
  }
};

DEFINE_FUNCTION_DETECTOR(MoveRange,
                         std::declval<T*>(),
                         std::declval<T*>(),
                         std::declval<T*>());

template <typename TypeOperations,
          typename T,
          bool hasMoveRangeFunction =
              HasMoveRangeFunction<TypeOperations, T>::value>
struct MoveRangeSupplement;

template <typename TypeOperations, typename T>
struct MoveRangeSupplement<TypeOperations, T, true> {};

template <typename TypeOperations, typename T>
struct MoveRangeSupplement<TypeOperations, T, false> {
  template <typename InT>
  static void MoveRange(InT* value_begin, InT* value_end, T* storage_begin) {
    T* storage = storage_begin;
    for (InT *value = value_begin; value != value_end; ++value, ++storage) {
      TypeOperations::Assign(*storage, std::move(*value));
    }
  }
};

DEFINE_FUNCTION_DETECTOR(CopyOverlappingRange,
                         std::declval<const T*>(),
                         std::declval<const T*>(),
                         std::declval<T*>());

template <typename TypeOperations,
          typename T,
          bool hasCopyOverlappingRangeFunction =
              HasCopyOverlappingRangeFunction<TypeOperations, T>::value>
struct CopyOverlappingRangeSupplement;

template <typename TypeOperations, typename T>
struct CopyOverlappingRangeSupplement<TypeOperations, T, true> {};

template <typename TypeOperations, typename T>
struct CopyOverlappingRangeSupplement<TypeOperations, T, false> {
  template <typename InT>
  static void CopyOverlappingRange(const T* value_begin,
                                   const T* value_end,
                                   InT* storage_begin) {
    const void* value_address = reinterpret_cast<const void*>(value_begin);
    const void* storage_address = reinterpret_cast<const void*>(storage_begin);

    if (value_address > storage_address) {
      // Copy forward.
      //
      // SupplementedTypeOperations::CopyRange() cannot be used here, because
      // user-supplied CopyRange() may not support overlapping cases.
      //
      // Our own implementation is safe, so we delegate the work to
      // CopyRangeSupplement<TypeOperations, T, false>.
      CopyRangeSupplement<TypeOperations, T, false>::CopyRange(
          value_begin, value_end, storage_begin);
    } else if (value_address < storage_address) {
      // Copy backward.
      if (value_begin == value_end)
        return;
      const T* value = value_end;
      T* storage = storage_begin + (value_end - value_begin);
      do {
        --value;
        --storage;
        TypeOperations::Assign(*storage, *value);
      } while (value != value_begin);
    }
  }
};

DEFINE_FUNCTION_DETECTOR(MoveOverlappingRange,
                         std::declval<T*>(),
                         std::declval<T*>(),
                         std::declval<T*>());

template <typename TypeOperations,
          typename T,
          bool hasMoveOverlappingRange =
              HasMoveOverlappingRangeFunction<TypeOperations, T>::value>
struct MoveOverlappingRangeSupplement;

template <typename TypeOperations, typename T>
struct MoveOverlappingRangeSupplement<TypeOperations, T, true> {};

template <typename TypeOperations, typename T>
struct MoveOverlappingRangeSupplement<TypeOperations, T, false> {
  template <typename InT>
  static void MoveOverlappingRange(T* value_begin,
                                   T* value_end,
                                   InT* storage_begin) {
    const void* value_address = reinterpret_cast<const void*>(value_begin);
    const void* storage_address = reinterpret_cast<const void*>(storage_begin);

    if (value_address > storage_address) {
      // Move forward.
      MoveRangeSupplement<TypeOperations, T, false>::MoveRange(
          value_begin, value_end, storage_begin);
    } else if (value_address < storage_address) {
      // Move backward.
      if (value_begin == value_end)
        return;
      const T* value = value_end;
      T* storage = storage_begin + (value_end - value_begin);
      do {
        --value;
        --storage;
        TypeOperations::Assign(*storage, std::move(*value));
      } while (value != value_begin);
    }
  }
};

DEFINE_FUNCTION_DETECTOR(UninitializedCopy,
                         std::declval<const T*>(),
                         std::declval<const T*>(),
                         std::declval<T*>());

template <typename TypeOperations,
          typename T,
          bool hasUninitializedCopy =
              HasUninitializedCopyFunction<TypeOperations, T>::value>
struct UninitializedCopySupplement;

template <typename TypeOperations, typename T>
struct UninitializedCopySupplement<TypeOperations, T, true> {};

template <typename TypeOperations, typename T>
struct UninitializedCopySupplement<TypeOperations, T, false> {
  template <typename InT>
  static void UninitializedCopy(const InT* value_begin,
                                const InT* value_end,
                                T* uninitialized_begin) {
    // To pick the correct version of DefaultInitialize().
    struct SupplementedTypeOperations
        : TypeOperations,
          DefaultInitializeSupplement<TypeOperations, T> {};
    T* uninitialized = uninitialized_begin;
    for (const InT *value = value_begin; value != value_end;
         ++value, ++uninitialized) {
      SupplementedTypeOperations::DefaultInitialize(*uninitialized);
      SupplementedTypeOperations::Assign(*uninitialized, *value);
    }
  }
};

DEFINE_FUNCTION_DETECTOR(UninitializedFill,
                         std::declval<const T&>(),
                         std::declval<T*>(),
                         std::declval<T*>());

template <typename TypeOperations,
          typename T,
          bool hasUninitializedFill =
              HasUninitializedFillFunction<TypeOperations, T>::value>
struct UninitializedFillSupplement;

template <typename TypeOperations, typename T>
struct UninitializedFillSupplement<TypeOperations, T, true> {};

template <typename TypeOperations, typename T>
struct UninitializedFillSupplement<TypeOperations, T, false> {
  template <typename InT>
  static void UninitializedFill(const InT& value,
                                T* uninitialized_begin,
                                T* uninitialized_end) {
    struct SupplementedTypeOperations
        : TypeOperations,
          DefaultInitializeSupplement<TypeOperations, T> {};
    for (T* uninitialized = uninitialized_begin;
         uninitialized != uninitialized_end; ++uninitialized) {
      SupplementedTypeOperations::DefaultInitialize(*uninitialized);
      SupplementedTypeOperations::Assign(*uninitialized, value);
    }
  }
};

DEFINE_FUNCTION_DETECTOR(Equal,
                         std::declval<const T&>(),
                         std::declval<const T&>());
DEFINE_FUNCTION_DETECTOR(EqualRange,
                         std::declval<const T*>(),
                         std::declval<const T*>(),
                         std::declval<const T*>());

template <typename TypeOperations,
          typename T,
          bool hasEqual = HasEqualFunction<TypeOperations, T>::value,
          bool hasEqualRange = HasEqualRangeFunction<TypeOperations, T>::value>
struct EqualSupplement;

template <typename TypeOperations, typename T>
struct EqualSupplement<TypeOperations, T, true, true> {};

template <typename TypeOperations, typename T>
struct EqualSupplement<TypeOperations, T, true, false> {
  template <typename InT>
  static bool EqualRange(const T* storage_begin,
                         const T* storage_end,
                         const InT* other_begin) {
    const InT* other = other_begin;
    for (const T *storage = storage_begin; storage != storage_end;
         ++storage, ++other) {
      if (!TypeOperations::Equal(*storage, *other))
        return false;
    }
    return true;
  }
};

template <typename TypeOperations, typename T>
struct EqualSupplement<TypeOperations, T, false, true> {
  template <typename InT>
  static bool Equal(const T& stored_value, const InT& other_value) {
    return TypeOperations::EqualRange(&stored_value, &stored_value + 1,
                                      &other_value);
  }
};

// EqualSupplement<TypeOperations, T, false, false> is invalid.

#undef DEFINE_FUNCTION_DETECTOR

}  // namespace internal

// Finally, CompleteContainerTypeOperations is defined by concerting all the
// supplement classes. This can be applied to any ContainerTypeOperations
// conforming to the requirements above.
template <typename TypeOperations, typename T>
struct CompleteContainerTypeOperations
    : TypeOperations,
      internal::DefaultInitializeSupplement<TypeOperations, T>,
      internal::DestructSupplement<TypeOperations, T>,
      internal::CopyRangeSupplement<TypeOperations, T>,
      internal::MoveRangeSupplement<TypeOperations, T>,
      internal::CopyOverlappingRangeSupplement<TypeOperations, T>,
      internal::MoveOverlappingRangeSupplement<TypeOperations, T>,
      internal::UninitializedCopySupplement<TypeOperations, T>,
      internal::UninitializedFillSupplement<TypeOperations, T>,
      internal::EqualSupplement<TypeOperations, T> {};

//
// CompletedContainerTypeOperations
//

// Complete*d*ContainerTypeOperations is ContainerTypeOperations<T> with
// supplemented functions.
template <typename T>
struct CompletedContainerTypeOperations
    : CompleteContainerTypeOperations<ContainerTypeOperations<T>, T> {};

//
// GenericContainerTypeOperations<T> definitions
//

template <typename T>
void GenericContainerTypeOperations<T>::DefaultInitialize(T& uninitialized) {
  new (&uninitialized) T;
}

template <typename T>
void GenericContainerTypeOperations<T>::Destruct(T& storage) {
  storage.~T();
}

template <typename T>
template <typename InT>
void GenericContainerTypeOperations<T>::Assign(T& storage, InT&& value) {
  storage = std::forward<InT>(value);
}

template <typename T>
template <typename InT>
void GenericContainerTypeOperations<T>::UninitializedCopy(
    const InT* value_begin,
    const InT* value_end,
    T* uninitialized_begin) {
  const InT* value = value_begin;
  T* uninitialized = uninitialized_begin;
  for (; value != value_end; ++value, ++uninitialized)
    new (uninitialized) T(*value);
}

template <typename T>
template <typename InT>
void GenericContainerTypeOperations<T>::UninitializedFill(
    const InT& value,
    T* uninitialized_begin,
    T* uninitialized_end) {
  for (T* uninitialized = uninitialized_begin;
       uninitialized != uninitialized_end; ++uninitialized) {
    new (uninitialized) T(value);
  }
}

template <typename T>
template <typename InT>
bool GenericContainerTypeOperations<T>::Equal(const T& stored_value,
                                              const InT& other_value) {
  return stored_value == other_value;
}

//
// PodContainerTypeOperations<T> definitions
//

template <typename T>
void PodContainerTypeOperations<T>::DefaultInitializeRange(
    T* uninitialized_begin,
    T* uninitialized_end) {
  std::memset(uninitialized_begin, 0,
              (uninitialized_end - uninitialized_begin) * sizeof(T));
}

template <typename T>
void PodContainerTypeOperations<T>::DestructRange(T* storage_begin,
                                                  T* storage_end) {
  // Nothing to do.
}

template <typename T>
template <typename InT>
void PodContainerTypeOperations<T>::Assign(T& storage, InT&& value) {
  GenericContainerTypeOperations<T>::Assign(storage, std::forward<InT>(value));
}

template <typename T>
template <typename InT, typename>
void PodContainerTypeOperations<T>::CopyRange(const InT* value_begin,
                                              const InT* value_end,
                                              T* storage_begin) {
  static_assert(sizeof(T) == sizeof(InT),
                "SafelyMemcpyable<> should make sure the sizes are same.");
  std::memcpy(storage_begin, value_begin,
              (value_end - value_begin) * sizeof(InT));
}

template <typename T>
template <typename InT, int, typename>
void PodContainerTypeOperations<T>::CopyRange(const InT* value_begin,
                                              const InT* value_end,
                                              T* storage_begin) {
  CompleteContainerTypeOperations<GenericContainerTypeOperations<T>,
                                  T>::CopyRange(value_begin, value_end,
                                                storage_begin);
}

template <typename T>
template <typename InT>
void PodContainerTypeOperations<T>::MoveRange(InT* value_begin,
                                              InT* value_end,
                                              T* storage_begin) {
  CopyRange(value_begin, value_end, storage_begin);
}

template <typename T>
template <typename InT, typename>
void PodContainerTypeOperations<T>::CopyOverlappingRange(const InT* value_begin,
                                                         const InT* value_end,
                                                         T* storage_begin) {
  static_assert(sizeof(T) == sizeof(InT),
                "SafelyMemcpyable<> should make sure the sizes are same.");
  std::memmove(storage_begin, value_begin,
               (value_end - value_begin) * sizeof(InT));
}

template <typename T>
template <typename InT, int, typename>
void PodContainerTypeOperations<T>::CopyOverlappingRange(const InT* value_begin,
                                                         const InT* value_end,
                                                         T* storage_begin) {
  CompleteContainerTypeOperations<GenericContainerTypeOperations<T>,
                                  T>::CopyOverlappingRange(value_begin,
                                                           value_end,
                                                           storage_begin);
}

template <typename T>
template <typename InT>
void PodContainerTypeOperations<T>::MoveOverlappingRange(InT* value_begin,
                                                         InT* value_end,
                                                         T* storage_begin) {
  CopyOverlappingRange(value_begin, value_end, storage_begin);
}

template <typename T>
template <typename InT>
void PodContainerTypeOperations<T>::UninitializedCopy(const InT* value_begin,
                                                      const InT* value_end,
                                                      T* uninitialized_begin) {
  // An uninitialized range and an initialized range won't overlap.
  CopyRange(value_begin, value_end, uninitialized_begin);
}

template <typename T>
template <typename InT, typename>
void PodContainerTypeOperations<T>::UninitializedFill(InT value,
                                                      T* uninitialized_begin,
                                                      T* uninitialized_end) {
  static_assert(sizeof(T) == 1, "sizeof(T) must be 1 for this overload.");
  static_assert(sizeof(InT) == 1, "sizeof(InT) must be 1 for this overload.");
  std::memset(uninitialized_begin, static_cast<int>(value),
              uninitialized_end - uninitialized_begin);
}

template <typename T>
template <typename InT, int, typename>
void PodContainerTypeOperations<T>::UninitializedFill(const InT& value,
                                                      T* uninitialized_begin,
                                                      T* uninitialized_end) {
  CompleteContainerTypeOperations<GenericContainerTypeOperations<T>,
                                  T>::UninitializedFill(value,
                                                        uninitialized_begin,
                                                        uninitialized_end);
}

template <typename T>
template <typename InT, typename>
bool PodContainerTypeOperations<T>::EqualRange(const T* storage_begin,
                                               const T* storage_end,
                                               const InT* other_begin) {
  static_assert(sizeof(T) == sizeof(InT),
                "Sizes of T and InT must match to use memcmp.");
  return std::memcmp(storage_begin, other_begin,
                     (storage_end - storage_begin) * sizeof(T)) == 0;
}

}  // namespace experimental
}  // namespace WTF

#endif  // WTF_ContainerTypeOperations_h
