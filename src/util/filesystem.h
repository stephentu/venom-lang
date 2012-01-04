#ifndef VENOM_UTIL_FILESYSTEM_H
#define VENOM_UTIL_FILESYSTEM_H

#include <string>
#include <vector>

#include <dirent.h>

namespace venom {
namespace util {

struct listfiles_default_filter {
  inline bool operator()(const std::string& name) const {
    return true;
  }
};

struct listfiles_ext_filter {
  /** ext w/o the . */
  listfiles_ext_filter(const std::string& ext) : ext(ext) {}
  inline bool operator()(const std::string& name) const {
    size_t p = name.rfind("." + ext);
    if (p == std::string::npos) return false;
    return p == (name.size() - (ext.size() + 1));
  }
  const std::string ext;
};

template <typename Filter>
bool listfiles(const std::string& dirname,
               std::vector<std::string>& files,
               Filter filter = listfiles_default_filter(),
               bool append_dirname = false) {
  DIR *dir = opendir(dirname.c_str());
  if (!dir) return false;
  struct dirent *ent;
  while ((ent = readdir(dir))) {
    if (filter(ent->d_name)) {
      if (append_dirname) {
        // TODO: OS-specific path sep
        files.push_back(dirname + "/" + ent->d_name);
      } else {
        files.push_back(ent->d_name);
      }
    }
  }
  closedir(dir);
  return true;
}

inline std::string strip_extension(const std::string& filename) {
  size_t p = filename.rfind('.');
  if (p == std::string::npos) return filename;
  return filename.substr(0, p);
}

}
}

#endif /* VENOM_UTIL_FILESYSTEM_H */
