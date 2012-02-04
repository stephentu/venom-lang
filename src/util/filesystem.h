/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
