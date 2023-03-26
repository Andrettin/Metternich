#pragma once

#include "util/fractional_int.h"

namespace metternich {

class country;
class culture;
class employment_type;
class icon;
class ideology;
class phenotype;
class population_type;
class religion;

class population_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::population_type* type READ get_type_unconst NOTIFY type_changed)
	Q_PROPERTY(metternich::culture* culture READ get_culture_unconst NOTIFY culture_changed)
	Q_PROPERTY(metternich::religion* religion READ get_religion_unconst NOTIFY religion_changed)
	Q_PROPERTY(metternich::phenotype* phenotype READ get_phenotype_unconst NOTIFY phenotype_changed)
	Q_PROPERTY(metternich::icon* icon READ get_icon_unconst NOTIFY icon_changed)
	Q_PROPERTY(metternich::country* country READ get_country_unconst NOTIFY country_changed)

public:
	static constexpr int base_score = 1;

	explicit population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const metternich::country *country);

	const population_type *get_type() const
	{
		return this->type;
	}

private:
	//for the Qt property (pointers there can't be const)
	population_type *get_type_unconst() const
	{
		return const_cast<population_type *>(this->get_type());
	}

public:
	void set_type(const population_type *type);

	const culture *get_culture() const
	{
		return this->culture;
	}

private:
	//for the Qt property (pointers there can't be const)
	culture *get_culture_unconst() const
	{
		return const_cast<metternich::culture *>(this->get_culture());
	}

public:
	void set_culture(const metternich::culture *culture);

	const religion *get_religion() const
	{
		return this->religion;
	}

private:
	//for the Qt property (pointers there can't be const)
	religion *get_religion_unconst() const
	{
		return const_cast<metternich::religion *>(this->get_religion());
	}

public:
	void set_religion(const metternich::religion *religion);

	const phenotype *get_phenotype() const
	{
		return this->phenotype;
	}

private:
	//for the Qt property (pointers there can't be const)
	phenotype *get_phenotype_unconst() const
	{
		return const_cast<metternich::phenotype *>(this->get_phenotype());
	}

public:
	void set_phenotype(const metternich::phenotype *phenotype);

	const icon *get_icon() const;

private:
	//for the Qt property (pointers there can't be const)
	icon *get_icon_unconst() const
	{
		return const_cast<icon *>(this->get_icon());
	}

public:
	const icon *get_small_icon() const;

	const metternich::country *get_country() const
	{
		return this->country;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::country *get_country_unconst() const
	{
		return const_cast<metternich::country *>(this->get_country());
	}

public:
	void set_country(const metternich::country *country);

	bool is_employed() const
	{
		return this->get_employment_type() != nullptr;
	}

	const metternich::employment_type *get_employment_type() const
	{
		return this->employment_type;
	}

	void set_employment_type(const metternich::employment_type *employment_type)
	{
		this->employment_type = employment_type;
	}

	centesimal_int get_employment_output(const employment_type *employment_type) const;

	centesimal_int get_employment_output() const
	{
		if (this->get_employment_type() != nullptr) {
			return this->get_employment_output(this->get_employment_type());
		}

		return centesimal_int(0);
	}

	bool produces_food() const;

	const metternich::ideology *get_ideology() const
	{
		return this->ideology;
	}

	void set_ideology(const metternich::ideology *ideology);

	void choose_ideology();

	void migrate_to(const metternich::country *country);

signals:
	void type_changed();
	void culture_changed();
	void religion_changed();
	void phenotype_changed();
	void icon_changed();
	void country_changed();

private:
	const population_type *type = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::phenotype *phenotype = nullptr;
	const metternich::country *country = nullptr;
	const metternich::employment_type *employment_type = nullptr;
	const metternich::ideology *ideology = nullptr;
};

}
