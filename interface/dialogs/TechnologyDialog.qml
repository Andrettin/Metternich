import QtQuick
import QtQuick.Controls
import ".."

ModifierDialog {
	id: technology_dialog
	title: technology ? technology.name : ""
	modifier_string: technology ? technology.get_effects_string(metternich.game.player_country) : ""
	date_string: technology && technology.year !== 0 ? ("Historical Year: " + year_string(technology.year)) : ""
	description: technology ? technology.description : ""
	
	property var technology: null
}
