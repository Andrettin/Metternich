#pragma once

namespace metternich {

class character;
class landed_title;
enum class landed_title_tier;

class landed_title_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString title_name READ get_title_name_qstring NOTIFY tier_changed)
	Q_PROPERTY(QString ruler_title_name READ get_ruler_title_name_qstring NOTIFY tier_changed)

public:
	explicit landed_title_game_data(const metternich::landed_title *landed_title);

	const std::string &get_title_name() const;

	QString get_title_name_qstring() const
	{
		return QString::fromStdString(this->get_title_name());
	}

	const std::string &get_ruler_title_name() const;

	QString get_ruler_title_name_qstring() const
	{
		return QString::fromStdString(this->get_ruler_title_name());
	}

	landed_title_tier get_tier() const
	{
		return this->tier;
	}

	void set_tier(const landed_title_tier tier);

	const character *get_holder() const
	{
		return this->holder;
	}

	void set_holder(const character *holder);

signals:
	void tier_changed();
	void holder_changed();
	void title_name_changed();
	void ruler_title_name_changed();

private:
	const metternich::landed_title *landed_title = nullptr;
	landed_title_tier tier;
	const character *holder = nullptr;
};

}
