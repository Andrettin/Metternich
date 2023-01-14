#pragma once

namespace metternich {

class character;
class country;
class trait;
enum class attribute;
enum class trait_type;

class character_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* employer READ get_employer_unconst NOTIFY employer_changed)
	Q_PROPERTY(int age READ get_age NOTIFY age_changed)
	Q_PROPERTY(int primary_attribute_value READ get_primary_attribute_value NOTIFY attributes_changed)

public:
	explicit character_game_data(const metternich::character *character);

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

	void add_trait(const trait *trait);
	void generate_trait(const trait_type trait_type);

	int get_unclamped_attribute_value(const attribute attribute) const
	{
		const auto find_iterator = this->attribute_values.find(attribute);
		if (find_iterator != this->attribute_values.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	int get_attribute_value(const attribute attribute) const
	{
		const int value = this->get_unclamped_attribute_value(attribute);

		if (value >= 0) {
			return value;
		}

		return 0;
	}

	void set_attribute_value(const attribute attribute, const int value);

	void change_attribute_value(const attribute attribute, const int change)
	{
		this->set_attribute_value(attribute, this->get_unclamped_attribute_value(attribute) + change);
	}

	int get_primary_attribute_value() const;

signals:
	void employer_changed();
	void age_changed();
	void traits_changed();
	void attributes_changed();

private:
	const metternich::character *character = nullptr;
	const metternich::country *employer = nullptr;
	std::vector<const trait *> traits;
	std::map<attribute, int> attribute_values;
};

}
