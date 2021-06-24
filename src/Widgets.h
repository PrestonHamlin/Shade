#include <imgui.h>

#include "Util.h"

void FilePickerWidget(char* buffer, const size_t bufferSize, const std::string startLocation = "");


bool SliderU64(const char* label, uint64_t* value, uint64_t min = 0, uint64_t max = 0, const char* format = "%llu");
