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
    using load_value_t =
        std::conditional_t<ContiguousRange<T>, T&, std::optional<std::reference_wrapper<T>>>;

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

            NET_LOG_WARN("Loaded size of %u at namespace %s key %s", size, ns.c_str(), key.c_str());

            if (size == 0)
            {
                NET_LOG_WARN("No data stored in namespace %s key %s", ns.c_str(), key.c_str());
                loaded = true;
                return stored;
            }

            stored.resize(size / sizeof(typename T::value_type));
            storage.Load(ns, key, reinterpret_cast<void*>(stored.data()), size);
            loaded = true;
            return stored;
        }
        else
        {
            const ssize_t ret_size = storage.Load(ns, key, &stored, sizeof(T));

            loaded = true;
            return stored;
        }
    }

    bool Save()
    {
        if constexpr (ContiguousRange<T>)
        {
            const size_t size = stored.size() * sizeof(typename T::value_type);
            if (ESP_OK
                != storage.Saveu32(ns, std::string(std::string{key} + "_size").c_str(), size))
            {
                return false;
            }

            NET_LOG_INFO("Saved size of %d, %d, %d", (int)size, (int)stored.size(),
                         (int)sizeof(typename T::value_type));

            if (ESP_OK != storage.Save(ns, key, reinterpret_cast<const void*>(stored.data()), size))
            {
                return false;
            }
        }
        else
        {
            if (ESP_OK != storage.Save(ns, key, &stored, sizeof(T)))
            {
                return false;
            }
        }

        loaded = true;
        return true;
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

public:
    T stored;
};
