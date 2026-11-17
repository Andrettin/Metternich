#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character;
class domain;
class province;
class site;
enum class event_trigger;

template <typename scope_type>
class scoped_event_base;

class event_random_group final : public named_data_entry, public data_type<event_random_group>
{
	Q_OBJECT

	Q_PROPERTY(metternich::event_trigger trigger MEMBER trigger READ get_trigger)
	Q_PROPERTY(int none_weight MEMBER none_weight READ get_none_weight)
	Q_PROPERTY(int delay MEMBER delay)
	Q_PROPERTY(int delay_days MEMBER delay_days)

private:
	template <typename scope_type>
	using event_vector = std::vector<const scoped_event_base<const scope_type> *>;

public:
	static const std::vector<event_random_group *> &get_all_of_trigger(const event_trigger trigger);

private:
	static inline std::map<event_trigger, std::vector<event_random_group *>> groups_by_trigger;

public:
	static constexpr const char class_identifier[] = "event_random_group";
	static constexpr const char property_class_identifier[] = "metternich::event_random_group*";
	static constexpr const char database_folder[] = "event_random_groups";

	explicit event_random_group(const std::string &identifier);

	virtual void initialize() override;
	virtual void check() const override;

	event_trigger get_trigger() const
	{
		return this->trigger;
	}

	int get_none_weight() const
	{
		return this->none_weight;
	}

	int get_delay(const int current_year) const;

	template <typename scope_type>
	const std::vector<const scoped_event_base<scope_type> *> &get_events() const
	{
		return std::get<event_vector<scope_type>>(this->events);
	}

	template <typename scope_type>
	void add_event(const scoped_event_base<const scope_type> *event)
	{
		std::get<event_vector<scope_type>>(this->events).push_back(event);
	}

private:
	event_trigger trigger;
	int none_weight = 0;
	int delay = 0;
	int delay_days = 0;
	std::tuple<event_vector<character>, event_vector<domain>, event_vector<province>, event_vector<site>> events;
};

}
