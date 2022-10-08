#pragma once

#include "util/singleton.h"

namespace metternich {

class scenario;

class game final : public QObject, public singleton<game>
{
	Q_OBJECT

	Q_PROPERTY(bool running READ is_running NOTIFY running_changed)

public:
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

	Q_INVOKABLE void setup_scenario(metternich::scenario *scenario);
	Q_INVOKABLE void start();
	Q_INVOKABLE void stop();

	void apply_history(const scenario *scenario);

	const QImage &get_diplomatic_map_image() const
	{
		return this->diplomatic_map_image;
	}

	void create_diplomatic_map_image();

signals:
	void running_changed();
	void diplomatic_map_image_changed();

private:
	bool running = false;
	QImage diplomatic_map_image;
};

}
