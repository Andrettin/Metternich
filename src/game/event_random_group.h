#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character;
class country;
class province;
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

public:
	static const std::vector<event_random_group *> &get_all_of_trigger(const event_trigger trigger)
	{
		static std::vector<event_random_group *> empty_vector;

		const auto find_iterator = event_random_group::groups_by_trigger.find(trigger);
		if (find_iterator != event_random_group::groups_by_trigger.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

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
		if constexpr (std::is_same_v<scope_type, const character>) {
			return this->character_events;
		} else if constexpr (std::is_same_v<scope_type, const country>) {
			return this->country_events;
		} else if constexpr (std::is_same_v<scope_type, const province>) {
			return this->province_events;
		}
	}

	void add_event(const scoped_event_base<const character> *event)
	{
		this->character_events.push_back(event);
	}

	void add_event(const scoped_event_base<const country> *event)
	{
		this->country_events.push_back(event);
	}

	void add_event(const scoped_event_base<const province> *event)
	{
		this->province_events.push_back(event);
	}

private:
	event_trigger trigger;
	int none_weight = 0;
	int delay = 0;
	int delay_days = 0;
	std::vector<const scoped_event_base<const character> *> character_events;
	std::vector<const scoped_event_base<const country> *> country_events;
	std::vector<const scoped_event_base<const province> *> province_events;
};

}
