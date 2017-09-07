// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// All the gory details for the helpers that define and implement valid
// relationships between CUs, and endow a concrete class with appropriate
// strongly typed accessors/mutators. Only meant to be included from
// coordination_unit_relationships.h.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_RELATIONSHIPS_DETAILS_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_RELATIONSHIPS_DETAILS_H_

#include "base/logging.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_collection.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_relationship_types.h"

namespace resource_coordinator {

template<class SelfType> class CoordinationUnitBaseImpl;

namespace details {

// Helper for write access to a CoordinationUnitCollection. Simply a
// safeguard to prevent casual direct modification. CUs should only be
// modified via type-safe functions.
class CoordinationUnitCollectionWriter {
 public:
  static bool Add(CoordinationUnitCollection* cu_collection,
                  CoordinationUnitBase* cu) {
    return cu_collection->Add(cu);
  }

  static bool Remove(CoordinationUnitCollection* cu_collection,
                     CoordinationUnitBase* cu) {
    return cu_collection->Remove(cu);
  }
};

// A base class for a relationship, minus the cardinality information. Allows
// SFINAE introspection of whether or not the relationship type is defined.
// Also simplifies building an RelationshipsAreSatisfied implementation for
// CoordinationUnitBaseImpl.
template<RelationshipType kRelationshipType,
         CoordinationUnitType kRelativeType>
class HasRelationshipBase {
 public:
  using RelativeType =
      typename CoordinationUnitClassFromType<kRelativeType>::Type;

