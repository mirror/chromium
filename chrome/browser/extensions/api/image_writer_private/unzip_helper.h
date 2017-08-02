#ifndef CHROME_BROWSER_EXTENSIONS_API_IMAGE_WRITER_PRIVATE_UNZIP_HELPER_H_
#define CHROME_BROWSER_EXTENSIONS_API_IMAGE_WRITER_PRIVATE_UNZIP_HELPER_H_


#include "base/memory/ref_counted_memory.h"
#include "base/sequenced_task_runner.h"
#include "build/build_config.h"

namespace base {
class FilePath;
}  // namespace base
namespace zip {
class ZipReader;
}

namespace extensions {
namespace image_writer {
class UnzipHelper : public base::RefCountedThreadSafe<UnzipHelper> {
 public:
  UnzipHelper(
              scoped_refptr<base::SequencedTaskRunner> owner_task_runner);

  void Unzip(
    const base::FilePath& image_path,
    const base::FilePath& temp_dir_path,
    const base::Callback<void(const base::FilePath&)>& open_callback,
    const base::Closure& complete_callback,
    const base::Callback<void(const std::string&)>& failure_callback,
    const base::Callback<void(int64_t, int64_t)>& progress_callback);
 private:
  friend class base::RefCountedThreadSafe<UnzipHelper>;
  ~UnzipHelper();

  void UnzipImpl(
      const base::FilePath& image_path,
      const base::FilePath& temp_dir);
  void OnError(const std::string& error);
  void OnOpenSuccess(const base::FilePath& image_path);
  void OnComplete();
  void OnProgress(int64_t total_bytes, int64_t curr_bytes);

  base::Callback<void(const base::FilePath&)> open_callback_;
  base::Closure complete_callback_;
  base::Callback<void(const std::string&)> failure_callback_;
  base::Callback<void(int64_t, int64_t)> progress_callback_;

  scoped_refptr<base::SequencedTaskRunner> owner_task_runner_;
  //scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::unique_ptr<zip::ZipReader> zip_reader_;

  DISALLOW_COPY_AND_ASSIGN(UnzipHelper);
};
}  // namespace image_writer
}  // namespace extensions
#endif  // CHROME_BROWSER_EXTENSIONS_API_IMAGE_WRITER_PRIVATE_UNZIP_HELPER_H_
