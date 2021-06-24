#include "Widgets.h"

#include <algorithm>
#include <locale>

#include <ctype.h>


enum class FilePickerWigetSortMode {
    None,
    Name,
    Size,
    Type,
    Date
};


// not very unicode friendly, but generic strings should be generic enough
bool FileSorterName(const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b)
{
    std::string nameA = a.path().generic_string();
    std::string nameB = b.path().generic_string();

    // template deduction issues when feeding std::tolower() directly into transform
    auto converter = [](unsigned char c) -> unsigned char { return std::tolower(c); };
    std::transform(nameA.begin(), nameA.end(), nameA.begin(), converter);
    std::transform(nameB.begin(), nameB.end(), nameB.begin(), converter);

    return (nameA < nameB);
};
bool FileSorterSize(const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b)
{
    return (a.file_size() > b.file_size());
};
bool FileSorterType(const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b)
{
    return (a.path().extension() < b.path().extension());
};
bool FileSorterDate(const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b)
{
    return (a.last_write_time() > b.last_write_time());
};


// Comparison function for sorting file browser widget results.
//
// This function is expected to be called upon each refresh or re-sort for the contents of a directory. It is called as
//  part of constructing a table, and uses ImGui::TableGetSortSpecs(). As such, it is not expected to be reentrant.
//  Further, the file browser is implemented as an ImGui popup to discourage mulitple open instances.
static bool CompareWithSortSpecs(const std::filesystem::directory_entry& left,
                                 const std::filesystem::directory_entry& right)
{
    ImGuiTableSortSpecs* pCurrentSortSpecs = ImGui::TableGetSortSpecs();
    assert(pCurrentSortSpecs != nullptr);
    bool result = false;
    bool descending = false;

    for (uint i = 0; i < pCurrentSortSpecs->SpecsCount; ++i)
    {
        const ImGuiTableColumnSortSpecs* pSortSpec = &pCurrentSortSpecs->Specs[i];
        descending = (pSortSpec->SortDirection == ImGuiSortDirection_Descending);

        switch (pSortSpec->ColumnIndex)
        {
        case 0: // name
        {
            std::locale::global(std::locale(""));
            std::wstringstream ss;
            ss.imbue(std::locale());
            auto& facet = std::use_facet<std::ctype<WCHAR>>(std::locale());

            //auto leftName = left.path().u8string();
            //auto rightName = right.path().u8string();
            auto leftName = left.path().generic_wstring();
            auto rightName = right.path().generic_wstring();

            facet.tolower(&leftName[0], &leftName[0] + leftName.size());
            facet.tolower(&rightName[0], &rightName[0] + rightName.size());

            result = (descending) ? leftName < rightName
                                  : leftName > rightName;

            //result = (descending) ? left.path().generic_string() < right.path().generic_string()
            //                      : left.path().generic_string() > right.path().generic_string();
            break;
        }
        case 1: // size
        {
            result = (descending) ? left.file_size() > right.file_size()
                                  : left.file_size() < right.file_size();
            break;
        }
        case 2: // type
        {
            result = (descending) ? left.path().extension().generic_string() < right.path().extension().generic_string()
                                  : left.path().extension().generic_string() > right.path().extension().generic_string();
            break;
        }
        case 3: // date
        {
            result = (descending) ? left.last_write_time() > right.last_write_time()
                                  : left.last_write_time() < right.last_write_time();
            break;
        }
        default:
            break;
        }
    }

    return result;
}


