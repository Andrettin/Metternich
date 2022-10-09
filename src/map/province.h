#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/color_container.h"

namespace metternich {

class province_game_data;
class province_history;
class site;

class province final : public named_data_entry, public data_type<province>
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ get_color WRITE set_color)
	Q_PROPERTY(bool water_zone MEMBER water_zone READ is_water_zone)
	Q_PROPERTY(metternich::site* capital_settlement MEMBER capital_settlement)

public:
	static constexpr const char class_identifier[] = "province";
	static constexpr const char property_class_identifier[] = "metternich::province*";
	static constexpr const char database_folder[] = "provinces";
	static constexpr bool history_enabled = true;

	static province *get_by_color(const QColor &color)
	{
		province *province = province::try_get_by_color(color);

		if (province == nullptr) {
			throw std::runtime_error("No province found for color: (" + std::to_string(color.red()) + ", " + std::to_string(color.green()) + ", " + std::to_string(color.blue()) + ").");
		}

		return province;
	}

	static province *try_get_by_color(const QColor &color)
	{
		const auto find_iterator = province::provinces_by_color.find(color);
		if (find_iterator != province::provinces_by_color.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	static void clear()
	{
		data_type::clear();
		province::provinces_by_color.clear();
	}

private:
	static inline color_map<province *> provinces_by_color;

public:
	explicit province(const std::string &identifier);
	~province();

	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	province_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	void reset_game_data();

	province_game_data *get_game_data() const
	{
		return this->game_data.get();
	}

	const QColor &get_color() const
	{
		return this->color;
	}

	void set_color(const QColor &color)
	{
		if (color == this->get_color()) {
			return;
		}

		if (province::try_get_by_color(color) != nullptr) {
			throw std::runtime_error("Color is already used by another province.");
		}

		this->color = color;
		province::provinces_by_color[color] = this;
	}

	bool is_water_zone() const
	{
		return this->water_zone;
	}

	const site *get_capital_settlement() const
	{
		return this->capital_settlement;
	}

private:
	QColor color;
	bool water_zone = false;
	site *capital_settlement = nullptr;
	std::unique_ptr<province_history> history;
	std::unique_ptr<province_game_data> game_data;
};

}
