#pragma once

#include "database/defines_base.h"
#include "util/centesimal_int.h"
#include "util/singleton.h"

namespace metternich {

class holding_defines final : public defines_base, public singleton<holding_defines>
{
	Q_OBJECT

public:
	holding_defines();
	~holding_defines();

	virtual std::string_view get_file_name() const override
	{
		return "holding_defines.txt";
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const centesimal_int &get_consumption_for_holding_level(const int holding_level) const;

signals:
	void changed();

private:
	std::map<int, centesimal_int> consumption_per_holding_level;
};

}
