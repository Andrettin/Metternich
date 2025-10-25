#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

template <typename scope_type>
class effect_list;

class trap_type final : public named_data_entry, public data_type<trap_type>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "trap_type";
	static constexpr const char property_class_identifier[] = "metternich::trap_type*";
	static constexpr const char database_folder[] = "trap_types";

	explicit trap_type(const std::string &identifier);
	~trap_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const effect_list<const character> *get_trigger_effects() const
	{
		return this->trigger_effects.get();
	}

signals:
	void changed();

private:
	std::unique_ptr<effect_list<const character>> trigger_effects;
};

}
