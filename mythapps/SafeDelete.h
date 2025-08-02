#ifndef SAFE_DELETE_H
#define SAFE_DELETE_H

#include <QObject>
#include <type_traits>

template <typename T> void SafeDelete(T *&ptr) {
    if (!ptr)
        return;

    if constexpr (std::is_base_of_v<QObject, T>) {
        if (!ptr->parent()) {
            if constexpr (std::is_destructible_v<T> && std::is_trivially_destructible_v<T>) {
                delete ptr;
            }
        }
    } else {
        if constexpr (std::is_destructible_v<T>) {
            delete ptr;
        }
    }

    ptr = nullptr;
}

#endif // SAFE_DELETE_H
