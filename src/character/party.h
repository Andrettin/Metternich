#pragma once

namespace metternich {

class character;
class domain;
class skill;

//group of characters who are engaging in combat or visiting a site
class party final
{
public:
	static int get_max_appropriate_dungeon_level(const std::vector<const character *> &characters);

	explicit party(const std::vector<const character *> &characters);
	~party();

	const metternich::domain *get_domain() const
	{
		return this->domain;
	}

	const std::vector<const character *> &get_characters() const
	{
		return this->characters;
	}

	QVariantList get_characters_qvariant_list() const;
	void remove_character(const character *character);

	void gain_experience(const int64_t experience);

	const character *get_best_skill_character(const skill *skill) const;

	int get_max_appropriate_dungeon_level() const;

private:
	const metternich::domain *domain = nullptr;
	std::vector<const character *> characters;
};

}
