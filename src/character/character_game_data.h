#pragma once

namespace metternich {

class character;
class country;
class landed_title;
class office;
class trait;
enum class attribute;
enum class trait_type;

class character_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* employer READ get_employer_unconst NOTIFY employer_changed)
	Q_PROPERTY(int age READ get_age NOTIFY age_changed)
	Q_PROPERTY(int primary_attribute_value READ get_primary_attribute_value NOTIFY attributes_changed)
	Q_PROPERTY(int diplomacy READ get_diplomacy NOTIFY attributes_changed)
	Q_PROPERTY(int martial READ get_martial NOTIFY attributes_changed)
	Q_PROPERTY(int stewardship READ get_stewardship NOTIFY attributes_changed)
	Q_PROPERTY(int intrigue READ get_intrigue NOTIFY attributes_changed)
	Q_PROPERTY(int learning READ get_learning NOTIFY attributes_changed)
	Q_PROPERTY(int prowess READ get_prowess NOTIFY attributes_changed)
	Q_PROPERTY(int vitality READ get_vitality NOTIFY attributes_changed)
	Q_PROPERTY(QVariantList traits READ get_traits_qvariant_list NOTIFY traits_changed)
	Q_PROPERTY(QVariantList landed_titles READ get_landed_titles_qvariant_list NOTIFY landed_titles_changed)
	Q_PROPERTY(bool ruler READ is_ruler NOTIFY landed_titles_changed)
	Q_PROPERTY(metternich::office* office READ get_office_unconst NOTIFY office_changed)
	Q_PROPERTY(QString country_modifier_string READ get_country_modifier_string NOTIFY traits_changed)
	Q_PROPERTY(QString province_modifier_string READ get_province_modifier_string NOTIFY traits_changed)
	Q_PROPERTY(int wealth READ get_wealth NOTIFY wealth_changed)

public:
	explicit character_game_data(const metternich::character *character);

	void on_game_started();

	void do_turn();
	void do_events();

	const metternich::country *get_employer() const
	{
		return this->employer;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::country *get_employer_unconst() const
	{
		return const_cast<metternich::country *>(this->get_employer());
	}

public:
	void set_employer(const metternich::country *employer);

	int get_age() const;

	const std::vector<const trait *> &get_traits() const
	{
		return this->traits;
	}

	QVariantList get_traits_qvariant_list() const;

	bool has_trait(const trait *trait) const;
	void add_trait(const trait *trait);
	void remove_trait(const trait *trait);
	const trait *generate_trait(const trait_type trait_type, const int max_level);
	void generate_expertise_traits();
	void sort_traits();
	int get_total_trait_level() const;

	int get_unclamped_attribute_value(const attribute attribute) const
	{
		const auto find_iterator = this->attribute_values.find(attribute);
		if (find_iterator != this->attribute_values.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	int get_attribute_value(const attribute attribute) const;
	void set_attribute_value(const attribute attribute, const int value);

	void change_attribute_value(const attribute attribute, const int change)
	{
		this->set_attribute_value(attribute, this->get_unclamped_attribute_value(attribute) + change);
	}

	int get_primary_attribute_value() const;
	int get_diplomacy() const;
	int get_martial() const;
	int get_stewardship() const;
	int get_intrigue() const;
	int get_learning() const;
	int get_prowess() const;
	int get_vitality() const;

	const std::vector<const landed_title *> &get_landed_titles() const
	{
		return this->landed_titles;
	}

	QVariantList get_landed_titles_qvariant_list() const;

	void add_landed_title(const landed_title *landed_title)
	{
		this->landed_titles.push_back(landed_title);
		emit landed_titles_changed();
	}

	void remove_landed_title(const landed_title *landed_title)
	{
		std::erase(this->landed_titles, landed_title);
		emit landed_titles_changed();
	}

	bool is_ruler() const;

	const metternich::office *get_office() const
	{
		return this->office;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::office *get_office_unconst() const
	{
		return const_cast<metternich::office *>(this->get_office());
	}

public:
	void set_office(const metternich::office *office);

	QString get_country_modifier_string() const;
	QString get_province_modifier_string() const;
	void apply_country_modifier(const country *country, const int multiplier);
	void apply_province_modifier(const province *province, const int multiplier);

	int get_wealth() const
	{
		return this->wealth;
	}

	void set_wealth(const int wealth)
	{
		if (wealth == this->get_wealth()) {
			return;
		}

		this->wealth = wealth;

		emit wealth_changed();
	}

	void change_wealth(const int change)
	{
		this->set_wealth(this->get_wealth() + change);
	}

signals:
	void employer_changed();
	void age_changed();
	void traits_changed();
	void attributes_changed();
	void landed_titles_changed();
	void office_changed();
	void wealth_changed();

private:
	const metternich::character *character = nullptr;
	const metternich::country *employer = nullptr;
	std::vector<const trait *> traits;
	std::map<attribute, int> attribute_values;
	std::vector<const landed_title *> landed_titles;
	const metternich::office *office = nullptr;
	int wealth = 0;
};

}
