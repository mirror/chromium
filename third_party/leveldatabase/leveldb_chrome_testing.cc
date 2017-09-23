#include "third_party/leveldatabase/leveldb_chrome_testing.h"

namespace leveldb_chrome {

namespace testing {

bool ParseFileName(const std::string& filename,
                   uint64_t* number,
                   leveldb::FileType* type) {
  return leveldb::ParseFileName(filename, number, type);
}

}  // namespace testing

}  // namespace leveldb_chrome
