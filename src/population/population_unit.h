#pragma once

namespace metternich {

class culture;
class employment_type;
class icon;
class phenotype;
class population_type;
class province;

class population_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::population_type* type READ get_type_unconst NOTIFY type_changed)
	Q_PROPERTY(metternich::culture* culture READ get_culture_unconst NOTIFY culture_changed)
	Q_PROPERTY(metternich::phenotype* phenotype READ get_phenotype_unconst NOTIFY phenotype_changed)
	Q_PROPERTY(metternich::icon* icon READ get_icon_unconst NOTIFY icon_changed)
	Q_PROPERTY(metternich::province* province READ get_province_unconst NOTIFY province_changed)

public:
	static constexpr int base_score = 1;

	explicit population_unit(const population_type *type, const metternich::culture *culture, const metternich::phenotype *phenotype, const metternich::province *province);

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

	const metternich::province *get_province() const
	{
		return this->province;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::province *get_province_unconst() const
	{
		return const_cast<metternich::province *>(this->get_province());
	}

public:
	void set_province(const metternich::province *province);

	bool is_employed() const
	{
		return this->employed;
	}

	void set_employed(const bool employed)
	{
		this->employed = employed;
	}

	centesimal_int get_employment_output(const employment_type *employment_type) const;

signals:
	void type_changed();
	void culture_changed();
	void phenotype_changed();
	void icon_changed();
	void province_changed();

private:
	const population_type *type = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::phenotype *phenotype = nullptr;
	const metternich::province *province = nullptr;
	bool employed = false;
};

}
