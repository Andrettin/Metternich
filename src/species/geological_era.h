#pragma once

namespace metternich {

enum class geological_era {
	none,
	devonian,
	carboniferous,
	permian,
	triassic,
	jurassic,
	cretaceous,
	paleocene,
	eocene,
	oligocene,
	miocene,
	pliocene,
	pleistocene,
	holocene
};

}

Q_DECLARE_METATYPE(metternich::geological_era)
