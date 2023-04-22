#pragma once

#include "game/event.h"
#include "game/scoped_event_base.h"

namespace metternich {

class province;

class province_event final : public event, public data_type<province_event>, public scoped_event_base<const province>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "province_event";
	static constexpr const char property_class_identifier[] = "metternich::province_event*";
	static constexpr const char database_folder[] = "events/province";

	static void clear()
	{
		data_type::clear();
		scoped_event_base::clear();
	}

	explicit province_event(const std::string &identifier) : event(identifier)
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
		named_data_entry::initialize();
	}

	virtual void check() const override
	{
		event::check();
		scoped_event_base::check();
	}

	virtual const std::string &get_identifier() const override
	{
		return event::get_identifier();
	}

	virtual const std::string &get_name() const override
	{
		return event::get_name();
	}

	virtual event_trigger get_trigger() const override
	{
		return event::get_trigger();
	}

	virtual event_random_group *get_random_group() const
	{
		return event::get_random_group();
	}

	virtual bool is_random() const override
	{
		return scoped_event_base::is_random();
	}

	virtual void set_random_weight(const int weight) override
	{
		scoped_event_base::set_random_weight(weight);
	}

	virtual bool fires_only_once() const override
	{
		return event::fires_only_once();
	}

	virtual int get_option_count() const override
	{
		return scoped_event_base::get_option_count();
	}

	virtual bool is_option_available(const int option_index, const read_only_context &ctx) const override
	{
		return scoped_event_base::is_option_available(option_index, ctx);
	}

	virtual const std::string &get_option_name(const int option_index) const override
	{
		return scoped_event_base::get_option_name(option_index);
	}

	virtual std::string get_option_tooltip(const int option_index, const read_only_context &ctx) const override
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
