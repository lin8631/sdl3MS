# wzlibcpp

modern first, more easier, cleaner api and support cross-platform

# Status

⬜️ utf8 formatted string with `std::string`

⬜️ remove all of older (deprecated) api -
`[[deprecated]]` attribute is currently marked.

☑️ modern api

☑️ cross-platform

# Dependencies

* zlib
* mio - for mmap (aka file mapping in windows)

# Usage

```cpp
#include <iostream>

#include <pystring.h>
#include <algorithm>

#include <wz/Wz.hpp>
#include <wz/Node.hpp>
#include <wz/File.hpp>
#include <wz/Directory.hpp>

template<typename T>
constexpr auto string(T iterable) {
    return std::wstring{iterable.begin(), iterable.end()};
}

template<typename T>
constexpr auto back(T iterable) {
    return std::string{iterable.begin(), iterable.end()};
}

template<typename T>
T pop(std::vector<T>& vec) {
    auto last = vec.back();
    vec.pop_back();
    return last;
}

wz::WzMap find_from_path(wz::Node* root, const std::wstring& path) {
    std::vector<std::string> next{};
    std::string s{};
    s.assign(path.begin(), path.end());
    pystring::split(s, next, "/");
    std::reverse(next.begin(), next.end());
    const auto current = string(pop(next));
    for (const auto& it : *root) {
        if (it.first == current) {
            if (next.empty()) {
                return root->get_children();
            }

            std::reverse(next.begin(), next.end());
            return find_from_path(root, string(pystring::join("/", next)));
        }
    }
    return {};
}

#define U8 static_cast<u8>
#define IV4(A,B,C,D) {U8(A),U8(B),U8(C),U8(D)}

int main() {

    const auto iv = IV4(0x45, 0x50, 0x33, 0x01);
    wz::File file(iv, "C:/classic maple/Character_original.wz");

    if (file.parse()) {
        auto node = find_from_path(file.get_root(), L"00002000.img");

        for (const auto& it : node) {
            std::wcout << it.first << ", " << it.second.size() << std::endl;
            auto* dir = dynamic_cast<wz::Directory*>(it.second[0]);
            if (dir && dir->is_image()) {
                auto* image = new wz::Node();
                dir->parse_image(image);

                for (const auto& n : *image) {
                    std::wcout << n.first << std::endl;
                }
            }
        }
    }

    return 0;
}
```

## output
https://gist.github.com/SeaniaTwix/f8b7e7cc34c5761e9679efa491816b63