#pragma once

#include "util/centesimal_int.h"

Q_MOC_INCLUDE("domain/culture.h")
Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("map/province.h")
Q_MOC_INCLUDE("map/site.h")
Q_MOC_INCLUDE("population/population_type.h")
Q_MOC_INCLUDE("religion/religion.h")
Q_MOC_INCLUDE("species/phenotype.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class culture;
class domain;
class icon;
class phenotype;
class population_type;
class province;
class religion;
class site;
class species;

class population_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::population_type* type READ get_type NOTIFY type_changed)
	Q_PROPERTY(const metternich::culture* culture READ get_culture NOTIFY culture_changed)
	Q_PROPERTY(const metternich::religion* religion READ get_religion NOTIFY religion_changed)
	Q_PROPERTY(const metternich::phenotype* phenotype READ get_phenotype NOTIFY phenotype_changed)
	Q_PROPERTY(const metternich::icon* icon READ get_icon NOTIFY icon_changed)
	Q_PROPERTY(const metternich::domain* country READ get_country NOTIFY country_changed)
	Q_PROPERTY(const metternich::province* province READ get_province NOTIFY province_changed)
	Q_PROPERTY(const metternich::site* site READ get_site NOTIFY site_changed)
	Q_PROPERTY(qint64 size READ get_size NOTIFY size_changed)

public:
	explicit population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const int64_t size, const site *site);

	std::string get_scope_name() const;

	const population_type *get_type() const
	{
		return this->type;
	}

	void set_type(const population_type *type);

	const culture *get_culture() const
	{
		return this->culture;
	}

	void set_culture(const metternich::culture *culture);

	const religion *get_religion() const
	{
		return this->religion;
	}

	void set_religion(const metternich::religion *religion);

	const phenotype *get_phenotype() const
	{
		return this->phenotype;
	}

	void set_phenotype(const metternich::phenotype *phenotype);

	const species *get_species() const;

	const icon *get_icon() const;
	const icon *get_small_icon() const;

	const metternich::domain *get_country() const
	{
		return this->domain;
	}

	void set_country(const metternich::domain *domain);

	const province *get_province() const;

	const site *get_site() const
	{
		return this->site;
	}

	void set_site(const site *site);

	int64_t get_size() const
	{
		return this->size;
	}

	void set_size(const int64_t size);

	void change_size(const int64_t change)
	{
		this->set_size(this->get_size() + change);
	}

	bool is_food_producer() const;

signals:
	void type_changed();
	void culture_changed();
	void religion_changed();
	void phenotype_changed();
	void icon_changed();
	void country_changed();
	void province_changed();
	void site_changed();
	void size_changed();

private:
	const population_type *type = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::phenotype *phenotype = nullptr;
	const metternich::domain *domain = nullptr;
	const metternich::site *site = nullptr;
	int64_t size = 0; //number of individuals in this population unit
};

}
