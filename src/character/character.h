#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character_type;
class culture;
class icon;
class phenotype;
class province;

class character final : public named_data_entry, public data_type<character>
{
	Q_OBJECT

	Q_PROPERTY(QString surname READ get_surname_qstring NOTIFY changed)
	Q_PROPERTY(QString full_name READ get_full_name_qstring NOTIFY changed)
	Q_PROPERTY(metternich::character_type* type MEMBER type NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::phenotype* phenotype MEMBER phenotype NOTIFY changed)
	Q_PROPERTY(metternich::icon* portrait READ get_portrait_unconst NOTIFY changed)
	Q_PROPERTY(metternich::province* home_province MEMBER home_province NOTIFY changed)
	Q_PROPERTY(QDateTime start_date MEMBER start_date READ get_start_date NOTIFY changed)
	Q_PROPERTY(QDateTime end_date MEMBER end_date READ get_end_date NOTIFY changed)
	Q_PROPERTY(QDateTime birth_date MEMBER birth_date READ get_birth_date NOTIFY changed)
	Q_PROPERTY(QDateTime death_date MEMBER death_date READ get_death_date NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "character";
	static constexpr const char property_class_identifier[] = "metternich::character*";
	static constexpr const char database_folder[] = "characters";

	explicit character(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const std::string &get_surname() const
	{
		return this->surname;
	}

	Q_INVOKABLE void set_surname(const std::string &surname)
	{
		this->surname = surname;
	}

	QString get_surname_qstring() const
	{
		return QString::fromStdString(this->get_surname());
	}

	std::string get_full_name() const;

	QString get_full_name_qstring() const
	{
		return QString::fromStdString(this->get_full_name());
	}

	const character_type *get_type() const
	{
		return this->type;
	}

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const metternich::phenotype *get_phenotype() const
	{
		return this->phenotype;
	}

	const icon *get_portrait() const;

private:
	//for the Qt property (pointers there can't be const)
	icon *get_portrait_unconst() const
	{
		return const_cast<icon *>(this->get_portrait());
	}

public:
	const province *get_home_province() const
	{
		return this->home_province;
	}

	const QDateTime &get_start_date() const
	{
		return this->start_date;
	}

	const QDateTime &get_end_date() const
	{
		return this->end_date;
	}

	const QDateTime &get_birth_date() const
	{
		return this->birth_date;
	}

	const QDateTime &get_death_date() const
	{
		return this->death_date;
	}

signals:
	void changed();

private:
	std::string surname;
	character_type *type = nullptr;
	metternich::culture *culture = nullptr;
	metternich::phenotype *phenotype = nullptr;
	province *home_province = nullptr;
	QDateTime start_date;
	QDateTime end_date;
	QDateTime birth_date;
	QDateTime death_date;
};

}
