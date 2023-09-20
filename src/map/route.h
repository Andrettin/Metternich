#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/color_container.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("map/route_game_data.h")

namespace metternich {

class route_game_data;
class route_history;

class route final : public named_data_entry, public data_type<route>
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ get_color WRITE set_color NOTIFY changed)
	Q_PROPERTY(bool hidden MEMBER hidden READ is_hidden NOTIFY changed)
	Q_PROPERTY(metternich::route_game_data* game_data READ get_game_data NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "route";
	static constexpr const char property_class_identifier[] = "metternich::route*";
	static constexpr const char database_folder[] = "routes";
	static constexpr bool history_enabled = true;

	static route *get_by_color(const QColor &color)
	{
		route *route = route::try_get_by_color(color);

		if (route == nullptr) {
			throw std::runtime_error(std::format("No route found for color: ({}, {}, {}).", color.red(), color.green(), color.blue()));
		}

		return route;
	}

	static route *try_get_by_color(const QColor &color)
	{
		const auto find_iterator = route::routes_by_color.find(color);
		if (find_iterator != route::routes_by_color.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	static void clear()
	{
		data_type::clear();
		route::routes_by_color.clear();
	}

private:
	static inline color_map<route *> routes_by_color;

public:
	explicit route(const std::string &identifier) : named_data_entry(identifier)
	{
		this->reset_game_data();
	}

	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	route_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;


	void reset_game_data();

	route_game_data *get_game_data() const
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

		if (route::try_get_by_color(color) != nullptr) {
			throw std::runtime_error("Color is already used by another route.");
		}

		this->color = color;
		route::routes_by_color[color] = this;
	}

	bool is_hidden() const
	{
		return this->hidden;
	}

signals:
	void changed();

private:
	QColor color;
	bool hidden = false;
	qunique_ptr<route_history> history;
	qunique_ptr<route_game_data> game_data;
};

}
