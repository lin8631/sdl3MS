#include <cassert>
#include "File.hpp"
#include "Wz.hpp"
#include "Directory.hpp"
#ifdef __EMSCRIPTEN__
#include "Emscripten.hpp"
#include <ranges>

bool wz::File::parse(const wzstring &name)
{
    reader.url = std::string{name.begin(), name.end()};
    parse_directories(root);
    return true;
}

bool wz::File::parse_directories(wz::Node *node)
{
    auto path = reader.url;
    auto url = "Img/" + reader.url + "/Directory.txt";

    Emscripten::load_file(url);

    std::string data((const char *)Emscripten::file_data, Emscripten::file_size);
    std::replace(data.begin(), data.end(), '\r', '\n');

    for (const auto &s : std::views::split(data, u'\n') | std::views::common)
    {
        if (s.size() == 0)
        {
            continue;
        }
        auto file_name = std::u16string{s.begin(), s.end()};
        Directory *dir = nullptr;
        if (file_name.find(u".img") != std::string::npos)
        {
            dir = new Directory(this, true, 0, 0, 0);
        }
        else
        {
            dir = new Directory(this, false, 0, 0, 0);
            // 如果是Directory,则继续解析
            reader.url = path + "/" + std::string{file_name.begin(), file_name.end()};
            parse_directories(dir);
        }
        node->path = std::u16string{reader.url.begin(), reader.url.end()};
        node->appendChild({file_name.begin(), file_name.end()}, dir);
    }
    return true;
}
#else
bool wz::File::parse(const wzstring &name)
{
    auto magic = reader.read_string(4);
    if (magic != u"PKG1")
        return false;

    auto fileSize = reader.read<u64>();
    auto startAt = reader.read<u32>();

    auto copyright = reader.read_string();

    reader.set_position(startAt);

    auto encryptedVersion = reader.read<i16>();

    for (int i = 0; i < 0x7FFF; ++i)
    {
        i16 file_version = static_cast<decltype(file_version)>(i);
        u32 version_hash = wz::get_version_hash(encryptedVersion, file_version);

        if (version_hash != 0)
        {
            desc.start = startAt;
            desc.hash = version_hash;
            desc.version = file_version;

            auto prev_position = reader.get_position();

            if (!parse_directories(nullptr))
            {
                reader.set_position(prev_position);
                continue;
            }
            else
            {
                if (root)
                {
                    root->path = name;
                    reader.set_position(prev_position);
                    parse_directories(root);
                }
                return true;
            }
        }
    }

    return false;
}

bool wz::File::parse_directories(wz::Node *node)
{
    auto entry_count = reader.read_compressed_int();

    for (int i = 0; i < entry_count; ++i)
    {
        auto type = reader.read_byte();
        size_t prevPos = 0;
        wzstring name;

        if (type == 1)
        {
            reader.skip(sizeof(i32) + sizeof(u16));

            get_wz_offset();
            continue;
        }
        else if (type == 2)
        {
            i32 stringOffset = reader.read<i32>();
            type = reader.read_wz_string_from_offset<u8>(desc.start + stringOffset, name);
        }
        else if (type == 3 || type == 4)
        {
            name = reader.read_wz_string();
        }
        else
        {
            assert(0);
        }

        i32 size = reader.read_compressed_int();
        i32 checksum = reader.read_compressed_int();
        u32 offset = get_wz_offset();

        if (node == nullptr && offset >= reader.size())
            return false;

        if (type == 3)
        {
            if (node != nullptr)
            {
                auto *dir = new Directory(this, false, size, checksum, offset);
                node->appendChild({name.begin(), name.end()}, dir);
            }
        }
        else
        {
            if (node != nullptr)
            {
                auto *dir = new Directory(this, true, size, checksum, offset);
                node->appendChild({name.begin(), name.end()}, dir);
            }
            else
            {
                prevPos = reader.get_position();
                reader.set_position(offset);

                if (!reader.is_wz_image())
                    return false;

                reader.set_position(prevPos);
            }
        }
    }

    if (node != nullptr)
    {
        for (auto &it : *node)
        {
            for (auto child : it.second)
            {
                auto *dir = dynamic_cast<Directory *>(child);

                if (dir != nullptr)
                {
                    if (!dir->is_image())
                    {
                        reader.set_position(dir->get_offset());
                        parse_directories(dir);
                    }
                }
            }
        }
    }

    return true;
}
#endif
[[maybe_unused]] wz::File::File(const std::initializer_list<u8> &new_iv, const char *path)
    : reader(Reader(key, path)), root(new Node(Type::NotSet, this)), key(), iv(nullptr)
{
    iv = new u8[4];
    memcpy(iv, new_iv.begin(), 4);
    init_key();
    reader.set_key(key);
}

[[maybe_unused]] wz::File::File(u8 *new_iv, const char *path)
    : reader(Reader(key, path)), root(new Node(Type::NotSet, this)), key(), iv(new_iv)
{
    init_key();
    reader.set_key(key);
}

wz::File::~File()
{
    delete[] iv;
    delete root;
}

u32 wz::File::get_wz_offset()
{
    u32 offset = static_cast<u32>(reader.get_position());
    offset = ~(offset - desc.start);
    offset *= desc.hash;
    offset -= wz::OffsetKey;
    offset = (offset << (offset & 0x1Fu)) | (offset >> (32 - (offset & 0x1Fu)));
    u32 encryptedOffset = reader.read<u32>();
    offset ^= encryptedOffset;
    offset += desc.start * 2;
    return offset;
}

wz::Node *wz::File::get_root() const
{
    return root;
}

void wz::File::init_key()
{
    std::vector<u8> aes_key_v(32);
    memcpy(aes_key_v.data(), wz::AesKey2, 32);
    key = MutableKey({iv[0], iv[1], iv[2], iv[3]}, aes_key_v);
}

wz::Node &wz::File::get_child(const wzstring &name)
{
    return *root->get_child(name);
}
