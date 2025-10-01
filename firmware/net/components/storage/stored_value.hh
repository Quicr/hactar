#pragma once

#include "logger.hh"
#include "storage.hh"
#include <optional>
#include <string>
#include <vector>

template <typename T>
concept StandardLayout = std::is_standard_layout_v<T>;

template <typename T>
concept ContiguousRange = std::is_standard_layout_v<typename T::value_type> && requires(T& v) {
    v.data();
    v.size();
};

template <StandardLayout T>
class StoredValue
{
    using load_value_t = std::
        conditional_t<ContiguousRange<T>, const T&, std::optional<std::reference_wrapper<const T>>>;

public:
    StoredValue(Storage& storage, const std::string ns, const std::string key) :
        storage(storage),
        ns(ns),
        key(key),
        loaded(false),
        stored()
    {
    }

    load_value_t Load()
    {
        if (loaded)
        {
            return stored;
        }

        if constexpr (ContiguousRange<T>)
        {
            const uint32_t size =
                storage.Loadu32(ns, std::string(std::string{key} + "_size").c_str());

            stored.resize(size);

            if (size == 0)
            {
                return stored;
            }

            storage.Load(ns, key, reinterpret_cast<void*>(stored.data()), size);
            loaded = true;
            return stored;
        }
        else
        {
            const ssize_t ret_size = storage.Load(ns, key, &stored, sizeof(T));
            if (ret_size <= 0)
            {
                return std::nullopt;
            }

            loaded = true;
            return stored;
        }
    }

    bool Save(const T& new_value)
    {
        if constexpr (ContiguousRange<T>)
        {
            const size_t size = new_value.size() * sizeof(typename T::value_type);
            if (ESP_OK
                != storage.Saveu32(ns, std::string(std::string{key} + "_size").c_str(), size))
            {
                return false;
            }

            if (ESP_OK
                != storage.Save(ns, key, reinterpret_cast<const void*>(new_value.data()), size))
            {
                return false;
            }
        }
        else
        {
            if (ESP_OK != storage.Save(ns, key, &new_value, sizeof(T)))
            {
                return false;
            }
        }

        stored = new_value;
        loaded = true;
        return true;
    }

private:
    Storage& storage;
    const std::string ns;
    const std::string key;
    bool loaded;
    T stored;
};
