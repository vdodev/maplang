#include "maplang/Buffer.h"

using namespace std;

namespace maplang {

Buffer::Buffer() : data(nullptr), length(0) {}

Buffer::Buffer(const shared_ptr<uint8_t>& data, size_t length)
    : data(data, data.get()), length(length) {}

Buffer::Buffer(const string& str)
    : data {new uint8_t[str.length()], default_delete<uint8_t[]>()},
      length(str.length()) {
  str.copy(reinterpret_cast<char*>(data.get()), str.length());
}

Buffer Buffer::slice(size_t offset, size_t length) const {
  if (offset >= this->length) {
    return Buffer();
  }

  const size_t maxLength = this->length - offset;

  return Buffer(
      shared_ptr<uint8_t>(data, data.get() + offset),
      min(maxLength, length));
}

}  // namespace maplang
