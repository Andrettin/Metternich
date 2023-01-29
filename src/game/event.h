#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

enum class event_trigger;
struct context;
struct read_only_context;

class event : public named_data_entry
{
	Q_OBJECT

	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)
	Q_PROPERTY(metternich::event_trigger trigger MEMBER trigger READ get_trigger)
	Q_PROPERTY(bool random READ is_random WRITE set_random)

public:
	static constexpr int default_random_weight = 100;
	static constexpr const char option_default_name[] = "OK";

	explicit event(const std::string &identifier);

	virtual void process_gsml_property(const gsml_property &property) override;

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

	virtual bool is_random() const = 0;

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

	virtual void set_random_weight(const int weight) = 0;

	virtual int get_option_count() const = 0;
	virtual const std::string &get_option_name(const int option_index) const = 0;
	virtual std::string get_option_tooltip(const int option_index, const read_only_context &ctx) const = 0;
	virtual void do_option_effects(const int option_index, context &ctx) const = 0;

	void create_instance(const context &ctx) const;

signals:
	void changed();

private:
	std::string description;
	event_trigger trigger;
};

}
