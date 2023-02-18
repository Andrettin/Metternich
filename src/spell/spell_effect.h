#pragma once

#include "database/basic_data_entry.h"
#include "util/qunique_ptr.h"

namespace metternich {

class military_unit;

class spell_effect : public basic_data_entry
{
	Q_OBJECT

public:
	static qunique_ptr<spell_effect> from_gsml_property(const gsml_property &property);
	static qunique_ptr<spell_effect> from_gsml_scope(const gsml_data &scope);

	virtual ~spell_effect()
	{
	}

	virtual const std::string &get_identifier() const = 0;
	virtual void apply(military_unit *caster, military_unit *target) const = 0;
	virtual std::string get_string(const military_unit *caster) const = 0;
};

}
