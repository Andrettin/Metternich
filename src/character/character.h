#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/fractional_int.h"
#include "util/qunique_ptr.h"

namespace archimedes {
	enum class gender;
}

namespace metternich {

class character_game_data;
class character_type;
class culture;
class dynasty;
class icon;
class phenotype;
class province;
class religion;
class trait;

class character final : public named_data_entry, public data_type<character>
{
	Q_OBJECT

	Q_PROPERTY(metternich::dynasty* dynasty MEMBER dynasty NOTIFY changed)
	Q_PROPERTY(QString surname READ get_surname_qstring NOTIFY changed)
	Q_PROPERTY(QString full_name READ get_full_name_qstring NOTIFY changed)
	Q_PROPERTY(metternich::character_type* type MEMBER type NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::religion* religion MEMBER religion NOTIFY changed)
	Q_PROPERTY(metternich::phenotype* phenotype MEMBER phenotype NOTIFY changed)
	Q_PROPERTY(metternich::icon* portrait READ get_portrait_unconst NOTIFY changed)
	Q_PROPERTY(metternich::province* home_province MEMBER home_province NOTIFY changed)
	Q_PROPERTY(archimedes::gender gender MEMBER gender NOTIFY changed)
	Q_PROPERTY(QDateTime start_date MEMBER start_date READ get_start_date NOTIFY changed)
	Q_PROPERTY(QDateTime end_date MEMBER end_date READ get_end_date NOTIFY changed)
	Q_PROPERTY(QDateTime birth_date MEMBER birth_date READ get_birth_date NOTIFY changed)
	Q_PROPERTY(QDateTime death_date MEMBER death_date READ get_death_date NOTIFY changed)
	Q_PROPERTY(int level MEMBER level READ get_level NOTIFY changed)
	Q_PROPERTY(archimedes::centesimal_int level_multiplier READ get_level_multiplier WRITE set_level_multiplier NOTIFY changed)
	Q_PROPERTY(metternich::character_game_data* game_data READ get_game_data NOTIFY game_data_changed)

public:
	static constexpr const char class_identifier[] = "character";
	static constexpr const char property_class_identifier[] = "metternich::character*";
	static constexpr const char database_folder[] = "characters";
	static constexpr int default_max_level = 6; //the maximum level in normal circumstances

	explicit character(const std::string &identifier);
	~character();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	void reset_game_data();

	character_game_data *get_game_data() const
	{
		return this->game_data.get();
	}

	const dynasty *get_dynasty() const
	{
		return this->dynasty;
	}

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

	const metternich::religion *get_religion() const
	{
		return this->religion;
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

	gender get_gender() const
	{
		return this->gender;
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

	int get_level() const
	{
		return this->level;
	}

	centesimal_int get_level_multiplier() const
	{
		return centesimal_int(this->get_level()) / character::default_max_level;
	}

	void set_level_multiplier(const centesimal_int &level_multiplier)
	{
		this->level = (level_multiplier * character::default_max_level).to_int();
	}

	const std::vector<const trait *> &get_traits() const
	{
		return this->traits;
	}

signals:
	void changed();
	void game_data_changed() const;

private:
	metternich::dynasty *dynasty = nullptr;
	std::string surname;
	character_type *type = nullptr;
	metternich::culture *culture = nullptr;
	metternich::religion *religion = nullptr;
	metternich::phenotype *phenotype = nullptr;
	province *home_province = nullptr;
	archimedes::gender gender;
	QDateTime start_date;
	QDateTime end_date;
	QDateTime birth_date;
	QDateTime death_date;
	int level = 1;
	std::vector<const trait *> traits;
	qunique_ptr<character_game_data> game_data;
};

}
