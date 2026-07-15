#pragma once

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class route;

class route_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(bool active MEMBER active READ is_active NOTIFY active_changed)

public:
	explicit route_game_data(const metternich::route *route) : route(route)
	{
	}

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	bool is_on_map() const;

	bool is_active() const
	{
		return this->active;
	}

	void set_active(const bool active);
	void check_active();

	centesimal_int get_output() const;
	void apply_output(const int multiplier);

	Q_INVOKABLE QString get_site_modifier_string() const;

signals:
	void active_changed();

private:
	const metternich::route *route = nullptr;
	bool active = false;
};

}