// ImGui's tables have some advanced features such as row sorting, column reordering, column hiding, etc...
//
// One nice side effect of using std::filesystem to navigate and change the current directory is that the dialogue will
//  remember the last location, making navigation a little faster. This could be annoying if another dialogue uses the
//  same mechanism, but it's not terribly important. If so, just replace std::filesystem::current_path() with temps. If
//  some other relative path operation comes into play, then this will need revising.
//
// TODO: style table buttons
void FilePickerWidget(char* buffer, const size_t bufferSize, const std::string startLocation)
{
    std::filesystem::directory_iterator currentDirectory(std::filesystem::current_path());

    // widget populates a destination string with a file path via an interactive popup
    ImGui::InputTextWithHint("##FilePickerWidgetInputTextBoxLabel", "file path", buffer, bufferSize);
    ImGui::SameLine();
    if (ImGui::Button("\xee\x81\xa7##testbutton"))
    {
        ImGui::OpenPopup("filePickerPopup");
    }

    if (ImGui::BeginPopup("filePickerPopup"))
    {
        // static state so we don't re-poll the filesystem every single frame
        static FilePickerWigetSortMode sortMode = FilePickerWigetSortMode::Size;
        static bool refresh = true;
        static bool sort = true;
        static std::vector<std::filesystem::directory_entry> directories;
        static std::vector<std::filesystem::directory_entry> files;
        std::stringstream ss;

        static ImGuiTableFlags flags = ImGuiTableFlags_Resizable    |
                                       ImGuiTableFlags_Reorderable  |
                                       ImGuiTableFlags_Sortable     |
                                       ImGuiTableFlags_Hideable     |
                                       ImGuiTableFlags_BordersOuter |
                                       ImGuiTableFlags_BordersV;

        // rebuild lists of content for current directory
        if (refresh)
        {
            directories.clear();
            files.clear();
            for (auto& entry : currentDirectory)
            {
                if (entry.is_directory())
                {
                    directories.push_back(entry);
                }
                else
                {
                    files.push_back(entry);
                }
            }
            refresh = false;
        }

        // TODO: add text box for pasting in path

        // string of buttons which navigate to all the parents of current path
        ImGui::Text("%s", std::filesystem::current_path().generic_u8string().c_str());
        std::filesystem::path builtPath = "";
        ImGui::Text("Navigation: ");
        for (const auto& element : currentDirectory->path().parent_path())
        {
            if (element == "/") continue;
            builtPath += element;
            builtPath += '/';

            ImGui::SameLine();
            if (ImGui::Button((const char*)(element.generic_u8string().c_str())))
            {
                std::filesystem::current_path(builtPath);
                refresh = true;
            }
        }
        ImGui::Separator();

        // table layout
        ImGui::BeginTable("Directory Contents", 4, flags);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Size");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Date");
        ImGui::TableHeadersRow();


        // sort directory contents
        if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs())
        {
            if (sorts_specs->SpecsDirty)
            {
                std::sort(files.begin(), files.end(), CompareWithSortSpecs);
                sorts_specs->SpecsDirty = false;
            }
        }


        // std::filesystem iterators do not include . and .. so add .. in
        ImGui::TableNextColumn(); // name
        std::filesystem::path dotDot("..");
        if (ImGui::Button((const char*)(dotDot.filename().generic_u8string().c_str())))
        {
            std::filesystem::current_path(dotDot);
            refresh = true;
        }
        ImGui::TableNextRow();
        ss.str("");

        for (const auto& entry : directories)
        {
            // it should be as simple as this, but MSVC implementation has incompatible clocks
            //const std::filesystem::file_time_type lastWriteTime = entry.last_write_time();
            //std::time_t time = std::chrono::system_clock::to_time_t(lastWriteTime);
            //ss << std::put_time()

            const std::filesystem::file_time_type lastWriteTime = entry.last_write_time();
            std::time_t tt = TimeConvert(lastWriteTime);
            //std::tm *gmt = std::gmtime(&tt); // std::gmtime not thread safe
            std::tm gmt;
            gmtime_s(&gmt, &tt);
            ss << std::put_time(&gmt, "%c");

            // name
            ImGui::TableNextColumn();
            // TODO: style these buttons
            if (ImGui::Button((const char*)(entry.path().filename().generic_u8string().c_str())))
            {
                std::filesystem::current_path(entry);
                refresh = true;
            }

            // size
            ImGui::TableNextColumn();
            // type
            ImGui::TableNextColumn();
            // date
            ImGui::TableNextColumn();
            ImGui::Text(ss.str().c_str());
            ImGui::TableNextRow();

            ss.str("");
        }
        ImGui::Separator(); // TODO: better separator between directory and file sections
        ImGui::TableNextRow();
        for (const auto& entry : files)
        {
            const std::filesystem::file_time_type lastWriteTime = entry.last_write_time();
            std::time_t tt = TimeConvert(lastWriteTime);
            std::tm gmt;
            gmtime_s(&gmt, &tt);
            ss << std::put_time(&gmt, "%c");

            // name
            ImGui::TableNextColumn();
            // TODO: style these buttons
            ImGui::Button((const char*)(entry.path().filename().generic_u8string().c_str()));

            // size
            ImGui::TableNextColumn();
            ImGui::Text(HumanReadableFileSize(entry.file_size()).c_str());
            // type
            ImGui::TableNextColumn();
            ImGui::Text("%S", entry.path().extension().c_str());
            // date
            ImGui::TableNextColumn();
            ImGui::Text(ss.str().c_str());
            ImGui::TableNextRow();

            ss.str("");
        }

        ImGui::EndTable();
        ImGui::EndPopup();
    }
}

bool SliderU64(const char* label, uint64_t* value, uint64_t min, uint64_t max, const char* format)
{
    return ImGui::SliderScalar(label, ImGuiDataType_S64, value, &min, &max, format);
}