  HasRelationshipBase() = default;
  virtual ~HasRelationshipBase() {}
  virtual bool RelationshipsAreSatisfiedImpl() const = 0;
  virtual bool AddImpl(RelativeType* relative) = 0;
  virtual bool RemoveImpl(RelativeType* relative) = 0;
  virtual bool HasImpl(RelativeType* relative) const = 0;
  virtual size_t GetImpl(std::vector<RelativeType*>* relatives) const = 0;
};

// A base class for a relationship with a max cardinality of 1.
template<RelationshipType kRelationshipType,
         CoordinationUnitType kRelativeType>
class HasSingleRelationshipBase
    : public HasRelationshipBase<kRelationshipType, kRelativeType> {
 public:
  using RelativeType =
      typename CoordinationUnitClassFromType<kRelativeType>::Type;
  virtual bool SetImpl(RelativeType* relative) = 0;
  virtual bool ClearImpl() = 0;
  virtual bool HasImpl() = 0;
  virtual RelativeType* GetImpl() const = 0;

  // Bring in the Has and Get that are shadowed in the base class.
  using HasRelationshipBase<kRelationshipType, kRelativeType>::HasImpl;
  using HasRelationshipBase<kRelationshipType, kRelativeType>::GetImpl;
};

// Fully specified relationship. This provides vector-backed storage
// for arbitrary relationship cardinalities.
template<class SelfType,
         RelationshipType kRelationshipType,
         size_t kMinRelativeCount,
         size_t kMaxRelativeCount,
         class RelativeType>
class HasRelationship
    : public HasRelationshipBase<
          kRelationshipType,
          CoordinationUnitTypeFromClass<RelativeType>::kType> {
 public:
  static constexpr CoordinationUnitType kRelativeType =
      CoordinationUnitTypeFromClass<RelativeType>::kType;

  static_assert(kMinRelativeCount <= kMaxRelativeCount,
                "kMinRelativeCount > kMaxRelativeCount");
  static_assert(kMaxRelativeCount != 0,
                "kMaxRelativeCount == 0");

  HasRelationship() = default;
  ~HasRelationship() override {}

  // Implementation of HasRelationshipBase:
  bool RelationshipsAreSatisfiedImpl() const override {
    return relatives_.size() >= kMinRelativeCount &&
        relatives_.size() <= kMaxRelativeCount;
  }

 protected:
  friend class CoordinationUnitBaseImpl<SelfType>;

  bool AddImpl(RelativeType* relative) override {
    if (HasImpl(relative))
      return true;
    DCHECK(!cu_base_relatives().Has(relative));
    if (relatives_.size() >= kMaxRelativeCount)
      return false;
    relatives_.push_back(relative);
    bool result = CoordinationUnitCollectionWriter::Add(
        &cu_base_relatives(), relative);
    DCHECK(result);
    return true;
  }

  bool RemoveImpl(RelativeType* relative) override {
    auto it = std::find(relatives_.begin(), relatives_.end(), relative);
    if (it == relatives_.end()) {
      DCHECK(!cu_base_relatives().Has(relative));
      return false;
    }
    relatives_.erase(it);
    bool result = CoordinationUnitCollectionWriter::Remove(
        &cu_base_relatives(), relative);
    DCHECK(result);
    return true;
  }

  bool HasImpl(RelativeType* relative) const override {
    bool result = std::find(relatives_.begin(), relatives_.end(), relative) !=
        relatives_.end();
    DCHECK_EQ(result, cu_base_relatives().Has(relative));
    return result;
  }

  size_t GetImpl(std::vector<RelativeType*>* relatives) const override {
    if (relatives != nullptr) {
      relatives->insert(
          relatives->end(), relatives_.begin(), relatives_.end());
    }
    size_t result = relatives_.size();
    DCHECK_EQ(result, cu_base_relatives().GetByType(kRelativeType).size());
    return result;
  }


 private:
  CoordinationUnitBase* cu_base() {
    return static_cast<CoordinationUnitBase*>(
        static_cast<SelfType*>(this));
  }
  const CoordinationUnitBase* cu_base() const {
    return static_cast<const CoordinationUnitBase*>(
        static_cast<const SelfType*>(this));
  }
  CoordinationUnitCollection& cu_base_relatives() {
    return cu_base()->relatives(kRelationshipType);
  }
  const CoordinationUnitCollection& cu_base_relatives() const {
    return cu_base()->relatives(kRelationshipType);
  }

  std::vector<RelativeType*> relatives_;
};

// Partial specialization for a max of 1 relative. Doesn't need vector storage.
template<class SelfType,
         RelationshipType kRelationshipType,
         size_t kMinRelativeCount,
         class RelativeType>
class HasRelationship<SelfType, kRelationshipType, kMinRelativeCount, 1u,
                      RelativeType>
    : public HasSingleRelationshipBase<
          kRelationshipType,
          CoordinationUnitTypeFromClass<RelativeType>::kType> {
 public:
  static constexpr CoordinationUnitType kRelativeType =
      CoordinationUnitTypeFromClass<RelativeType>::kType;

  HasRelationship() : relative_(nullptr) {}
  ~HasRelationship() override {}

  static_assert(kMinRelativeCount <= 1u,
                "invalid kMinRelativeCount / kMaxRelativeCount");

  // Implementation of HasRelationshipBase:
  bool RelationshipsAreSatisfiedImpl() const override {
    if (relative_)
      return true;
    return kMinRelativeCount == 0;
  }

 protected:
  friend class CoordinationUnitBaseImpl<SelfType>;

  // HasRelationshipBase implementation:
  bool AddImpl(RelativeType* relative) override {
    if (!relative || relative_)
      return false;
    if (relative_ == relative)
      return true;
    relative_ = relative;
    bool result = CoordinationUnitCollectionWriter::Add(
        &cu_base_relatives(),
        static_cast<CoordinationUnitBase*>(relative));
    DCHECK(result);
    return true;
  }

  // HasRelationshipBase implementation:
  bool RemoveImpl(RelativeType* relative) override {
    if (!relative || relative_ != relative)
      return false;
    relative_ = nullptr;
    bool result = CoordinationUnitCollectionWriter::Remove(
        &cu_base_relatives(), relative);
    DCHECK(result);
    return true;
  }

  // HasRelationshipBase implementation:
  bool HasImpl(RelativeType* relative) const override {
    bool result = (relative != nullptr && relative_ == relative);
    DCHECK_EQ(result, cu_base_relatives().Has(relative));
    return result;
  }

  // HasRelationshipBase implementation:
  size_t GetImpl(std::vector<RelativeType*>* relatives) const override {
    if (relative_ && relatives)
      relatives->push_back(relative_);
    size_t count = relative_ ? 1 : 0;
    DCHECK_EQ(count, cu_base_relatives().GetByType(kRelativeType).size());
    return count;
  }

  // HasSingleRelationshipBase implementation:
  bool SetImpl(RelativeType* relative) override {
    if (relative == relative_)
      return false;
    bool overwrite = false;
    if (relative_) {
      overwrite = true;
      RemoveImpl(relative_);
    }
    if (relative)
      AddImpl(relative);
    return overwrite;
  }

  // HasSingleRelationshipBase implementation:
  bool ClearImpl() override {
    if (!relative_)
      return false;
    bool result = CoordinationUnitCollectionWriter::Remove(
        &cu_base_relatives(), relative_);
    DCHECK(result);
    relative_ = nullptr;
    return true;
  }

  // HasSingleRelationshipBase implementation:
  bool HasImpl() override {
    return relative_ != nullptr;
  }

  // HasSingleRelationshipBase implementation:
  RelativeType* GetImpl() const override {
    auto relatives = cu_base_relatives().GetByType(kRelativeType);
    if (relative_) {
      DCHECK_EQ(1u, relatives.size());
      DCHECK_EQ(static_cast<CoordinationUnitBase*>(relative_), relatives[0]);
    } else {
      DCHECK_EQ(0u, relatives.size());
    }
    return relative_;
  }

 private:
  CoordinationUnitBase* cu_base() {
    return static_cast<CoordinationUnitBase*>(
        static_cast<SelfType*>(this));
  }
  const CoordinationUnitBase* cu_base() const {
    return static_cast<const CoordinationUnitBase*>(
        static_cast<const SelfType*>(this));
  }
  CoordinationUnitCollection& cu_base_relatives() {
    return cu_base()->relatives(kRelationshipType);
  }
  const CoordinationUnitCollection& cu_base_relatives() const {
    return cu_base()->relatives(kRelationshipType);
  }

  RelativeType* relative_;
};

}  // namespace details

// Full definitions of the forward declarations found in
// coordination_unit_relationships.h. See there for comments.
template<class SelfType, class ChildType>struct HasOptionalChildren
    : public details::HasRelationship<SelfType, RelationshipType::kChild, 0u,
                                      kInfinite, ChildType> {};

template<class SelfType, class ChildType>
struct HasAtMostOneChild
    : public details::HasRelationship<SelfType, RelationshipType::kChild, 0u,
                                      1u, ChildType> {};

template<class SelfType, class ChildType>
struct HasExactlyOneChild
    : public details::HasRelationship<SelfType, RelationshipType::kChild, 1u,
                                      1u, ChildType> {};

template<class SelfType, class ParentType>
struct HasExactlyOneParent
    : public details::HasRelationship<SelfType, RelationshipType::kParent, 1u,
                                      1u, ParentType> {};

template<class SelfType, class ParentType>
struct HasAtMostOneParent
    : public details::HasRelationship<SelfType, RelationshipType::kParent, 0u,
                                      1u, ParentType> {};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_RELATIONSHIPS_DETAILS_H_
