#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/color_container.h"

namespace metternich {

class province final : public named_data_entry, public data_type<province>
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ get_color WRITE set_color)

public:
	static constexpr const char class_identifier[] = "province";
	static constexpr const char property_class_identifier[] = "metternich::province*";
	static constexpr const char database_folder[] = "provinces";

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

	explicit province(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

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

private:
	QColor color;
};

}
