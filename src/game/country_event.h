#pragma once

#include "game/event.h"
#include "game/scoped_event_base.h"

namespace metternich {

class country;

class country_event final : public event, public data_type<country_event>, public scoped_event_base<country>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "country_event";
	static constexpr const char property_class_identifier[] = "metternich::country_event*";
	static constexpr const char database_folder[] = "events/country";

	static void clear()
	{
		data_type::clear();
		scoped_event_base::clear();
	}

	explicit country_event(const std::string &identifier) : event(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		if (!scoped_event_base::process_gsml_scope(scope)) {
			data_entry::process_gsml_scope(scope);
		}
	}

	virtual void initialize() override
	{
		scoped_event_base::initialize();
		data_entry::initialize();
	}

	virtual void check() const override
	{
		scoped_event_base::check();
	}

	virtual event_trigger get_trigger() const override
	{
		return event::get_trigger();
	}

	virtual bool is_random() const override
	{
		return scoped_event_base::is_random();
	}

	virtual void set_random_weight(const int weight) override
	{
		scoped_event_base::set_random_weight(weight);
	}

	virtual int get_option_count() const override
	{
		return scoped_event_base::get_option_count();
	}

	const std::string &get_option_name(const int option_index) const
	{
		return scoped_event_base::get_option_name(option_index);
	}

	std::string get_option_tooltip(const int option_index, const read_only_context &ctx) const
	{
		return scoped_event_base::get_option_tooltip(option_index, ctx);
	}

	virtual void do_option_effects(const int option_index, context &ctx) const override
	{
		scoped_event_base::do_option_effects(option_index, ctx);
	}

	virtual void create_instance(const context &ctx) const override
	{
		event::create_instance(ctx);
	}
};

}