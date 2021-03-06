#pragma once

#include <QApplication>
#include <QObject>
#include <QThread>

#include <memory>
#include <mutex>
#include <type_traits>

namespace metternich {

template <typename T>
class singleton
{
public:
	static T *get()
	{
		std::call_once(singleton<T>::once_flag, [](){
			T::create();
		});

		return singleton<T>::instance.get();
	}

private:
	static void create()
	{
		singleton<T>::instance = std::make_unique<T>();

		if constexpr (std::is_base_of_v<QObject, T>) {
			if (QApplication::instance()->thread() != QThread::currentThread()) {
				singleton<T>::instance->moveToThread(QApplication::instance()->thread());
			}
		}
	}

private:
	static inline std::unique_ptr<T> instance;
	static inline std::once_flag once_flag;
};

}
