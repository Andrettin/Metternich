import QtQuick
import QtQuick.Controls
import ".."

GarrisonDialogBase {
	id: garrison_dialog
	title: "Garrison"
	units_model: (selected_province !== null && selected_garrison && selected_province.game_data.military_units.length > 0) ? selected_province.game_data.get_country_military_units_qvariant_list(metternich.game.player_country) : []
}
