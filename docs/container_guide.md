# Container guide 

## Maps and sets

### Usage advice

  * Generally avoid **std::unordered\_set** and **std::unordered\_map**. In the
    common case, query performance is only marginally better than the
    alternatives, insert performance is worse, and the memory overhead is
    high. This makes sense mostly for large tables where you expect a lot of
    lookups.

  * Most maps and sets in Chrome are small and contain objects that can be
    moved efficiently. In this case, consider **base::flat\_map** and
    **base::flat\_set**. You need to be aware of the maximum expected size of
    the container size since inserts and deletes are O(n). But because it
    avoids mallocs in most cases, inserts are better or comparable to other
    containers even for several dozen items, and efficiently-moved types are
    unlikely to have performance problems for most cases until you have
    hundreds of items. They have optimizations for one-shot construction that
    are efficient for any size of container if your code can be structured that
    way.

  * **base::SmallMap** has better runtime memory usage without the poor insertion
    into large sets that base::flat_map has. But this advantage is partially
    offset by the code size increase. Prefer in cases where there are a lot
    of objects, relatively few code paths (so that the code/heap tradeoff is
    good), and you may have larger-sizes of containers that base::flat\_map
    won't handle well.

  * Use **std::map** and **std::set** if you can't decide. Even if they're not
    great, they're unlikely to be bad.

## Map and set details

Sizes are on 64-bit platforms.

| Container                                | Empty size | Per-item overhead |
| ---------------------------------------- | ---------- | ----------------- |
| std::map, std::set                       | 48 bytes   | 32 bytes          |
| std::unordered\_map, std::unordered\_set | 112 bytes  | 16-24 bytes       |
| base::flat\_map and base::flat\_set      |            | 0 (see notes)     |
| base::SmallMap                           |            | 32 bytes          |

Code size comparisons for a block of code (see appendix) on Windows using
strings as keys.

| Container           | Code size  |
| ------------------- | ---------- |
| std::unordered\_map | 1646 bytes |
| std::map            | 1759 bytes |
| base::flat_map      | 1872 bytes |
| base::SmallMap      | 2410 bytes |

### std::map and std::set

A red-black tree. Each inserted item requires the memory allocation of a node
on the heap. Each node contains a left pointer, a right pointer, a parent
pointer, and a "color" for the red-black tree (32-bytes per item on 64-bits).

### std::unordered\_map and std::unordered\_set

A hash table. Implemented on Windows as a std::vector + std::list and in libc++
as the equivalent of a std::vector + a std::forward_list. Both implementations
allocate an 8-entry hash table (containing iterators into the list)
on initialization, and grow to 64 entries once 8 items are inserted. This gives
very poor memory overhead for small (common) map and set sizes). Above 64 items,
the size doubles every time the load factor exceeds 1.

The per-item overhead in the table above counts the list node (2 pointers on
Windows, 1 pointer in libc++), plus amortizes the hash table assuming a 0.5
load factor on average.

In a microbenchmark on Windows, inserts of 1M integers into a
std::unordered\_set took 1.07x the time of std::set, and queries took 0.67x the
time of std::set. For a typical 4-entry set (the statistical mode of map sizes
in the browser), query performance is identical to std::set and base::flat\_set.
The takeaway is that you should not default to using unordered maps because
"they're faster."

### base::flat\_map and base::flat\_set

A sorted std::vector. Seached via binary search, inserts in the middle require
moving elements to make room. Good cache locality. For large objects and large
set sizes, std::vector's doubling-when-full strategy can waste memory.

Supports efficient construction from a vector of items which avoids the O(n^2)
insertion time of each element separately.

### base::SmallMap

Since instantiations require both code for a std::map and a brute-force
search of the "small" unordered container, code size is larger.


# Appendix

### Code for map code size comparison

```
TEST(Foo, Bar) {
  base::SmallMap<std::map<std::string, Flubber>> foo;
  foo.insert(std::make_pair("foo", Flubber(8, "bar")));
  foo.insert(std::make_pair("bar", Flubber(8, "bar")));
  foo.insert(std::make_pair("foo1", Flubber(8, "bar")));
  foo.insert(std::make_pair("bar1", Flubber(8, "bar")));
  foo.insert(std::make_pair("foo", Flubber(8, "bar")));
  foo.insert(std::make_pair("bar", Flubber(8, "bar")));
  auto found = foo.find("asdf");
  printf("Found is %d\n", (int)(found == foo.end()));
  found = foo.find("foo");
  printf("Found is %d\n", (int)(found == foo.end()));
  found = foo.find("bar");
  printf("Found is %d\n", (int)(found == foo.end()));
  found = foo.find("asdfhf");
  printf("Found is %d\n", (int)(found == foo.end()));
  found = foo.find("bar1");
  printf("Found is %d\n", (int)(found == foo.end()));
}
```
