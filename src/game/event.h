#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("game/event_random_group.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class event_random_group;
class portrait;
enum class event_trigger;
struct context;
struct read_only_context;

class event : public named_data_entry
{
	Q_OBJECT

	Q_PROPERTY(const metternich::portrait* portrait READ get_portrait WRITE set_portrait NOTIFY changed)
	Q_PROPERTY(bool circular_portrait MEMBER circular_portrait READ has_circular_portrait NOTIFY changed)
	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)
	Q_PROPERTY(metternich::event_trigger trigger MEMBER trigger READ get_trigger)
	Q_PROPERTY(metternich::event_random_group* random_group MEMBER random_group)
	Q_PROPERTY(bool random READ is_random WRITE set_random)
	Q_PROPERTY(bool hidden MEMBER hidden READ is_hidden)
	Q_PROPERTY(bool only_once MEMBER only_once READ fires_only_once)
	Q_PROPERTY(bool news MEMBER news READ is_news NOTIFY changed)
	Q_PROPERTY(QString newspaper READ get_newspaper_qstring NOTIFY changed)

public:
	static constexpr int default_random_weight = 100;
	static constexpr const char option_default_name[] = "OK";

	explicit event(const std::string &identifier);

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void initialize() override;

	virtual std::string_view get_event_type_string() const = 0;

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	void set_portrait(const metternich::portrait *portrait)
	{
		this->portrait = portrait;
	}

	bool has_circular_portrait() const
	{
		return this->circular_portrait;
	}

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

	event_random_group *get_random_group() const
	{
		return this->random_group;
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

	bool is_hidden() const
	{
		return this->hidden;
	}

	bool fires_only_once() const
	{
		return this->only_once;
	}

	bool is_news() const
	{
		return this->news;
	}

	const std::string &get_newspaper() const
	{
		return this->newspaper;
	}

	Q_INVOKABLE void set_newspaper(const std::string &newspaper)
	{
		this->newspaper = newspaper;
	}

	QString get_newspaper_qstring() const
	{
		return QString::fromStdString(this->get_newspaper());
	}

	virtual int get_option_count() const = 0;
	virtual bool is_option_available(const int option_index, const read_only_context &ctx) const = 0;
	virtual const std::string &get_option_name(const int option_index) const = 0;
	virtual std::string get_option_tooltip(const int option_index, const read_only_context &ctx) const = 0;
	virtual void do_option_effects(const int option_index, context &ctx) const = 0;

	void create_instance(const context &ctx) const;

signals:
	void changed();

private:
	const metternich::portrait *portrait = nullptr;
	bool circular_portrait = false;
	std::string description;
	event_trigger trigger;
	event_random_group *random_group = nullptr;
	bool hidden = false;
	bool only_once = false;
	bool news = false;
	std::string newspaper;
};

}
