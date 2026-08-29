#pragma once

#include "database/defines_base.h"
#include "util/decimillesimal_int.h"
#include "util/singleton.h"

Q_MOC_INCLUDE("population/population_class.h")

namespace metternich {

class population_class;
class population_unit;

template <typename scope_type>
class factor;

class population_defines final : public defines_base, public singleton<population_defines>
{
	Q_OBJECT

	Q_PROPERTY(metternich::population_class* default_population_class MEMBER default_population_class)
	Q_PROPERTY(metternich::population_class* default_tribal_population_class MEMBER default_tribal_population_class)
	Q_PROPERTY(qint64 base_population_needs_size MEMBER base_population_needs_size READ get_base_population_needs_size NOTIFY changed)
	Q_PROPERTY(archimedes::decimillesimal_int base_monthly_population_growth_rate MEMBER base_monthly_population_growth_rate READ get_base_monthly_population_growth_rate NOTIFY changed)
	Q_PROPERTY(archimedes::decimillesimal_int base_monthly_promotion_rate MEMBER base_monthly_promotion_rate READ get_base_monthly_promotion_rate NOTIFY changed)
	Q_PROPERTY(archimedes::decimillesimal_int base_monthly_literacy_change_rate MEMBER base_monthly_literacy_change_rate READ get_base_monthly_literacy_change_rate NOTIFY changed)
	Q_PROPERTY(archimedes::decimillesimal_int base_literacy_educator_percent MEMBER base_literacy_educator_percent READ get_base_literacy_educator_percent NOTIFY changed)
	Q_PROPERTY(archimedes::decimillesimal_int max_literacy_educator_percent MEMBER max_literacy_educator_percent READ get_max_literacy_educator_percent NOTIFY changed)

public:
	population_defines();
	~population_defines();

	virtual std::string_view get_file_name() const override
	{
		return "population_defines.txt";
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const population_class *get_default_population_class() const
	{
		return this->default_population_class;
	}

	const population_class *get_default_tribal_population_class() const
	{
		return this->default_tribal_population_class;
	}

	int64_t get_base_population_needs_size() const
	{
		return this->base_population_needs_size;
	}

	const decimillesimal_int &get_base_monthly_population_growth_rate() const
	{
		return this->base_monthly_population_growth_rate;
	}

	const decimillesimal_int &get_base_monthly_promotion_rate() const
	{
		return this->base_monthly_promotion_rate;
	}

	const decimillesimal_int &get_base_monthly_literacy_change_rate() const
	{
		return this->base_monthly_literacy_change_rate;
	}

	const decimillesimal_int &get_base_literacy_educator_percent() const
	{
		return this->base_literacy_educator_percent;
	}

	const decimillesimal_int &get_max_literacy_educator_percent() const
	{
		return this->max_literacy_educator_percent;
	}

	const factor<population_unit> *get_promotion_chance() const
	{
		return this->promotion_chance.get();
	}

	const factor<population_unit> *get_demotion_chance() const
	{
		return this->demotion_chance.get();
	}

signals:
	void changed();

private:
	population_class *default_population_class = nullptr;
	population_class *default_tribal_population_class = nullptr;
	int64_t base_population_needs_size = 0;
	decimillesimal_int base_monthly_population_growth_rate;
	decimillesimal_int base_monthly_promotion_rate;
	decimillesimal_int base_monthly_literacy_change_rate;
	decimillesimal_int base_literacy_educator_percent;
	decimillesimal_int max_literacy_educator_percent;
	std::unique_ptr<factor<population_unit>> promotion_chance;
	std::unique_ptr<factor<population_unit>> demotion_chance;
};

}
