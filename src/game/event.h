#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class country;
enum class event_trigger;

template <typename scope_type>
class condition;

template <typename scope_type>
class factor;

class event final : public named_data_entry, public data_type<event>
{
	Q_OBJECT

	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)
	Q_PROPERTY(metternich::event_trigger trigger MEMBER trigger READ get_trigger)
	Q_PROPERTY(bool random READ is_random WRITE set_random)

public:
	static constexpr const char class_identifier[] = "event";
	static constexpr const char property_class_identifier[] = "metternich::event*";
	static constexpr const char database_folder[] = "events";
	static constexpr int default_random_weight = 100;

	static const std::vector<const event *> &get_trigger_events(const event_trigger trigger)
	{
		static std::vector<const event *> empty_list;

		const auto find_iterator = event::trigger_events.find(trigger);
		if (find_iterator != event::trigger_events.end()) {
			return find_iterator->second;
		}

		return empty_list;
	}

	static const std::vector<const event *> &get_trigger_random_events(const event_trigger trigger)
	{
		static std::vector<const event *> empty_list;

		const auto find_iterator = event::trigger_random_events.find(trigger);
		if (find_iterator != event::trigger_random_events.end()) {
			return find_iterator->second;
		}

		return empty_list;
	}

	static void clear()
	{
		data_type::clear();
		event::trigger_events.clear();
		event::trigger_random_events.clear();
	}

private:
	static inline std::map<event_trigger, std::vector<const event *>> trigger_events;
	static inline std::map<event_trigger, std::vector<const event *>> trigger_random_events;

public:
	explicit event(const std::string &identifier);
	~event();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	void initialize() override;

	const std::string &get_description() const
	{
		return this->description;
	}

	Q_INVOKABLE void set_description(const std::string &description)
	{
		this->description = description;
	}

	QString get_description_qstring() const
	{
		return QString::fromStdString(this->get_description());
	}

	event_trigger get_trigger() const
	{
		return this->trigger;
	}

	bool is_random() const
	{
		return this->get_random_weight_factor() != nullptr;
	}

	void set_random(const bool random)
	{
		if (random == this->is_random()) {
			return;
		}

		if (random) {
			this->set_random_weight(event::default_random_weight);
		} else {
			this->set_random_weight(0);
		}
	}

	void set_random_weight(const int weight);

	const factor<country> *get_random_weight_factor() const
	{
		return this->random_weight_factor.get();
	}

	const condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	void fire(const country *country) const;

signals:
	void changed();

private:
	std::string description;
	event_trigger trigger;
	std::unique_ptr<factor<country>> random_weight_factor;
	std::unique_ptr<const condition<country>> conditions;
};

}
