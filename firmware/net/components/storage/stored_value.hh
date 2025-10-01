#pragma once

#include "logger.hh"
#include "storage.hh"
#include <optional>
#include <string>
#include <vector>

class StoredValueInterface
{
protected:
    StoredValueInterface(Storage& storage, const std::string ns, const std::string key) :
        storage(storage),
        ns(ns),
        key(key),
        loaded(false)
    {
    }

    Storage& storage;
    const std::string ns;
    const std::string key;
    bool loaded;
};

template <typename T>
class StoredValue;

template <typename T>
concept StandardLayout = std::is_standard_layout_v<T>;

template <StandardLayout T>
class StoredValue<T> : public StoredValueInterface
{
public:
    StoredValue(Storage& storage, const std::string ns, const std::string key) :
        StoredValueInterface(storage, ns, key),
        stored()
    {
        memset(&stored, 0, sizeof(T));
    }

    std::optional<std::reference_wrapper<const T>> Load()
    {
        if (loaded)
        {
            return stored;
        }

        ssize_t ret_size = storage.Load(ns, key, &stored, sizeof(T));

        if (ret_size < 0)
        {
            return std::nullopt;
        }

        loaded = ret_size > 0;
        return stored;
    }

    bool Save(const T& new_value)
    {
        if (ESP_OK != storage.Save(ns, key, &new_value, sizeof(T)))
        {
            return false;
        }

        stored = new_value;
        loaded = true;
        return true;
    }

private:
    T stored;
};

template <typename T>
concept ContiguousRange = std::is_standard_layout_v<typename T::value_type> && requires(T& v)
{
    v.data();
    v.size();
};

template <ContiguousRange T>
class StoredValue<T> : public StoredValueInterface
{
public:
    StoredValue(Storage& storage, const std::string ns, const std::string key) :
        StoredValueInterface(storage, ns, key),
        key_size(key + "_size"),
        stored()
    {
    }

    const T& Load()
    {
        if (loaded)
        {
            return stored;
        }

        // Load the size of the value
        const uint32_t size = storage.Loadu32(ns, key_size);

        stored.resize(size);

        if (size == 0)
        {
            return stored;
        }

        // Try to load from nvs
        storage.Load(ns, key, reinterpret_cast<void*>(stored.data()), size);

        loaded = true;

        return stored;
    }

    bool Save(const T& new_values)
    {
        const size_t size = new_values.size() * sizeof(T);
        if (ESP_OK != storage.Saveu32(ns, key_size, size))
        {
            return false;
        }

        if (ESP_OK != storage.Save(ns, key, reinterpret_cast<const void*>(new_values.data()), size))
        {
            return false;
        }

        stored = new_values;
        loaded = true;

        return true;
    }

private:
    const std::string key_size;
    T stored;
};