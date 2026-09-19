#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/dice.h"

Q_MOC_INCLUDE("character/save_type.h")

namespace metternich {

class save_type;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class modifier;

class status_effect final : public named_data_entry, public data_type<status_effect>
{
	Q_OBJECT

	Q_PROPERTY(QString adjective READ get_adjective_qstring NOTIFY changed)
	Q_PROPERTY(const metternich::save_type*save_type MEMBER save_type READ get_save_type NOTIFY changed)
	Q_PROPERTY(int save_modifier MEMBER save_modifier READ get_save_modifier NOTIFY changed)
	Q_PROPERTY(archimedes::dice duration_rounds MEMBER duration_rounds READ get_duration_rounds NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "status_effect";
	static constexpr const char property_class_identifier[] = "metternich::status_effect*";
	static constexpr const char database_folder[] = "status_effects";

	explicit status_effect(const std::string &identifier);
	~status_effect();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const std::string &get_adjective() const
	{
		return this->adjective;
	}

	Q_INVOKABLE void set_adjective(const std::string &adjective)
	{
		this->adjective = adjective;
	}

	QString get_adjective_qstring() const
	{
		return QString::fromStdString(this->get_adjective());
	}

	const metternich::save_type *get_save_type() const
	{
		return this->save_type;
	}

	int get_save_modifier() const
	{
		return this->save_modifier;
	}

	const dice &get_duration_rounds() const
	{
		return this->duration_rounds;
	}

	const std::chrono::seconds &get_duration_per_caster_level() const
	{
		return this->duration_per_caster_level;
	}

	std::chrono::seconds get_duration(const std::optional<int> &caster_level) const;

	const metternich::modifier<const character> *get_modifier() const
	{
		return this->modifier.get();
	}

	const effect_list<const character> *get_end_effects() const
	{
		return this->end_effects.get();
	}

signals:
	void changed();

private:
	std::string adjective;
	const metternich::save_type *save_type = nullptr;
	int save_modifier = 0;
	dice duration_rounds;
	std::chrono::seconds duration_per_caster_level = std::chrono::seconds(0);
	std::unique_ptr<const metternich::modifier<const character>> modifier;
	std::unique_ptr<effect_list<const character>> end_effects; //effects after the duration has passed
};

}
