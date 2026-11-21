// 87.488: C++ API smoke for F6 native source provenance.
// No ROMs, no file writes, no pictures rendered.
#include "../../recomp/video/video.h"
#include "../xmvsf_native_capture.h"
#include <cstdint>
#include <string>
#include <type_traits>
using cps2recomp::Video;
using port::xmvsf_home::NativeCapture;
using Dump = bool (Video::*)(const char *, std::uint64_t) const;
using Last = const std::string &(NativeCapture::*)() const;
static_assert(std::is_same_v<decltype(&Video::ExportGraphicsSources), Dump>);
static_assert(std::is_same_v<decltype(&NativeCapture::LastPath), Last>);
int main() { return 0; }
