#pragma once

namespace metternich {

enum class spell_target {
	none,
	enemy,
	ally
};

}

Q_DECLARE_METATYPE(metternich::spell_target)
