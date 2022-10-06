#pragma once

#include "util/singleton.h"

namespace metternich {

//interface for the engine, to be used in the context of QML
class engine_interface final : public QObject, public singleton<engine_interface>
{
	Q_OBJECT

	Q_PROPERTY(bool running READ is_running NOTIFY running_changed)

public:
	engine_interface();
	~engine_interface();

	bool is_running() const
	{
		return this->running;
	}

	void set_running(const bool running)
	{
		if (running == this->is_running()) {
			return;
		}

		this->running = running;

		emit running_changed();
	}

	Q_INVOKABLE QObject *get_map_template(const QString &identifier) const;

signals:
	void running_changed();

private:
	bool running = false;
};

}
