#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/color_container.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("map/route_game_data.h")

namespace metternich {

class commodity;
class province;
class route_game_data;
class route_history;
class site;

template <typename scope_type>
class and_condition;

class route final : public named_data_entry, public data_type<route>
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ get_color WRITE set_color NOTIFY changed)
	Q_PROPERTY(const metternich::commodity* output_commodity MEMBER output_commodity READ get_output_commodity NOTIFY changed)
	Q_PROPERTY(metternich::site* start_site MEMBER start_site NOTIFY changed)
	Q_PROPERTY(metternich::site* end_site MEMBER end_site NOTIFY changed)
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
	explicit route(const std::string &identifier);
	~route();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
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

	const commodity *get_output_commodity() const
	{
		return this->output_commodity;
	}

	const site *get_start_site() const
	{
		return this->start_site;
	}

	const site *get_end_site() const
	{
		return this->end_site;
	}

	bool is_hidden() const
	{
		return this->hidden;
	}

	const std::vector<const province *> &get_path_provinces() const
	{
		return this->path_provinces;
	}

	const and_condition<province> *get_conditions() const
	{
		return this->conditions.get();
	}

signals:
	void changed();

private:
	QColor color;
	const commodity *output_commodity = nullptr;
	site *start_site = nullptr;
	site *end_site = nullptr;
	bool hidden = false;
	std::vector<const province *> path_provinces;
	std::unique_ptr<const and_condition<province>> conditions;
	qunique_ptr<route_history> history;
	qunique_ptr<route_game_data> game_data;
};

}
