import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: construction_choice_dialog
	title: "Choose Construction"
	width: 256 * scale_factor
	height: content_height
	
	readonly property int content_height: construction_button_column.y + construction_button_column.height + 8 * scale_factor
	
	property var buildable_locations: []
	
	SmallText {
		id: text_label
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		text: "Which construction project shall we undertake next?"
		wrapMode: Text.WordWrap
	}
	
	Column {
		id: construction_button_column
		anchors.top: text_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		
		Repeater {
			model: buildable_locations
			
			TextButton {
				id: construction_button
				text: format_text(construction_name + " (" + buildable_location_name + ")")
				width: construction_choice_dialog.width - 16 * scale_factor
				tooltip: effects_string.length > 0 ? format_text(small_text(effects_string)) : small_text("No effect")
				
				readonly property var buildable_location: model.modelData
				readonly property var province: buildable_location.class_name === "metternich::province" ? buildable_location : null
				readonly property var building_slot: province !== null ? null : buildable_location
				readonly property var building: building_slot ? building_slot.get_buildable_building() : null
				readonly property var pathway: province ? province.game_data.get_buildable_pathway() : null
				readonly property var construction_name: building ? building.name : pathway.name
				readonly property var buildable_location_name: building_slot ? (building_slot.holding.game_data.current_cultural_name + ", " + building_slot.holding.game_data.province.game_data.current_cultural_name) : province.game_data.current_cultural_name
				readonly property string effects_string: building ? building.get_effects_string(building_slot.holding, false) : pathway.get_modifier_string(province, false)
				
				onClicked: {
					construction_choice_dialog.close()
					if (building_slot !== null) {
						building_slot.build_building(building)
					} else {
						province.game_data.build_pathway(pathway)
					}
				}
			}
		}
	}
}
