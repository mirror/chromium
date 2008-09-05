// Copyright 2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_HASHMAP_H_
#define V8_HASHMAP_H_

namespace v8 { namespace internal {


// Allocator defines the memory allocator interface
// used by HashMap and implements a default allocator.
class Allocator BASE_EMBEDDED {
 public:
  virtual ~Allocator()  {}
  virtual void* New(size_t size)  { return Malloced::New(size); }
  virtual void Delete(void* p)  { Malloced::Delete(p); }
};


class HashMap {
 public:
  static Allocator DefaultAllocator;

  typedef bool (*MatchFun) (void* key1, void* key2);

  // Dummy constructor.  This constructor doesn't set up the hash
  // map properly so don't use it unless you have good reason.
  HashMap();

  // initial_capacity is the size of the initial hash map;
  // it must be a power of 2 (and thus must not be 0).
  HashMap(MatchFun match,
          Allocator* allocator = &DefaultAllocator,
          uint32_t initial_capacity = 8);

  ~HashMap();

  // HashMap entries are (key, value, hash) tripplets.
  // Some clients may not need to use the value slot
  // (e.g. implementers of sets, where the key is the value).
  struct Entry {
    void* key;
    void* value;
    uint32_t hash;  // the full hash value for key
  };

  // If an entry with matching key is found, Lookup()
  // returns that entry. If no matching entry is found,
  // but insert is set, a new entry is inserted with
  // corresponding key, key hash, and NULL value.
  // Otherwise, NULL is returned.
  Entry* Lookup(void* key, uint32_t hash, bool insert);

  // Empties the hash map (occupancy() == 0).
  void Clear();

  // The number of (non-empty) entries in the table.
  uint32_t occupancy() const  { return occupancy_; }

  // The capacity of the table. The implementation
  // makes sure that occupancy is at most 80% of
  // the table capacity.
  uint32_t capacity() const  { return capacity_; }

  // Iteration
  //
  // for (Entry* p = map.Start(); p != NULL; p = map.Next(p)) {
  //   ...
  // }
  //
  // If entries are inserted during iteration, the effect of
  // calling Next() is undefined.
  Entry* Start() const;
  Entry* Next(Entry* p) const;

 private:
  Allocator* allocator_;
  MatchFun match_;
  Entry* map_;
  uint32_t capacity_;
  uint32_t occupancy_;

  Entry* map_end() const  { return map_ + capacity_; }
  Entry* Probe(void* key, uint32_t hash);
  void Initialize(uint32_t capacity);
  void Resize();
};


} }  // namespace v8::internal

#endif  // V8_HASHMAP_H_
