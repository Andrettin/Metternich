#pragma once

namespace metternich {

enum class age_category {
	none,
	child,
	adult,
	middle_aged,
	old,
	venerable
};

inline std::string_view get_age_category_name(const age_category age_category)
{
	switch (age_category) {
		case age_category::child:
			return "Child";
		case age_category::adult:
			return "Adult";
		case age_category::middle_aged:
			return "Middle Aged";
		case age_category::old:
			return "Old";
		case age_category::venerable:
			return "Venerable";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid age category: {}", static_cast<int>(age_category)));
}

}

Q_DECLARE_METATYPE(metternich::age_category)
